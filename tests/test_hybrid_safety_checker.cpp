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

#include "pme/util/hybrid_safety_checker.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/variable_manager.h"
#include "pme/util/proof_checker.h"

#define BOOST_TEST_MODULE HybridSafetyCheckerTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct SafetyFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, a0, a1, a2, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<HybridSafetyChecker> checker;

    SafetyFixture()
    {
        aig = aiger_init();

        l0 = 2;
        l1 = 4;
        l2 = 6;
        l3 = 8;
        a0 = 10;
        a1 = 12;
        a2 = 14;

        // l0' = l3
        aiger_add_latch(aig, l0, l3, "l0");

        // l1' = l0
        aiger_add_latch(aig, l1, l0, "l1");

        // l2' = l1
        aiger_add_latch(aig, l2, l1, "l2");

        // l3' = l2
        aiger_add_latch(aig, l3, l2, "l3");

        // o0 = l3 & ~l2 & ~l1 & l0
        aiger_add_and(aig, a0, l3, aiger_not(l2));
        aiger_add_and(aig, a1, aiger_not(l1), l0);
        aiger_add_and(aig, a2, a1, a0);
        aiger_add_output(aig, a2, "o0");
        o0 = a2;

        tr.reset(new TransitionRelation(vars, aig));
        prepareChecker();
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    void prepareChecker()
    {
        checker.reset(new HybridSafetyChecker(vars, *tr));
    }

    ~SafetyFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(safe)
{
    SafetyFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.setInit(l0, ID_FALSE);
    f.setInit(l1, ID_FALSE);
    f.setInit(l2, ID_FALSE);
    f.setInit(l3, ID_FALSE);

    f.prepareChecker();

    SafetyResult result = f.checker->prove();
    BOOST_CHECK_EQUAL(result.result, SAFE);

    ProofChecker pc(*f.tr, result.proof);
    BOOST_CHECK(pc.checkProof());
}

BOOST_AUTO_TEST_CASE(unsafe_bmc)
{
    SafetyFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.setInit(l0, ID_FALSE);
    f.setInit(l1, ID_TRUE);
    f.setInit(l2, ID_TRUE);
    f.setInit(l3, ID_FALSE);

    f.prepareChecker();
    f.checker->setKMax(10);

    SafetyResult result = f.checker->prove();
    BOOST_CHECK_EQUAL(result.result, UNSAFE);

    std::vector<Cube> expected_states;
    expected_states.push_back({negate(l0), l1, l2, negate(l3)});
    expected_states.push_back({negate(l0), negate(l1), l2, l3});
    // Last state is the property violation. Since the property
    // does not depend directly on the inputs, anything can be
    // in that spot.
    expected_states.push_back({});

    // Check the counter-example
    SafetyCounterExample cex = result.cex;
    BOOST_CHECK(cex.size() == expected_states.size());

    for (size_t i = 0; i < expected_states.size(); ++i)
    {
        Cube actual, expected;
        expected = expected_states[i];
        actual = cex[i].state;
        BOOST_CHECK(cex[i].inputs.empty());
        std::sort(expected.begin(), expected.end());
        std::sort(actual.begin(), actual.end());
        BOOST_CHECK(expected.empty() || expected == actual);
    }
}

BOOST_AUTO_TEST_CASE(unsafe_ic3)
{
    SafetyFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.setInit(l0, ID_FALSE);
    f.setInit(l1, ID_TRUE);
    f.setInit(l2, ID_TRUE);
    f.setInit(l3, ID_FALSE);

    f.prepareChecker();
    f.checker->setKMax(1);

    SafetyResult result = f.checker->prove();
    BOOST_CHECK_EQUAL(result.result, UNSAFE);

    std::vector<Cube> expected_states;
    expected_states.push_back({negate(l0), l1, l2, negate(l3)});
    expected_states.push_back({negate(l0), negate(l1), l2, l3});
    // Last state is the property violation. Since the property
    // does not depend directly on the inputs, anything can be
    // in that spot.
    expected_states.push_back({});

    // Check the counter-example
    SafetyCounterExample cex = result.cex;
    BOOST_CHECK(cex.size() == expected_states.size());

    for (size_t i = 0; i < expected_states.size(); ++i)
    {
        Cube actual, expected;
        expected = expected_states[i];
        actual = cex[i].state;
        BOOST_CHECK(cex[i].inputs.empty());
        std::sort(expected.begin(), expected.end());
        std::sort(actual.begin(), actual.end());
        BOOST_CHECK(expected.empty() || expected == actual);
    }
}


