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
#include "pme/util/timer.h"

#include <algorithm>
#include <cassert>

namespace PME
{
    MARCOMinimizer::MARCOMinimizer(VariableManager & vars,
                                   const TransitionRelation & tr,
                                   const ClauseVec & proof)
        : ProofMinimizer(vars, tr, proof),
          m_seed_solver_down(vars),
          m_seed_solver_up(vars),
          m_ind_solver(vars, tr),
          m_seed_count(0),
          m_lower_bound(0)
    {
        initSolvers();
    }

    bool MARCOMinimizer::isDirectionUp() const
    {
        return opts().marco_direction_up &&
               !opts().marco_direction_down;
    }

    bool MARCOMinimizer::isDirectionDown() const
    {
        return opts().marco_direction_down &&
               !opts().marco_direction_up;
    }

    bool MARCOMinimizer::isDirectionZigZag() const
    {
        return opts().marco_direction_down &&
               opts().marco_direction_up;
    }

    bool MARCOMinimizer::isDirectionArbitrary() const
    {
        return !opts().marco_direction_down &&
               !opts().marco_direction_up;
    }

    bool MARCOMinimizer::isNextSeedMinimum() const
    {
        if (isDirectionUp())
        {
            return true;
        }
        else
        {
            bool odd = (m_seed_count % 2) == 1;
            return isDirectionZigZag() && odd;
        }
    }

    bool MARCOMinimizer::isNextSeedMaximum() const
    {
        if (isDirectionDown())
        {
            return true;
        }
        else
        {
            bool odd = (m_seed_count % 2) == 1;
            return isDirectionZigZag() && !odd;
        }
    }

    PBOMaxSATSolver & MARCOMinimizer::getSeedSolver()
    {
        if (isDirectionUp() || isDirectionArbitrary())
        {
            return m_seed_solver_up;
        }
        else if (isDirectionDown())
        {
            return m_seed_solver_down;
        }
        else if (isDirectionZigZag())
        {
            if (isNextSeedMinimum())
            {
                return m_seed_solver_up;
            }
            else
            {
                assert(isNextSeedMaximum());
                return m_seed_solver_down;
            }
        }
        else
        {
            assert(false);
        }
    }

    void MARCOMinimizer::initSolvers()
    {
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            // Add clauses to m_ind_solver
            const Clause & cls = clauseOf(id);
            m_ind_solver.addClause(id, cls);

            // Add variables to m_seed_solver.
            ID cls_seed = vars().getNewID("seed");

            // In MARCO-ARB, we use one solver with no MaxSAT
            // In other modes, we use one or both with MaxSAT
            if (!isDirectionArbitrary())
            {
                m_seed_solver_up.addForOptimization(negate(cls_seed));
                m_seed_solver_down.addForOptimization(cls_seed);
            }

            m_clause_to_seed_var[id] = cls_seed;

            // Add a clause to the seed solver enforcing that the property is
            // always present
            if (cls.size() == 1 && cls.at(0) == negate(tr().bad()))
            {
                m_seed_solver_up.addClause({cls_seed});
                m_seed_solver_down.addClause({cls_seed});
            }
        }
    }

    std::ostream & MARCOMinimizer::log(int verbosity) const
    {
        return ProofMinimizer::log(LOG_MARCO, verbosity);
    }

    void MARCOMinimizer::doMinimize()
    {
        while (true)
        {
            bool sat;
            Seed seed;

            bool minimum = isNextSeedMinimum();
            bool maximum = isNextSeedMaximum();
            assert(!(minimum && maximum));
            std::tie(sat, seed) = getUnexplored();

            Seed mis = seed;

            if (!sat) { break; }
            else if (findSIS(mis))
            {
                log(3) << "Found a SIS seed of size " << mis.size()
                       << (minimum ? " [minimum]" : maximum ? " [maximum]" : "")
                       << std::endl;
                if (!minimum) { shrink(mis); }

                log(2) << "MSIS of size " << mis.size() << std::endl;
                log(3) << "MSIS: " << seed << std::endl;
                blockUp(mis);
                updateProofs(mis);
            }
            else
            {
                log(3) << "Found a non-SIS seed of size " << seed.size()
                       << (minimum ? " [minimum]" : maximum ? " [maximum]" : "")
                       << std::endl;
                if (!maximum) { grow(seed); }
                log(2) << "MNIS of size " << seed.size() << std::endl;
                log(3) << "MNIS: " << seed << std::endl;
                blockDown(seed);
            }
        }

        assert(!m_smallest_proof.empty());
        if (!minimumProofKnown()) { setMinimumProof(m_smallest_proof); }
    }

    void MARCOMinimizer::updateProofs(const Seed & seed)
    {
        assert(!seed.empty());
        if (m_smallest_proof.empty() || seed.size() < m_smallest_proof.size())
        {
            m_smallest_proof = seed;
        }

        addMinimalProof(seed);
        if (!minimumProofKnown() && m_smallest_proof.size() <= m_lower_bound)
        {
            setMinimumProof(m_smallest_proof);
        }
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

        m_seed_solver_up.addClause(cls);
        m_seed_solver_down.addClause(cls);
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
            m_seed_solver_up.addClause({ID_FALSE});
            m_seed_solver_down.addClause({ID_FALSE});
        }
        else
        {
            m_seed_solver_up.addClause(cls);
            m_seed_solver_down.addClause(cls);
        }
    }

    bool MARCOMinimizer::isSIS(const Seed & seed)
    {
        stats().marco_issis_calls++;
        AutoTimer t(stats().marco_issis_time);
        // We don't need to check initiation, we always assume all clauses
        // are initiated. We only check for induction and safety
        bool safe = std::find(seed.begin(), seed.end(), propertyID()) != seed.end();
        return safe && m_ind_solver.isInductive(seed);
    }

    void MARCOMinimizer::grow(Seed & seed)
    {
        stats().marco_grow_calls++;
        AutoTimer t(stats().marco_grow_time);
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
                if (!m_ind_solver.solve(seed, id))
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
        stats().marco_shrink_calls++;
        AutoTimer t(stats().marco_shrink_time);

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
        stats().marco_findsis_calls++;
        AutoTimer t(stats().marco_findsis_time);
        // Given a potentially non-inductive seed, find the a maximal inductive
        // subset (MIS) that is safe, if any exist
        return findSafeMIS(m_ind_solver, seed, propertyID());
    }

    MARCOMinimizer::UnexploredResult MARCOMinimizer::getUnexplored()
    {
        stats().marco_get_unexplored_calls++;
        AutoTimer t(stats().marco_get_unexplored_time);

        PBOMaxSATSolver & seed_solver = getSeedSolver();
        bool minimum = isNextSeedMinimum();
        m_seed_count++;

        UnexploredResult result;
        Seed & seed = result.second;
        if (seed_solver.solve())
        {
            result.first = true;
            for (ClauseID id = 0; id < numClauses(); ++id)
            {
                ID seed_var = seedVarOf(id);
                ModelValue assignment = seed_solver.safeGetAssignmentToVar(seed_var);
                if (assignment == SAT::TRUE) { seed.push_back(id); }

                // UNDEF should only be possible in MARCO-ARB
                // We treat it as true (it indicates variable does not yet
                // appear in the map, so it's a don't care)
                if (assignment == SAT::UNDEF)
                {
                    assert(isDirectionArbitrary());
                    seed.push_back(id);
                }
            }

            if (minimum)
            {
                assert(seed.size() >= m_lower_bound);
                m_lower_bound = seed.size();
                if (!minimumProofKnown() && m_smallest_proof.size() <= m_lower_bound)
                {
                    setMinimumProof(m_smallest_proof);
                }
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
        return m_clause_to_seed_var.at(cls);
    }
}

