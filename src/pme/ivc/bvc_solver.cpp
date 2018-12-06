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

#include "pme/ivc/bvc_solver.h"

#include <limits>
#include <cassert>

namespace PME {
    const unsigned CARDINALITY_INF = std::numeric_limits<unsigned>::max();

    BVCSolver::BVCSolver(VariableManager & varman, const TransitionRelation & tr)
        : m_vars(varman),
          m_tr(tr),
          m_debug_tr(tr),
          m_cardinality_constraint(varman),
          m_cardinality(0),
          m_solver0_inited(false),
          m_solverN_inited(false),
          m_abstraction_frames(0)
    {
        m_cardinality_constraint.addInputs(m_debug_tr.begin_debug_latches(),
                                           m_debug_tr.end_debug_latches());
    }

    void BVCSolver::initSolver0()
    {
        // TODO: handle abstraction
        assert(m_abstraction_frames == 0);

        m_solver0.reset();

        m_solver0.addClauses(m_tr.unrollFrame(0));
        m_solver0.addClauses(m_tr.unrollFrame(1));
        m_solver0.addClauses(m_tr.initState());

        m_solver0.addClause({prime(m_tr.bad())});

        m_solver0_inited = true;

        for (const BVCSolution & soln : m_blocked_solutions)
        {
            Clause block = blockingClause(soln);
            m_solver0.addClause(block);
        }
    }

    void BVCSolver::initSolverN()
    {
        // TODO: handle abstraction
        assert(m_abstraction_frames == 0);

        m_solverN.reset();

        m_solverN.addClauses(m_debug_tr.unrollFrame(0));
        m_solverN.addClauses(m_debug_tr.unrollFrame(1));
        m_solverN.addClauses(m_debug_tr.initState());

        m_solverN.addClause({prime(m_debug_tr.bad())});

        for (const BVCSolution & soln : m_blocked_solutions)
        {
            Clause block = blockingClause(soln);
            m_solverN.addClause(block);
        }

        m_solverN_inited = true;
    }

    void BVCSolver::initCardinality(unsigned n)
    {
        // Need to make a cardinality n+1 constraint to assume <= n
        // TODO: handle corner cases with very few gates
        m_cardinality_constraint.setCardinality(n + 1);
        m_solverN.addClauses(m_cardinality_constraint.CNFize());
    }

    void BVCSolver::setAbstraction(std::vector<ID> gates)
    {
        // TODO: if gates is a superset of m_abstraction_gates
        // we can just add the new gates to the solver.
        // Otherwise, we might have to clear and start over.
        m_abstraction_gates.clear();
        m_abstraction_gates.insert(gates.begin(), gates.end());

        markSolversDirty();
    }

    BVCResult BVCSolver::solve()
    {
        BVCResult result;
        result = solveAtCardinality(0);
        if (result.sat)
        {
            std::cout << "SAT at 0" << std::endl;
            return result;
        }

        result = solveAtCardinality(1);
        if (result.sat)
        {
            std::cout << "SAT at 1" << std::endl;
            assert(result.solution.size() == 1);
            return result;
        }

        return BVCResult();
    }

    bool BVCSolver::solutionExists()
    {
        BVCResult result = solveAtCardinality(CARDINALITY_INF);
        return result.sat;
    }

    BVCResult BVCSolver::solveAtCardinality(unsigned n)
    {
        SATAdaptor & solver = (n == 0) ? m_solver0 : m_solverN;

        if (n == 0 && !solver0Ready()) { initSolver0(); }
        if (n > 0 && !solverNReady())  { initSolverN(); }

        Cube assumps;

        if (n > 0 && n != CARDINALITY_INF)
        {
            initCardinality(n);
            if (n > m_cardinality_constraint.getOutputCardinality())
            {
                initCardinality(n);
            }
            assumps = m_cardinality_constraint.assumeLEq(n);
        }

        bool sat = solver.solve(assumps);

        // TODO: cleanup the weirdness surrounding predecessors, solvers, and
        // n = 0 versus n > 0
        if (sat && n == 0)
        {
            BVCPredecessor pred = extractPredecessor();
            BVCSolution empty_soln;
            return BVCResult(true, empty_soln, pred);
        }
        else if (sat && n > 0)
        {
            BVCPredecessor empty_pred;
            BVCSolution soln = extractSolution();
            return BVCResult(true, soln, empty_pred);
        }
        else
        {
            assert(!sat);
            return BVCResult();
        }
    }

    void BVCSolver::blockSolution(const BVCSolution & soln)
    {
        m_blocked_solutions.push_back(soln);
        blockSolutionInSolvers(soln);
    }

    BVCPredecessor BVCSolver::extractPredecessor() const
    {
        assert(m_solver0.isSAT());

        BVCPredecessor pred;
        for (auto it = m_tr.begin_latches(); it != m_tr.end_latches(); ++it)
        {
            ID latch = *it;
            ID platch = prime(latch, m_abstraction_frames);

            SAT::ModelValue val = m_solver0.getAssignmentToVar(platch);
            assert(val == SAT::TRUE || val == SAT::FALSE);
            bool neg = val == SAT::FALSE;

            ID lit = neg ? negate(latch) : latch;
            pred.push_back(lit);
        }

        return pred;
    }

    BVCSolution BVCSolver::extractSolution() const
    {
        assert(m_solverN.isSAT());

        BVCSolution soln;
        for (auto it = m_debug_tr.begin_debug_latches();
                  it != m_debug_tr.end_debug_latches(); ++it)
        {
            ID dl_id = *it;
            if (m_solverN.getAssignmentToVar(dl_id) == SAT::TRUE)
            {
                ID gate = m_debug_tr.gateForDebugLatch(dl_id);
                soln.push_back(gate);
            }
        }

        return soln;
    }

    Clause BVCSolver::blockingClause(const BVCSolution & soln) const
    {
        Clause blocking_clause;

        for (ID id : soln)
        {
            ID to_block = negate(prime(id, m_abstraction_frames));
            blocking_clause.push_back(to_block);
        }

        return blocking_clause;
    }

    void BVCSolver::blockSolutionInSolvers(const BVCSolution & soln)
    {
        Clause blocking_clause = blockingClause(soln);

        if (solver0Ready()) { m_solver0.addClause(blocking_clause); }
        if (solverNReady()) { m_solverN.addClause(blocking_clause); }
    }
}

