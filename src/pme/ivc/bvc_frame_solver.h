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

#ifndef BVC_FRAME_SOLVER_H_INCLUDED
#define BVC_FRAME_SOLVER_H_INCLUDED

#include "pme/util/debugger.h"
#include "pme/util/cardinality_constraint.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/debug_transition_relation.h"
#include "pme/engine/sat_adaptor.h"

#include <set>
#include <tuple>

namespace PME {

    typedef std::vector<ID> BVCSolution;
    typedef std::vector<ID> BVCPredecessor;

    struct BVCBlockResult
    {
        typedef std::tuple<bool &, BVCSolution &, BVCPredecessor &> BVCBlockResultTuple;

        BVCBlockResult(bool sat, const BVCSolution & soln, const BVCPredecessor & pred)
            : sat(sat), solution(soln), predecessor(pred)
        { }

        BVCBlockResult() : sat(false)
        { }

        operator BVCBlockResultTuple()
        {
            return BVCBlockResultTuple(sat, solution, predecessor);
        }

        bool sat;
        BVCSolution solution;
        BVCPredecessor predecessor;
    };

    class BVCFrameSolver
    {
        public:
            BVCFrameSolver(VariableManager & varman,
                           const TransitionRelation & tr,
                           unsigned level);

            void setAbstraction(const std::vector<ID> & gates);
            const std::set<ID> & getAbstraction() { return m_abstraction_gates; }

            BVCBlockResult solve();
            BVCBlockResult solve(const Cube & target);

            BVCBlockResult solveAtCardinality(unsigned n);
            BVCBlockResult solveAtCardinality(unsigned n, const Cube & target);

            bool solutionExists();
            bool predecessorExists();
            bool solutionExists(const Cube & target);
            bool predecessorExists(const Cube & target);
            bool solutionAtCardinality(unsigned n);
            bool solutionAtCardinality(unsigned n, const Cube & target);
            void blockSolution(const BVCSolution & soln);

        private:
            void blockKnownSolutions(SATAdaptor & solver);
            void unrollAbstraction(SATAdaptor & solver);
            void initSolver0();
            void initSolverN();
            void initCardinality(unsigned n);

            void markSolver0Dirty() { m_solver0_inited = false; }
            void markSolverNDirty() { m_solverN_inited = false; }
            void markSolversDirty() { markSolver0Dirty(); markSolverNDirty(); }
            bool solver0Ready() const { return m_solver0_inited; }
            bool solverNReady() const { return m_solverN_inited; }

            BVCPredecessor extractPredecessor() const;
            BVCSolution extractSolution() const;
            void blockSolutionInSolvers(const BVCSolution & soln);
            Clause blockingClause(const BVCSolution & soln) const;

            bool abstractionIsSubsetOf(const std::vector<ID> & gates) const;

            VariableManager & m_vars;
            const TransitionRelation & m_tr;
            DebugTransitionRelation m_debug_tr;

            SortingCardinalityConstraint m_cardinality_constraint;
            unsigned m_cardinality;

            bool m_solver0_inited;
            bool m_solverN_inited;
            ClauseDeduplicatingSATAdaptor m_solver0;
            ClauseDeduplicatingSATAdaptor m_solverN;

            std::set<ID> m_abstraction_gates;
            std::vector<BVCSolution> m_blocked_solutions;
            unsigned m_abstraction_frames;
    };
}

#endif

