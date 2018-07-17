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
#include <limits>

namespace PME {

    std::string debugPPIName(ExternalID id)
    {
        std::ostringstream ss;
        ss << "di_" << id;
        return ss.str();
    }

    std::string debugLatchName(ExternalID id)
    {
        std::ostringstream ss;
        ss << "dl_" << id;
        return ss.str();
    }

    DebugTransitionRelation::DebugTransitionRelation(VariableManager & varman, aiger * aig)
        : TransitionRelation(varman, aig)
    {
       enhanceModel();
    }

    DebugTransitionRelation::DebugTransitionRelation(VariableManager & varman,
                                                     aiger * aig,
                                                     unsigned property)
        : TransitionRelation(varman, aig, property)
    {
        enhanceModel();
    }

    DebugTransitionRelation::DebugTransitionRelation(const TransitionRelation & tr)
        : TransitionRelation(tr)
    {
        enhanceModel();
    }

    void DebugTransitionRelation::enhanceModel()
    {
        for (auto it = begin_gates(); it != end_gates(); ++it)
        {
            createDebugFor(*it);
        }
    }

    ID DebugTransitionRelation::debugPPIForGate(ID id) const
    {
        assert(m_IDToDebugPPI.count(id) > 0);
        return m_IDToDebugPPI.at(id);
    }

    ID DebugTransitionRelation::debugLatchForGate(ID id) const
    {
        assert(m_IDToDebugLatch.count(id) > 0);
        return m_IDToDebugLatch.at(id);
    }

    ID DebugTransitionRelation::gateForDebugLatch(ID id) const
    {
        assert(m_debugLatchToID.count(id) > 0);
        return m_debugLatchToID.at(id);
    }

    void DebugTransitionRelation::createDebugFor(const AndGate & gate)
    {
        ID lhs = gate.lhs;
        assert(m_IDToDebugLatch.count(lhs) == 0);

        std::string dl_name = debugLatchName(toExternal(lhs));
        const Variable& dl_var = createInternalVar(dl_name);
        ID debug_latch = dl_var.id;
        assert(m_debugLatchToID.count(debug_latch) == 0);
        assert(m_IDToDebugLatch.count(lhs) == 0);

        createLatch(debug_latch, debug_latch, ID_NULL);

        std::string di_name = debugPPIName(toExternal(lhs));
        const Variable& di_var = createInternalVar(di_name);
        ID debug_input = di_var.id;
        assert(m_IDToDebugPPI.count(lhs) == 0);

        createInput(debug_input);

        m_debugLatchIDs.push_back(debug_latch);
        m_debugPPIs.push_back(debug_input);

        m_IDToDebugLatch[lhs] = debug_latch;
        m_debugLatchToID[debug_latch] = lhs;

        m_IDToDebugPPI[lhs] = debug_input;
    }

    ID DebugTransitionRelation::debugLatchFor(const AndGate & gate) const
    {
        ID lhs = gate.lhs;
        assert(m_IDToDebugLatch.count(lhs) > 0);

        return m_IDToDebugLatch.at(lhs);
    }

    ID DebugTransitionRelation::debugPPIFor(const AndGate & gate) const
    {
        ID lhs = gate.lhs;
        return debugPPIForGate(lhs);
    }

    ClauseVec DebugTransitionRelation::toCNF(const AndGate & gate) const
    {
        ID debug_latch = debugLatchFor(gate);
        ClauseVec clauses = TransitionRelation::toCNF(gate);

        for (Clause & cls : clauses)
        {
            cls.push_back(debug_latch);
        }

        // Also need lhs = input if dl = 1
        ID lhs = gate.lhs;
        ID ppi = debugPPIFor(gate);

        clauses.push_back({lhs, negate(ppi), negate(debug_latch)});
        clauses.push_back({negate(lhs), ppi, negate(debug_latch)});

        return clauses;
    }
}
