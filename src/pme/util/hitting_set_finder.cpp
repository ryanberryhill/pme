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

#include "pme/util/hitting_set_finder.h"

#include <cassert>
#include <algorithm>

namespace PME {

    HittingSetFinder::HittingSetFinder(VariableManager & varman)
        : m_vars(varman), m_solver(new MSU4MaxSATSolver(varman))
    { }

    bool HittingSetFinder::checkSubsumption(const std::vector<ID> & s)
    {
        assert(std::is_sorted(s.begin(), s.end()));

        for (auto it = m_sets.begin(); it != m_sets.end(); )
        {
            const std::vector<ID> & t = *it;
            assert(std::is_sorted(t.begin(), t.end()));

            if (subsumes(s, t))
            {
                // s is a subset of t, delete t
                it = m_sets.erase(it);
            }
            else if (subsumes(t, s))
            {
                // s is a superset of t, ignore s
                return false;
            }
            else
            {
                ++it;
            }
        }

        return true;
    }

    void HittingSetFinder::addSet(const std::vector<ID> & s)
    {
        // Cleanup subsumed sets, or ignore s because it is subsumed
        if (checkSubsumption(s))
        {
            std::vector<ID> copy = s;
            std::sort(copy.begin(), copy.end());

            for (ID lit : copy)
            {
                assert(!is_negated(lit));
                addVar(lit);
            }

            m_solver->addClause(copy);
            m_sets.push_back(copy);
        }
    }

    void HittingSetFinder::addVar(ID lit)
    {
        if (m_known.count(lit) == 0)
        {
            m_known.insert(lit);
            m_solver->addForOptimization(negate(lit));
        }
    }

    std::vector<ID> HittingSetFinder::solve()
    {
        bool sat = m_solver->solve();

        if (sat)
        {
            std::vector<ID> soln;

            for (ID lit : m_known)
            {
                if (m_solver->getAssignment(lit) == SAT::TRUE)
                {
                    soln.push_back(lit);
                }
            }

            return soln;
        }
        else
        {
            std::vector<ID> empty;
            return empty;
        }
    }

    void HittingSetFinder::blockSolution(const std::vector<ID> & soln)
    {
        m_blocked.push_back(soln);
        Clause block = negateVec(soln);
        m_solver->addClause(block);
    }

    void HittingSetFinder::renew()
    {
        m_solver.reset(new MSU4MaxSATSolver(m_vars));

        for (ID lit : m_known)
        {
            m_solver->addForOptimization(negate(lit));
        }

        for (const auto & s : m_sets)
        {
            m_solver->addClause(s);
        }

        for (const auto & soln : m_blocked)
        {
            Clause block = negateVec(soln);
            m_solver->addClause(block);
        }
    }
}

