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

#include "pme/util/map_solver.h"

#include <set>
#include <cassert>

namespace PME {

    template <class ForwardIterator>
    Clause downClause(ForwardIterator begin, ForwardIterator end, const Seed & seed)
    {
        assert(!seed.empty());

        // Clause that blocks seed and all subsets.
        // For c_1,...,c_n not in seed: (c_1 V ... V c_n)
        Clause cls;

        std::set<ID> seed_set(seed.begin(), seed.end());
        for (auto it = begin; it != end; ++it)
        {
            ID gate = *it;
            assert(!is_negated(gate));
            if (seed_set.count(gate) == 0)
            {
                cls.push_back(gate);
            }
        }

        return cls;
    }

    Clause upClause(const Seed & seed)
    {
        assert(!seed.empty());

        // Clause that blocks seed and all supersets.
        // For seed = c_1,...,c_n: (~c_1 V ... V ~c_n)

        Clause cls;
        cls.reserve(seed.size());

        for (ID gate : seed)
        {
            assert(!is_negated(gate));
            ID ngate = negate(gate);
            cls.push_back(ngate);
        }

        return cls;
    }

    void MapSolver::blockUp(const Seed & seed)
    {
        Clause cls = upClause(seed);
        addClauseToSolver(cls);
    }

    void MapSolver::blockDown(const Seed & seed)
    {
        Clause cls = downClause(begin_gates(), end_gates(), seed);

        // When blocking the whole search space (e.g., seed = all gates),
        // the clause will be empty and the SAT solver will probably
        // assert on that. Add the false clause in that case

        if (cls.empty())
        {
            cls = { ID_FALSE };
        }

        addClauseToSolver(cls);
    }

    UnexploredResult MapSolver::findMinimalSeed()
    {
        throw std::logic_error("Called findMinimalSeed on a Map Solver "
                               "that doesn't support it");
    }

    UnexploredResult MapSolver::findMaximalSeed()
    {
        throw std::logic_error("Called findMaximalSeed on a Map Solver "
                               "that doesn't support it");
    }

    void MinmaxMapSolver::initIfNecessary()
    {
        if (!m_map_inited) { initSolver(); }
        m_map_inited = true;
    }

    UnexploredResult MinmaxMapSolver::findMinimalSeed()
    {
        initIfNecessary();
        return doFindMinimalSeed();
    }

    UnexploredResult MinmaxMapSolver::findMaximalSeed()
    {
        initIfNecessary();
        return doFindMaximalSeed();
    }

    UnexploredResult MinmaxMapSolver::findSeed()
    {
        initIfNecessary();
        return doFindSeed();
    }

    UnexploredResult MinmaxMapSolver::doFindMinimalSeed()
    {
        throw std::logic_error("Called doFindMinimalSeed on a MinmaxMapSolver "
                               "that doesn't support it");
    }

    UnexploredResult MinmaxMapSolver::doFindMaximalSeed()
    {
        throw std::logic_error("Called doFindMaximalSeed on a MinmaxMapSolver "
                               "that doesn't support it");
    }

    void MinmaxMapSolver::addClauseToSolver(const Clause & cls)
    {
        map().addClause(cls);
    }

    void MaximalMapSolver::initSolver()
    {
        for (auto it = begin_gates(); it != end_gates(); ++it)
        {
            map().addForOptimization(*it);
        }
    }

    UnexploredResult MaximalMapSolver::doFindMaximalSeed()
    {
        UnexploredResult result;
        Seed & seed = result.second;
        if (map().solve())
        {
            result.first = true;
            for (auto it = begin_gates(); it != end_gates(); ++it)
            {
                ID gate = *it;
                ModelValue assignment = map().getAssignmentToVar(gate);
                if (assignment == SAT::TRUE) { seed.push_back(gate); }
            }
        }
        else
        {
            result.first = false;
        }
        return result;
    }

    UnexploredResult MaximalMapSolver::doFindSeed()
    {
        return doFindMaximalSeed();
    }
}

