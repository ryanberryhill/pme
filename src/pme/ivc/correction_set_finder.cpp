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
#include <limits>

namespace PME {

    MCSFinder::MCSFinder(VariableManager & varman,
                         DebugTransitionRelation & tr,
                         GlobalState & gs)
        : m_cardinality(0),
          m_solver(varman, tr, gs),
          m_solver_inf(varman, tr, gs)
    {
        if (!gs.opts.caivc_use_bmc)
        {
            m_solver.setKMax(0);
            m_solver_inf.setKMax(0);
        }
    }

    void MCSFinder::setCardinality(unsigned n)
    {
        m_cardinality = n;
        m_solver.setCardinality(n);
    }

    bool MCSFinder::moreCorrectionSets()
    {
        bool more;
        std::tie(more, std::ignore) = m_solver_inf.debug();
        return more;
    }

    MCSFinder::FindResult
    MCSFinder::findAndBlockOverGates(const std::vector<ID> & gates)
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

    MCSFinder::FindResult
    MCSFinder::findAndBlock()
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

    void MCSFinder::blockSolution(const CorrectionSet & corr)
    {
        m_solver.blockSolution(corr);
        m_solver_inf.blockSolution(corr);
    }

    ApproximateMCSFinder::ApproximateMCSFinder(VariableManager & varman,
                                               DebugTransitionRelation & tr,
                                               GlobalState & gs)
        : m_cardinality(0),
          m_fallback(varman, tr, gs),
          m_solver(varman, tr, gs),
          m_gs(gs)
    { }


    ApproximateMCSFinder::FindResult
    ApproximateMCSFinder::findAndBlockOverGates(const std::vector<ID> & gates)
    {
        bool found;
        CorrectionSet corr;

        // TODO investigate strategies for this process
        unsigned k_max = m_gs.opts.caivc_ar_bmc_kmax;
        unsigned n_max = gates.size();

        assert(k_max < std::numeric_limits<unsigned>::max());
        assert(n_max < std::numeric_limits<unsigned>::max());

        // Start from n = 2 under the assumption that all n = 1 MCSes were
        // already blocked
        unsigned n = std::min(2u, n_max);
        // Start k on the range [0, 4]
        unsigned k_lo = 0, k_hi = std::min(4u, k_max);

        bool n_cycle = true;
        while (n <= n_max)
        {
            // Debug at cardinality n on [k_lo, k_hi]
            m_solver.setCardinality(n);
            std::tie(found, corr) = m_solver.debugOverGatesRangeAndBlock(gates, k_lo, k_hi);

            // Return solution if we found one
            if (found) { return std::make_pair(true, corr); }

            // If not, we need to increase k or n
            if (n_cycle || k_hi >= k_max)
            {
                n++;
                k_lo = 0;
            }
            else
            {
                k_lo = k_hi + 1;
                k_hi = std::min(k_hi * 2, k_max);
            }

            n_cycle = !n_cycle;
        }

        // We failed to find an MCS within our k_max parameter, fallback to IC3
        FindResult fallback_result = findFallback(gates);

        return fallback_result;
    }

    ApproximateMCSFinder::FindResult
    ApproximateMCSFinder::findFallback(const std::vector<ID> & gates)
    {
        bool found;
        CorrectionSet corr;

        std::tie(found, corr) = m_fallback.debugAndBlockOverGates(gates);
        if (found)
        {
            m_solver.blockSolution(corr);
        }

        return std::make_pair(found, corr);
    }

    void ApproximateMCSFinder::blockSolution(const CorrectionSet & corr)
    {
        m_fallback.blockSolution(corr);
        m_solver.blockSolution(corr);
    }

}
