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

#ifndef DEBUG_TRANSITION_RELATION_H_INCLUDED
#define DEBUG_TRANSITION_RELATION_H_INCLUDED

#include "pme/engine/transition_relation.h"
#include "pme/util/cardinality_constraint.h"

#include <vector>
#include <unordered_map>

namespace PME {
    class DebugTransitionRelation : public TransitionRelation
    {
        public:
            DebugTransitionRelation(VariableManager & varman, aiger * aig);
            DebugTransitionRelation(VariableManager & varman, aiger * aig, unsigned property);
            DebugTransitionRelation(const TransitionRelation & tr);

            virtual ~DebugTransitionRelation() { }

            id_iterator begin_debug_latches() const { return m_debugLatchIDs.cbegin(); }
            id_iterator end_debug_latches() const { return m_debugLatchIDs.cend(); }
            id_iterator begin_debug_inputs() const { return m_debugPPIs.cbegin(); }
            id_iterator end_debug_inputs() const { return m_debugPPIs.cend(); }

            ID debugLatchForGate(ID id) const;
            ID debugPPIForGate(ID id) const;
            ID gateForDebugLatch(ID id) const;

            size_t numSuspects() const { return m_debugLatchIDs.size(); }

        private:
            std::vector<ID> m_debugLatchIDs;
            std::vector<ID> m_debugPPIs;
            std::unordered_map<ID, ID> m_IDToDebugLatch;
            std::unordered_map<ID, ID> m_debugLatchToID;
            std::unordered_map<ID, ID> m_IDToDebugPPI;

            void enhanceModel();
            void createDebugFor(ID gate);

        protected:
            virtual ClauseVec toCNF(const AndGate & gate) const override;
    };
}

#endif
