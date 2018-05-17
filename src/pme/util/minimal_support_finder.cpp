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

#include "src/pme/util/minimal_support_finder.h"

#include <algorithm>
#include <cassert>

namespace PME
{
    MinimalSupportFinder::MinimalSupportFinder(ConsecutionChecker & solver)
        : m_solver(solver)
    { }

    ClauseIDVec MinimalSupportFinder::findSupport(const ClauseIDVec & frame, ClauseID id)
    {
        Clause cls = m_solver.clauseOf(id);
        return findSupport(frame, cls);
    }

    ClauseIDVec MinimalSupportFinder::findSupport(const ClauseIDVec & frame, const Clause & cls)
    {
        ClauseIDVec frame_copy = frame;
        std::sort(frame_copy.begin(), frame_copy.end());
        for (auto it = frame_copy.begin(); it != frame_copy.end(); )
        {
            ClauseID id = *it;
            ClauseIDVec test_frame;
            test_frame.reserve(frame_copy.size());

            // test_frame = frame_copy - id
            std::remove_copy(frame_copy.begin(), frame_copy.end(),
                             std::back_inserter(test_frame), id);

            ClauseIDVec support;
            if (m_solver.supportSolve(frame_copy, cls, support))
            {
                // update frame copy to support, sort, and place the iterator
                // at the right position (the first element > id)
                assert(support.size() <= frame_copy.size());
                frame_copy = support;
                std::sort(frame_copy.begin(), frame_copy.end());
                it = std::upper_bound(frame_copy.begin(), frame_copy.end(), id);
            }
            else
            {
                ++it;
            }
        }

        return frame_copy;
    }
}

