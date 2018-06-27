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

#ifndef FRAME_SOLVER_H_INCLUDED
#define FRAME_SOLVER_H_INCLUDED

#include "pme/ic3/ic3.h"
#include "pme/ic3/inductive_trace.h"
#include "pme/ic3/transition_relation_solver.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/global_state.h"
#include "pme/engine/sat_adaptor.h"

namespace PME { namespace IC3 {

    struct ConsecutionOptions {
        unsigned level;
        const Cube * c;
        Cube * core;
        Cube * pred;
        Cube * inp;
        Cube * pinp;

        ConsecutionOptions()
            : level(LEVEL_INF),
              c(nullptr),
              core(nullptr),
              pred(nullptr),
              inp(nullptr),
              pinp(nullptr)
        { }
    };

    class FrameSolver : public TransitionRelationSolver {
        public:
            // pred, inp, pinp, core
            typedef std::tuple<bool, Cube, Cube, Cube, Cube> ConsecutionResult;
            FrameSolver(VariableManager & varman,
                        const TransitionRelation & tr,
                        const InductiveTrace & trace,
                        GlobalState & gs = g_null_gs);

            void renewSAT();
            void addLemma(LemmaID id);

            ConsecutionResult consecutionFull(unsigned level, const Cube & c);
            bool consecutionCore(unsigned level, const Cube & c, Cube & core);
            bool consecutionPred(unsigned level, const Cube & c, Cube & pred);
            bool consecution(unsigned level, const Cube & c);
            bool consecution(ConsecutionOptions & opts);

            bool intersection(unsigned level, const Cube & c);

        private:
            Clause activatedClauseOf(LemmaID id);
            void sendFrame(unsigned level);
            void sendLemma(LemmaID id);
            ID levelAct(unsigned level);
            Cube extractCoreOf(const Cube & c, const Cube & crits) const;
            Cube levelAssumps(unsigned level);
            Cube extractPredecessor() const;
            Cube extractInputs() const;
            Cube extractPrimedInputs() const;
            Cube extract(std::vector<ID> vars, unsigned nprimes = 0) const;

            const InductiveTrace & m_trace;
            bool m_solverInited;
            std::vector<ID> m_activation;
    };

} }

#endif

