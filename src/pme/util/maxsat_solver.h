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
    class MaxSATSolver
    {
        public:
            virtual ~MaxSATSolver() { }
            virtual void addClause(const Clause & cls) = 0;
            virtual void addClauses(const ClauseVec & vec) = 0;
            virtual bool solve();
            virtual bool doSolve() = 0;
            virtual void addForOptimization(ID lit) = 0;
            virtual bool isSAT() const = 0;
            virtual ModelValue getAssignment(ID lit) const = 0;
            virtual ModelValue getAssignmentToVar(ID var) const = 0;
            const PMEOptions& opts() const;
    };

    class PBOMaxSATSolver : public MaxSATSolver
    {
        public:
            PBOMaxSATSolver(VariableManager & varman);
            virtual void addClause(const Clause & cls) override;
            virtual void addClauses(const ClauseVec & vec) override;
            virtual bool doSolve() override;
            virtual void addForOptimization(ID lit) override;
            virtual bool isSAT() const override;
            virtual ModelValue getAssignment(ID lit) const override;
            virtual ModelValue getAssignmentToVar(ID var) const override;

            bool assumpSolve(const Cube & assumps);

        private:
            unsigned lastCardinality(const Cube & assumps) const;
            void recordCardinality(const Cube & assumps, unsigned c);
            void initSolver();

            VariableManager & m_vars;
            SortingCardinalityConstraint m_cardinality;
            SATAdaptor m_solver;
            bool m_sat;
            bool m_solverInited;
            ClauseVec m_clauses;

            std::vector<ID> m_optimizationSet;
            std::map<Cube, unsigned> m_lastCardinality;
    };

    class MSU4MaxSATSolver : public MaxSATSolver
    {
        public:
            MSU4MaxSATSolver(VariableManager & varman);
            virtual void addClause(const Clause & cls) override;
            virtual void addClauses(const ClauseVec & vec) override;
            virtual bool doSolve() override;
            virtual void addForOptimization(ID lit) override;
            virtual bool isSAT() const override;
            virtual ModelValue getAssignment(ID lit) const override;
            virtual ModelValue getAssignmentToVar(ID var) const override;
        private:
            void reset();
            void resetCardinality();
            void resetSolver();
            std::set<ID> extractSolution() const;
            Cube extractCore(const Cube & crits, const std::set<ID> & initial_assumps) const;
            Cube addCardinality(const Cube & inputs, unsigned n);
            unsigned numSatisfied(const Cube & lits) const;

            bool m_isSAT;
            bool m_absoluteUNSAT;
            VariableManager & m_vars;
            std::set<ID> m_optimizationSet;
            SATAdaptor m_solver;
            ClauseVec m_clauses;
            unsigned m_solves;
            unsigned m_unsatRounds;
            unsigned m_lb, m_ub;
            std::set<ID> m_currentSoln;
            std::set<ID> m_initialAssumps;
            Cube m_blockedAssumps;

            Cube m_lastCardinalityInput;
            std::unique_ptr<CardinalityConstraint> m_cardinality;
    };
}

#endif

