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

#ifndef UNSAT_CORE_LIFTER_H_INCLUDED
#define UNSAT_CORE_LIFTER_H_INCLUDED

#include "pme/ic3/ic3.h"
#include "pme/ic3/inductive_trace.h"
#include "pme/ic3/transition_relation_solver.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/global_state.h"
#include "pme/engine/sat_adaptor.h"

namespace PME { namespace IC3 {

    struct LiftOptions {
        const Cube & pred;
        const Cube & succ;
        const Cube & inp;
        const Cube & pinp;

        LiftOptions(const Cube & pred,
                    const Cube & succ,
                    const Cube & inp,
                    const Cube & pinp)
            : pred(pred),
              succ(succ),
              inp(inp),
              pinp(pinp)
        { }
    };

    class UNSATCoreLifter : public TransitionRelationSolver {
        public:
            UNSATCoreLifter(VariableManager & varman,
                            const TransitionRelation & tr,
                            const InductiveTrace & trace,
                            GlobalState & gs = g_null_gs);

            void renewSAT();
            void addLemma(LemmaID id);

            Cube lift(const LiftOptions & opts);

        private:
            void sendLemma(LemmaID id);

            VariableManager & m_vars;
            const InductiveTrace & m_trace;
            GlobalState & m_gs;
            bool m_solverInited;
    };

} }

#endif

