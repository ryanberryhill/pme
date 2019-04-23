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

    const unsigned CARDINALITY_INF = std::numeric_limits<unsigned>::max();

    MCSFinder::MCSFinder(VariableManager & varman,
                         DebugTransitionRelation & tr)
        : m_cardinality(0),
          m_solver(varman, tr),
          m_solver_inf(varman, tr)
    {
        if (!GlobalState::options().caivc_use_bmc)
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

    FindMCSResult MCSFinder::findAndBlockOverGates(const std::vector<ID> & gates)
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

    FindMCSResult MCSFinder::findAndBlock()
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
                                               DebugTransitionRelation & tr)
        : m_cardinality(0),
          m_fallback(varman, tr),
          m_solver(varman, tr)
    { }

    FindMCSResult ApproximateMCSFinder::findAndBlockWithBMC(unsigned n)
    {
        m_solver.setCardinality(n);
        unsigned k_max = GlobalState::options().caivc_ar_bmc_kmax;
        return m_solver.debugRangeAndBlock(0, k_max);
    }

    FindMCSResult ApproximateMCSFinder::findAndBlockOverGatesWithBMC(const std::vector<ID> & gates, unsigned n)
    {
        m_solver.setCardinality(n);
        unsigned k_max = GlobalState::options().caivc_ar_bmc_kmax;
        return m_solver.debugOverGatesRangeAndBlock(gates, 0, k_max);
    }

    FindMCSResult ApproximateMCSFinder::findAndBlockOverGates(const std::vector<ID> & gates)
    {
        bool found;
        CorrectionSet corr;

        // TODO investigate strategies for this process, and especially
        // try to avoid the fallback situation
        unsigned k_max = GlobalState::options().caivc_ar_bmc_kmax;
        unsigned n_max = GlobalState::options().caivc_ar_bmc_nmax;

        n_max = std::min(n_max, (unsigned) gates.size());

        for (unsigned n = 1; n <= n_max; ++n)
        {
            // Debug at cardinality n on [0, k_max]
            m_solver.setCardinality(n);
            std::tie(found, corr) = m_solver.debugOverGatesRangeAndBlock(gates, 0, k_max);

            // Return solution if we found one
            if (found) { return std::make_pair(true, corr); }
        }

        // We failed to find an MCS within our k_max parameter, fallback to IC3
        FindMCSResult fallback_result = findFallback(gates);

        return fallback_result;
    }

    FindMCSResult ApproximateMCSFinder::findFallback(const std::vector<ID> & gates)
    {
        bool found;
        CorrectionSet corr;

        // We shouldn't assume all cardinality 1 MCSes were already found.
        // Sometimes the approximate procedure misses them.
        for (unsigned n = 1; n <= gates.size(); ++n)
        {
            m_fallback.setCardinality(n);

            std::tie(found, corr) = m_fallback.debugAndBlockOverGates(gates);
            if (found)
            {
                m_solver.blockSolution(corr);
                return std::make_pair(true, corr);
            }
        }

        return std::make_pair(false, corr);
    }

    void ApproximateMCSFinder::blockSolution(const CorrectionSet & corr)
    {
        m_fallback.blockSolution(corr);
        m_solver.blockSolution(corr);
    }


    ///////////////////////////////////////////////////////
    // CorrectionSetFinder
    ///////////////////////////////////////////////////////

    CorrectionSetFinder::CorrectionSetFinder(VariableManager & varman,
                                             const DebugTransitionRelation & tr)
        : m_vars(varman), m_tr(tr)
    { }

    FindMCSResult CorrectionSetFinder::findNext(const std::vector<ID> & gates)
    {
        return findNext(gates, CARDINALITY_INF);
    }

    FindMCSResult CorrectionSetFinder::findNext(unsigned n)
    {
        std::vector<ID> gates(tr().begin_gate_ids(), tr().end_gate_ids());
        return findNext(gates, n);
    }

    FindMCSResult CorrectionSetFinder::findNext()
    {
        return findNext(CARDINALITY_INF);
    }

    bool CorrectionSetFinder::moreCorrectionSets()
    {
        return moreCorrectionSets(CARDINALITY_INF);
    }

    ///////////////////////////////////////////////////////
    // BasicMCSFinder
    ///////////////////////////////////////////////////////

    BasicMCSFinder::BasicMCSFinder(VariableManager & varman,
                                   const DebugTransitionRelation & tr)
        : CorrectionSetFinder(varman, tr),
          m_solver(varman, tr),
          m_cardinality(1)
    { }

    FindMCSResult BasicMCSFinder::doFind(const std::vector<ID> & gates, unsigned n)
    {
        unsigned start_cardinality = gates.empty() ? m_cardinality : 1;
        for (unsigned cardinality = start_cardinality; cardinality <= n; ++cardinality)
        {
            // When we're searching globally, we need to track the member
            // variable that stores the current cardinality
            if (gates.empty()) { m_cardinality = cardinality; }

            setCardinality(cardinality);

            FindMCSResult result = doDebug(gates);

            if (result.first) { return result; }

            if (!moreCorrectionSets(n)) { break; }
        }

        CorrectionSet empty;
        return std::make_pair(false, empty);
    }

    FindMCSResult BasicMCSFinder::doDebug(const std::vector<ID> & gates)
    {
        if (gates.empty())
        {
            return m_solver.debugAndBlock();
        }
        else
        {
            return m_solver.debugAndBlockOverGates(gates);
        }
    }

    FindMCSResult BasicMCSFinder::findNext(const std::vector<ID> & gates, unsigned n)
    {
        return doFind(gates, n);
    }

    FindMCSResult BasicMCSFinder::findNext(unsigned n)
    {
        std::vector<ID> empty;
        return doFind(empty, n);
    }

    FindMCSResult BasicMCSFinder::findNext()
    {
        return findNext(CARDINALITY_INF);
    }

    bool BasicMCSFinder::moreCorrectionSets(unsigned n)
    {
        setCardinality(n);

        bool sat;
        std::tie(sat, std::ignore) = m_solver.debug();

        return sat;
    }

    void BasicMCSFinder::setCardinality(unsigned n)
    {
        if (n >= tr().numGates())
        {
            // including CARDINALITY_INF
            m_solver.clearCardinality();
        }
        else
        {
            m_solver.setCardinality(n);
        }
    }

    void BasicMCSFinder::block(const CorrectionSet & corr)
    {
        m_solver.blockSolution(corr);
    }
}
