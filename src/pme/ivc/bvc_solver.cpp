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

    BVCSolver::BVCSolver(VariableManager & varman,
                         const TransitionRelation & tr,
                         unsigned level)
        : m_vars(varman),
          m_tr(tr),
          m_debug_tr(tr),
          m_cardinality_constraint(varman),
          m_cardinality(0),
          m_solver0_inited(false),
          m_solverN_inited(false),
          m_abstraction_frames(level)
    {
        m_cardinality_constraint.addInputs(m_debug_tr.begin_debug_latches(),
                                           m_debug_tr.end_debug_latches());
    }

    void BVCSolver::blockKnownSolutions(SATAdaptor & solver)
    {
        for (const BVCSolution & soln : m_blocked_solutions)
        {
            Clause block = blockingClause(soln);
            solver.addClause(block);
        }
    }

    void BVCSolver::unrollAbstraction(SATAdaptor & solver)
    {
        assert(!m_abstraction_gates.empty());

        std::vector<ID> abstraction(m_abstraction_gates.begin(), m_abstraction_gates.end());
        TransitionRelation abs_tr(m_tr, abstraction);
        for (unsigned i = 0; i < m_abstraction_frames; ++i)
        {
            solver.addClauses(abs_tr.unrollFrame(i));
        }

        solver.addClauses(abs_tr.initState());
    }

    void BVCSolver::initSolver0()
    {
        m_solver0.reset();

        if (m_abstraction_frames == 0)
        {
            m_solver0.addClauses(m_tr.initState());
        }
        else
        {
            unrollAbstraction(m_solver0);
        }

        // Add the final concrete frame (and primed form)
        m_solver0.addClauses(m_tr.unrollFrame(m_abstraction_frames));
        m_solver0.addClauses(m_tr.unrollFrame(m_abstraction_frames + 1));

        m_solver0_inited = true;
    }

    void BVCSolver::initSolverN()
    {
        m_solverN.reset();

        if (m_abstraction_frames == 0)
        {
            m_solverN.addClauses(m_debug_tr.initState());
        }
        else
        {
            unrollAbstraction(m_solverN);

            // Add "initial" state for debug latches
            ClauseVec debug_init(m_debug_tr.begin_debug_latches(),
                                 m_debug_tr.end_debug_latches());
            debug_init = primeClauses(debug_init, m_abstraction_frames);

            m_solverN.addClauses(debug_init);
        }

        // Add the final concrete frame (and primed form)
        m_solverN.addClauses(m_debug_tr.unrollFrame(m_abstraction_frames));
        m_solverN.addClauses(m_debug_tr.unrollFrame(m_abstraction_frames + 1));

        // Block known solutions (only applies to solverN, as solver0 finds
        // predecessors)
        blockKnownSolutions(m_solverN);

        m_solverN_inited = true;
    }

    void BVCSolver::initCardinality(unsigned n)
    {
        // Need to make a cardinality n+1 constraint to assume <= n
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
        return solve({m_tr.bad()});
    }

    BVCResult BVCSolver::solve(const Cube & target)
    {
        BVCResult result;

        result = solveAtCardinality(0, target);
        if (result.sat)
        {
            std::cout << "SAT at 0" << std::endl;
            assert(!result.predecessor.empty());
            return result;
        }

        result = solveAtCardinality(1, target);
        if (result.sat)
        {
            std::cout << "SAT at 1" << std::endl;
            assert(result.solution.size() == 1);
            return result;
        }

        return BVCResult();
    }

    bool BVCSolver::predecessorExists()
    {
        return solutionAtCardinality(0);
    }

    bool BVCSolver::solutionExists()
    {
        return solutionAtCardinality(CARDINALITY_INF);
    }

    bool BVCSolver::predecessorExists(const Cube & target)
    {
        return solutionAtCardinality(0, target);
    }

    bool BVCSolver::solutionExists(const Cube & target)
    {
        return solutionAtCardinality(CARDINALITY_INF, target);
    }

    bool BVCSolver::solutionAtCardinality(unsigned n)
    {
        BVCResult result = solveAtCardinality(n);
        return result.sat;
    }

    bool BVCSolver::solutionAtCardinality(unsigned n, const Cube & target)
    {
        BVCResult result = solveAtCardinality(n, target);
        return result.sat;
    }

    BVCResult BVCSolver::solveAtCardinality(unsigned n)
    {
        return solveAtCardinality(n, { m_tr.bad() });
    }

    BVCResult BVCSolver::solveAtCardinality(unsigned n, const Cube & target)
    {
        SATAdaptor & solver = (n == 0) ? m_solver0 : m_solverN;

        if (n == 0 && !solver0Ready()) { initSolver0(); }
        if (n > 0 && !solverNReady())  { initSolverN(); }

        // Assume the target state e.g., (bad') or (r_1' & r_2')
        Cube assumps = primeVec(target, m_abstraction_frames + 1);

        if (n > 0 && n != CARDINALITY_INF)
        {
            unsigned c = std::min((size_t) n, m_debug_tr.numSuspects());
            initCardinality(c);
            Cube cassumps = m_cardinality_constraint.assumeLEq(c);
            assumps.insert(assumps.end(), cassumps.begin(), cassumps.end());
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
            ID pdl_id = prime(dl_id, m_abstraction_frames);
            if (m_solverN.getAssignmentToVar(pdl_id) == SAT::TRUE)
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
            ID dl_id = m_debug_tr.debugLatchForGate(id);
            ID to_block = negate(prime(dl_id, m_abstraction_frames));
            blocking_clause.push_back(to_block);
        }

        return blocking_clause;
    }

    void BVCSolver::blockSolutionInSolvers(const BVCSolution & soln)
    {
        if (solverNReady())
        {
            Clause blocking_clause = blockingClause(soln);
            m_solverN.addClause(blocking_clause);
        }
    }
}

