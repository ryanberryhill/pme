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

#include "pme/ivc/bvc_frame_solver.h"

#include <limits>
#include <cassert>
#include <algorithm>

namespace PME {
    const unsigned CARDINALITY_INF = std::numeric_limits<unsigned>::max();

    BVCFrameSolver::BVCFrameSolver(VariableManager & varman,
                         const TransitionRelation & tr,
                         unsigned abstracted_frames)
        : m_vars(varman),
          m_tr(tr),
          m_debug_tr(tr),
          m_cardinality_constraint(varman),
          m_solver0_inited(false),
          m_solverN_inited(false),
          m_abstraction_frames(abstracted_frames)
    {
        m_cardinality_constraint.addInputs(m_debug_tr.begin_debug_latches(),
                                           m_debug_tr.end_debug_latches());
    }

    void BVCFrameSolver::blockKnownSolutions(SATAdaptor & solver)
    {
        for (const BVCSolution & soln : m_blocked_solutions)
        {
            Clause block = blockingClause(soln);
            solver.addClause(block);
        }
    }

    void BVCFrameSolver::unrollAbstraction(SATAdaptor & solver)
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

    void BVCFrameSolver::initSolver0()
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

    void BVCFrameSolver::initSolverN()
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

    void BVCFrameSolver::initCardinality(unsigned n)
    {
        // Need to make a cardinality n+1 constraint to assume <= n
        m_cardinality_constraint.setCardinality(n + 1);
        m_solverN.addClauses(m_cardinality_constraint.CNFize());
    }

    void BVCFrameSolver::setAbstraction(const std::vector<ID> & gates)
    {
        std::set<ID> gate_set(gates.begin(), gates.end());
        setAbstraction(gate_set);
    }

    void BVCFrameSolver::setAbstraction(const std::set<ID> & gates)
    {
        // TODO: add a config for this behavior
        // Check if the new abstraction is a superset of the old
        bool is_subset = abstractionIsSubsetOf(gates);
        m_abstraction_gates = gates;

        if (is_subset && !gates.empty())
        {
            // Just add the new gates instead of restarting
            unrollAbstraction(m_solver0);
            unrollAbstraction(m_solverN);
        }
        else
        {
            markSolversDirty();
        }
    }

    bool BVCFrameSolver::abstractionIsSubsetOf(const std::set<ID> & gates) const
    {
        return std::includes(gates.begin(), gates.end(),
                             m_abstraction_gates.begin(), m_abstraction_gates.end());
    }

    bool BVCFrameSolver::predecessorExists(const Cube & target)
    {
        return solutionAtCardinality(0, target);
    }

    bool BVCFrameSolver::solutionExists(const Cube & target)
    {
        return solutionAtCardinality(CARDINALITY_INF, target);
    }

    bool BVCFrameSolver::solutionAtCardinality(unsigned n, const Cube & target)
    {
        BVCBlockResult result = solve(n, target);
        return result.sat;
    }

    bool BVCFrameSolver::predecessorExistsUnprimed(const Cube & target)
    {
        return solutionAtCardinalityUnprimed(0, target);
    }

    bool BVCFrameSolver::solutionExistsUnprimed(const Cube & target)
    {
        return solutionAtCardinalityUnprimed(CARDINALITY_INF, target);
    }

    bool BVCFrameSolver::solutionAtCardinalityUnprimed(unsigned n, const Cube & target)
    {
        BVCBlockResult result = solve(n, target, false);
        return result.sat;
    }

    BVCBlockResult BVCFrameSolver::solve(unsigned n, const Cube & target)
    {
        return solve(n, target, true);
    }

    BVCBlockResult BVCFrameSolver::solveUnprimed(unsigned n, const Cube & target)
    {
        return solve(n, target, false);
    }

    BVCBlockResult BVCFrameSolver::solve(unsigned n, const Cube & target, bool prime)
    {
        SATAdaptor & solver = (n == 0) ? m_solver0 : m_solverN;

        if (n == 0 && !solver0Ready()) { initSolver0(); }
        if (n > 0 && !solverNReady())  { initSolverN(); }

        // Assume the target state e.g., (bad') or (r_1' & r_2')
        Cube assumps = prime ? primeVec(target, m_abstraction_frames + 1) : target;

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
            BVCBlockResult result(true, empty_soln, pred);

            result.inputs = extractInputs();
            result.state = pred;
            result.inputs = extractPrimedInputs();

            return result;
        }
        else if (sat && n > 0)
        {
            BVCPredecessor empty_pred;
            BVCSolution soln = extractSolution();
            return BVCBlockResult(true, soln, empty_pred);
        }
        else
        {
            assert(!sat);
            return BVCBlockResult();
        }
    }

    void BVCFrameSolver::blockSolution(const BVCSolution & soln)
    {
        m_blocked_solutions.push_back(soln);
        blockSolutionInSolvers(soln);
    }

    BVCPredecessor BVCFrameSolver::extractPredecessor() const
    {
        assert(m_solver0.isSAT());
        Cube platches(m_tr.begin_latches(), m_tr.end_latches());
        platches = primeVec(platches, m_abstraction_frames);
        return extract(m_solver0, platches);
    }

    Cube BVCFrameSolver::extractInputs() const
    {
        assert(m_solver0.isSAT());
        Cube pinp(m_tr.begin_inputs(), m_tr.end_inputs());
        pinp = primeVec(pinp, m_abstraction_frames);
        return extract(m_solver0, pinp);
    }

    Cube BVCFrameSolver::extractPrimedInputs() const
    {
        assert(m_solver0.isSAT());
        Cube pinp(m_tr.begin_inputs(), m_tr.end_inputs());
        pinp = primeVec(pinp, m_abstraction_frames + 1);
        return extract(m_solver0, pinp);
    }

    Cube BVCFrameSolver::extract(const SATAdaptor & solver, const Cube & vars) const
    {
        assert(solver.isSAT());

        Cube ext;
        for (ID var : vars)
        {
            SAT::ModelValue val = solver.getAssignmentToVar(var);
            assert(val == SAT::TRUE || val == SAT::FALSE);
            bool neg = val == SAT::FALSE;

            ID lit = neg ? negate(var) : var;
            ext.push_back(lit);
        }

        ext = unprimeVec(ext);
        std::sort(ext.begin(), ext.end());
        return ext;
    }

    BVCSolution BVCFrameSolver::extractSolution() const
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

        std::sort(soln.begin(), soln.end());
        return soln;
    }

    Clause BVCFrameSolver::blockingClause(const BVCSolution & soln) const
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

    void BVCFrameSolver::blockSolutionInSolvers(const BVCSolution & soln)
    {
        if (solverNReady())
        {
            Clause blocking_clause = blockingClause(soln);
            m_solverN.addClause(blocking_clause);
        }
    }
}

