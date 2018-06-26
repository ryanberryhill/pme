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
#include "pme/ic3/unsat_core_lifter.h"

#define BOOST_TEST_MODULE UNSATCoreLifterTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <algorithm>

using namespace PME;
using namespace PME::IC3;

struct LifterFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, o0;
    VariableManager vars;
    InductiveTrace trace;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<UNSATCoreLifter> lifter;
    GlobalState gs;

    LifterFixture(bool simplify = true)
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
        lifter.reset(new UNSATCoreLifter(vars, *tr, trace, gs));
    }

    ~LifterFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    void addClause(const Clause & cls)
    {
        Cube negc = negateVec(cls);
        LemmaID id = trace.addLemma(negc, LEVEL_INF);
        lifter->addLemma(id);
    }
};

BOOST_AUTO_TEST_CASE(basic_lift)
{
    LifterFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Cube pred = {l3, l2, l1, l0};
    Cube succ = {l1};
    Cube empty_inp, empty_pinp;

    LiftOptions opts(pred, succ, empty_inp, empty_pinp);
    Cube lifted = f.lifter->lift(opts);

    BOOST_REQUIRE(!lifted.empty());

    std::sort(lifted.begin(), lifted.end());
    std::sort(pred.begin(), pred.end());

    BOOST_CHECK(std::includes(pred.begin(), pred.end(), lifted.begin(), lifted.end()));
    auto it = std::find(lifted.begin(), lifted.end(), l0);
    BOOST_CHECK(it != lifted.end());
}

