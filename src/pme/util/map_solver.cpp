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
        // Clause that blocks seed and all subsets.
        // For c_1,...,c_n not in seed: (c_1 V ... V c_n)
        Clause cls;

        std::set<ID> seed_set(seed.begin(), seed.end());
        for (auto it = begin; it != end; ++it)
        {
            ID id = *it;
            assert(!is_negated(id));
            if (seed_set.count(id) == 0)
            {
                cls.push_back(id);
            }
        }

        return cls;
    }

    Clause upClause(const Seed & seed)
    {
        // Clause that blocks seed and all supersets.
        // For seed = c_1,...,c_n: (~c_1 V ... V ~c_n)

        Clause cls;
        cls.reserve(seed.size());

        for (ID id : seed)
        {
            assert(!is_negated(id));
            ID nid = negate(id);
            cls.push_back(nid);
        }

        return cls;
    }

    void MapSolver::blockUp(const Seed & seed)
    {
        Clause cls = upClause(seed);

        // When blocking the whole search space (i.e., seed = empty),
        // the clause will be empty and the SAT solver will probably
        // assert on that. Add the false clause in that case
        if (cls.empty())
        {
            cls = { ID_FALSE };
        }

        addClauseToSolver(cls);
    }

    void MapSolver::blockDown(const Seed & seed)
    {
        Clause cls = downClause(begin_ids(), end_ids(), seed);

        // When blocking the whole search space (e.g., seed = all IDs),
        // the clause will be empty and the SAT solver will probably
        // assert on that. Add the false clause in that case
        if (cls.empty())
        {
            cls = { ID_FALSE };
        }

        addClauseToSolver(cls);
    }

    void MapSolver::addClause(const Clause & cls)
    {
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

    void SATArbitraryMapSolver::addClauseToSolver(const Clause & cls)
    {
        m_map.addClause(cls);
    }

    UnexploredResult SATArbitraryMapSolver::findSeed()
    {
        UnexploredResult result;
        if (m_map.solve())
        {
            result.first = true;
            result.second = extractSeed();
        }
        else
        {
            result.first = false;
        }
        return result;
    }

    bool SATArbitraryMapSolver::checkSeed(const Seed & seed)
    {
        std::unordered_set<ID> seed_set(seed.begin(), seed.end());
        Cube assumps;

        for (auto it = begin_ids(); it != end_ids(); ++it)
        {
            ID id = *it;
            if (seed_set.count(id) > 0)
            {
                assumps.push_back(id);
            }
            else
            {
                assumps.push_back(negate(id));
            }
        }

        return m_map.solve(assumps);
    }

    Seed SATArbitraryMapSolver::extractSeed() const
    {
        assert(m_map.isSAT());

        Seed seed;

        for (auto it = begin_ids(); it != end_ids(); ++it)
        {
            ID id = *it;
            ModelValue assignment = m_map.safeGetAssignmentToVar(id);
            // Treat undefined (i.e., the variable does not appear in the
            // formula, which can happen and indicates don't care) as true
            if (assignment != SAT::FALSE) { seed.push_back(id); }
        }

        return seed;
    }

    void MSU4MapSolver::initIfNecessary()
    {
        if (!m_map_inited) { initSolver(); }
        m_map_inited = true;
    }

    UnexploredResult MSU4MapSolver::findMinimalSeed()
    {
        initIfNecessary();
        return doFindMinimalSeed();
    }

    UnexploredResult MSU4MapSolver::findMaximalSeed()
    {
        initIfNecessary();
        return doFindMaximalSeed();
    }

    UnexploredResult MSU4MapSolver::findSeed()
    {
        initIfNecessary();
        return doFindSeed();
    }

    UnexploredResult MSU4MapSolver::doFindMinimalSeed()
    {
        throw std::logic_error("Called doFindMinimalSeed on a MSU4MapSolver "
                               "that doesn't support it");
    }

    UnexploredResult MSU4MapSolver::doFindMaximalSeed()
    {
        throw std::logic_error("Called doFindMaximalSeed on a MSU4MapSolver "
                               "that doesn't support it");
    }

    void MSU4MapSolver::addClauseToSolver(const Clause & cls)
    {
        map().addClause(cls);
    }

    Seed MSU4MapSolver::extractSeed() const
    {
        assert(map().isSAT());

        Seed seed;

        for (auto it = begin_ids(); it != end_ids(); ++it)
        {
            ID id = *it;
            ModelValue assignment = map().getAssignmentToVar(id);
            if (assignment == SAT::TRUE) { seed.push_back(id); }
        }

        return seed;
    }

    bool MSU4MapSolver::checkSeed(const Seed & seed)
    {
        std::unordered_set<ID> seed_set(seed.begin(), seed.end());
        Cube assumps;

        for (auto it = begin_ids(); it != end_ids(); ++it)
        {
            ID id = *it;
            if (seed_set.count(id) > 0)
            {
                assumps.push_back(id);
            }
            else
            {
                assumps.push_back(negate(id));
            }
        }

        return map().check(assumps);
    }

    void MSU4MaximalMapSolver::initSolver()
    {
        for (auto it = begin_ids(); it != end_ids(); ++it)
        {
            map().addForOptimization(*it);
        }
    }

    UnexploredResult MSU4MaximalMapSolver::doFindMaximalSeed()
    {
        UnexploredResult result;
        if (map().solve())
        {
            result.first = true;
            result.second = extractSeed();
        }
        else
        {
            result.first = false;
        }
        return result;
    }

    UnexploredResult MSU4MaximalMapSolver::doFindSeed()
    {
        return doFindMaximalSeed();
    }

    void MSU4MinimalMapSolver::initSolver()
    {
        for (auto it = begin_ids(); it != end_ids(); ++it)
        {
            map().addForOptimization(negate(*it));
        }
    }

    UnexploredResult MSU4MinimalMapSolver::doFindMinimalSeed()
    {
        UnexploredResult result;
        if (map().solve())
        {
            result.first = true;
            result.second = extractSeed();
        }
        else
        {
            result.first = false;
        }
        return result;
    }

    UnexploredResult MSU4MinimalMapSolver::doFindSeed()
    {
        return doFindMinimalSeed();
    }

    UnexploredResult MSU4ArbitraryMapSolver::findMinimalSeed()
    {
        return m_min.findMinimalSeed();
    }

    UnexploredResult MSU4ArbitraryMapSolver::findMaximalSeed()
    {
        return m_max.findMaximalSeed();
    }

    UnexploredResult MSU4ArbitraryMapSolver::findSeed()
    {
        return m_max.findMaximalSeed();
    }

    bool MSU4ArbitraryMapSolver::checkSeed(const Seed & seed)
    {
        return m_max.checkSeed(seed);
    }

    void MSU4ArbitraryMapSolver::addClauseToSolver(const Clause & cls)
    {
        m_min.addClauseToSolver(cls);
        m_max.addClauseToSolver(cls);
    }
}

