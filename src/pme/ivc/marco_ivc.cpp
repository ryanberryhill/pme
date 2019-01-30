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
#include "pme/util/hybrid_safety_checker.h"

#include <cassert>

namespace PME {

    MARCOIVCFinder::MARCOIVCFinder(VariableManager & varman,
                                   const TransitionRelation & tr)
        : IVCFinder(varman, tr),
          m_seed_solver(varman),
          m_debug_tr(tr),
          m_gates(tr.begin_gate_ids(), tr.end_gate_ids()),
          m_incr_ivc_checker(varman, m_debug_tr),
          m_mcs(varman, m_debug_tr)
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
                // There is no need to call grow in the current implementation,
                // as we search from the top-down
                //log(3) << "Found a non-IVC seed of size " << seed.size() << std::endl;
                //grow(seed);
                log(1) << "MNVC of size " << seed.size() << std::endl;
                log(4) << "MNVC: " << seed << std::endl;
                blockDown(seed);
            }
        }

        assert(!m_smallest_ivc.empty());
        setMinimumIVC(m_smallest_ivc);
    }

    void MARCOIVCFinder::recordMIVC(const Seed & mivc)
    {
        if (m_smallest_ivc.empty() || mivc.size() < m_smallest_ivc.size())
        {
            m_smallest_ivc = mivc;
        }

        addMIVC(mivc);
    }

    MARCOIVCFinder::UnexploredResult MARCOIVCFinder::getUnexplored()
    {
        stats().marcoivc_get_unexplored_calls++;
        AutoTimer timer(stats().marcoivc_get_unexplored_time);
        UnexploredResult result;
        Seed & seed = result.second;
        if (m_seed_solver.solve())
        {
            result.first = true;
            for (ID gate : m_gates)
            {
                ID seed_var = debugVarOf(gate);
                ModelValue assignment = m_seed_solver.getAssignmentToVar(seed_var);
                if (assignment == SAT::TRUE) { seed.push_back(gate); }
            }
        }
        else
        {
            result.first = false;
        }
        return result;
    }

    Seed MARCOIVCFinder::negateSeed(const Seed & seed) const
    {
        std::set<ID> gate_set(seed.begin(), seed.end());

        Seed neg;
        for (ID gate : m_gates)
        {
            if (gate_set.count(gate) == 0)
            {
                neg.push_back(gate);
            }
        }

        return neg;
    }

    bool MARCOIVCFinder::isSafe(const Seed & seed)
    {
        stats().marcoivc_issafe_calls++;
        AutoTimer timer(stats().marcoivc_issafe_time);

        if (opts().marcoivc_incr_issafe)
        {
            return isSafeIncremental(seed);
        }
        else if (opts().marcoivc_hybrid_issafe)
        {
            return isSafeHybrid(seed);
        }
        else
        {
            return isSafeIC3(seed);
        }
    }

    bool MARCOIVCFinder::isSafeIC3(const Seed & seed)
    {
        TransitionRelation partial(tr(), seed);
        IC3::IC3Solver ic3(vars(), partial);
        SafetyResult safe = ic3.prove();

        return safe.result == SAFE;
    }

    bool MARCOIVCFinder::isSafeHybrid(const Seed & seed)
    {
        TransitionRelation partial(tr(), seed);
        HybridSafetyChecker checker(vars(), partial);
        SafetyResult safe = checker.prove();

        return safe.result == SAFE;
    }

    bool MARCOIVCFinder::isSafeIncremental(const Seed & seed)
    {
        Seed neg = negateSeed(seed);

        bool unsafe;
        std::tie(unsafe, std::ignore) = m_incr_ivc_checker.debugOverGates(neg);

        return !unsafe;
    }

    void MARCOIVCFinder::grow(Seed & seed)
    {
        // grow is implemented in the obvious way. Try to add a gate
        // and see if it's still unsafe.
        // If unsafe, keep the gate, otherwise, remove it.
        stats().marcoivc_grow_calls++;
        AutoTimer timer(stats().marcoivc_grow_time);

        if (opts().marcoivc_debug_grow)
        {
            debugGrow(seed);
        }
        else
        {
            bruteForceGrow(seed);
        }
    }

    void MARCOIVCFinder::bruteForceGrow(Seed & seed)
    {
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

    void MARCOIVCFinder::debugGrow(Seed & seed)
    {
        // TODO: magic number should be a parameter
        unsigned n_max = 5;
        Seed neg = negateSeed(seed);
        bool found = false;
        Seed mcs;

        std::tie(found, mcs) = m_mcs.findAndBlockOverGatesWithBMC(neg, n_max);

        if (found)
        {
            assert(!mcs.empty());
            seed = negateSeed(mcs);
        }
        else
        {
            bruteForceGrow(seed);
        }
    }

    void MARCOIVCFinder::shrink(Seed & seed)
    {
        stats().marcoivc_shrink_calls++;
        AutoTimer timer(stats().marcoivc_shrink_time);

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

        m_seed_solver.addClause(cls);
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
            m_seed_solver.addClause({ID_FALSE});
        }
        else
        {
            m_seed_solver.addClause(cls);
        }
    }

    void MARCOIVCFinder::initSolvers()
    {
        for (ID gate_id : m_gates)
        {
            ID dv_id = debugVarOf(gate_id);
            m_seed_solver.addForOptimization(dv_id);
        }

        if (opts().marcoivc_explore_basic_hints ||
            opts().marcoivc_explore_complex_hints)
        {
            addExploreHints();
        }
    }

    void MARCOIVCFinder::addExploreHints()
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
        for (ID gate_id : m_gates)
        {
            ID lhs_dv_id = debugVarOf(gate_id);

            const AndGate & gate = tr().getGate(gate_id);
            ID rhs0 = gate.rhs0;
            ID rhs1 = gate.rhs1;

            if (tr().isGate(rhs0))
            {
                ID rhs_dv_id = debugVarOf(rhs0);
                gate_to_fanout[rhs_dv_id].push_back(lhs_dv_id);
            }

            if (tr().isGate(rhs1))
            {
                ID rhs_dv_id = debugVarOf(rhs1);
                gate_to_fanout[rhs_dv_id].push_back(lhs_dv_id);
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

            if (fanout.size() == 1 && opts().marcoivc_explore_basic_hints)
            {
                m_seed_solver.addClause(cls);
            }
            else if (opts().marcoivc_explore_complex_hints)
            {
                m_seed_solver.addClause(cls);
            }
        }
    }

    ID MARCOIVCFinder::debugVarOf(ID gate) const
    {
        return m_debug_tr.debugLatchForGate(gate);
    }
}

