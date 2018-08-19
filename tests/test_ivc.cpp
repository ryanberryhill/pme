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
#include "pme/ivc/marco_ivc.h"
#include "pme/ivc/ivc_bf.h"

#define BOOST_TEST_MODULE IVCTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <limits>

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

void sortIVCs(std::vector<IVC> & ivcs)
{
    for (IVC ivc : ivcs)
    {
        std::sort(ivc.begin(), ivc.end());
    }
    std::sort(ivcs.begin(), ivcs.end());
}

template<class T>
void runIVCTest(bool find_all)
{
    IVCFixture f;

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);
    ID a3 = f.tr->toInternal(f.a3);
    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    std::vector<IVC> mivcs = { {a0, a4, a6},
                               {a1, a4, a6},
                               {a2, a5, a6},
                               {a3, a5, a6} };
    sortIVCs(mivcs);

    T finder(f.vars, *f.tr, f.gs);
    finder.findIVCs();

    IVC min_ivc;
    if (find_all)
    {
        BOOST_CHECK_EQUAL(finder.numMIVCs(), mivcs.size());
        BOOST_REQUIRE(finder.minimumIVCKnown());
        min_ivc = finder.getMinimumIVC();
        std::sort(min_ivc.begin(), min_ivc.end());
    }

    bool min_found = false;

    size_t min_size = std::numeric_limits<size_t>::max();
    for (size_t i = 0; i < finder.numMIVCs(); ++i)
    {
        IVC mivc = finder.getMIVC(i);

        std::sort(mivc.begin(), mivc.end());

        auto it = std::find(mivcs.begin(), mivcs.end(), mivc);
        BOOST_CHECK(it != mivcs.end());

        min_size = std::min(min_size, mivc.size());

        if (mivc == min_ivc)
        {
            min_found = true;
        }
    }

    if (find_all)
    {
        BOOST_CHECK(min_found);
        BOOST_CHECK_EQUAL(finder.getMinimumIVC().size(), min_size);
    }
}

template<class T>
void runAllMIVCTest()
{
    runIVCTest<T>(true);
}

template<class T>
void runMIVCTest()
{
    runIVCTest<T>(false);
}

BOOST_AUTO_TEST_CASE(basic_ivc_caivc)
{
    runAllMIVCTest<CAIVCFinder>();
}

BOOST_AUTO_TEST_CASE(basic_ivc_marco)
{
    runAllMIVCTest<MARCOIVCFinder>();
}

BOOST_AUTO_TEST_CASE(basic_ivc_ivcbf)
{
    runMIVCTest<IVCBFFinder>();
}

