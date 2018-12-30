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

#include "pme/util/check_cex.h"

#define BOOST_TEST_MODULE CheckCexTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct CEXFixture
{
    aiger * aig;
    ExternalID i0, l0, l1, l2, l3, o0, a0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;

    CEXFixture()
    {
        aig = aiger_init();

        i0 = 2;
        l0 = 4;
        l1 = 6;
        l2 = 8;
        l3 = 10;
        a0 = 12;

        aiger_add_input(aig, i0, "i0");

        // l0' = i0
        aiger_add_latch(aig, l0, i0, "l0");

        // l1' = l0
        aiger_add_latch(aig, l1, l0, "l1");

        // l2' = l1
        aiger_add_latch(aig, l2, l1, "l2");

        // l3' = l2
        aiger_add_latch(aig, l3, l2, "l3");

        // a0 = l2 & l3
        aiger_add_and(aig, a0, l2, l3);

        // o0 = a0 = l2 & l3
        aiger_add_output(aig, a0, "o0");
        o0 = a0;

        tr.reset(new TransitionRelation(vars, aig));
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    ~CEXFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(trivial_good_cex)
{
    CEXFixture f;

    ID i0 = f.tr->toInternal(f.i0);
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.setInit(l0, ID_TRUE);
    f.setInit(l1, ID_TRUE);
    f.setInit(l2, ID_TRUE);
    f.setInit(l3, ID_TRUE);

    SafetyCounterExample cex;
    Cube inputs = {i0};
    Cube state = {l0, l1, l2, l3};
    cex.emplace_back(inputs, state);

    bool is_valid = checkCounterExample(f.vars, *f.tr, cex);
    bool is_valid_sim = checkSimulation(f.vars, *f.tr, cex);
    BOOST_CHECK(is_valid);
    BOOST_CHECK(is_valid_sim);

    // Input doesn't matter, try negating it
    inputs = {negate(i0)};
    cex.clear();
    cex.emplace_back(inputs, state);

    is_valid = checkCounterExample(f.vars, *f.tr, cex);
    is_valid_sim = checkSimulation(f.vars, *f.tr, cex);
    BOOST_CHECK(is_valid);
    BOOST_CHECK(is_valid_sim);
}

BOOST_AUTO_TEST_CASE(trivial_bad_cex_no_bad_state)
{
    CEXFixture f;

    ID i0 = f.tr->toInternal(f.i0);
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    SafetyCounterExample cex;
    Cube inputs = {i0};
    Cube state = {negate(l0), negate(l1), negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    bool is_valid = checkCounterExample(f.vars, *f.tr, cex);
    BOOST_CHECK(!is_valid);

    bool is_valid_sim = checkSimulation(f.vars, *f.tr, cex);
    BOOST_CHECK(is_valid_sim);
}

BOOST_AUTO_TEST_CASE(nontrivial_bad_cex_no_bad_state)
{
    CEXFixture f;

    ID i0 = f.tr->toInternal(f.i0);
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    SafetyCounterExample cex;

    Cube inputs = {i0};
    Cube state = {negate(l0), negate(l1), negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {negate(i0)};
    state = {l0, negate(l1), negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {negate(l0), l1, negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {l0, negate(l1), l2, negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {l0, l1, negate(l2), l3};
    cex.emplace_back(inputs, state);

    bool is_valid = checkCounterExample(f.vars, *f.tr, cex);
    BOOST_CHECK(!is_valid);

    bool is_valid_sim = checkSimulation(f.vars, *f.tr, cex);
    BOOST_CHECK(is_valid_sim);
}


BOOST_AUTO_TEST_CASE(good_cex_one_step_short)
{
    CEXFixture f;

    ID i0 = f.tr->toInternal(f.i0);
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    SafetyCounterExample cex;

    Cube inputs = {i0};
    Cube state = {negate(l0), negate(l1), negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {negate(i0)};
    state = {l0, negate(l1), negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {negate(l0), l1, negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {l0, negate(l1), l2, negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {l0, l1, negate(l2), l3};
    cex.emplace_back(inputs, state);

    inputs = {negate(i0)};
    state = {l0, l1, l2, negate(l3)};
    cex.emplace_back(inputs, state);

    bool is_valid = checkCounterExample(f.vars, *f.tr, cex);
    BOOST_CHECK(!is_valid);

    bool is_valid_sim = checkSimulation(f.vars, *f.tr, cex);
    BOOST_CHECK(is_valid_sim);
}

BOOST_AUTO_TEST_CASE(good_cex)
{
    CEXFixture f;

    ID i0 = f.tr->toInternal(f.i0);
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    SafetyCounterExample cex;

    Cube inputs = {i0};
    Cube state = {negate(l0), negate(l1), negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {negate(i0)};
    state = {l0, negate(l1), negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {negate(l0), l1, negate(l2), negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {l0, negate(l1), l2, negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {l0, l1, negate(l2), l3};
    cex.emplace_back(inputs, state);

    inputs = {negate(i0)};
    state = {l0, l1, l2, negate(l3)};
    cex.emplace_back(inputs, state);

    inputs = {i0};
    state = {negate(l0), l1, l2, l3};
    cex.emplace_back(inputs, state);

    bool is_valid = checkCounterExample(f.vars, *f.tr, cex);
    BOOST_CHECK(is_valid);

    bool is_valid_sim = checkSimulation(f.vars, *f.tr, cex);
    BOOST_CHECK(is_valid_sim);
}

