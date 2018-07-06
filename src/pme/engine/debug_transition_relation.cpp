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

#include "pme/engine/debug_transition_relation.h"

#include <sstream>
#include <cassert>

namespace PME {

    std::string debugName(ExternalID id)
    {
        std::ostringstream ss;
        ss << "debug_" << id;
        return ss.str();
    }

    DebugTransitionRelation::DebugTransitionRelation(VariableManager & varman, aiger * aig)
        : TransitionRelation(varman, aig),
          m_cardinalityConstraint(varman),
          m_cardinality(0)
    {
       enhanceModel();
    }

    DebugTransitionRelation::DebugTransitionRelation(VariableManager & varman,
                                                     aiger * aig,
                                                     unsigned property)
        : TransitionRelation(varman, aig, property),
          m_cardinalityConstraint(varman),
          m_cardinality(0)
    {
        enhanceModel();
    }

    DebugTransitionRelation::DebugTransitionRelation(const TransitionRelation & tr)
        : TransitionRelation(tr),
          m_cardinalityConstraint(vars()),
          m_cardinality(0)
    {
        enhanceModel();
    }

    void DebugTransitionRelation::enhanceModel()
    {
        for (auto it = begin_gates(); it != end_gates(); ++it)
        {
            createDebugLatchFor(*it);
        }

        setCardinality(1);
    }

    void DebugTransitionRelation::setCardinality(unsigned n)
    {
        m_cardinality = n;
        // Need (n+1) to assume cardinality <= n (or = n)
        m_cardinalityConstraint.setCardinality(n + 1);
        ClauseVec new_clauses = m_cardinalityConstraint.CNFize();
        m_cardinalityClauses.insert(m_cardinalityClauses.end(),
                                    new_clauses.begin(), new_clauses.end());
    }

    void DebugTransitionRelation::createDebugLatchFor(const AndGate & gate)
    {
        ID lhs = gate.lhs;
        assert(m_IDToDebugLatch.count(lhs) == 0);

        std::string name = debugName(toExternal(lhs));
        const Variable& var = createInternalVar(name);
        ID debug_latch = var.id;

        createLatch(debug_latch, debug_latch, ID_NULL);

        m_debugLatchIDs.push_back(debug_latch);
        m_IDToDebugLatch[lhs] = debug_latch;

        m_cardinalityConstraint.addInput(debug_latch);
    }

    ID DebugTransitionRelation::debugLatchFor(const AndGate & gate) const
    {
        ID lhs = gate.lhs;
        assert(m_IDToDebugLatch.count(lhs) > 0);

        return m_IDToDebugLatch.at(lhs);
    }

    ClauseVec DebugTransitionRelation::toCNF(const AndGate & gate) const
    {
        ID debug_latch = debugLatchFor(gate);
        ClauseVec clauses = TransitionRelation::toCNF(gate);

        for (Clause & cls : clauses)
        {
            cls.push_back(debug_latch);
        }

        return clauses;
    }

    ClauseVec DebugTransitionRelation::initState() const
    {
        ClauseVec init = TransitionRelation::initState();
        init.insert(init.end(), m_cardinalityClauses.begin(), m_cardinalityClauses.end());

        Cube assumps = m_cardinalityConstraint.assumeLEq(m_cardinality);
        for (ID id : assumps)
        {
            init.push_back({id});
        }

        return init;
    }
}
