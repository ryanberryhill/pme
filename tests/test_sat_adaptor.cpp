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
#include "pme/engine/sat_adaptor.h"

#define BOOST_TEST_MODULE SATAdaptorTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

BOOST_AUTO_TEST_CASE(constants)
{
    SATAdaptor slv;
    BOOST_CHECK(!slv.isSAT());

    BOOST_CHECK(slv.solve());
    BOOST_CHECK(slv.isSAT());

    BOOST_CHECK_EQUAL(slv.getAssignment(ID_TRUE), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(ID_FALSE), SAT::FALSE);

    slv.addClause({MIN_ID});
    BOOST_CHECK(slv.solve());
    BOOST_CHECK(slv.isSAT());

    BOOST_CHECK_EQUAL(slv.getAssignment(ID_TRUE), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(ID_FALSE), SAT::FALSE);
}

BOOST_AUTO_TEST_CASE(one_var)
{
    SATAdaptor slv;

    ID a = MIN_ID;

    // (a)
    slv.addClause({a});
    BOOST_CHECK(slv.solve());
    BOOST_CHECK(slv.isSAT());

    // (a) & (-a)[assumption]
    BOOST_CHECK(!slv.solve({negate(a)}));
    BOOST_CHECK(!slv.isSAT());

    // (a) again
    BOOST_CHECK(slv.solve());
    BOOST_CHECK(slv.isSAT());

    // (a) & (-a)
    slv.addClause({negate(a)});
    BOOST_CHECK(!slv.solve());
    BOOST_CHECK(!slv.isSAT());
}

BOOST_AUTO_TEST_CASE(three_var)
{
    SATAdaptor slv;

    ID a = MIN_ID, b = a + ID_INCR, c = b + ID_INCR;

    // (a) & (b V c) & (-b V -c)
    slv.addClause({a});
    slv.addClause({b, c});
    slv.addClause({negate(b), negate(c)});

    BOOST_CHECK(slv.solve());

    BOOST_CHECK(slv.solve({b}));
    BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::FALSE);
    BOOST_CHECK_EQUAL(slv.getAssignmentToVar(b), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignmentToVar(c), SAT::FALSE);
    BOOST_CHECK_EQUAL(slv.getAssignment(negate(b)), SAT::FALSE);
    BOOST_CHECK_EQUAL(slv.getAssignment(negate(c)), SAT::TRUE);

    BOOST_CHECK(slv.solve({negate(b)}));
    BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::FALSE);
    BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignmentToVar(b), SAT::FALSE);
    BOOST_CHECK_EQUAL(slv.getAssignmentToVar(c), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(negate(b)), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(negate(c)), SAT::FALSE);
}

