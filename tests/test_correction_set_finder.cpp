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
    std::unique_ptr<DebugTransitionRelation> tr;

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
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    // Shrink a correction set to an MCS in order to simplify testing
    // for correction set finders that don't guarantee minimality
    void shrink(CorrectionSet & corr)
    {
        std::vector<std::set<ID>> mcses;

        ID b0 = tr->toInternal(a0);
        ID b1 = tr->toInternal(a1);
        ID b2 = tr->toInternal(a2);
        ID b3 = tr->toInternal(a3);
        ID b4 = tr->toInternal(a4);
        ID b5 = tr->toInternal(a5);
        ID b6 = tr->toInternal(a6);

        mcses.push_back({b6});
        mcses.push_back({b4, b5});
        mcses.push_back({b0, b1, b5});
        mcses.push_back({b2, b3, b4});
        mcses.push_back({b0, b1, b2, b3});

        std::sort(corr.begin(), corr.end());

        for (const auto & mcs : mcses)
        {
            if (std::includes(corr.begin(), corr.end(), mcs.begin(), mcs.end()))
            {
                corr.clear();
                corr.insert(corr.end(), mcs.begin(), mcs.end());
                return;
            }
        }

        // Reaching this line indicates failure
        BOOST_REQUIRE(false);
    }

    ~CorrectionSetFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(mcs)
{
    CorrectionSetFixture f;

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);
    ID a3 = f.tr->toInternal(f.a3);
    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    MCSFinder finder(f.vars, *f.tr);

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

BOOST_AUTO_TEST_CASE(mcs_over_gates)
{
    CorrectionSetFixture f;

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);
    ID a3 = f.tr->toInternal(f.a3);
    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    std::vector<ID> all = {a0, a1, a2, a3, a4, a5, a6};

    MCSFinder finder(f.vars, *f.tr);

    BOOST_REQUIRE(finder.moreCorrectionSets());

    bool found;
    CorrectionSet corr;

    // MCS {a6}
    finder.setCardinality(1);

    std::tie(found, corr) = finder.findAndBlockOverGates({a4});
    BOOST_CHECK(!found);

    std::tie(found, corr) = finder.findAndBlockOverGates({a4, a1});
    BOOST_CHECK(!found);

    std::tie(found, corr) = finder.findAndBlockOverGates(all);
    BOOST_REQUIRE(found);
    BOOST_CHECK_EQUAL(corr.size(), 1);
    BOOST_CHECK_EQUAL(corr.at(0), a6);

    BOOST_REQUIRE(finder.moreCorrectionSets());

    // MCS {a4, a5}
    finder.setCardinality(2);
    std::tie(found, corr) = finder.findAndBlockOverGates({a6, a5});
    BOOST_CHECK(!found);

    std::tie(found, corr) = finder.findAndBlockOverGates({a6, a4});
    BOOST_CHECK(!found);

    std::tie(found, corr) = finder.findAndBlockOverGates({a0, a1, a2, a3, a4, a5});

    BOOST_REQUIRE(found);
    BOOST_CHECK_EQUAL(corr.size(), 2);
    CorrectionSet expected = {a4, a5};
    std::sort(expected.begin(), expected.end());
    std::sort(corr.begin(), corr.end());
    BOOST_CHECK(corr == expected);

    std::tie(found, corr) = finder.findAndBlock();
    BOOST_CHECK(!found);

    std::tie(found, corr) = finder.findAndBlockOverGates({a4, a5});
    BOOST_CHECK(!found);
}

// Returns true if one element of all is contained within soln
bool containsOneOf(CorrectionSet soln,
                   const std::vector<CorrectionSet> all)
{
    std::sort(soln.begin(), soln.end());

    for (const CorrectionSet & corr : all)
    {
        bool subset = std::includes(soln.begin(), soln.end(), corr.begin(), corr.end());
        if (subset) { return true; }
    }

    return false;
}

BOOST_AUTO_TEST_CASE(approximate_mcs_over_gates)
{
    CorrectionSetFixture f;

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);
    ID a3 = f.tr->toInternal(f.a3);
    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    std::vector<ID> all = {a0, a1, a2, a3, a4, a5, a6};

    ApproximateMCSFinder finder(f.vars, *f.tr);

    // block MCS {a6} as the approximate finder assumes size-1 MCSes are
    // already blocked
    finder.blockSolution({a6});

    // Remaining MCSes are:
    // {a4, a5}
    // {a0, a1, a5}
    // {a2, a3, a4}
    // {a0, a1, a2, a3}
    std::vector<CorrectionSet> all_mcs;
    all_mcs.push_back({a4, a5});
    all_mcs.push_back({a0, a1, a5});
    all_mcs.push_back({a2, a3, a4});
    all_mcs.push_back({a0, a1, a2, a3});

    for (CorrectionSet & corr : all_mcs)
    {
        std::sort(corr.begin(), corr.end());
    }

    // Set an upper limit of 63 solutions (sum over i=1..6 of 6 choose i)
    unsigned count = 0;
    bool found = true;
    while (found && count <= 63)
    {
        CorrectionSet corr;
        std::tie(found, corr) = finder.findAndBlockOverGates(all);
        if (found)
        {
            BOOST_CHECK(containsOneOf(corr, all_mcs));
            count++;
        }
    }

    // All solutions should've been found
    std::tie(found, std::ignore) = finder.findAndBlockOverGates(all);
    BOOST_CHECK(!found);
}






template<class Finder>
void testFindMCS(bool guaranteed_minimal)
{
    // The MCSes are:
    // {a6}
    // {a4, a5}
    // {a0, a1, a5}
    // {a2, a3, a4}
    // {a0, a1, a2, a3}
    CorrectionSetFixture f;

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);
    ID a3 = f.tr->toInternal(f.a3);
    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    Finder finder(f.vars, *f.tr);

    BOOST_REQUIRE(finder.moreCorrectionSets());
    BOOST_REQUIRE(finder.moreCorrectionSets(1));
    BOOST_REQUIRE(finder.moreCorrectionSets(4));
    BOOST_REQUIRE(finder.moreCorrectionSets(6));
    BOOST_REQUIRE(finder.moreCorrectionSets(10));

    bool found;
    CorrectionSet corr;

    // MCS {a6}
    std::tie(found, corr) = finder.findNext();

    BOOST_REQUIRE(found);
    if (!guaranteed_minimal) { f.shrink(corr); }
    BOOST_CHECK_EQUAL(corr.size(), 1);
    BOOST_CHECK_EQUAL(corr.at(0), a6);

    BOOST_REQUIRE(!finder.moreCorrectionSets(1));
    BOOST_REQUIRE(finder.moreCorrectionSets());
    BOOST_REQUIRE(finder.moreCorrectionSets(2));

    // MCS {a4, a5}
    std::tie(found, corr) = finder.findNext();

    BOOST_REQUIRE(found);
    if (!guaranteed_minimal) { f.shrink(corr); }
    BOOST_CHECK_EQUAL(corr.size(), 2);
    CorrectionSet expected = {a4, a5};
    std::sort(expected.begin(), expected.end());
    std::sort(corr.begin(), corr.end());
    BOOST_CHECK(corr == expected);

    BOOST_REQUIRE(finder.moreCorrectionSets());
    BOOST_REQUIRE(finder.moreCorrectionSets(3));
    BOOST_REQUIRE(!finder.moreCorrectionSets(2));
    BOOST_REQUIRE(!finder.moreCorrectionSets(1));

    // MCSes {a0, a1, a5} and {a2, a3, a4}
    CorrectionSet expected1 = {a0, a1, a5};
    CorrectionSet expected2 = {a2, a3, a4};
    std::sort(expected1.begin(), expected1.end());
    std::sort(expected2.begin(), expected2.end());

    CorrectionSet actual1, actual2;
    std::tie(found, actual1) = finder.findNext();
    BOOST_REQUIRE(found);
    if (!guaranteed_minimal) { f.shrink(actual1); }
    BOOST_REQUIRE(finder.moreCorrectionSets(3));

    std::tie(found, actual2) = finder.findNext();
    BOOST_REQUIRE(found);
    if (!guaranteed_minimal) { f.shrink(actual2); }

    BOOST_CHECK(!finder.moreCorrectionSets(3));
    BOOST_CHECK(finder.moreCorrectionSets());
    BOOST_CHECK(finder.moreCorrectionSets(4));

    BOOST_CHECK_EQUAL(actual1.size(), 3);
    BOOST_CHECK_EQUAL(actual2.size(), 3);

    std::sort(actual1.begin(), actual1.end());
    std::sort(actual2.begin(), actual2.end());

    BOOST_REQUIRE(actual1 != actual2);
    BOOST_CHECK(actual1 == expected1 || actual1 == expected2);
    BOOST_CHECK(actual2 == expected1 || actual2 == expected2);

    // MCS {a0, a1, a2, a3}
    std::tie(found, corr) = finder.findNext();
    BOOST_REQUIRE(found);
    if (!guaranteed_minimal) { f.shrink(corr); }
    BOOST_CHECK_EQUAL(corr.size(), 4);
    expected = {a0, a1, a2, a3};
    std::sort(expected.begin(), expected.end());
    BOOST_CHECK(corr == expected);

    std::tie(found, corr) = finder.findNext();
    BOOST_REQUIRE(!found);

    BOOST_REQUIRE(!finder.moreCorrectionSets(1));
    BOOST_REQUIRE(!finder.moreCorrectionSets(4));
    BOOST_REQUIRE(!finder.moreCorrectionSets(10));
    BOOST_REQUIRE(!finder.moreCorrectionSets());
}

template<class Finder>
void testFindMCSExternallyBlocked(bool guaranteed_minimal)
{
    // The MCSes are:
    // {a6}
    // {a4, a5}
    // {a0, a1, a5}
    // {a2, a3, a4}
    // {a0, a1, a2, a3}
    CorrectionSetFixture f;

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);
    ID a3 = f.tr->toInternal(f.a3);
    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    Finder finder(f.vars, *f.tr);

    finder.block({a5, a4});
    finder.block({a0, a1, a5});

    BOOST_REQUIRE(finder.moreCorrectionSets());
    BOOST_REQUIRE(finder.moreCorrectionSets(1));
    BOOST_REQUIRE(finder.moreCorrectionSets(4));
    BOOST_REQUIRE(finder.moreCorrectionSets(6));
    BOOST_REQUIRE(finder.moreCorrectionSets(10));

    bool found;
    CorrectionSet corr;

    // MCS {a6}
    std::tie(found, corr) = finder.findNext();

    BOOST_REQUIRE(found);
    if (!guaranteed_minimal) { f.shrink(corr); }
    BOOST_CHECK_EQUAL(corr.size(), 1);
    BOOST_CHECK_EQUAL(corr.at(0), a6);

    BOOST_REQUIRE(!finder.moreCorrectionSets(1));
    BOOST_REQUIRE(finder.moreCorrectionSets());
    BOOST_REQUIRE(finder.moreCorrectionSets(3));

    // MCSes {a2, a3, a4}
    CorrectionSet expected = {a2, a3, a4};
    std::sort(expected.begin(), expected.end());

    std::tie(found, corr) = finder.findNext();
    BOOST_REQUIRE(found);
    if (!guaranteed_minimal) { f.shrink(corr); }

    BOOST_CHECK(!finder.moreCorrectionSets(3));
    BOOST_CHECK(finder.moreCorrectionSets(4));

    BOOST_CHECK_EQUAL(corr.size(), 3);
    std::sort(corr.begin(), corr.end());

    BOOST_CHECK(corr == expected);

    // MCS {a0, a1, a2, a3}
    std::tie(found, corr) = finder.findNext();
    BOOST_REQUIRE(found);
    if (!guaranteed_minimal) { f.shrink(corr); }
    BOOST_CHECK_EQUAL(corr.size(), 4);
    expected = {a0, a1, a2, a3};
    std::sort(expected.begin(), expected.end());
    BOOST_CHECK(corr == expected);

    std::tie(found, corr) = finder.findNext();
    BOOST_REQUIRE(!found);

    BOOST_REQUIRE(!finder.moreCorrectionSets());
}

BOOST_AUTO_TEST_CASE(basic_mcs_finder)
{
    testFindMCS<BasicMCSFinder>(true);
    testFindMCSExternallyBlocked<BasicMCSFinder>(true);
}

BOOST_AUTO_TEST_CASE(bmc_finder)
{
    testFindMCS<BMCCorrectionSetFinder>(false);
    testFindMCSExternallyBlocked<BMCCorrectionSetFinder>(false);
}
