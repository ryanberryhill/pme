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
#include "pme/engine/transition_relation.h"
#include "pme/util/minimal_support_finder.h"

#define BOOST_TEST_MODULE MinimalSupportFinderTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct MinimalSupportFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<ConsecutionChecker> checker;
    std::unique_ptr<MinimalSupportFinder> finder;

    MinimalSupportFixture()
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
        finder.reset(new MinimalSupportFinder(*checker));
    }

    ~MinimalSupportFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(test_minimal_support_sets)
{
    MinimalSupportFixture f;

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
    ClauseIDVec support, expected;

    support = f.finder->findSupport(frame, 0);
    expected = {3};
    BOOST_CHECK(support == expected);

    support = f.finder->findSupport(frame, 1);
    expected = {0};
    BOOST_CHECK(support == expected);

    support = f.finder->findSupport(frame, 2);
    expected = {1};
    BOOST_CHECK(support == expected);

    support = f.finder->findSupport(frame, 3);
    expected = {2};
    BOOST_CHECK(support == expected);
}

