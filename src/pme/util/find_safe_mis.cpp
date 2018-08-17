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

#include "pme/util/find_safe_mis.h"

#include <algorithm>
#include <cassert>

namespace PME
{
    bool contains(const ClauseIDVec & vec, ClauseID id)
    {
        return std::find(vec.begin(), vec.end(), id) != vec.end();
    }

    bool containsAllOf(const ClauseIDVec & vec, const ClauseIDVec & props)
    {
        for (ClauseID id : props)
        {
            if (!contains(vec, id))
            {
                return false;
            }
        }
        return true;
    }

    bool isSafeInductive(ConsecutionChecker & solver,
                         const ClauseIDVec & vec,
                         const ClauseIDVec & props)
    {
        return containsAllOf(vec, props) && solver.isInductive(vec);
    }

    bool findSafeMIS(ConsecutionChecker & solver, ClauseIDVec & vec, ClauseID property)
    {
        ClauseIDVec nec = {property};
        return findSafeMIS(solver, vec, nec);
    }

    // Remove from vec every clause for which the current SAT assignent to
    // solver is a witness to non-inductiveness. That is, if a literal in ~c'
    // is satisfied.  return false, indicating the result is not a safe
    // inductive invariant.  Otherwise, return true indicating that we don't
    // know yet.
    bool removeSATClauses(ConsecutionChecker & solver, ClauseIDVec & vec, const ClauseIDVec & nec)
    {
        for (auto it = vec.begin(); it != vec.end(); )
        {
            ClauseID id = *it;
            const Clause & cls = solver.clauseOf(id);

            bool negcp_sat = true;
            for (ID lit : cls)
            {
                assert(nprimes(lit) == 0);
                // The corresponding literal of ~c'
                ID nplit = negate(prime(lit));
                if (solver.getAssignment(nplit) == SAT::FALSE)
                {
                    negcp_sat = false;
                    break;
                }
            }

            // Remove the clause if it's not supported
            if (negcp_sat) { it = vec.erase(it); }
            else { ++it; }

            // If it was in NEC, indicate there is no safe MIS
            if (negcp_sat && contains(nec, id)) { return false; }
        }

        return true;
    }

    bool findSafeMIS(ConsecutionChecker & solver, ClauseIDVec & vec, const ClauseIDVec & nec)
    {
        // Given a potentially non-inductive seed, find the a maximal inductive
        // subset (MIS) that is safe, if any exist

        // Check if the seed is unsafe
        if (!containsAllOf(vec, nec))
        {
            return false;
        }

        if (isSafeInductive(solver, vec, nec)) { return true; }

        // Remove all clauses that are non-inductive
        // TODO simple optimization: remove clauses that are satisfied
        // by the current assignment
        std::sort(vec.begin(), vec.end());
        bool removed = true;
        while (removed)
        {
            removed = false;

            for (auto it = vec.begin(); it != vec.end(); )
            {
                ClauseID id = *it;
                if (!solver.solve(vec, id))
                {
                    if (contains(nec, id)) { return false; }
                    removed = true;
                    it = vec.erase(it);

                    // Remove all clauses satisfied by the assignment
                    bool maybe_proof = removeSATClauses(solver, vec, nec);
                    if (!maybe_proof) { return false; }
                    // Point it to the first element greater than id
                    // (under the assumption that vec is sorted)
                    it = std::lower_bound(vec.begin(), vec.end(), id);
                }
                else
                {
                    ++it;
                }
            }
        }

        return true;
    }
}

