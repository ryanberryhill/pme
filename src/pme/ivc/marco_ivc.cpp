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

#include "pme/ivc/marco_ivc.h"
#include "pme/ivc/ivc_bf.h"
#include "pme/ivc/ivc_ucbf.h"
#include "pme/ic3/ic3_solver.h"

#include <cassert>

namespace PME {

    MARCOIVCFinder::MARCOIVCFinder(VariableManager & varman,
                                   const TransitionRelation & tr)
        : IVCFinder(varman, tr),
          m_seedSolver(varman),
          m_debug_tr(tr),
          m_gates(tr.begin_gate_ids(), tr.end_gate_ids())
    {
        initSolvers();
    }

    std::ostream & MARCOIVCFinder::log(int verbosity) const
    {
        return IVCFinder::log(LOG_MARCOIVC, verbosity);
    }

    void MARCOIVCFinder::doFindIVCs()
    {
        // Check for constant output
        if (tr().bad() == ID_FALSE)
        {
            log(3) << "Output is a literal 0" << std::endl;
            Seed empty;
            recordMIVC(empty);
            return;
        }

        while (true)
        {
            bool sat;
            Seed seed;
            std::tie(sat, seed) = getUnexplored();

            if (!sat) { break; }
            else if (isSafe(seed))
            {
                log(3) << "Found an IVC of size " << seed.size() << std::endl;
                shrink(seed);
                log(1) << "MIVC of size " << seed.size() << std::endl;
                log(4) << "MIVC: " << seed << std::endl;
                blockUp(seed);
                recordMIVC(seed);
            }
            else
            {
                log(3) << "Found a non-IVC seed of size " << seed.size() << std::endl;
                grow(seed);
                log(1) << "MNVC of size " << seed.size() << std::endl;
                log(4) << "MNVC: " << seed << std::endl;
                blockDown(seed);
            }
        }

        assert(!m_smallestIVC.empty());
        setMinimumIVC(m_smallestIVC);
    }

    void MARCOIVCFinder::recordMIVC(const Seed & mivc)
    {
        if (m_smallestIVC.empty() || mivc.size() < m_smallestIVC.size())
        {
            m_smallestIVC = mivc;
        }

        addMIVC(mivc);
    }

    MARCOIVCFinder::UnexploredResult MARCOIVCFinder::getUnexplored()
    {
        UnexploredResult result;
        Seed & seed = result.second;
        if (m_seedSolver.solve())
        {
            result.first = true;
            for (ID gate : m_gates)
            {
                ID seed_var = debugVarOf(gate);
                ModelValue assignment = m_seedSolver.getAssignmentToVar(seed_var);
                if (assignment == SAT::TRUE) { seed.push_back(gate); }
            }
        }
        else
        {
            result.first = false;
        }
        return result;
    }

    bool MARCOIVCFinder::isSafe(const Seed & seed)
    {
        // TODO: incremental debugging-based version
        // TODO: non-incremental hybrid BMC/IC3 version
        TransitionRelation partial(tr(), seed);
        IC3::IC3Solver ic3(vars(), partial);

        SafetyResult safe = ic3.prove();
        return (safe.result == SAFE);
    }

    void MARCOIVCFinder::grow(Seed & seed)
    {
        // grow is implemented in the obvious way. Try to add a gate
        // and see if it's still unsafe. If unsafe, keep the gate, otherwise
        // TODO: incremental version, possibly in conjunction with isSafe
        std::set<ID> seed_set(seed.begin(), seed.end());

        for (ID gate : m_gates)
        {
            if (seed_set.count(gate) > 0) { continue; }
            seed.push_back(gate);
            if (isSafe(seed))
            {
                seed.pop_back();
            }
        }
    }

    void MARCOIVCFinder::shrink(Seed & seed)
    {
        if (opts().marcoivc_use_ivcucbf)
        {
            IVCUCBFFinder ivc_ucbf(vars(), tr());
            ivc_ucbf.shrink(seed);
        }
        else
        {
            IVCBFFinder ivc_bf(vars(), tr());
            ivc_bf.shrink(seed);
        }
    }

    void MARCOIVCFinder::blockUp(const Seed & seed)
    {
        assert(!seed.empty());
        // Block seed and all supersets. For seed = c_1,...,c_n: (~c_1 V ... V ~c_n)
        Clause cls;
        cls.reserve(seed.size());

        for (ID gate : seed)
        {
            ID svar = debugVarOf(gate);
            cls.push_back(negate(svar));
        }

        m_seedSolver.addClause(cls);
    }

    void MARCOIVCFinder::blockDown(const Seed & seed)
    {
        assert(!seed.empty());
        // Block seed and all subsets. For c_1,...,c_n not in seed: (c_1 V ... V c_n)
        Clause cls;
        cls.reserve(m_debug_tr.numGates() - seed.size());

        std::set<ID> seed_set(seed.begin(), seed.end());
        for (auto it = begin_gates(); it != end_gates(); ++it)
        {
            ID gate = *it;
            if (seed_set.count(gate) == 0)
            {
                ID svar = debugVarOf(gate);
                cls.push_back(svar);
            }
        }

        // It's possible for original circuit to be the only IVC. In this case,
        // the clause will be empty. Add clause (false) in that case because
        // MaxSATSolver doesn't like empty clauses.
        if (cls.empty())
        {
            m_seedSolver.addClause({ID_FALSE});
        }
        else
        {
            m_seedSolver.addClause(cls);
        }
    }

    void MARCOIVCFinder::initSolvers()
    {
        for (ID gate : m_gates)
        {
            ID dv_id = debugVarOf(gate);
            m_seedSolver.addForOptimization(dv_id);
        }
    }

    ID MARCOIVCFinder::debugVarOf(ID gate) const
    {
        return m_debug_tr.debugLatchForGate(gate);
    }
}

