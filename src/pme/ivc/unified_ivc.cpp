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

#include "pme/ivc/unified_ivc.h"
#include "pme/ivc/ivc_ucbf.h"
#include "pme/util/hybrid_safety_checker.h"
#include "pme/pme.h"

#include <cassert>

namespace PME {

    UnifiedIVCFinder::UnifiedIVCFinder(VariableManager & varman,
                                       const TransitionRelation & tr)
        : IVCFinder(varman, tr),
          m_debug_tr(tr),
          m_map(createMap()),
          m_cs_finder(createFinder()),
          m_mivc_lb(0),
          m_seed_count(0)
    {
        if (opts().uivc_coi_hints)
        {
            addCOIToMap();
        }
    }

    std::ostream & UnifiedIVCFinder::log(int verbosity) const
    {
        return IVCFinder::log(LOG_UNIFIED_IVC, verbosity);
    }

    MapSolver * UnifiedIVCFinder::createMap()
    {
        MapSolverType type = opts().uivc_map_solver_type;
        if (type == MAP_SOLVER_MSU4)
        {
            return new MSU4ArbitraryMapSolver(vars(), tr().begin_gate_ids(), tr().end_gate_ids());
        }
        else if (type == MAP_SOLVER_SAT)
        {
            return new SATArbitraryMapSolver(vars(), tr().begin_gate_ids(), tr().end_gate_ids());
        }
        else
        {
            throw new std::logic_error("Unknown map solver type in UnifiedIVCSolver");
        }
    }

    CorrectionSetFinder * UnifiedIVCFinder::createFinder()
    {
        MCSFinderType type = opts().uivc_mcs_finder_type;
        if (type == MCS_FINDER_BASIC)
        {
            return new BasicMCSFinder(vars(), m_debug_tr);
        }
        else if (type == MCS_FINDER_BMC)
        {
            return new BMCCorrectionSetFinder(vars(), m_debug_tr);
        }
        else
        {
            throw new std::logic_error("Unknown MCS finder type in UnifiedIVCSolver");
        }
    }

    void UnifiedIVCFinder::doFindIVCs()
    {
        // Check for constant output
        if (tr().bad() == ID_FALSE)
        {
            log(3) << "Output is a literal 0" << std::endl;
            IVC empty;
            addMIVC(empty);
            return;
        }

        // Find correction sets upfront (if any)
        findMCSesUpfront();

        // Find candidates
        while (getUnexplored()) { }

        // Update SMIVC (if necessary)
        if (!minimumIVCKnown()) { setMinimumIVC(m_smallest_ivc); }
    }

    bool UnifiedIVCFinder::getUnexplored()
    {
        bool found = false;

        if (m_seed_count % 2 == 0)
        {
            // Even seed => down direction for zigzag.
            // Prioritize down, then up, then neither.
            if (opts().uivc_direction_down)
            {
                found = getUnexploredMax();
            }
            else if (opts().uivc_direction_up)
            {
                found = getUnexploredMin();
            }
            else
            {
                found = getUnexploredArb();
            }
        }
        else
        {
            // Odd seed => up direction for zigzag.
            // Prioritize up, then down, then neither.
            if (opts().uivc_direction_up)
            {
                found = getUnexploredMin();
            }
            else if (opts().uivc_direction_down)
            {
                found = getUnexploredMax();
            }
            else
            {
                found = getUnexploredArb();
            }
        }

        m_seed_count++;

        return found;
    }

    bool UnifiedIVCFinder::getUnexploredMin()
    {
        stats().uivc_get_unexplored_min_calls++;

        bool sat;
        Seed seed;

        {
            AutoTimer t(stats().uivc_get_unexplored_min_time);
            std::tie(sat, seed) = m_map->findMinimalSeed();
        }

        if (sat)
        {
            handleMinimalSeed(seed);
        }

        return sat;
    }

    bool UnifiedIVCFinder::getUnexploredMax()
    {
        stats().uivc_get_unexplored_max_calls++;

        bool sat;
        Seed seed;

        {
            AutoTimer t(stats().uivc_get_unexplored_max_time);
            std::tie(sat, seed) = m_map->findMaximalSeed();
        }

        if (sat)
        {
            handleMaximalSeed(seed);
        }

        return sat;
    }

    bool UnifiedIVCFinder::getUnexploredArb()
    {
        stats().uivc_get_unexplored_arb_calls++;

        bool sat;
        Seed seed;

        {
            std::tie(sat, seed) = m_map->findSeed();
            AutoTimer t(stats().uivc_get_unexplored_arb_time);
        }

        if (sat)
        {
            handleSeed(seed);
        }

        return sat;
    }

    void UnifiedIVCFinder::handleMinimalSeed(Seed & seed)
    {
        handleSeed(seed, true, false);
    }

    void UnifiedIVCFinder::handleMaximalSeed(Seed & seed)
    {
        handleSeed(seed, false, true);
    }

    void UnifiedIVCFinder::handleSeed(Seed & seed, bool is_minimal, bool is_maximal)
    {
        assert(!is_minimal || !is_maximal);

        // Update lower bound (if possible)
        if (is_minimal)
        {
            assert(seed.size() >= m_mivc_lb);
            m_mivc_lb = seed.size();
        }

        // check for safety (if necessary)
        if (isSafe(seed))
        {
            log(3) << "Found an IVC of size " << seed.size() << std::endl;

            // Set minimum IVC if this is both minimal and safe
            if (is_minimal && !minimumIVCKnown())
            {
                // Record that an SMIVC has been found
                setMinimumIVC(seed);
            }

            // shrink (if necessary) and refine using blockUp
            refineSafe(seed, !is_minimal);

            recordMIVC(seed);

            // Check if the size is equal to the lower bound,
            // indicating that it is an SMIVC
            assert(seed.size() >= m_mivc_lb);
            if (seed.size() == m_mivc_lb && !minimumIVCKnown())
            {
                setMinimumIVC(seed);
            }
        }
        else
        {
            log(3) << "Found an unsafe seed of size " << seed.size() << std::endl;

            // grow (if necessary) and refine using blockDown
            refineUnsafe(seed, !is_maximal);

            log(1) << "MNVC of size " << seed.size() << std::endl;
            log(4) << "MNVC: " << seed << std::endl;
        }
    }

    void UnifiedIVCFinder::shrink(Seed & seed)
    {
        stats().uivc_shrink_calls++;
        AutoTimer t(stats().uivc_shrink_time);

        IVCUCBFFinder ivc_ucbf(vars(), tr());
        ivc_ucbf.shrink(seed);
    }

    void UnifiedIVCFinder::grow(Seed & seed)
    {
        stats().uivc_grow_calls++;
        AutoTimer t(stats().uivc_grow_time);

        if (opts().uivc_mcs_grow)
        {
            growByMCS(seed);
        }
        else
        {
            growByBruteForce(seed);
        }
    }

    void UnifiedIVCFinder::growByMCS(Seed & seed)
    {
        Seed neg_seed = negateSeed(seed);

        bool sat;
        CorrectionSet corr;
        std::tie(sat, corr) = m_cs_finder->findNext(neg_seed);

        assert(sat);
        assert(!corr.empty());

        seed = negateSeed(corr);
    }

    void UnifiedIVCFinder::growByBruteForce(Seed & seed)
    {
        std::set<ID> seed_set(seed.begin(), seed.end());

        for (auto it = tr().begin_gate_ids(); it != tr().end_gate_ids(); ++it)
        {
            ID gate = *it;
            if (seed_set.count(gate) > 0) { continue; }
            seed.push_back(gate);
            if (isSafe(seed))
            {
                seed.pop_back();
            }
        }
    }

    void UnifiedIVCFinder::refineSafe(Seed & seed, bool do_shrink)
    {
        if (do_shrink) { shrink(seed); }
        m_map->blockUp(seed);
    }

    void UnifiedIVCFinder::refineUnsafe(Seed & seed, bool do_grow)
    {
        if (do_grow) { grow(seed); }
        m_map->blockDown(seed);
    }

    void UnifiedIVCFinder::findMCSesUpfront()
    {
        unsigned n_max = opts().uivc_upfront_nmax;

        bool sat = true;

        AutoTimer t(stats().uivc_prep_time);

        while (sat && n_max > 0)
        {
            CorrectionSet corr;
            std::tie(sat, corr) = m_cs_finder->findNext(n_max);

            if (!sat) { break; }

            log(3) << "Found a correction set of size " << corr.size() << std::endl;
            log(4) << "CS " << corr << std::endl;

            assert(!corr.empty());

            Seed mss = negateSeed(corr);
            m_map->blockDown(mss);
        }
    }

    bool UnifiedIVCFinder::isSafe(const Seed & seed)
    {
        // Don't need to check safety if we're doing CAMUS-style enumeration
        if (!shouldCheckSafety()) { return true; }

        stats().uivc_issafe_calls++;
        AutoTimer t(stats().uivc_issafe_time);

        TransitionRelation partial(tr(), seed);
        HybridSafetyChecker checker(vars(), partial);
        SafetyResult safe = checker.prove();

        return safe.result == SAFE;
    }

    void UnifiedIVCFinder::recordMIVC(const Seed & mivc)
    {
        log(1) << "MIVC of size " << mivc.size() << std::endl;
        log(4) << "MIVC " << mivc << std::endl;

        if (m_smallest_ivc.empty() || mivc.size() < m_smallest_ivc.size())
        {
            m_smallest_ivc = mivc;
        }

        addMIVC(mivc);
    }

    bool UnifiedIVCFinder::shouldCheckSafety() const
    {
        return opts().uivc_upfront_nmax < tr().numGates();
    }

    Seed UnifiedIVCFinder::negateSeed(const Seed & seed) const
    {
        std::set<ID> seed_set(seed.begin(), seed.end());

        Seed neg;
        for (auto it = tr().begin_gate_ids(); it != tr().end_gate_ids(); ++it)
        {
            ID gate = *it;
            if (seed_set.count(gate) == 0)
            {
                neg.push_back(gate);
            }
        }

        return neg;
    }

    void UnifiedIVCFinder::addCOIToMap()
    {
        // Map each gate to its fanout (if any)
        //
        // Basic hints: add binary clauses for gates with no fanout.
        // If g_b's only output is g_a, then we have !g_a => !g_b
        // which is the same as g_a V !g_b
        //
        // Complex hints: add larger clauses for gates with fanout.
        // If g_c has g_b and g_a as output, then we have !g_b & !g_a => !g_c
        // or equivalently g_b V g_a V !g_c
        std::map<ID, std::vector<ID>> gate_to_fanout;
        for (auto it = tr().begin_gate_ids(); it != tr().end_gate_ids(); ++it)
        {
            ID gate_id = *it;
            const AndGate & gate = tr().getGate(gate_id);
            ID rhs0 = gate.rhs0;
            ID rhs1 = gate.rhs1;

            if (tr().isGate(rhs0))
            {
                gate_to_fanout[rhs0].push_back(gate_id);
            }

            if (tr().isGate(rhs1))
            {
                gate_to_fanout[rhs1].push_back(gate_id);
            }
        }

        for (const auto & p : gate_to_fanout)
        {
            ID gate = p.first;
            const std::vector<ID> & fanout = p.second;

            assert(!fanout.empty());

            Clause cls;
            cls.reserve(fanout.size() + 1);
            cls.push_back(negate(gate));
            for (ID gate_fanout : fanout)
            {
                cls.push_back(gate_fanout);
            }

            assert(!cls.empty());
            assert(cls.size() == fanout.size() + 1);
            m_map->addClause(cls);
        }
    }
}
