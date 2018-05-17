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

#include "pme/engine/consecution_checker.h"

#define BOOST_TEST_MODULE ConsecutionCheckerTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct ConsecutionFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<ConsecutionChecker> checker;

    ConsecutionFixture()
    {
        aig = aiger_init();

        l0 = 2;
        l1 = 4;
        l2 = 6;
        l3 = 8;

        // l0' = l3
        aiger_add_latch(aig, l0, l3, "l0");

        // l1' = l0
        aiger_add_latch(aig, l1, l0, "l1");

        // l2' = l1
        aiger_add_latch(aig, l2, l1, "l2");

        // l3' = l2
        aiger_add_latch(aig, l3, l2, "l3");

        // o0 = l3
        aiger_add_output(aig, l3, "o0");
        o0 = l3;

        tr.reset(new TransitionRelation(vars, aig));
        checker.reset(new ConsecutionChecker(vars, *tr));
    }

    ~ConsecutionFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(test_incremental)
{
    ConsecutionFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause c0 = {negate(l0)};
    Clause c1 = {negate(l1)};
    Clause c2 = {negate(l2)};
    Clause c3 = {negate(l3)};

    f.checker->addClause(0, c0);

    BOOST_CHECK(f.checker->solve({0}, c1));

    f.checker->addClause(1, c1);

    BOOST_CHECK(f.checker->solve({0}, c1));
    BOOST_CHECK(f.checker->solve({0,1}, c1));
    BOOST_CHECK(f.checker->solve({1}, c2));
    BOOST_CHECK(f.checker->solve({0,1}, c2));

    BOOST_CHECK(!f.checker->solve({1}, c1));
    BOOST_CHECK(!f.checker->solve({0}, c2));
    BOOST_CHECK(!f.checker->solve({0}, c3));
    BOOST_CHECK(!f.checker->solve({0}, c0));

    f.checker->addClause(2, c2);

    BOOST_CHECK(f.checker->solve({2}, c3));
    BOOST_CHECK(f.checker->solve({1,2}, c3));
    BOOST_CHECK(f.checker->solve({0,2}, c3));
    BOOST_CHECK(f.checker->solve({0,1,2}, c3));

    BOOST_CHECK(!f.checker->solve({0}, c3));
    BOOST_CHECK(!f.checker->solve({1}, c3));
    BOOST_CHECK(!f.checker->solve({0,1}, c3));

    f.checker->addClause(3, c3);

    BOOST_CHECK(f.checker->solve(c1));
    BOOST_CHECK(f.checker->solve(c2));
    BOOST_CHECK(f.checker->solve(c3));
    BOOST_CHECK(f.checker->solve(c0));
}

BOOST_AUTO_TEST_CASE(test_nonincremental)
{
    ConsecutionFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause c0 = {negate(l0)};
    Clause c1 = {negate(l1)};
    Clause c2 = {negate(l2)};
    Clause c3 = {negate(l3)};

    f.checker->addClause(0, c0);
    f.checker->addClause(1, c1);
    f.checker->addClause(2, c2);
    f.checker->addClause(3, c3);

    BOOST_CHECK(f.checker->solve({0}, c1));
    BOOST_CHECK(f.checker->solve({0,1}, c1));
    BOOST_CHECK(f.checker->solve({1}, c2));
    BOOST_CHECK(f.checker->solve({0,1}, c2));

    BOOST_CHECK(!f.checker->solve({1}, c1));
    BOOST_CHECK(!f.checker->solve({0}, c2));
    BOOST_CHECK(!f.checker->solve({0}, c3));
    BOOST_CHECK(!f.checker->solve({0}, c0));

    BOOST_CHECK(f.checker->solve({2}, c3));
    BOOST_CHECK(f.checker->solve({1,2}, c3));
    BOOST_CHECK(f.checker->solve({0,2}, c3));
    BOOST_CHECK(f.checker->solve({0,1,2}, c3));

    BOOST_CHECK(!f.checker->solve({0}, c3));
    BOOST_CHECK(!f.checker->solve({1}, c3));
    BOOST_CHECK(!f.checker->solve({0,1}, c3));

    BOOST_CHECK(f.checker->solve({3}, c0));
    BOOST_CHECK(f.checker->solve({2,3}, c0));
    BOOST_CHECK(f.checker->solve({1,2,3}, c0));
    BOOST_CHECK(f.checker->solve({0,1,2,3}, c0));

    BOOST_CHECK(!f.checker->solve({2}, c0));
    BOOST_CHECK(!f.checker->solve({1}, c0));
    BOOST_CHECK(!f.checker->solve({0}, c0));

    BOOST_CHECK(f.checker->solve(c1));
    BOOST_CHECK(f.checker->solve(c2));
    BOOST_CHECK(f.checker->solve(c3));
    BOOST_CHECK(f.checker->solve(c0));
}

// The two above tests don't depend on adding c unprimed
BOOST_AUTO_TEST_CASE(test_adding_c_unprimed)
{
    ConsecutionFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    // l0 V l1 V l2 V l3 is clearly inductive (if one or more is active,
    // one or more will also be active next cycle). However, it is only
    // inductive relative to itself or any of (l0), (l1), etc.
    Clause ind = {l0, l1, l2, l3};
    Clause c0 = {negate(l0)};
    Clause c1 = {negate(l1)};
    Clause c2 = {negate(l2)};

    BOOST_CHECK(f.checker->solve(ind));

    // ind is clearly not inductive relative to any of the below, but
    // relative to them & ind it is
    f.checker->addClause(0, c0);
    f.checker->addClause(1, c1);
    f.checker->addClause(2, c2);

    BOOST_CHECK(f.checker->solve(ind));
    BOOST_CHECK(f.checker->solve({0,1,2}, ind));

    f.checker->addClause(3, ind);
    BOOST_CHECK(f.checker->solve(ind));
    BOOST_CHECK(f.checker->solve({0,1,2,3}, ind));
    BOOST_CHECK(f.checker->solve({3}, ind));
}

BOOST_AUTO_TEST_CASE(test_is_inductive)
{
    ConsecutionFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause c0 = {negate(l0)};
    Clause c1 = {negate(l1)};
    Clause c2 = {negate(l2)};
    Clause c3 = {negate(l3)};

    f.checker->addClause(0, c0);
    f.checker->addClause(1, c1);
    f.checker->addClause(2, c2);
    f.checker->addClause(3, c3);

    BOOST_CHECK(f.checker->isInductive({0,1,2,3}));
    BOOST_CHECK(!f.checker->isInductive({0,1,2}));
    BOOST_CHECK(!f.checker->isInductive({0,1,3}));
    BOOST_CHECK(!f.checker->isInductive({0,1}));
    BOOST_CHECK(!f.checker->isInductive({0}));
}

BOOST_AUTO_TEST_CASE(test_support_set)
{
    ConsecutionFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause c0 = {negate(l0)};
    Clause c1 = {negate(l1)};
    Clause c2 = {negate(l2)};
    Clause c3 = {negate(l3)};

    f.checker->addClause(0, c0);
    f.checker->addClause(1, c1);
    f.checker->addClause(2, c2);
    f.checker->addClause(3, c3);

    ClauseIDVec frame = {0, 1, 2, 3};

    for (ClauseID id = 0; id <= 3; ++id)
    {
        BOOST_REQUIRE(f.checker->solve(frame, id));

        ClauseIDVec support;
        BOOST_CHECK(f.checker->supportSolve(frame, id, support));

        // Support only contains clauses from frame
        BOOST_CHECK(!support.empty());
        for (ClauseID cls : support)
        {
            BOOST_CHECK(std::find(frame.begin(), frame.end(), cls) != frame.end());
        }

        // Support is sufficient to prove relative induction
        BOOST_CHECK(f.checker->solve(support, id));

        // Should also work without a frame
        support.clear();
        BOOST_CHECK(f.checker->supportSolve(id, support));
        BOOST_CHECK(!support.empty());
        BOOST_CHECK(f.checker->solve(support, id));
    }
}

