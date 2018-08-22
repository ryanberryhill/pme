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

#ifndef BMC_DEBUGGER_H_INCLUDED
#define BMC_DEBUGGER_H_INCLUDED

#include "pme/bmc/bmc_solver.h"
#include "pme/util/debugger.h"
#include "pme/util/cardinality_constraint.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/debug_transition_relation.h"
#include "pme/engine/global_state.h"
#include "pme/engine/sat_adaptor.h"

namespace PME {

    class BMCDebugger : public Debugger
    {
        public:
            BMCDebugger(VariableManager & varman,
                        const DebugTransitionRelation & tr);

            virtual void setCardinality(unsigned n) override;
            virtual void clearCardinality() override;

            virtual Result debug() override;
            virtual Result debugOverGates(const std::vector<ID> & gates) override;
            virtual void blockSolution(const std::vector<ID> & soln) override;

            virtual Result debugAtK(unsigned k);
            virtual Result debugOverGatesAtK(const std::vector<ID> & gates, unsigned k);
            virtual Result debugRange(unsigned k_min, unsigned k_max);
            virtual Result debugOverGatesRange(const std::vector<ID> & gates,
                                               unsigned k_min, unsigned k_max);

            Result debugAtKAndBlock(unsigned k);
            Result debugOverGatesAtKAndBlock(const std::vector<ID> & gates, unsigned k);
            Result debugRangeAndBlock(unsigned k_min, unsigned k_max);
            Result debugOverGatesRangeAndBlock(const std::vector<ID> & gates,
                                               unsigned k_min, unsigned k_max);

            void setKMax(unsigned k) { m_kmax = k; }

        private:
            Result debugWithAssumptions(const Cube & assumps, unsigned k_min, unsigned k_max);
            std::vector<ID> extractSolution(const SafetyCounterExample & cex) const;
            Cube onlyTheseGates(const std::vector<ID> & gates) const;

            VariableManager & m_vars;
            const DebugTransitionRelation & m_tr;

            unsigned m_kmax;
            unsigned m_cardinality;
            CardinalityConstraint m_cardinalityConstraint;
            std::set<ID> m_debug_latches;

            BMC::BMCSolver m_solver;
    };

}

#endif

