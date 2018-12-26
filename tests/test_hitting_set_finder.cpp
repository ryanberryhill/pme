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

#include "pme/id.h"
#include "pme/util/hitting_set_finder.h"

#define BOOST_TEST_MODULE HittingSetFinderTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

BOOST_AUTO_TEST_CASE(basic_hitting_sets)
{
    VariableManager varman;
    HittingSetFinder finder(varman);

    ID a = varman.getNewID("a");
    ID b = varman.getNewID("b");
    ID c = varman.getNewID("c");

    finder.addSet({a, b});
    finder.addSet({b, c});

    std::vector<ID> soln = finder.solve();
    std::vector<ID> expected = {b};

    BOOST_REQUIRE(!soln.empty());
    BOOST_REQUIRE(soln == expected);

    finder.blockSolution(soln);
    soln = finder.solve();
    std::sort(soln.begin(), soln.end());

    expected = {a, c};
    std::sort(expected.begin(), expected.end());
    BOOST_REQUIRE(soln == expected);
    finder.blockSolution(soln);

    soln = finder.solve();

    BOOST_CHECK(soln.empty());
}

BOOST_AUTO_TEST_CASE(incremental_hitting_sets)
{
    VariableManager varman;
    HittingSetFinder finder(varman);

    ID a = varman.getNewID("a");
    ID b = varman.getNewID("b");
    ID c = varman.getNewID("c");
    ID d = varman.getNewID("d");
    ID e = varman.getNewID("e");
    ID f = varman.getNewID("f");

    finder.addSet({a});
    std::vector<ID> soln = finder.solve();
    std::vector<ID> expected = {a};

    finder.addSet({a, b, c});
    finder.addSet({c, d});
    finder.addSet({c, e});

    soln = finder.solve();
    expected = {a, c};
    std::sort(soln.begin(), soln.end());
    std::sort(expected.begin(), expected.end());

    BOOST_REQUIRE(soln == expected);

    finder.addSet({f});

    soln = finder.solve();
    expected = {a, c, f};
    std::sort(soln.begin(), soln.end());
    std::sort(expected.begin(), expected.end());

    BOOST_REQUIRE(soln == expected);
}

