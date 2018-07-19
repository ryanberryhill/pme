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

#include "pme/ivc/correction_set_finder.h"

#include <cassert>

namespace PME {

    CorrectionSetFinder::CorrectionSetFinder(VariableManager & varman,
                                             DebugTransitionRelation & tr,
                                             GlobalState & gs)
        : m_cardinality(0),
          m_solver(varman, tr, gs),
          m_solver_inf(varman, tr, gs)
    {
        if (!gs.opts.caivc_use_bmc)
        {
            m_solver.setKMax(0);
        }
    }

    bool CorrectionSetFinder::moreCorrectionSets()
    {
        bool more;
        std::tie(more, std::ignore) = m_solver_inf.debug();
        return more;
    }

    std::pair<bool, CorrectionSet>
    CorrectionSetFinder::findAndBlockOverGates(const std::vector<ID> & gates)
    {
        bool found;
        CorrectionSet corr;

        std::tie(found, corr) = m_solver.debugAndBlockOverGates(gates);
        if (found)
        {
            m_solver_inf.blockSolution(corr);
        }

        return std::make_pair(found, corr);
    }

    std::pair<bool, CorrectionSet> CorrectionSetFinder::findAndBlock()
    {
        bool found;
        CorrectionSet corr;

        std::tie(found, corr) = m_solver.debugAndBlock();
        if (found)
        {
            m_solver_inf.blockSolution(corr);
        }

        return std::make_pair(found, corr);
    }

    void CorrectionSetFinder::setCardinality(unsigned n)
    {
        m_cardinality = n;
        m_solver.setCardinality(n);
    }
}
