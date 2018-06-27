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
#include "pme/ic3/ic3_solver.h"

#define BOOST_TEST_MODULE IC3Test
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <algorithm>

using namespace PME;
using namespace PME::IC3;

struct IC3Fixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, a0, a1, a2, o0;
    VariableManager vars;
    InductiveTrace trace;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<IC3Solver> solver;
    GlobalState gs;

    IC3Fixture(bool simplify = true)
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

        gs.opts.simplify = simplify;
        tr.reset(new TransitionRelation(vars, aig));
        prepareSolver();
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    void prepareSolver()
    {
        solver.reset(new IC3Solver(vars, *tr, gs));
    }

    ~IC3Fixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(safe)
{
    IC3Fixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.setInit(l0, ID_FALSE);
    f.setInit(l1, ID_FALSE);
    f.setInit(l2, ID_FALSE);
    f.setInit(l3, ID_FALSE);

    f.prepareSolver();

    IC3Result result = f.solver->prove();
    BOOST_CHECK(result.result == SAFE);
}

BOOST_AUTO_TEST_CASE(unsafe)
{
    IC3Fixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.setInit(l0, ID_TRUE);
    f.setInit(l1, ID_FALSE);
    f.setInit(l2, ID_FALSE);
    f.setInit(l3, ID_TRUE);

    f.prepareSolver();

    IC3Result result = f.solver->prove();
    BOOST_CHECK(result.result == UNSAFE);
}


