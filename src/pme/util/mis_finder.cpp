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

#include "pme/util/mis_finder.h"

#include <algorithm>

namespace PME
{
    MISFinder::MISFinder(ConsecutionChecker & solver, ClauseID propertyClause)
        : m_solver(solver),
          m_property(propertyClause)
    { }

    bool MISFinder::containsProperty(const ClauseIDVec & vec) const
    {
        return std::find(vec.begin(), vec.end(), m_property) != vec.end();
    }

    bool MISFinder::isSafeInductive(const ClauseIDVec & vec)
    {
        return containsProperty(vec) && m_solver.isInductive(vec);
    }

    bool MISFinder::findSafeMIS(ClauseIDVec & vec)
    {
        // Given a potentially non-inductive seed, find the a maximal inductive
        // subset (MIS) that is safe, if any exist

        // Check if the seed is unsafe
        if (!containsProperty(vec))
        {
            return false;
        }

        if (isSafeInductive(vec)) { return true; }

        // Remove all clauses that are non-inductive
        // TODO simple optimization: remove clauses that are satisfied
        // by the current assignment
        bool removed = true;
        while (removed)
        {
            removed = false;

            for (size_t i = 0; i < vec.size(); )
            {
                ClauseID id = vec.at(i);
                if (!m_solver.solve(vec, id))
                {
                    if (id == m_property) { return false; }
                    removed = true;
                    vec.erase(vec.begin() + i);
                }
                else
                {
                    ++i;
                }
            }
        }

        return true;
    }
}

