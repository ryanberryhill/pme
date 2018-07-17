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

#ifndef DEBUG_IC3_SOLVER_H_INCLUDED
#define DEBUG_IC3_SOLVER_H_INCLUDED

#include "pme/pme.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/debug_transition_relation.h"
#include "pme/engine/global_state.h"
#include "pme/ic3/ic3_solver.h"

namespace PME { namespace IC3 {

    class IC3Debugger
    {
        public:
            typedef std::pair<bool, std::vector<ID>> Result;
            IC3Debugger(VariableManager & varman,
                        DebugTransitionRelation & tr,
                        GlobalState & gs);

            void setCardinality(unsigned n);
            void clearCardinality();

            Result debug();
            Result debugAndBlock();

            void blockSolution(const std::vector<ID> & soln);

            LemmaID addLemma(const Cube & c, unsigned level);
            std::vector<Cube> getFrameCubes(unsigned n) const;
            unsigned numFrames() const;

        private:
            std::vector<ID> extractSolution(const SafetyCounterExample & cex) const;
            bool isDebugLatch(ID latch) const;

            DebugTransitionRelation & m_debug_tr;
            IC3Solver m_ic3;
            GlobalState & m_gs;
            unsigned m_lastCardinality;
            std::set<ID> m_debug_latches;
    };

} }

#endif

