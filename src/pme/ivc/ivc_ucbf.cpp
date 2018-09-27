/*
 * Copyright (c) 2018 Ryan Berryhill, University of Toronto
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "pme/ivc/ivc_ucbf.h"
#include "pme/minimization/simple.h"
#include "pme/minimization/sisi.h"
#include "pme/util/mus_finder.h"
#include "pme/util/cardinality_constraint.h"
#include "pme/util/simplify_tr.h"

#include <cassert>

namespace PME {
    IVCUCBFFinder::IVCUCBFFinder(VariableManager & varman,
                                 const TransitionRelation & tr)
        : IVCFinder(varman, tr),
          m_ivcbf(varman, tr),
          m_debugTR(tr)
    { }

    std::ostream & IVCUCBFFinder::log(int verbosity) const
    {
        return IVCFinder::log(LOG_IVCUCBF, verbosity);
    }

    bool IVCUCBFFinder::isSafe(const Seed & seed)
    {
        return m_ivcbf.isSafe(seed);
    }

    bool IVCUCBFFinder::isSafe(const Seed & seed, SafetyProof & proof)
    {
        return m_ivcbf.isSafe(seed, proof);
    }

    void IVCUCBFFinder::shrink(Seed & seed)
    {
        SafetyProof proof;

        bool safe = isSafe(seed, proof);
        assert(safe);

        return shrink(seed, proof);
    }

    void IVCUCBFFinder::shrink(Seed & seed, const SafetyProof & proof)
    {
        // Optionally shrink the proof
        SafetyProof shrunk_proof;
        if (opts().ivc_ucbf_use_simple_min)
        {
            TransitionRelation seed_tr(tr(), seed);
            SimpleMinimizer pmin(vars(), seed_tr, proof);
            pmin.minimize();
            assert(pmin.numProofs() == 1);
            shrunk_proof = pmin.getProof(0);
        }
        else if (opts().ivc_ucbf_use_sisi)
        {
            TransitionRelation seed_tr(tr(), seed);
            SISIMinimizer pmin(vars(), seed_tr, proof);
            pmin.minimize();
            assert(pmin.numProofs() == 1);
            shrunk_proof = pmin.getProof(0);
        }
        else
        {
            shrunk_proof = proof;
        }

        log(2) << "Shrunk proof from " << proof.size()
               << " clauses down to " << shrunk_proof.size()
               << std::endl;

        // Inv & Tr & ~Inv'
        // Inv and ~Inv' are hard clauses.
        // Ideally, Tr would constructed from soft clause groups where each
        // group represents a logic gate. Instead, we use a Debug TR (as hard
        // clauses) and add soft clauses enforcing that the debug latches are
        // zero.  For each gate, (~dl) is a soft clause.
        MUSFinderWrapper finder(vars());

        // Inv
        finder.addHardClauses(shrunk_proof.begin(), shrunk_proof.end());

        // ~Inv'
        ClauseVec neg_invp = negatePrimeAndCNFize(shrunk_proof);
        finder.addHardClauses(neg_invp.begin(), neg_invp.end());

        // Tr
        ClauseVec debug_tr = opts().simplify ? simplifyTR(m_debugTR) : m_debugTR.unroll(2);
        finder.addHardClauses(debug_tr.begin(), debug_tr.end());

        // Soft clause groups for debug latchess
        for (ID gate : seed)
        {
            ID dl = m_debugTR.debugLatchForGate(gate);
            finder.addSoftClause(gate, {negate(dl)});
        }

        // Find MUS or core
        Seed core;
        if (opts().ivc_ucbf_use_core)
        {
            core = finder.findCore();
            log(2) << "Shrunk seed from " << seed.size()
                   << " gates down to " << core.size() << " via UNSAT core"
                   << std::endl;
        }
        else if (opts().ivc_ucbf_use_mus)
        {
            core = finder.findMUS();
            log(2) << "Shrunk seed from " << seed.size()
                   << " gates down to " << core.size() << " via MUS"
                   << std::endl;
        }
        else
        {
            // This case probably shouldn't happen, this just becomes IVC_BF
            core = seed;
            log(2) << "Did not shrink seed due to settings, size is " << seed.size() << std::endl;
        }

        // Run IVC_BF
        m_ivcbf.shrink(core);
        log(2) << "Further shrunk down to " << core.size() << " using IVC_BF" << std::endl;

        seed = core;
    }

    void IVCUCBFFinder::doFindIVCs()
    {
        Seed seed(tr().begin_gate_ids(), tr().end_gate_ids());
        shrink(seed);
        addMIVC(seed);
    }

    ClauseVec IVCUCBFFinder::negatePrimeAndCNFize(const ClauseVec & vec)
    {
        // CNFize the DNF by adding activation and a cardinality constraint
        // That is, (x V y) becomes (~x V ~a) & (~y V ~a) [each clause gets
        // a unique a] and a cardinality constraint enforces exactly one
        // a is equal to 1.
        TotalizerCardinalityConstraint cardinality(vars());

        // Need cardinality 2 to assume = 1
        cardinality.setCardinality(2);

        ClauseVec dnf;
        for (const Clause & c : vec)
        {
            ID act = vars().getNewID("cnfization_var");
            cardinality.addInput(act);
            ID nact = negate(act);

            for (ID lit : c)
            {
                ID neglp = prime(negate(lit));
                Clause activated = {nact, neglp};
                dnf.push_back(activated);
            }
        }

        ClauseVec cardinality_clauses = cardinality.CNFize();
        dnf.insert(dnf.end(), cardinality_clauses.cbegin(), cardinality_clauses.cend());

        Cube cardinality_assumps = cardinality.assumeEq(1);
        for (ID lit : cardinality_assumps)
        {
            dnf.push_back({lit});
        }

        return dnf;
    }
}

