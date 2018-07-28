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

#include "pme/ivc/caivc.h"

#include <cassert>

namespace PME {

    CAIVCFinder::CAIVCFinder(VariableManager & varman,
                             TransitionRelation & tr,
                             GlobalState & gs)
        : IVCFinder(varman, tr, gs),
          m_debug_tr(tr),
          m_gates(tr.begin_gate_ids(), tr.end_gate_ids()),
          m_finder(varman, m_debug_tr, gs),
          m_approx_finder(varman, m_debug_tr, gs),
          m_solver(varman),
          m_ivc_checker(varman, m_debug_tr, gs)
    {
        for (ID gate : m_gates)
        {
            m_solver.addForOptimization(negate(gate));
        }
    }

    std::ostream & CAIVCFinder::log(int verbosity) const
    {
        return IVCFinder::log(LOG_CAIVC, verbosity);
    }

    void CAIVCFinder::findIVCs()
    {
        log(2) << "Starting CAIVC (there are " << m_gates.size() << " gates)" << std::endl;

        if (opts().caivc_abstraction_refinement)
        {
            abstractionRefinementFindIVCs();
        }
        else
        {
            naiveFindIVCs();
        }
    }

    void CAIVCFinder::abstractionRefinementFindIVCs()
    {
        // Find all cardinality 1 MCSes
        m_finder.setCardinality(1);
        while (true)
        {
            bool found;
            CorrectionSet corr;
            std::tie(found, corr) = findCorrectionSet();
            if (!found) { break; }

            logMCS(corr);
            m_solver.addClause(corr);
        }

        // Repeatedly find candidate MIVCs. If the candidate is not safe, then
        // find a new MCS consiting only of clauses outside the candidate.
        while (true)
        {
            bool found;
            IVC candidate;
            std::tie(found, candidate) = findCandidateMIVC();

            if (found) { logCandidate(candidate); }
            else
            {
                log(2) << "No more candidates" << std::endl;
                break;
            }

            if (isIVC(candidate))
            {
                if (!minimumIVCKnown()) { setMinimumIVC(candidate); }
                addMIVC(candidate);
                logMIVC(candidate);
                blockMIVC(candidate);
            }
            else
            {
                std::vector<ID> neg = negateGateSet(candidate);
                CorrectionSet corr = findCorrectionSetOverGates(neg);
                logMCS(corr);
                m_solver.addClause(corr);
            }
        }
    }

    void CAIVCFinder::naiveFindIVCs()
    {
        unsigned cardinality = 1;

        unsigned count = 0;
        do {
            m_finder.setCardinality(cardinality);

            while (true)
            {
                bool found;
                CorrectionSet corr;
                std::tie(found, corr) = findCorrectionSet();
                if (!found) { break; }

                count++;
                logMCS(corr);
                m_solver.addClause(corr);
            }

            cardinality++;

        } while (moreCorrectionSets());

        log(2) << "Done finding correction sets (" << count << " found)" << std::endl;

        while (true)
        {
            bool found;
            IVC mivc;
            std::tie(found, mivc) = findAndBlockCandidateMIVC();
            if (!found) { break; }

            if (!minimumIVCKnown()) { setMinimumIVC(mivc); }
            addMIVC(mivc);
            logMIVC(mivc);
        }
    }

    bool CAIVCFinder::moreCorrectionSets()
    {
        return m_finder.moreCorrectionSets();
    }

    std::pair<bool, CorrectionSet> CAIVCFinder::findCorrectionSet()
    {
        return m_finder.findAndBlock();
    }

    CorrectionSet CAIVCFinder::findCorrectionSetOverGates(const std::vector<ID> & gates)
    {
        assert(!gates.empty());

        if (opts().caivc_approx_mcs)
        {
            return findApproxMCSOverGates(gates);
        }
        else
        {
            return findMCSOverGates(gates);
        }
    }

    CorrectionSet CAIVCFinder::findApproxMCSOverGates(const std::vector<ID> & gates)
    {
        bool found;
        CorrectionSet corr;
        std::tie(found, corr) = m_approx_finder.findAndBlockOverGates(gates);

        // This should only be called when we know a correction set exists
        assert(found);
        return corr;
    }

    CorrectionSet CAIVCFinder::findMCSOverGates(const std::vector<ID> & gates)
    {
        // Assuming we already found all cardinality 1 MCSes
        for (unsigned cardinality = 2; cardinality <= gates.size(); ++cardinality)
        {
            bool found;
            CorrectionSet corr;
            m_finder.setCardinality(cardinality);
            std::tie(found, corr) = m_finder.findAndBlockOverGates(gates);
            if (found) { return corr; }
        }

        // This should only be called when we know a correction set exists
        assert(false);

        CorrectionSet empty;
        return empty;
    }

    std::pair<bool, IVC> CAIVCFinder::findAndBlockCandidateMIVC()
    {
        return findCandidateMIVC(true);
    }

    std::pair<bool, IVC> CAIVCFinder::findCandidateMIVC()
    {
        return findCandidateMIVC(false);
    }

    std::pair<bool, IVC> CAIVCFinder::findCandidateMIVC(bool block)
    {
        bool found;
        IVC mivc;

        found = m_solver.solve();

        if (!found) { return std::make_pair(false, mivc); }

        mivc = extractIVC();

        if (block) { blockMIVC(mivc); }

        return std::make_pair(true, mivc);
    }

    void CAIVCFinder::blockMIVC(const IVC & mivc)
    {
        Clause block_cls = negateVec(mivc);
        m_solver.addClause(block_cls);
    }

    bool CAIVCFinder::isIVC(const IVC & candidate)
    {
        // Debug over the gates not in candidate with unlimited cardinality
        // (i.e., all non-candidate gates are removed)
        // TODO: we don't get very much incrementality at all here (indeed,
        // none in IC3, only whatever we get in BMC)

        std::vector<ID> neg = negateGateSet(candidate);

        bool unsafe;
        std::tie(unsafe, std::ignore) = m_ivc_checker.debugOverGates(neg);

        return !unsafe;
    }

    IVC CAIVCFinder::extractIVC() const
    {
        assert(m_solver.isSAT());

        IVC mivc;

        for (ID gate : m_gates)
        {
            if (m_solver.getAssignmentToVar(gate) == SAT::TRUE)
            {
                mivc.push_back(gate);
            }
        }

        return mivc;
    }

    std::vector<ID> CAIVCFinder::negateGateSet(const std::vector<ID> & gates) const
    {
        std::set<ID> gate_set(gates.begin(), gates.end());

        std::vector<ID> neg;
        for (ID gate : m_gates)
        {
            if (gate_set.count(gate) == 0)
            {
                neg.push_back(gate);
            }
        }

        return neg;
    }

    void CAIVCFinder::logMCS(const CorrectionSet & mcs) const
    {
        log(2) << "Found correction set of size " << mcs.size() << std::endl;
        log(3) << "[ ";
        for (ID id : mcs) { log(3) << id << " "; }
        log(3) << "]" << std::endl;
    }

    void CAIVCFinder::logMIVC(const IVC & mivc) const
    {
        log(2) << "Found MIVC of size " << mivc.size() << std::endl;
        log(3) << "[ ";
        for (ID id : mivc) { log(3) << id << " "; }
        log(3) << "]" << std::endl;
    }

    void CAIVCFinder::logCandidate(const IVC & candidate) const
    {
        log(3) << "Found candidate MIVC of size " << candidate.size() << std::endl;
        log(3) << "[ ";
        for (ID id : candidate) { log(3) << id << " "; }
        log(3) << "]" << std::endl;
    }
}

