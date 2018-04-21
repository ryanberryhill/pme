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

#include "sat/sat.h"

#define BOOST_TEST_MODULE SATTest

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace SAT;

BOOST_AUTO_TEST_CASE(one_var)
{
    Solver slv;
    BOOST_CHECK(!slv.isSAT());
    Variable v = slv.newVariable();

    // (v)
    slv.addClause({v});
    BOOST_CHECK(slv.solve({}));
    BOOST_CHECK(slv.isSAT());

    // (v) & (-v)[assumption]
    BOOST_CHECK(!slv.solve({negate(v)}));
    BOOST_CHECK(!slv.isSAT());

    // (v) & (-v)
    slv.addClause({negate(v)});
    BOOST_CHECK(!slv.isSAT());
    BOOST_CHECK(!slv.solve({}));
}

BOOST_AUTO_TEST_CASE(three_var)
{
    Solver slv;
    Variable a = slv.newVariable();
    Variable b = slv.newVariable();
    Variable c = slv.newVariable();

    // (a) & (b V c) & (-c)
    Cube empty_assumps;
    slv.addClause({a});
    slv.addClause({b, c});
    slv.addClause({negate(c)});
    BOOST_CHECK(slv.solve({}));

    BOOST_CHECK(!slv.solve({c}));

    BOOST_CHECK(slv.solve({b}));

    slv.addClause({b});
    BOOST_CHECK(slv.solve(empty_assumps));
    BOOST_CHECK(!slv.solve({negate(b)}));
}

BOOST_AUTO_TEST_CASE(get_assignment)
{
    Solver slv;
    Variable a = slv.newVariable();
    Variable b = slv.newVariable();

    // (-a V -b)
    slv.addClause({negate(a), negate(b)});
    BOOST_CHECK(slv.solve({a}));
    BOOST_CHECK_EQUAL(slv.getAssignment(a), TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(b), FALSE);

    BOOST_CHECK(slv.solve({b}));
    BOOST_CHECK_EQUAL(slv.getAssignment(a), FALSE);
    BOOST_CHECK_EQUAL(slv.getAssignment(b), TRUE);
}

