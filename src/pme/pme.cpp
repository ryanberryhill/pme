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

#include "config.h"
#include "pme/pme.h"

#include <algorithm>
#include <cassert>

namespace PME
{
    const static std::string pme_version_string = PACKAGE_VERSION;

    const std::string& pme_version()
    {
        return pme_version_string;
    }

    std::ostream & operator<<(std::ostream & os, const ClauseIDVec & vec)
    {
        os << "[ ";
        for (ClauseID id : vec)
        {
            os << id << " ";
        }
        os << "]";
        return os;
    }

    bool subsumes(const Cube & a, const Cube & b)
    {
        assert(std::is_sorted(a.begin(), a.end()));
        assert(std::is_sorted(b.begin(), b.end()));

        if (a.size() > b.size()) { return false; }

        auto a_it = a.begin();
        auto b_it = b.begin();

        while (a_it != a.end())
        {
            // Advance the index in b until it is no longer < the one in a
            while (b_it != b.end() && *a_it != *b_it)
            {
                b_it++;
            }

            // If the updated b index does not match the value from a,
            // return false (a contains something not in b)
            // Otherwise the value from a is found, advance the index of a
            if (b_it == b.end()) { return false; }
            else if (*a_it != *b_it) { return false; }
            else { a_it++; }
        }

        return true;
    }

}

