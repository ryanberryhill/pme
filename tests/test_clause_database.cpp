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

#include "pme/util/clause_database.h"
#include "pme/engine/variable_manager.h"

#define BOOST_TEST_MODULE ClauseDatabaseTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

BOOST_AUTO_TEST_CASE(test_clause_database)
{
    VariableManager varman;
    ClauseDatabase db;

    ID a = varman.getNewID();
    ID b = varman.getNewID();

    Clause c0 = {a, b};
    Clause c1 = {a, negate(b)};

    ID act_c0 = varman.getNewID();
    ID act_c1 = varman.getNewID();

    db.addClause(0, act_c0, c0);

    BOOST_CHECK_EQUAL(db.activationOfID(0), act_c0);
    BOOST_CHECK_EQUAL(db.IDOfActivation(act_c0), 0);

    Clause c0_actual = db.clauseOf(0);
    std::sort(c0.begin(), c0.end());
    std::sort(c0_actual.begin(), c0_actual.end());
    BOOST_CHECK(c0 == c0_actual);

    db.addClause(1, act_c1, c1);

    // Clause 0 should still be the same
    BOOST_CHECK_EQUAL(db.activationOfID(0), act_c0);
    BOOST_CHECK_EQUAL(db.IDOfActivation(act_c0), 0);

    c0_actual = db.clauseOf(0);
    std::sort(c0.begin(), c0.end());
    std::sort(c0_actual.begin(), c0_actual.end());
    BOOST_CHECK(c0 == c0_actual);

    // Clause 1 should be there too
    BOOST_CHECK_EQUAL(db.activationOfID(1), act_c1);
    BOOST_CHECK_EQUAL(db.IDOfActivation(act_c1), 1);

    Clause c1_actual = db.clauseOf(1);
    std::sort(c1.begin(), c1.end());
    std::sort(c1_actual.begin(), c1_actual.end());
    BOOST_CHECK(c1 == c1_actual);
}

BOOST_AUTO_TEST_CASE(test_extra_data)
{
    VariableManager varman;
    DualActivationClauseDatabase db;

    ID a = varman.getNewID();
    ID b = varman.getNewID();

    Clause c0 = {a, b};
    Clause c1 = {a, negate(b)};

    ID a_c0 = varman.getNewID();
    ID a_c1 = varman.getNewID();
    ID b_c0 = varman.getNewID();
    ID b_c1 = varman.getNewID();

    db.addClause(0, a_c0, c0, b_c0);
    db.addClause(1, a_c1, c1, b_c1);

    BOOST_CHECK_EQUAL(db.activationOfID(0), a_c0);
    BOOST_CHECK_EQUAL(db.getData(0), b_c0);
    BOOST_CHECK_EQUAL(db.IDOfActivation(a_c0), 0);

    BOOST_CHECK_EQUAL(db.activationOfID(1), a_c1);
    BOOST_CHECK_EQUAL(db.getData(1), b_c1);
    BOOST_CHECK_EQUAL(db.IDOfActivation(a_c1), 1);
}

