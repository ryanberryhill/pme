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

#include "pme/util/maxsat_solver.h"
#include "pme/util/timer.h"
#include "pme/engine/global_state.h"

#include <cassert>
#include <algorithm>

namespace PME
{
    bool MaxSATSolver::solve()
    {
        GlobalState::stats().maxsat_calls++;
        AutoTimer timer(GlobalState::stats().maxsat_runtime);

        return doSolve();
    }

    bool MaxSATSolver::check(const Cube & assumps)
    {
        // TODO: check probably shouldn't count as MaxSAT runtime
        GlobalState::stats().maxsat_calls++;
        AutoTimer timer(GlobalState::stats().maxsat_runtime);

        return doCheck(assumps);
    }

    const PMEOptions& MaxSATSolver::opts() const
    {
        return GlobalState::options();
    }

    PBOMaxSATSolver::PBOMaxSATSolver(VariableManager & varman) :
        m_vars(varman),
        m_cardinality(varman),
        m_sat(false),
        m_solverInited(false)
    { }

    void PBOMaxSATSolver::initSolver()
    {
        m_solver.addClauses(m_clauses);

        // TODO: avoid needing to do the full size up front
        if (!m_optimizationSet.empty())
        {
            m_cardinality.setCardinality(m_optimizationSet.size());
            m_solver.addClauses(m_cardinality.CNFize());
        }

        m_solverInited = true;
    }

    void PBOMaxSATSolver::addClause(const Clause & cls)
    {
        m_sat = false;
        m_clauses.push_back(cls);

        if (m_solverInited) { m_solver.addClause(cls); }
    }

    void PBOMaxSATSolver::addClauses(const ClauseVec & vec)
    {
        m_clauses.reserve(m_clauses.size() + vec.size());
        for (const Clause & cls : vec)
        {
            addClause(cls);
        }
    }

    void PBOMaxSATSolver::addForOptimization(ID lit)
    {
        m_lastCardinality.clear();
        m_cardinality.addInput(lit);
        m_optimizationSet.push_back(lit);

        if (m_solverInited)
        {
            m_solver.reset();
            m_solverInited = false;
        }
    }

    unsigned PBOMaxSATSolver::lastCardinality(const Cube & assumps) const
    {
        assert(std::is_sorted(assumps.begin(), assumps.end()));
        auto it = m_lastCardinality.find(assumps);
        if (it == m_lastCardinality.end())
        {
            return m_optimizationSet.size();
        }
        else
        {
            return it->second;
        }
    }

    void PBOMaxSATSolver::recordCardinality(const Cube & assumps, unsigned c)
    {
        assert(std::is_sorted(assumps.begin(), assumps.end()));
        m_lastCardinality[assumps] = c;
    }

    bool PBOMaxSATSolver::doSolve()
    {
        std::vector<ID> assumps;
        return assumpSolve(assumps);
    }

    bool PBOMaxSATSolver::doCheck(const Cube & assumps)
    {
        if (!m_solverInited) { initSolver(); }
        return m_solver.solve(assumps);
    }

    bool PBOMaxSATSolver::assumpSolve(const Cube & assumps)
    {
        if (!m_solverInited) { initSolver(); }

        Cube assumps_sorted = assumps;
        std::sort(assumps_sorted.begin(), assumps_sorted.end());

        unsigned cardinality = lastCardinality(assumps_sorted);

        bool sat = false;

        // Do a linear search for the maximum cardinality that is SAT
        while (!sat)
        {
            Cube solver_assumps = m_cardinality.assumeGEq(cardinality);
            solver_assumps.insert(solver_assumps.end(), assumps.begin(), assumps.end());

            sat = m_solver.solve(solver_assumps);

            if (!sat)
            {
                if (cardinality > 0)
                {
                    cardinality--;
                }
                else { break; }
            }

        }

        m_sat = sat;
        recordCardinality(assumps_sorted, cardinality);

        return m_sat;
    }

    bool PBOMaxSATSolver::isSAT() const
    {
        return m_sat;
    }

    ModelValue PBOMaxSATSolver::getAssignment(ID lit) const
    {
        assert(isSAT());
        return m_solver.getAssignment(lit);
    }

    ModelValue PBOMaxSATSolver::getAssignmentToVar(ID var) const
    {
        assert(isSAT());
        return m_solver.getAssignmentToVar(var);
    }

    ModelValue PBOMaxSATSolver::safeGetAssignment(ID lit) const
    {
        assert(isSAT());
        return m_solver.safeGetAssignment(lit);
    }

    ModelValue PBOMaxSATSolver::safeGetAssignmentToVar(ID var) const
    {
        assert(isSAT());
        return m_solver.safeGetAssignmentToVar(var);
    }

    MSU4MaxSATSolver::MSU4MaxSATSolver(VariableManager & varman)
        : m_isSAT(false),
          m_absoluteUNSAT(false),
          m_vars(varman),
          m_solves(0),
          m_unsatRounds(0),
          m_lb(0),
          m_ub(std::numeric_limits<unsigned>::max())
    { }

    void MSU4MaxSATSolver::addClause(const Clause & cls)
    {
        m_clauses.push_back(cls);
        m_solver.addClause(cls);

        m_isSAT = false;
        m_lb = 0;
        m_currentSoln.clear();
    }

    void MSU4MaxSATSolver::resetCardinality()
    {
        m_cardinality.reset();
        m_lastCardinalityInput.clear();
    }

    void MSU4MaxSATSolver::reset()
    {
        // Reset incremental cardinality
        resetCardinality();

        // Reset existing lower bound and solution
        m_isSAT = false;
        m_lb = 0;
        m_currentSoln.clear();

        // Reset upper bound
        m_ub = std::numeric_limits<unsigned>::max();

        // Reset current state
        m_blockedAssumps.clear();
        m_initialAssumps = m_optimizationSet;
        m_solves = 0;
        m_unsatRounds = 0;
        m_absoluteUNSAT = false;
    }

    void MSU4MaxSATSolver::resetSolver()
    {
        resetCardinality();
        m_solver.reset();
        for (const Clause & cls : m_clauses)
        {
            m_solver.addClause(cls);
        }
    }

    void MSU4MaxSATSolver::addClauses(const ClauseVec & vec)
    {
        m_clauses.reserve(m_clauses.size() + vec.size());
        for (const Clause & cls : vec)
        {
            addClause(cls);
        }
    }

    std::set<ID> MSU4MaxSATSolver::extractSolution() const
    {
        std::set<ID> soln;

        for (ID lit : m_optimizationSet)
        {
            if (m_solver.getAssignment(lit) == SAT::TRUE)
            {
                soln.insert(lit);
            }
        }

        return soln;
    }

    Cube MSU4MaxSATSolver::extractCore(const Cube & crits,
                                       const std::set<ID> & initial_assumps) const
    {
        Cube core;

        for (ID crit_lit : crits)
        {
            if (initial_assumps.count(crit_lit) > 0)
            {
                core.push_back(crit_lit);
            }
        }

        return core;
    }

    Cube MSU4MaxSATSolver::addCardinality(const Cube & inputs, unsigned n)
    {
        assert(!inputs.empty());
        unsigned neg_n = inputs.size() - n;
        Cube sorted = inputs;
        std::sort(sorted.begin(), sorted.end());

        if (sorted == m_lastCardinalityInput && m_cardinality->getOutputCardinality() > neg_n)
        {
            return m_cardinality->assumeLT(neg_n);
        }
        else
        {
            if (neg_n < n * 8)
            {
                m_lastCardinalityInput = sorted;

                // sum(!a for a in inputs) < |inputs| - n
                Cube neg = negateVec(inputs);

                m_cardinality.reset(new SortingLEqConstraint(m_vars));
                m_cardinality->addInputs(neg.begin(), neg.end());
                m_cardinality->setCardinality(neg_n + 1);

                m_solver.addClauses(m_cardinality->CNFize());
                return m_cardinality->assumeLT(neg_n);
            }
            else
            {
                // sum(inputs) > n

                // In this case, don't do incrementality because we can't just
                // keep incrementally weakening the constraint (but for the <=
                // case above, we can incrementally strengthen).
                m_lastCardinalityInput.clear();

                SortingGEqConstraint cardinality(m_vars);
                cardinality.addInputs(inputs.begin(), inputs.end());
                cardinality.setCardinality(n + 1);

                m_solver.addClauses(cardinality.CNFize());
                return cardinality.assumeGT(n);
            }
        }
    }

    unsigned MSU4MaxSATSolver::numSatisfied(const Cube & lits) const
    {
        unsigned count = 0;

        for (ID lit : lits)
        {
            if (m_solver.getAssignment(lit) == SAT::TRUE)
            {
                count++;
            }
        }

        return count;
    }

    bool MSU4MaxSATSolver::doSolve()
    {
        // Reset solver occasionally (for performance)
        // TODO: some kind of better heursitic (maybe the number of cardinality
        // constraints added that are no longer needed).
        if (m_solves > opts().msu4_reset_all_period)
        {
            resetSolver();
            reset();
        }
        else if (m_solves % opts().msu4_reset_solver_period == 0)
        {
            resetSolver();
        }

        m_solves++;

        // Check if the hard clauses are satisfiable
        if (m_absoluteUNSAT || !m_solver.solve())
        {
            m_absoluteUNSAT = true;
            return false;
        }

        Cube cardinality_assumps;
        ID hint_lit = m_vars.getNewID("hint");

        while (true)
        {
            Cube assumps(m_initialAssumps.begin(), m_initialAssumps.end());
            assumps.insert(assumps.end(), cardinality_assumps.begin(),
                                          cardinality_assumps.end());

            if (opts().msu4_use_hint_clauses) { assumps.push_back(hint_lit); }

            Cube crits;
            bool sat = m_solver.solve(assumps, &crits);

            if (sat)
            {
                std::set<ID> soln = extractSolution();

                // Update the current solution and lower bound if the new one
                // is better
                if (m_currentSoln.size() < soln.size())
                {
                    assert(m_lb == m_currentSoln.size());
                    m_currentSoln = soln;
                    m_lb = soln.size();
                }

                // Put in a cardinality constraint that asks if we can find
                // a better solution. Let n = |{satisfied blocked_assumps}|.
                // We want sum(b_a) > n or equivalently
                //         sum(!a for a in b_a) < |b_a| - n
                unsigned n = numSatisfied(m_blockedAssumps);
                if (n == m_blockedAssumps.size())
                {
                    // All clauses are satsifiable
                    m_isSAT = true;
                    return true;
                }
                cardinality_assumps = addCardinality(m_blockedAssumps, n);
            }
            else
            {
                Cube core = extractCore(crits, m_initialAssumps);

                if (core.empty())
                {
                    // The highest computed lower bound is a solution
                    m_isSAT = true;
                    return true;
                }
                else
                {
                    // Subtract core from initial assumptions and add to
                    // blocked assumptions
                    for (ID core_lit : core)
                    {
                        m_initialAssumps.erase(core_lit);
                        m_blockedAssumps.push_back(core_lit);
                    }

                    if (opts().msu4_use_hint_clauses)
                    {
                        // Add optional hint clause saying one literal from
                        // ~core must be active. This clause can't persist
                        // incrementally so we add an activation variable.
                        Clause hint_cls = negateVec(core);
                        hint_cls.push_back(negate(hint_lit));
                        m_solver.addClause(hint_cls);
                    }

                    // Update upper bound
                    m_unsatRounds++;
                    assert(m_ub > m_optimizationSet.size() - m_unsatRounds);
                    m_ub = m_optimizationSet.size() - m_unsatRounds;
                }
            }

            if (m_lb == m_ub)
            {
                // We already have a solution
                m_isSAT = true;
                return true;
            }
        }

        assert(false);
        return false;
    }

    bool MSU4MaxSATSolver::doCheck(const Cube & assumps)
    {
        // Don't count as a solve, as it doesn't add cardinality constraints
        // or the like
        return m_solver.solve(assumps);
    }

    void MSU4MaxSATSolver::addForOptimization(ID lit)
    {
        // We don't support passing the same literal twice for simplicity
        // in our implementation.
        if (m_optimizationSet.count(lit) > 0)
        {
            throw std::runtime_error("Given the same literal twice in MSU4 MaxSAT");
        }

        m_optimizationSet.insert(lit);
        reset();
    }

    bool MSU4MaxSATSolver::isSAT() const
    {
        return m_isSAT;
    }

    ModelValue MSU4MaxSATSolver::getAssignment(ID lit) const
    {
        assert(m_isSAT);

        ID nlit = negate(lit);
        if (m_optimizationSet.count(lit) == 0 && m_optimizationSet.count(nlit) == 0)
        {
            throw std::runtime_error("Cannot get assignments to non-optimization variables");
        }

        if (m_currentSoln.count(lit) > 0)
        {
            return SAT::TRUE;
        }
        else if (m_currentSoln.count(nlit) > 0)
        {
            return SAT::FALSE;
        }
        else if (m_optimizationSet.count(lit) > 0)
        {
            return SAT::FALSE;
        }
        else
        {
            assert(m_optimizationSet.count(nlit));
            return SAT::TRUE;
        }
    }

    ModelValue MSU4MaxSATSolver::getAssignmentToVar(ID var) const
    {
        assert(!is_negated(var));
        return getAssignment(var);
    }
}

