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

#include <cassert>
#include <algorithm>

namespace PME
{
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
        m_cardinality.setCardinality(m_optimizationSet.size());
        m_solver.addClauses(m_cardinality.CNFize());

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
        assert(!m_optimizationSet.empty());
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

    bool PBOMaxSATSolver::solve()
    {
        std::vector<ID> assumps;
        return solve(assumps);
    }

    bool PBOMaxSATSolver::solve(const Cube & assumps)
    {
        GlobalState::stats().maxsat_calls++;
        AutoTimer timer(GlobalState::stats().maxsat_runtime);

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

    MSU4MaxSATSolver::MSU4MaxSATSolver(VariableManager & varman)
        : m_isSAT(false),
          m_vars(varman),
          m_lb(0),
          m_ub(std::numeric_limits<unsigned>::max())
    { }

    void MSU4MaxSATSolver::addClause(const Clause & cls)
    {
        m_clauses.push_back(cls);
        m_solver.addClause(cls);

        // Reset existing lower bound and solution
        m_isSAT = false;
        m_lb = 0;
        m_currentSoln.clear();

        // Reset upper bound
        // TODO this isn't really necessary if we're careful
        m_ub = std::numeric_limits<unsigned>::max();
    }

    void MSU4MaxSATSolver::resetSolver()
    {
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
        // TODO: dynamically choose whether the >= or <= makes more sense
        // TODO: incrementality. If inputs don't change, just change
        // the cardinality
        SortingCardinalityConstraint cardinality(m_vars);
        cardinality.addInputs(inputs.begin(), inputs.end());
        cardinality.setCardinality(n + 1);

        m_solver.addClauses(cardinality.CNFize());
        return cardinality.assumeGT(n);
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

    bool MSU4MaxSATSolver::solve()
    {
        std::set<ID> initial_assumps = m_optimizationSet;
        std::vector<ID> blocked_assumps;
        Cube cardinality_assumps;

        // Check if the hard clauses are satisfiable
        if (!m_solver.solve()) { return false; }

        unsigned unsat_rounds = 0;
        while (true)
        {
            Cube assumps(initial_assumps.begin(), initial_assumps.end());
            assumps.insert(assumps.end(), cardinality_assumps.begin(),
                                          cardinality_assumps.end());

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
                unsigned n = numSatisfied(blocked_assumps);
                if (n == blocked_assumps.size())
                {
                    // All clauses are satsifiable
                    m_isSAT = true;
                    return true;
                }
                cardinality_assumps = addCardinality(blocked_assumps, n);
            }
            else
            {
                Cube core = extractCore(crits, initial_assumps);

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
                        initial_assumps.erase(core_lit);
                        blocked_assumps.push_back(core_lit);
                    }

                    // Update upper bound
                    unsat_rounds++;
                    assert(m_ub > m_optimizationSet.size() - unsat_rounds);
                    m_ub = m_optimizationSet.size() - unsat_rounds;
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

    void MSU4MaxSATSolver::addForOptimization(ID lit)
    {
        // We don't support passing the same literal twice for simplicity
        // in our implementation.
        if (m_optimizationSet.count(lit) > 0)
        {
            throw std::runtime_error("Given the same literal twice in MSU4 MaxSAT");
        }

        m_optimizationSet.insert(lit);
        m_isSAT = false;
        m_currentSoln.clear();
        m_lb = 0;
        m_ub = std::numeric_limits<unsigned>::max();
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

