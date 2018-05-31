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

#include "pme/engine/collapse_set_finder.h"
#include "pme/util/proof_checker.h"

#define BOOST_TEST_MODULE CollapseSetFinderTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct SimpleCollapseFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<CollapseSetFinder> finder;
    GlobalState gs;

    SimpleCollapseFixture(bool simplify = true)
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

        gs.opts.simplify = simplify;
        tr.reset(new TransitionRelation(vars, aig));
        finder.reset(new CollapseSetFinder(vars, *tr, gs));
    }

    ~SimpleCollapseFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

struct CollapseFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<CollapseSetFinder> finder;
    GlobalState gs;

    CollapseFixture(bool simplify = true)
    {
        aig = aiger_init();

        l0 = 2;
        l1 = 4;
        l2 = 6;

        // l0' = 1
        aiger_add_latch(aig, l0, 1, "l0");

        // l1' = l0
        aiger_add_latch(aig, l1, l0, "l1");

        // l2' = l1
        aiger_add_latch(aig, l2, l1, "l2");

        // o0 = 0
        aiger_add_output(aig, 0, "o0");
        o0 = 0;

        gs.opts.simplify = simplify;
        tr.reset(new TransitionRelation(vars, aig));
        finder.reset(new CollapseSetFinder(vars, *tr, gs));
    }

    ~CollapseFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(simple_find)
{
    SimpleCollapseFixture f(false);
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause c0 = {negate(l0)};
    Clause c1 = {negate(l1)};
    Clause c2 = {negate(l2)};
    Clause c3 = {negate(l3)};

    ClauseVec proof = {c0, c1, c2, c3};
    ProofChecker pc(*f.tr, proof);
    BOOST_REQUIRE(pc.checkInduction());

    f.finder->addClause(0, c0);
    f.finder->addClause(1, c1);
    f.finder->addClause(2, c2);
    f.finder->addClause(3, c3);

    // Support graph: c0 -> c1 -> c2 -> c3 -> back to c0
    CollapseSet actual, expected;
    BOOST_REQUIRE(f.finder->find(0, actual));
    expected = {3};
    BOOST_CHECK(actual == expected);

    BOOST_REQUIRE(f.finder->find(1, actual));
    expected = {0};
    BOOST_CHECK(actual == expected);

    BOOST_REQUIRE(f.finder->find(2, actual));
    expected = {1};
    BOOST_CHECK(actual == expected);

    BOOST_REQUIRE(f.finder->find(3, actual));
    expected = {2};
    BOOST_CHECK(actual == expected);
}

BOOST_AUTO_TEST_CASE(simple_find_and_block)
{
    SimpleCollapseFixture f;
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause c0 = {negate(l0)};
    Clause c1 = {negate(l1)};
    Clause c2 = {negate(l2)};
    Clause c3 = {negate(l3)};

    ClauseVec proof = {c0, c1, c2, c3};
    ProofChecker pc(*f.tr, proof);
    BOOST_REQUIRE(pc.checkInduction());

    f.finder->addClause(0, c0);
    f.finder->addClause(1, c1);
    f.finder->addClause(2, c2);
    f.finder->addClause(3, c3);

    // Support graph: c0 -> c1 -> c2 -> c3 -> back to c0
    CollapseSet actual, expected;
    BOOST_REQUIRE(f.finder->findAndBlock(0, actual));
    expected = {3};
    BOOST_CHECK(actual == expected);
    BOOST_CHECK(!f.finder->find(0));

    BOOST_REQUIRE(f.finder->findAndBlock(1, actual));
    expected = {0};
    BOOST_CHECK(actual == expected);
    BOOST_CHECK(!f.finder->find(1));

    BOOST_REQUIRE(f.finder->findAndBlock(2, actual));
    expected = {1};
    BOOST_CHECK(actual == expected);
    BOOST_CHECK(!f.finder->find(2));

    BOOST_REQUIRE(f.finder->findAndBlock(3, actual));
    expected = {2};
    BOOST_CHECK(actual == expected);
    BOOST_CHECK(!f.finder->find(3));
}

BOOST_AUTO_TEST_CASE(multiple_collapse_sets)
{
    CollapseFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);

    Clause c0 = {l0, negate(l1), l2}; // ~010
    Clause c1 = {l0, l1, negate(l2)}; // ~100
    Clause c2 = {negate(l0), l1, negate(l2)}; // ~101
    Clause c3 = {l0, negate(l1), negate(l2)}; // ~110

    ClauseVec proof = {c0, c1, c2, c3};

    ProofChecker pc(*f.tr, proof);
    BOOST_REQUIRE(pc.checkInduction());

    f.finder->addClause(0, c0);
    f.finder->addClause(1, c1);
    f.finder->addClause(2, c2);
    f.finder->addClause(3, c3);

    // Support graph
    // ~010 -> ~101
    // ~100 -----^

    BOOST_CHECK(!f.finder->find(0));
    BOOST_CHECK(!f.finder->find(1));
    BOOST_CHECK(!f.finder->find(3));

    CollapseSet actual1, actual2, expected1, expected2;
    expected1 = {0};
    expected2 = {1};

    BOOST_REQUIRE(f.finder->findAndBlock(2, actual1));

    BOOST_CHECK(actual1 == expected1 || actual1 == expected2);

    BOOST_REQUIRE(f.finder->findAndBlock(2, actual2));
    BOOST_CHECK(actual2 != actual1);
    BOOST_CHECK(actual1 == expected1 || actual1 == expected2);

    BOOST_CHECK(!f.finder->find(2));
}

BOOST_AUTO_TEST_CASE(repeated_collapse_sets)
{
    CollapseFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);

    Clause c0 = {l0}; // l0 (~000, ~010, ~100, ~110)
    Clause c1 = {negate(l0), l1, l2}; // ~001
    Clause c2 = {negate(l0), l1, negate(l2)}; // ~101
    Clause c3 = {negate(l0), negate(l1), l2}; // ~011

    ClauseVec proof = {c0, c1, c2, c3};

    ProofChecker pc(*f.tr, proof);
    BOOST_REQUIRE(pc.checkInduction());

    f.finder->addClause(0, c0);
    f.finder->addClause(1, c1);
    f.finder->addClause(2, c2);
    f.finder->addClause(3, c3);

    // l0 should be the support for ~001 and ~101

    CollapseSet actual, expected;
    expected = {0};

    BOOST_REQUIRE(f.finder->findAndBlock(1, actual));
    BOOST_CHECK(actual == expected);
    BOOST_CHECK(!f.finder->find(1));

    actual.clear();
    BOOST_REQUIRE(f.finder->findAndBlock(2, actual));
    BOOST_CHECK(actual == expected);
    BOOST_CHECK(!f.finder->find(2));

    CollapseSet actual1, actual2;
    CollapseSet expected1 = {1};
    CollapseSet expected2 = {2};
    BOOST_REQUIRE(f.finder->findAndBlock(3, actual1));
    BOOST_CHECK(actual1 == expected1 || actual1 == expected2);
    BOOST_REQUIRE(f.finder->findAndBlock(3, actual2));
    BOOST_CHECK(actual2 == expected1 || actual2 == expected2);
    BOOST_CHECK(actual1 != actual2);
    BOOST_CHECK(!f.finder->find(3));
}

