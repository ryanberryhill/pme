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
#include "pme/ic3/ic3.h"
#include "pme/util/ic3_debugger.h"
#include "pme/util/bmc_debugger.h"
#include "pme/util/hybrid_debugger.h"
#include "pme/engine/debug_transition_relation.h"

#define BOOST_TEST_MODULE DebuggerTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;
using namespace PME::IC3;

template<class DebuggerType>
struct DebugFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, a0, a1, a2, o0;
    VariableManager vars;
    std::unique_ptr<DebugTransitionRelation> debug_tr;
    std::unique_ptr<Debugger> debugger;
    GlobalState gs;

    DebugFixture(bool simplify = true)
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
        debug_tr.reset(new DebugTransitionRelation(vars, aig));
        prepareDebugger();
    }

    void setInit(ID latch, ID val)
    {
        debug_tr->setInit(latch, val);
    }

    void prepareDebugger()
    {
        debugger.reset(new DebuggerType(vars, *debug_tr, gs));
    }

    ~DebugFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

template<class T>
void testBasicDebug()
{
    DebugFixture<T> f;

    f.prepareDebugger();

    // All zero initial state, 0 cardinality = SAFE
    f.debugger->setCardinality(0);

    bool found = false;
    std::vector<ID> soln;
    std::tie(found, soln) = f.debugger->debug();
    BOOST_CHECK(!found);
    BOOST_CHECK(soln.empty());
}

template<class T>
void testIncrementalDebug()
{
    DebugFixture<T> f;

    ID a0 = f.debug_tr->toInternal(f.a0);
    ID a1 = f.debug_tr->toInternal(f.a1);
    ID a2 = f.debug_tr->toInternal(f.a2);

    f.prepareDebugger();

    // All zero initial state, 0 cardinality = SAFE
    f.debugger->setCardinality(0);

    bool found = false;
    std::vector<ID> soln;
    std::tie(found, soln) = f.debugger->debug();
    BOOST_CHECK(!found);
    BOOST_CHECK(soln.empty());

    // a2 is a solution of cardinality 1
    f.debugger->setCardinality(1);

    std::tie(found, soln) = f.debugger->debug();
    BOOST_CHECK(found);
    BOOST_CHECK(!soln.empty());
    BOOST_CHECK_EQUAL(soln.size(), 1);
    BOOST_CHECK_EQUAL(soln.at(0), a2);

    f.debugger->blockSolution(soln);

    // (a0, a1) is a solution of cardinality 2
    f.debugger->setCardinality(2);

    // This time call debugAndBlock
    std::tie(found, soln) = f.debugger->debugAndBlock();
    BOOST_CHECK(found);
    BOOST_CHECK(!soln.empty());

    BOOST_CHECK_EQUAL(soln.size(), 2);
    std::sort(soln.begin(), soln.end());
    std::vector<ID> expected_soln = {a0, a1};
    std::sort(expected_soln.begin(), expected_soln.end());

    BOOST_CHECK(soln == expected_soln);

    // There should be no further solutions
    std::tie(found, std::ignore) = f.debugger->debug();
    BOOST_CHECK(!found);

    f.debugger->clearCardinality();

    std::tie(found, std::ignore) = f.debugger->debug();
    BOOST_CHECK(!found);

    f.debugger->setCardinality(3);

    std::tie(found, std::ignore) = f.debugger->debug();
    BOOST_CHECK(!found);
}

BOOST_AUTO_TEST_CASE(basic_debug_ic3)
{
    testBasicDebug<IC3Debugger>();
}

BOOST_AUTO_TEST_CASE(basic_debug_bmc)
{
    testBasicDebug<BMCDebugger>();
}

BOOST_AUTO_TEST_CASE(basic_debug_hybrid)
{
    testBasicDebug<HybridDebugger>();
}

BOOST_AUTO_TEST_CASE(incremental_debug_ic3)
{
    testIncrementalDebug<IC3Debugger>();
}

BOOST_AUTO_TEST_CASE(incremental_debug_bmc)
{
    testIncrementalDebug<BMCDebugger>();
}

BOOST_AUTO_TEST_CASE(incremental_debug_hyrbid)
{
    testIncrementalDebug<HybridDebugger>();
}

BOOST_AUTO_TEST_CASE(ic3debugger_lemma_access)
{
    DebugFixture<IC3Debugger> f;

    IC3Debugger * debugger = dynamic_cast<IC3Debugger *>(f.debugger.get());

    ID l0 = f.debug_tr->toInternal(f.l0);
    ID l1 = f.debug_tr->toInternal(f.l1);
    ID l2 = f.debug_tr->toInternal(f.l2);
    ID l3 = f.debug_tr->toInternal(f.l3);

    debugger->addLemma({negate(l0)}, 1);
    debugger->addLemma({negate(l1)}, 2);
    debugger->addLemma({negate(l2)}, 2);
    debugger->addLemma({negate(l3)}, 2);

    std::vector<Cube> f1 = debugger->getFrameCubes(1);
    std::vector<Cube> f2 = debugger->getFrameCubes(2);
    std::vector<Cube> f3 = debugger->getFrameCubes(3);
    std::vector<Cube> finf = debugger->getFrameCubes(LEVEL_INF);

    BOOST_CHECK_EQUAL(f1.size(), 1);
    BOOST_CHECK_EQUAL(f2.size(), 3);
    BOOST_CHECK_EQUAL(f3.size(), 0);
    BOOST_CHECK_EQUAL(finf.size(), 0);

    std::vector<Cube> expected = {{negate(l0)}};
    BOOST_CHECK(f1 == expected);

    expected = {{negate(l1)}, {negate(l2)}, {negate(l3)}};
    std::sort(expected.begin(), expected.end());
    std::sort(f2.begin(), f2.end());

    BOOST_CHECK(f2 == expected);

    debugger->addLemma({negate(l0)}, LEVEL_INF);
    debugger->addLemma({negate(l1)}, LEVEL_INF);
    debugger->addLemma({negate(l2)}, LEVEL_INF);
    debugger->addLemma({negate(l3)}, LEVEL_INF);

    f1 = debugger->getFrameCubes(1);
    f2 = debugger->getFrameCubes(2);
    f3 = debugger->getFrameCubes(3);
    finf = debugger->getFrameCubes(LEVEL_INF);

    BOOST_CHECK_EQUAL(f1.size(), 0);
    BOOST_CHECK_EQUAL(f2.size(), 0);
    BOOST_CHECK_EQUAL(f3.size(), 0);
    BOOST_CHECK_EQUAL(finf.size(), 4);
}
