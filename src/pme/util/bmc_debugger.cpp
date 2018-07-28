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

#include "pme/util/bmc_debugger.h"

#include <limits>
#include <cassert>

namespace PME {

    const unsigned CARDINALITY_INF = std::numeric_limits<unsigned>::max();

    BMCDebugger::BMCDebugger(VariableManager & varman,
                             const DebugTransitionRelation & tr,
                             GlobalState & gs)
        : m_vars(varman),
          m_tr(tr),
          m_gs(gs),
          m_kmax(4),
          m_cardinality(CARDINALITY_INF),
          m_cardinalityConstraint(varman),
          m_solver(varman, tr, gs)
    {
       clearCardinality();
       m_debug_latches.insert(m_tr.begin_debug_latches(),
                              m_tr.end_debug_latches());
       for (ID id : m_debug_latches)
       {
           m_cardinalityConstraint.addInput(id);
       }
    }

    void BMCDebugger::setCardinality(unsigned n)
    {
        m_cardinality = n;
        // Need to use n+1 if we want to assume <= n
        m_cardinalityConstraint.setCardinality(n + 1);
        m_solver.restrictInitialStates(m_cardinalityConstraint.CNFize());
    }

    void BMCDebugger::clearCardinality()
    {
        m_cardinality = CARDINALITY_INF;
    }

    Debugger::Result BMCDebugger::debugRange(unsigned k_min, unsigned k_max)
    {
        Cube assumps;
        return debugWithAssumptions(assumps, k_min, k_max);
    }

    Debugger::Result BMCDebugger::debugOverGatesRange(const std::vector<ID> & gates,
                                                      unsigned k_min,
                                                      unsigned k_max)
    {
        Cube assumps = onlyTheseGates(gates);
        return debugWithAssumptions(assumps, k_min, k_max);
    }

    Debugger::Result BMCDebugger::debugAtK(unsigned k)
    {
        Cube assumps;
        return debugWithAssumptions(assumps, k, k);
    }

    Debugger::Result BMCDebugger::debugOverGatesAtK(const std::vector<ID> & gates, unsigned k)
    {
        Cube assumps = onlyTheseGates(gates);
        return debugWithAssumptions(assumps, k, k);
    }

    Debugger::Result BMCDebugger::debugOverGates(const std::vector<ID> & gates)
    {
        Cube assumps = onlyTheseGates(gates);
        return debugWithAssumptions(assumps, 0, m_kmax);
    }

    Debugger::Result BMCDebugger::debugAtKAndBlock(unsigned k)
    {
        return debugRangeAndBlock(k, k);
    }

    Debugger::Result
    BMCDebugger::debugOverGatesAtKAndBlock(const std::vector<ID> & gates, unsigned k)
    {
        return debugOverGatesRangeAndBlock(gates, k, k);
    }

    Debugger::Result BMCDebugger::debugRangeAndBlock(unsigned k_min, unsigned k_max)
    {
        Debugger::Result result = debugRange(k_min, k_max);

        if (result.first)
        {
            blockSolution(result.second);
        }

        return result;
    }

    Debugger::Result BMCDebugger::debugOverGatesRangeAndBlock(const std::vector<ID> & gates,
                                                              unsigned k_min, unsigned k_max)
    {
        Debugger::Result result = debugOverGatesRange(gates, k_min, k_max);

        if (result.first)
        {
            blockSolution(result.second);
        }

        return result;
    }

    Debugger::Result BMCDebugger::debug()
    {
        Cube assumps;
        return debugWithAssumptions(assumps, 0, m_kmax);
    }

    Debugger::Result BMCDebugger::debugWithAssumptions(const Cube & assumps,
                                                       unsigned k_min,
                                                       unsigned k_max)
    {
        Debugger::Result result;

        // Assume cardinality if needed
        Cube local_assumps = assumps;
        if (m_cardinality < CARDINALITY_INF)
        {
            Cube cardinality_assumps = m_cardinalityConstraint.assumeLEq(m_cardinality);
            local_assumps.insert(local_assumps.end(), cardinality_assumps.begin(),
                                                      cardinality_assumps.end());
        }

        // Solve
        SafetyResult bmc;
        bmc = m_solver.solveRange(k_min, k_max, local_assumps);

        if (bmc.result == UNSAFE)
        {
            result.first = true;
            result.second = extractSolution(bmc.cex);
            return result;
        }
        else
        {
            result.first = false;
            return result;
        }
    }

    void BMCDebugger::blockSolution(const std::vector<ID> & soln)
    {
        Clause block;

        for (ID id : soln)
        {
            ID debug_latch = m_tr.debugLatchForGate(id);
            block.push_back(negate(debug_latch));
        }

        m_solver.restrictInitialStates(block);
    }

    std::vector<ID> BMCDebugger::extractSolution(const SafetyCounterExample & cex) const
    {
        std::vector<ID> soln;
        assert(cex.size() > 0);
        Cube init = cex.at(0).state;

        for (ID latch : init)
        {
            if (is_negated(latch)) { continue; }
            if (m_debug_latches.count(latch) > 0)
            {
                ID gate_id = m_tr.gateForDebugLatch(latch);
                soln.push_back(gate_id);
            }
        }

        return soln;
    }

    Cube BMCDebugger::onlyTheseGates(const std::vector<ID> & gates) const
    {
        Cube assumps;
        std::set<ID> gate_dl_set;

        for (ID gate : gates)
        {
            ID dl = m_tr.debugLatchForGate(gate);
            gate_dl_set.insert(dl);
        }

        for (ID dl : m_debug_latches)
        {
            if (gate_dl_set.count(dl) == 0)
            {
                assumps.push_back(negate(dl));
            }
        }

        return assumps;
    }

}

