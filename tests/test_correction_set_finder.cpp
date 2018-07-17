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
#include "pme/engine/variable_manager.h"
#include "pme/engine/global_state.h"
#include "pme/ivc/correction_set_finder.h"

#define BOOST_TEST_MODULE CorrectionSetFinderTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct CorrectionSetFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, a0, a1, a2, a3, a4, a5, a6, o0;
    VariableManager vars;
    GlobalState gs;
    std::unique_ptr<DebugTransitionRelation> tr;
    std::unique_ptr<CorrectionSetFinder> finder;

    CorrectionSetFixture()
    {
        aig = aiger_init();

        l0 = 2;
        l1 = 4;
        l2 = 6;
        l3 = 8;
        a0 = 10;
        a1 = 12;
        a2 = 14;
        a3 = 16;
        a4 = 18;
        a5 = 20;
        a6 = 22;

        // l0' = l3
        aiger_add_latch(aig, l0, l3, "l0");

        // l1' = l0
        aiger_add_latch(aig, l1, l0, "l1");

        // l2' = l1
        aiger_add_latch(aig, l2, l1, "l2");

        // l3' = l2
        aiger_add_latch(aig, l3, l2, "l3");

        // a0-a3 are ands of each pair of adjacent latches
        // a4 and a5 are ands of adjacent pairs of a0 and a3
        // a6 = a4 & a5 = <every adjacent pair is high>
        // a6 <=> ALL(l0-l3), but the structure is important
        aiger_add_and(aig, a0, l0, l1);
        aiger_add_and(aig, a1, l1, l2);
        aiger_add_and(aig, a2, l2, l3);
        aiger_add_and(aig, a3, l3, l0);
        aiger_add_and(aig, a4, a0, a1);
        aiger_add_and(aig, a5, a2, a3);
        aiger_add_and(aig, a6, a4, a5);
        aiger_add_output(aig, a6, "o0");
        o0 = a6;
        tr.reset(new DebugTransitionRelation(vars, aig));
        finder.reset(new CorrectionSetFinder(vars, *tr, gs));
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    ~CorrectionSetFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(correction_sets)
{
    CorrectionSetFixture f;

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);
    ID a3 = f.tr->toInternal(f.a3);
    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    CorrectionSetFinder & finder = *f.finder;

    BOOST_REQUIRE(finder.moreCorrectionSets());

    bool found;
    CorrectionSet corr;

    // MCS {a6}
    finder.setCardinality(1);
    std::tie(found, corr) = finder.findAndBlock();

    BOOST_REQUIRE(found);
    BOOST_CHECK_EQUAL(corr.size(), 1);
    BOOST_CHECK_EQUAL(corr.at(0), a6);

    std::tie(found, corr) = finder.findAndBlock();
    BOOST_REQUIRE(!found);

    BOOST_REQUIRE(finder.moreCorrectionSets());

    // MCS {a4, a5}
    finder.setCardinality(2);
    std::tie(found, corr) = finder.findAndBlock();

    BOOST_REQUIRE(found);
    BOOST_CHECK_EQUAL(corr.size(), 2);
    CorrectionSet expected = {a4, a5};
    std::sort(expected.begin(), expected.end());
    std::sort(corr.begin(), corr.end());
    BOOST_CHECK(corr == expected);

    std::tie(found, corr) = finder.findAndBlock();
    BOOST_REQUIRE(!found);

    BOOST_REQUIRE(finder.moreCorrectionSets());

    // MCSes {a0, a1, a5} and {a2, a3, a4}
    finder.setCardinality(3);
    CorrectionSet expected1 = {a0, a1, a5};
    CorrectionSet expected2 = {a2, a3, a4};
    std::sort(expected1.begin(), expected1.end());
    std::sort(expected2.begin(), expected2.end());

    CorrectionSet actual1, actual2;
    std::tie(found, actual1) = finder.findAndBlock();
    BOOST_REQUIRE(found);
    std::tie(found, actual2) = finder.findAndBlock();
    BOOST_REQUIRE(found);

    BOOST_CHECK_EQUAL(actual1.size(), 3);
    BOOST_CHECK_EQUAL(actual2.size(), 3);

    std::tie(found, corr) = finder.findAndBlock();
    BOOST_REQUIRE(!found);

    std::sort(actual1.begin(), actual1.end());
    std::sort(actual2.begin(), actual2.end());

    BOOST_REQUIRE(actual1 != actual2);
    BOOST_CHECK(actual1 == expected1 || actual1 == expected2);
    BOOST_CHECK(actual2 == expected1 || actual2 == expected2);

    BOOST_REQUIRE(finder.moreCorrectionSets());

    // MCS {a0, a1, a2, a3}
    finder.setCardinality(4);
    std::tie(found, corr) = finder.findAndBlock();
    BOOST_REQUIRE(found);
    BOOST_CHECK_EQUAL(corr.size(), 4);
    expected = {a0, a1, a2, a3};
    std::sort(expected.begin(), expected.end());
    BOOST_CHECK(corr == expected);

    std::tie(found, corr) = finder.findAndBlock();
    BOOST_REQUIRE(!found);

    BOOST_REQUIRE(!finder.moreCorrectionSets());
}

