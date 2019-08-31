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

    std::vector<CorrectionSet> CorrectionSetFinder::findAll(unsigned n)
    {
        std::vector<CorrectionSet> results;

        bool sat = true;

        while (sat)
        {
            CorrectionSet corr;
            std::tie(sat, corr) = findNext(n);
            if (sat)
            {
                results.push_back(corr);
            }
        }

        return results;
    }

    std::vector<CorrectionSet> CorrectionSetFinder::findBatch(unsigned n)
    {
        return findAll(n);
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
            // including n = CARDINALITY_INF
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

    ///////////////////////////////////////////////////////
    // BMCCorrectionSetFinder
    ///////////////////////////////////////////////////////
    BMCCorrectionSetFinder::BMCCorrectionSetFinder(VariableManager & varman,
                           const DebugTransitionRelation & tr)
        : CorrectionSetFinder(varman, tr),
          m_bmc(varman, tr),
          m_ic3(varman, tr),
          m_cardinality(1),
          m_exhausted_cardinality(0),
          m_current_k(0),
          m_k_max(opts().mcs_bmc_kmax),
          m_k_min(opts().mcs_bmc_kmin)
    { }

    void BMCCorrectionSetFinder::setBMCCardinality(unsigned n)
    {
        if (n >= tr().numGates())
        {
            // including n = CARDINALITY_INF
            m_bmc.clearCardinality();
        }
        else
        {
            m_bmc.setCardinality(n);
        }

        stats().uivc_k_max = m_k_max;
    }

    FindMCSResult BMCCorrectionSetFinder::findNext(const std::vector<ID> & gates, unsigned n)
    {
        // The version with gates is normally called only when we know a
        // correction set exists, so the fallback shouldn't occur unless we
        // exceed k_max.
        unsigned n_max = std::min(n, (unsigned) gates.size());
        n_max = std::min(n_max, (unsigned) opts().mcs_bmc_nmax);

        unsigned start_cardinality = m_exhausted_cardinality + 1;
        for (unsigned cardinality = start_cardinality; cardinality <= n_max; ++cardinality)
        {
            for (unsigned k = 0; k <= m_k_max; ++k)
            {
                FindMCSResult result = findAt(gates, k, cardinality);
                if (result.first)
                {
                    return result;
                }
                else if (opts().mcs_try_to_exhaust && !checkAt(k, cardinality))
                {
                    // Try to exhaust it if possible
                    exhaust(k, cardinality);
                }
            }
        }

        return findFallback(gates, n);
    }

    FindMCSResult BMCCorrectionSetFinder::findNext(unsigned n)
    {
        unsigned n_max = std::min(n, (unsigned) tr().numGates());
        n_max = std::min(n_max, (unsigned) opts().mcs_bmc_nmax);

        unsigned start_cardinality = m_exhausted_cardinality + 1;
        for (unsigned cardinality = start_cardinality; cardinality <= n_max; ++cardinality)
        {
            for (unsigned k = 0; k <= m_k_max; ++k)
            {
                FindMCSResult result = findAt(k, cardinality);
                if (result.first) { return result; }

                exhaust(k, cardinality);
            }
        }

        CorrectionSet empty;
        return std::make_pair(false, empty);
    }

    bool BMCCorrectionSetFinder::checkAt(unsigned k, unsigned cardinality)
    {
        if (isExhausted(k, cardinality)) { return false; }

        setBMCCardinality(cardinality);

        bool sat;
        std::tie(sat, std::ignore) = m_bmc.debugAtK(k);

        return sat;
    }

    FindMCSResult BMCCorrectionSetFinder::findAt(unsigned k, unsigned cardinality)
    {
        CorrectionSet corr;

        if (isExhausted(k, cardinality)) { return std::make_pair(false, corr); }

        setBMCCardinality(cardinality);

        bool sat;
        std::tie(sat, corr) = m_bmc.debugAtK(k);

        if (sat)
        {
            block(corr);
        }

        assert(sat || corr.empty());
        return std::make_pair(sat, corr);
    }

    FindMCSResult BMCCorrectionSetFinder::findAt(const std::vector<ID> & gates,
                                                 unsigned k, unsigned cardinality)
    {
        CorrectionSet corr;

        if (isExhausted(k, cardinality)) { return std::make_pair(false, corr); }

        setBMCCardinality(cardinality);

        bool sat;
        std::tie(sat, corr) = m_bmc.debugOverGatesAtK(gates, k);

        if (sat)
        {
            block(corr);
        }

        assert(sat || corr.empty());
        return std::make_pair(sat, corr);
    }

    FindMCSResult BMCCorrectionSetFinder::findFallback(unsigned n)
    {
        stats().mcs_fallbacks++;

        unsigned n_max = std::min(n, (unsigned) tr().numGates());

        bool more_cs_called = false;
        bool more_cs_exist = false;
        unsigned start_cardinality = m_exhausted_cardinality + 1;
        for (unsigned cardinality = start_cardinality; cardinality <= n_max; cardinality++)
        {
            m_ic3.setCardinality(cardinality);

            bool sat;
            CorrectionSet corr;
            std::tie(sat, corr) = m_ic3.debug();

            if (sat)
            {
                block(corr);
                return std::make_pair(true, corr);
            }

            m_exhausted_cardinality = cardinality;

            if (!more_cs_called) { more_cs_exist = moreCorrectionSets(); }

            if (!more_cs_exist) { break; }
        }

        CorrectionSet empty;
        return std::make_pair(false, empty);
    }

    FindMCSResult BMCCorrectionSetFinder::findFallback(const std::vector<ID> & gates,
                                                       unsigned n)
    {
        stats().mcs_fallbacks++;

        unsigned n_max = std::min(n, (unsigned) gates.size());

        bool more_cs_called = false;
        bool more_cs_exist = false;
        unsigned start_cardinality = m_exhausted_cardinality + 1;
        for (unsigned cardinality = start_cardinality; cardinality <= n_max; cardinality++)
        {
            m_ic3.setCardinality(cardinality);

            bool sat;
            CorrectionSet corr;
            std::tie(sat, corr) = m_ic3.debugOverGates(gates);

            if (sat)
            {
                block(corr);
                return std::make_pair(true, corr);
            }

            if (!more_cs_called) { more_cs_exist = moreCorrectionSets(); }

            if (!more_cs_exist) { break; }
        }

        CorrectionSet empty;
        return std::make_pair(false, empty);
    }

    std::vector<CorrectionSet> BMCCorrectionSetFinder::findBatch(unsigned n)
    {
        std::vector<CorrectionSet> result;

        // Minimum k before give-up
        unsigned n_max = std::min(n, (unsigned) tr().numGates());
        n_max = std::min(n_max, (unsigned) opts().mcs_bmc_nmax);

        if (n_max == 0) { return result; }

        for (unsigned n = 1; n <= n_max; ++n)
        {
            unsigned last_soln = 0;
            for (unsigned k = 0; k <= m_k_max; )
            {
                bool sat;
                CorrectionSet corr;
                std::tie(sat, corr) = findAt(k, n);

                if (sat)
                {
                    result.push_back(corr);
                    last_soln = k;
                    m_k_min = std::max(m_k_min, k);
                }
                else
                {
                    exhaust(k, n);
                    ++k;
                }

                // Heuristic: Give up if no solutions are found for a few
                // consecutive values of k. Reduce k_max to the one we gave up
                // at.  This ensures every correction set we find is minimal
                // for that k, as when we get to (N, k), we've already been to
                // (N, k') for all k' <= k. We still may find non-minimal
                // correction sets, but this process will never find {a, b} and
                // then {a}.
                assert(m_k_min <= m_k_max);
                if (k - last_soln >= 3 && k >= m_k_min)
                {
                    m_k_max = k;
                    stats().uivc_k_max = m_k_max;
                    break;
                }
            }
        }

        return result;
    }

    bool BMCCorrectionSetFinder::moreCorrectionSetsBMC(unsigned n)
    {
        setBMCCardinality(n);

        bool sat;
        std::tie(sat, std::ignore) = m_bmc.debugRange(0, m_k_max);

        return sat;
    }

    bool BMCCorrectionSetFinder::moreCorrectionSetsIC3(unsigned n)
    {
        // TODO: consider separate IC3 solvers for checking for more sets
        // and finding MCSes
        if (n >= tr().numGates())
        {
            m_ic3.clearCardinality();
        }
        else
        {
            m_ic3.setCardinality(n);
        }

        bool sat;
        std::tie(sat, std::ignore) = m_ic3.debug();

        return sat;
    }

    bool BMCCorrectionSetFinder::moreCorrectionSets(unsigned n)
    {
        if (moreCorrectionSetsBMC(n)) { return true; }

        return moreCorrectionSetsIC3(n);
    }

    void BMCCorrectionSetFinder::block(const CorrectionSet & corr)
    {
        m_bmc.blockSolution(corr);
        m_ic3.blockSolution(corr);
    }

    void BMCCorrectionSetFinder::exhaust(unsigned k, unsigned n)
    {
        auto p = std::make_pair(k, n);
        m_exhausted.insert(p);
    }

    bool BMCCorrectionSetFinder::isExhausted(unsigned k, unsigned n) const
    {
        if (n <= m_exhausted_cardinality) { return true; }
        auto p = std::make_pair(k, n);
        return m_exhausted.count(p) > 0;
    }
}
