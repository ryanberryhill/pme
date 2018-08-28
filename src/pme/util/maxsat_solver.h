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

#ifndef MAXSAT_SOLVER_H
#define MAXSAT_SOLVER_H

#include "pme/pme.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/sat_adaptor.h"
#include "pme/engine/global_state.h"
#include "pme/util/cardinality_constraint.h"

#include <set>
#include <map>

namespace PME
{
    class PBOMaxSATSolver
    {
        public:
            PBOMaxSATSolver(VariableManager & varman);
            void addClause(const Clause & cls);
            void addClauses(const ClauseVec & vec);
            bool solve();
            bool solve(const Cube & assumps);
            void addForOptimization(ID lit);
            bool isSAT() const;
            ModelValue getAssignment(ID lit) const;
            ModelValue getAssignmentToVar(ID var) const;

        private:
            unsigned lastCardinality(const Cube & assumps) const;
            void recordCardinality(const Cube & assumps, unsigned c);
            void initSolver();

            VariableManager m_vars;
            SortingCardinalityConstraint m_cardinality;
            SATAdaptor m_solver;
            bool m_sat;
            bool m_solverInited;
            ClauseVec m_clauses;

            std::multiset<ID> m_optimizationSet;
            std::map<Cube, unsigned> m_lastCardinality;
    };
}

#endif

