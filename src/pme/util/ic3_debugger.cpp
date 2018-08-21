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

#include "pme/ic3/ic3.h"
#include "pme/util/ic3_debugger.h"

#include <limits>
#include <cassert>

namespace PME {
    const unsigned CARDINALITY_INF = std::numeric_limits<unsigned>::max();

    IC3Debugger::IC3Debugger(VariableManager & varman,
                             const DebugTransitionRelation & tr,
                             GlobalState & gs)
        : m_debug_tr(tr),
          m_ic3(varman, m_debug_tr, gs),
          m_gs(gs),
          m_cardinality(CARDINALITY_INF),
          m_cardinalityConstraint(varman)
    {
       clearCardinality();
       m_debug_latches.insert(m_debug_tr.begin_debug_latches(),
                              m_debug_tr.end_debug_latches());
       for (ID id : m_debug_latches)
       {
           m_cardinalityConstraint.addInput(id);
       }
    }

    void IC3Debugger::setupInitialStates()
    {
        m_ic3.clearRestrictions();
        if (m_cardinality < CARDINALITY_INF) { addCardinalityCNF(m_cardinality); }
        addBlockingClauses();
    }

    void IC3Debugger::setCardinality(unsigned n)
    {
        if (m_cardinality == n) { return; }

        unsigned old_cardinality = m_cardinality;
        m_cardinality = n;

        setupInitialStates();

        if (old_cardinality < n)
        {
            m_ic3.initialStatesExpanded();
        }
        else
        {
            m_ic3.initialStatesRestricted();
        }
    }

    void IC3Debugger::clearCardinality()
    {
        m_ic3.clearRestrictions();

        addBlockingClauses();

        if (m_cardinality < CARDINALITY_INF)
        {
            m_ic3.initialStatesExpanded();
        }

        m_cardinality = CARDINALITY_INF;
    }

    void IC3Debugger::addCardinalityCNF(unsigned n)
    {
        // Need to use (n+1) in order to assume <= n
        m_cardinalityConstraint.setCardinality(n + 1);
        m_cardinalityConstraint.clearIncrementality();

        ClauseVec cnf = m_cardinalityConstraint.CNFize();
        for (const Clause & cls : cnf)
        {
            m_ic3.restrictInitialStates(cls);
        }

        Cube assumps = m_cardinalityConstraint.assumeLEq(n);
        for (ID id : assumps)
        {
            Clause assump = {id};
            m_ic3.restrictInitialStates(assump);
        }
    }

    void IC3Debugger::addBlockingClauses()
    {
        for (const Clause & cls : m_blockingClauses)
        {
            m_ic3.restrictInitialStates(cls);
            m_ic3.addClausalLemma(cls, IC3::LEVEL_INF);
        }
    }

    Debugger::Result IC3Debugger::debug()
    {
        SafetyResult result = m_ic3.prove();

        if (result.result == SAFE)
        {
            return std::make_pair(false, std::vector<ID>());
        }
        else
        {
            assert(result.result == UNSAFE);
            SafetyCounterExample cex = result.cex;

            std::vector<ID> soln = extractSolution(cex);

            return std::make_pair(true, soln);
        }
    }

    Debugger::Result IC3Debugger::debugOverGates(const std::vector<ID> & gates)
    {
        // TODO try to find a good way to do this incrementally

        // Restrict to only these gates
        ClauseVec restrict = onlyTheseGates(gates);
        m_ic3.restrictInitialStates(restrict);
        // Add the clause to F_inf. NOTE: we assume that this is taken away
        // by initialStatesExpanded below.
        m_ic3.addClausalLemmas(restrict, IC3::LEVEL_INF);
        m_ic3.initialStatesRestricted();

        // Debug
        SafetyResult result = m_ic3.prove();
        Result debug_result;

        if (result.result == SAFE)
        {
            debug_result = std::make_pair(false, std::vector<ID>());
        }
        else
        {
            assert(result.result == UNSAFE);
            SafetyCounterExample cex = result.cex;

            std::vector<ID> soln = extractSolution(cex);

            debug_result = std::make_pair(true, soln);
        }

        // Clear restrictions
        setupInitialStates();
        m_ic3.initialStatesExpanded();

        return debug_result;
    }

    ClauseVec IC3Debugger::onlyTheseGates(const std::vector<ID> & gates) const
    {
        ClauseVec restrict;
        std::set<ID> gate_dl_set;

        for (ID gate : gates)
        {
            ID dl = m_debug_tr.debugLatchForGate(gate);
            gate_dl_set.insert(dl);
        }

        for (ID dl : m_debug_latches)
        {
            if (gate_dl_set.count(dl) == 0)
            {
                restrict.push_back({negate(dl)});
            }
        }

        return restrict;
    }

    void IC3Debugger::blockSolution(const std::vector<ID> & soln)
    {
        Clause block;

        for (ID id : soln)
        {
            ID debug_latch = m_debug_tr.debugLatchForGate(id);
            block.push_back(negate(debug_latch));
        }

        m_blockingClauses.push_back(block);
        m_ic3.restrictInitialStates(block);
        // Note: we assume the lemma we add below is taken away when we
        // take away the restriction on the initial states (such as by
        // calling initialStatesExpanded).
        m_ic3.addClausalLemma(block, IC3::LEVEL_INF);
        m_ic3.initialStatesRestricted();
    }

    std::vector<ID> IC3Debugger::extractSolution(const SafetyCounterExample & cex) const
    {
        std::vector<ID> soln;
        assert(cex.size() > 0);
        Cube init = cex.at(0).state;

        for (ID latch : init)
        {
            if (is_negated(latch)) { continue; }
            if (isDebugLatch(latch))
            {
                ID gate_id = m_debug_tr.gateForDebugLatch(latch);
                soln.push_back(gate_id);
            }
        }

        return soln;
    }

    bool IC3Debugger::isDebugLatch(ID latch) const
    {
        return m_debug_latches.count(strip(latch)) > 0;
    }

    IC3::LemmaID IC3Debugger::addLemma(const Cube & c, unsigned level)
    {
        return m_ic3.addLemma(c, level);
    }

    std::vector<Cube> IC3Debugger::getFrameCubes(unsigned n) const
    {
        return m_ic3.getFrameCubes(n);
    }

    unsigned IC3Debugger::numFrames() const
    {
        return m_ic3.numFrames();
    }

}

