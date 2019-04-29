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
          m_current_k(0)
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
    }

    FindMCSResult BMCCorrectionSetFinder::findNext(const std::vector<ID> & gates, unsigned n)
    {
        // This function is normally called only when we know a correction set
        // exists, so the fallback shouldn't occur unless we exceed k_max.
        unsigned k_max = GlobalState::options().mcs_bmc_loose_kmax;

        for (unsigned cardinality = 1; cardinality <= n; ++cardinality)
        {
            setBMCCardinality(cardinality);

            bool sat;
            CorrectionSet corr;
            std::tie(sat, corr) = m_bmc.debugRange(0, k_max);

            if (sat)
            {
                block(corr);
                return std::make_pair(true, corr);
            }

            if (cardinality >= tr().numGates()) { break; }
        }

        stats().mcs_fallbacks++;
        for (unsigned cardinality = 1; cardinality <= n; cardinality++)
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

            if (!moreCorrectionSets()) { break; }
        }

        CorrectionSet empty;
        return std::make_pair(false, empty);
    }

    FindMCSResult BMCCorrectionSetFinder::findNext(unsigned n)
    {
        unsigned k_max = GlobalState::options().mcs_bmc_kmax;
        unsigned start_cardinality = m_cardinality;

        for (unsigned cardinality = start_cardinality; cardinality <= n; ++cardinality)
        {
            m_cardinality = cardinality;
            setBMCCardinality(cardinality);

            for (unsigned k = m_current_k; k <= k_max; ++k)
            {
                m_current_k = k;
                bool sat;
                CorrectionSet corr;
                std::tie(sat, corr) = m_bmc.debugAtK(m_current_k);

                if (sat)
                {
                    block(corr);
                    return std::make_pair(true, corr);
                }
            }

            m_current_k = 0;

            if (cardinality >= tr().numGates()) { break; }
        }

        stats().mcs_fallbacks++;
        for (unsigned cardinality = 1; cardinality <= n; cardinality++)
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

            if (!moreCorrectionSetsIC3(n)) { break; }
        }

        CorrectionSet empty;
        return std::make_pair(false, empty);
    }

    bool BMCCorrectionSetFinder::moreCorrectionSetsBMC(unsigned n)
    {
        unsigned k_max = GlobalState::options().mcs_bmc_kmax;
        setBMCCardinality(n);

        bool sat;
        std::tie(sat, std::ignore) = m_bmc.debugRange(0, k_max);

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
}
