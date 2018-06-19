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

#include "pme/minimization/marco.h"
#include "pme/util/find_safe_mis.h"

#include <algorithm>
#include <cassert>

namespace PME
{
    MARCOMinimizer::MARCOMinimizer(VariableManager & vars,
                                   const TransitionRelation & tr,
                                   const ClauseVec & proof,
                                   GlobalState & gs)
        : ProofMinimizer(vars, tr, proof, gs),
          m_seedSolver(vars),
          m_indSolver(vars, tr)
    {
        initSolvers();
    }

    void MARCOMinimizer::initSolvers()
    {
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            // Add clauses to m_indSolver
            const Clause & cls = clauseOf(id);
            m_indSolver.addClause(id, cls);

            // Add variables to m_seedSolver.
            ID cls_seed = vars().getNewID("seed");
            m_seedSolver.addForOptimization(cls_seed);
            m_clauseToSeedVar[id] = cls_seed;

            // Add a clause to the seed solver enforcing that the property is
            // always present
            if (cls.size() == 1 && cls.at(0) == negate(tr().bad()))
            {
                m_seedSolver.addClause({cls_seed});
            }
        }
    }

    std::ostream & MARCOMinimizer::log(int verbosity) const
    {
        return ProofMinimizer::log(LOG_MARCO, verbosity);
    }

    void MARCOMinimizer::minimize()
    {
        while (true)
        {
            bool sat;
            Seed seed;
            std::tie(sat, seed) = getUnexplored();

            Seed mis = seed;

            if (!sat) { break; }
            else if (findSIS(mis))
            {
                log(3) << "Found a SIS seed of size " << mis.size() << std::endl;
                shrink(mis);
                log(1) << "MSIS of size " << mis.size() << std::endl;
                log(3) << "MSIS: " << seed << std::endl;
                blockUp(mis);
                updateProofs(mis);
            }
            else
            {
                log(3) << "Found a non-SIS seed of size " << seed.size() << std::endl;
                grow(seed);
                log(1) << "MNIS of size " << seed.size() << std::endl;
                log(3) << "MNIS: " << seed << std::endl;
                blockDown(seed);
            }
        }

        assert(!m_smallestProof.empty());
        setMinimumProof(m_smallestProof);
    }

    void MARCOMinimizer::updateProofs(const Seed & seed)
    {
        assert(!seed.empty());
        if (m_smallestProof.empty() || seed.size() < m_smallestProof.size())
        {
            m_smallestProof = seed;
        }

        addMinimalProof(seed);
    }

    void MARCOMinimizer::blockUp(const Seed & seed)
    {
        assert(!seed.empty());
        // Block seed and all supersets. For seed = c_1,...,c_n:
        // (~c_1 V ... V ~c_n)
        Clause cls;
        cls.reserve(seed.size());

        for (ClauseID id : seed)
        {
            ID svar = seedVarOf(id);
            cls.push_back(negate(svar));
        }

        m_seedSolver.addClause(cls);
    }

    void MARCOMinimizer::blockDown(const Seed & seed)
    {
        assert(!seed.empty());
        // Block seed and all subsets. For c_1,...,c_n not in seed:
        // (c_1 V ... V c_n)
        Clause cls;
        cls.reserve(numClauses() - seed.size());

        std::set<ClauseID> seed_set(seed.begin(), seed.end());
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            if (seed_set.count(id) == 0)
            {
                ID svar = seedVarOf(id);
                cls.push_back(svar);
            }
        }

        // It's possible for original proof to be the only MSIS. In this case,
        // the clause will be empty, which the SATAdaptor doesn't accept. Add
        // clause (false) in that case
        if (cls.empty())
        {
            m_seedSolver.addClause({ID_FALSE});
        }
        else
        {
            m_seedSolver.addClause(cls);
        }
    }

    bool MARCOMinimizer::isSIS(const Seed & seed)
    {
        // We don't need to check initiation, we always assume all clauses
        // are initiated. We only check for induction and safety
        bool safe = std::find(seed.begin(), seed.end(), propertyID()) != seed.end();
        return safe && m_indSolver.isInductive(seed);
    }

    void MARCOMinimizer::grow(Seed & seed)
    {
        // Grow is implemented in the obvious way. Add clauses and check for
        // induction. If non-inductive, add the clause. If inductive, back it
        // out.
        std::set<ClauseID> seed_set(seed.begin(), seed.end());
        std::set<ClauseID> notseed;
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            if (seed_set.count(id) == 0)
            {
                notseed.insert(id);
            }
        }

        bool added = true;
        while (added)
        {
            added = false;
            // Copy the ones to try since we'll be deleting from notseed
            std::vector<ClauseID> to_try(notseed.begin(), notseed.end());
            for (ClauseID id : to_try)
            {
                if (!m_indSolver.solve(seed, id))
                {
                    added = true;
                    seed.push_back(id);
                    notseed.erase(id);
                }
            }
        }
    }

    void MARCOMinimizer::shrink(Seed & seed)
    {
        Seed seed_copy = seed;
        std::sort(seed_copy.begin(), seed_copy.end());
        assert(std::adjacent_find(seed_copy.begin(), seed_copy.end()) == seed_copy.end());

        for (auto it = seed_copy.begin(); it != seed_copy.end(); )
        {
            ClauseID id = *it;
            if (id == propertyID()) { ++it; continue; }
            Seed test_seed;
            test_seed.reserve(seed_copy.size());
            // test_seed = seed_copy - id
            std::remove_copy(seed_copy.begin(), seed_copy.end(),
                             std::back_inserter(test_seed), id);

            if (findSIS(test_seed))
            {
                seed_copy = test_seed;
                std::sort(seed_copy.begin(), seed_copy.end());
                // Set it to the first element greater than id (seed_copy is
                // sorted, so this is the first element we haven't tried to
                // remove)
                it = std::upper_bound(seed_copy.begin(), seed_copy.end(), id);
            }
            else
            {
                ++it;
            }
        }

        if (seed_copy.size() < seed.size()) { seed = seed_copy; }
    }

    bool MARCOMinimizer::findSIS(Seed & seed)
    {
        // Given a potentially non-inductive seed, find the a maximal inductive
        // subset (MIS) that is safe, if any exist
        return findSafeMIS(m_indSolver, seed, propertyID());
    }

    MARCOMinimizer::UnexploredResult MARCOMinimizer::getUnexplored()
    {
        UnexploredResult result;
        Seed & seed = result.second;
        if (m_seedSolver.solve())
        {
            result.first = true;
            for (ClauseID id = 0; id < numClauses(); ++id)
            {
                ID seed_var = seedVarOf(id);
                ModelValue assignment = m_seedSolver.getAssignmentToVar(seed_var);
                if (assignment == SAT::TRUE) { seed.push_back(id); }
            }
        }
        else
        {
            result.first = false;
        }
        return result;
    }

    ID MARCOMinimizer::seedVarOf(ClauseID cls) const
    {
        return m_clauseToSeedVar.at(cls);
    }
}

