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

#ifndef BMC_SOLVER_H_INCLUDED
#define BMC_SOLVER_H_INCLUDED

#include "pme/safety.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/global_state.h"
#include "pme/engine/sat_adaptor.h"

namespace PME { namespace BMC {

    class BMCSolver
    {
        public:
            BMCSolver(VariableManager & varman,
                      const TransitionRelation & tr,
                      GlobalState & gs);

            SafetyResult solve(unsigned k_max);
            SafetyResult solve(unsigned k_max, const Cube & assumps);
            SafetyResult solveAtK(unsigned k);
            SafetyResult solveAtK(unsigned k, const Cube & assumps);

            void restrictInitialStates(const Clause & cls);
            void restrictInitialStates(const ClauseVec & vec);
            void clearRestrictions();

        private:
            void unroll(unsigned n);
            void initSolver();
            SafetyCounterExample extractTrace(unsigned k);
            Cube extract(Cube vars, unsigned k);

            VariableManager & m_vars;
            const TransitionRelation & m_tr;
            GlobalState & m_gs;
            SATAdaptor m_solver;
            unsigned m_numFrames;
            bool m_solverInited;
            std::vector<Clause> m_init_constraints;
    };

} }

#endif

