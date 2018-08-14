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

#ifndef MARCO_IVC_INCLUDED
#define MARCO_IVC_INCLUDED

#include "pme/ivc/ivc.h"
#include "pme/util/maxsat_solver.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/debug_transition_relation.h"

namespace PME {
    class MARCOIVCFinder : public IVCFinder {
        public:
            MARCOIVCFinder(VariableManager & varman,
                           const TransitionRelation & tr,
                           GlobalState & gs);
            virtual void findIVCs() override;
        protected:
            virtual std::ostream & log(int verbosity) const override;
        private:
            typedef std::vector<ID> Seed;
            typedef std::pair<bool, Seed> UnexploredResult;

            UnexploredResult getUnexplored();
            void recordMIVC(const Seed & mivc);
            bool isSafe(const Seed & seed);
            void grow(Seed & seed);
            void shrink(Seed & seed);
            void blockUp(const Seed & seed);
            void blockDown(const Seed & seed);

            void initSolvers();
            ID debugVarOf(ID gate) const;

            id_iterator begin_gates() const { return m_debug_tr.begin_gate_ids(); }
            id_iterator end_gates() const { return m_debug_tr.end_gate_ids(); }

            MaxSATSolver m_seedSolver;
            DebugTransitionRelation m_debug_tr;
            Seed m_smallestIVC;
            std::vector<ID> m_gates;
    };
}

#endif
