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
          m_solver(varman)
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

        unsigned cardinality = 1;

        do {
            m_finder.setCardinality(cardinality);

            while (true)
            {
                bool found;
                CorrectionSet corr;
                std::tie(found, corr) = findCorrectionSet();
                if (!found) {
                    log(2) << "Done cardinality " << cardinality << std::endl;
                    break;
                }

                log(2) << "Found correction set of size " << corr.size() << std::endl;
                for (ID id : corr) { log(2) << id << " "; }
                log(2) << std::endl;

                m_solver.addClause(corr);
            }

            cardinality++;

        } while (moreCorrectionSets());

        log(2) << "Done finding correction sets" << std::endl;

        while (true)
        {
            bool found;
            IVC mivc;
            std::tie(found, mivc) = findMIVC();
            if (!found) { break; }

            if (!minimumIVCKnown()) { setMinimumIVC(mivc); }
            addMIVC(mivc);

            log(2) << "Found MIVC of size " << mivc.size() << std::endl;
            for (ID id : mivc) { log(2) << id << " "; }
            log(2) << std::endl;
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

    std::pair<bool, IVC> CAIVCFinder::findMIVC()
    {
        bool found;
        IVC mivc;

        found = m_solver.solve();

        if (!found) { return std::make_pair(false, mivc); }

        mivc = extractIVC();

        Clause block = negateVec(mivc);
        m_solver.addClause(block);

        return std::make_pair(true, mivc);
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
}

