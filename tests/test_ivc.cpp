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
#include "pme/ivc/ivc.h"
#include "pme/ivc/caivc.h"

#define BOOST_TEST_MODULE IVCTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct IVCFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, a0, a1, a2, a3, a4, a5, a6, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    GlobalState gs;

    IVCFixture()
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
        // a6 <=> ALL(l0-l3), but the structure is important for IVCs
        // In particular, this circuit has different IVCs depending on the
        // initial state.
        aiger_add_and(aig, a0, l0, l1);
        aiger_add_and(aig, a1, l1, l2);
        aiger_add_and(aig, a2, l2, l3);
        aiger_add_and(aig, a3, l3, l0);
        aiger_add_and(aig, a4, a0, a1);
        aiger_add_and(aig, a5, a2, a3);
        aiger_add_and(aig, a6, a4, a5);
        aiger_add_output(aig, a6, "o0");
        o0 = a6;
        tr.reset(new TransitionRelation(vars, aig));
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    ~IVCFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(basic_ivc_caivc)
{
    IVCFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.setInit(l0, ID_FALSE);
    f.setInit(l1, ID_FALSE);
    f.setInit(l2, ID_FALSE);
    f.setInit(l3, ID_FALSE);

    CAIVCFinder finder(f.vars, *f.tr, f.gs);
    finder.findIVCs();
}

