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
#include "pme/util/check_cex.h"
#include "pme/util/find_safe_mis.h"

#include <cassert>

namespace PME {

    UnifiedIVCFinder::UnifiedIVCFinder(VariableManager & varman,
                                       const TransitionRelation & tr)
        : IVCFinder(varman, tr),
          m_debug_tr(tr),
          m_map(createMap()),
          m_check_map(vars(), tr.begin_gate_ids(), tr.end_gate_ids()),
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
        assert(seed.size() >= m_mivc_lb);

        // Update lower bound (if possible)
        if (is_minimal)
        {
            m_mivc_lb = seed.size();
        }

        // check for safety (if necessary)
        SafetyProof proof;
        if (isSafe(seed, &proof))
        {
            log(3) << "Found an IVC of size " << seed.size() << std::endl;

            // Set minimum IVC if this is both minimal and safe
            if (is_minimal && !minimumIVCKnown())
            {
                // Record that an SMIVC has been found
                setMinimumIVC(seed);
            }

            // shrink (if necessary) and refine using blockUp
            refineSafe(seed, proof, !is_minimal);

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

    void UnifiedIVCFinder::shrink(Seed & seed, const SafetyProof & proof)
    {
        stats().uivc_shrink_calls++;
        AutoTimer t(stats().uivc_shrink_time);

        MapSolver * map = opts().uivc_check_map ? &m_check_map : nullptr;

        IVCUCBFFinder ivc_ucbf(vars(), tr());
        ivc_ucbf.shrinkUC(seed, proof, map);

        // Do the brute force shrinking here to make use of the isSafe caches
        for (size_t i = 0; i < seed.size(); )
        {
            Seed seed_copy = seed;
            seed_copy.erase(seed_copy.begin() + i);
            if (opts().uivc_check_map && !map->checkSeed(seed_copy))
            {
                log(4) << "Cannot remove " << seed[i] << std::endl;
                stats().uivc_map_checks++;
                ++i;
            }
            else if (isSafe(seed_copy))
            {
                log(4) << "Successfully removed " << seed[i] << std::endl;
                seed.erase(seed.begin() + i);
            }
            else
            {
                log(4) << "Cannot remove " << seed[i] << std::endl;
                ++i;
            }
        }

        log(2) << "Further shrunk down to " << seed.size() << " using IVC_BF" << std::endl;
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

        stats().uivc_cs_found++;
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

            if (opts().uivc_check_map && !m_check_map.checkSeed(seed))
            {
                stats().uivc_map_checks++;
                seed.pop_back();
            }
            else if (isSafe(seed))
            {
                seed.pop_back();
            }
        }
    }

    void UnifiedIVCFinder::refineSafe(Seed & seed, const SafetyProof & proof, bool do_shrink)
    {
        if (do_shrink) { shrink(seed, proof); }
        m_map->blockUp(seed);
        m_check_map.blockUp(seed);
    }

    void UnifiedIVCFinder::refineUnsafe(Seed & seed, bool do_grow)
    {
        if (do_grow) { grow(seed); }
        m_map->blockDown(seed);
        m_check_map.blockDown(seed);
    }

    void UnifiedIVCFinder::findMCSesUpfront()
    {
        AutoTimer t(stats().uivc_prep_time);

        unsigned n_max = opts().uivc_upfront_nmax;

        std::vector<CorrectionSet> upfront;
        if (n_max == UINFINITY)
        {
            upfront = m_cs_finder->findAll(n_max);
        }
        else
        {
            upfront = m_cs_finder->findBatch(n_max);
        }

        // Ideally we would log these as they come in instead
        for (const auto & corr : upfront)
        {
            stats().uivc_cs_found++;

            log(3) << "Found a correction set of size " << corr.size()
                   << " [#" << stats().uivc_cs_found << "]"
                   << std::endl;
            log(4) << "CS " << corr << std::endl;

            assert(!corr.empty());

            Seed mss = negateSeed(corr);

            m_map->blockDown(mss);
            m_check_map.blockDown(mss);
        }
    }

    bool UnifiedIVCFinder::isSafe(const Seed & seed, SafetyProof * proof)
    {
        // Don't need to check safety if we're doing CAMUS-style enumeration
        if (!shouldCheckSafety()) { return true; }

        stats().uivc_issafe_calls++;
        AutoTimer t(stats().uivc_issafe_time);

        TransitionRelation partial(tr(), seed);

        SafetyResult cached = checkSafetyCache(partial);

        if (!cached.unknown())
        {
            if (proof) { *proof = cached.proof; }
            return cached.safe();
        }

        HybridSafetyChecker checker(vars(), partial);
        SafetyResult safe = checker.prove();

        if (safe.safe())
        {
            stats().uivc_safe_cache_misses++;
            if (proof) { *proof = safe.proof; }
            cacheProof(safe.proof);
        }
        else if (safe.unsafe())
        {
            stats().uivc_unsafe_cache_misses++;
            cacheCounterExample(safe.cex);
        }

        return safe.safe();
    }

    SafetyResult UnifiedIVCFinder::checkSafetyCache(const TransitionRelation & partial)
    {
        SafetyResult result;

        // Checking counter-examples is much less expensive, so always do
        // it first.
        for (auto it = m_cexes.begin(); it != m_cexes.end(); ++it)
        {
            AutoTimer t(stats().uivc_unsafe_cache_time);
            const auto & cex = *it;
            if (checkCounterExample(vars(), partial, cex))
            {
                // Unsafe, witnessed by cex
                // Move cex to the front of the LRU cache and return UNSAFE
                m_cexes.push_front(cex);
                m_cexes.erase(it);

                stats().uivc_unsafe_cache_hits++;
                log(4) << "Found seed unsafe using cache" << std::endl;

                //SafetyProof empty_proof;
                //return SafetyResult(UNSAFE, empty_proof, cex);
                result.result = UNSAFE;
                result.cex = m_cexes.front();
                return result;
            }
        }

        for (auto it = m_proofs.begin(); it != m_proofs.end(); ++it)
        {
            AutoTimer t(stats().uivc_safe_cache_time);
            const auto & proof = *it;
            if (findSafeMIS(vars(), partial, proof))
            {
                // Safe, witnessed by proof
                // Move proof to the front of the LRU cache and return SAFE.
                // What about the safe MIS? We throw it away. It's a subset of
                // the proof so while it might be faster to check sometimes,
                // checking a proof is relatively inexpensive already.
                m_proofs.push_front(proof);
                m_proofs.erase(it);

                stats().uivc_safe_cache_hits++;
                log(4) << "Found seed safe using cache" << std::endl;

                //SafetyCounterExample empty_cex;
                //return SafetyResult(SAFE, proof, empty_cex);
                result.result = SAFE;
                result.proof = m_proofs.front();
                return result;
            }
        }

        result.result = UNKNOWN;
        return result;
    }

    void UnifiedIVCFinder::cacheCounterExample(const SafetyCounterExample & cex)
    {
        unsigned size = opts().uivc_cex_cache;
        if (size == 0) { return; }

        m_cexes.push_front(cex);

        assert(m_cexes.size() <= size + 1);
        if (m_cexes.size() > size)
        {
            m_cexes.pop_back();
        }
    }

    void UnifiedIVCFinder::cacheProof(const SafetyProof & proof)
    {
        unsigned size = opts().uivc_proof_cache;
        if (size == 0) { return; }

        m_proofs.push_front(proof);

        assert(m_proofs.size() <= size + 1);
        if (m_proofs.size() > size)
        {
            m_proofs.pop_back();
        }
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
            ID rhs0 = strip(gate.rhs0);
            ID rhs1 = strip(gate.rhs1);

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

            assert(cls.size() >= 2);
            assert(cls.size() == fanout.size() + 1);
            m_map->addClause(cls);
        }
    }
}
