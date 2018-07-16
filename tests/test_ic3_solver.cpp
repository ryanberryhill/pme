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
#include "pme/util/proof_checker.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/debug_transition_relation.h"

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
    std::unique_ptr<DebugTransitionRelation> debug_tr;
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
        debug_tr.reset(new DebugTransitionRelation(vars, aig));
        prepareSolver();
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    void prepareSolver(TransitionRelation & tr_to_use)
    {
        solver.reset(new IC3Solver(vars, tr_to_use, gs));
    }

    void prepareSolver()
    {
        prepareSolver(*tr);
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
    BOOST_CHECK_EQUAL(result.result, SAFE);

    ProofChecker pc(*f.tr, result.proof, f.gs);
    BOOST_CHECK(pc.checkProof());
}

BOOST_AUTO_TEST_CASE(trivual_unsafe)
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
    BOOST_CHECK_EQUAL(result.result, UNSAFE);

    // Check the counter-example
    SafetyCounterExample cex = result.cex;
    BOOST_CHECK(cex.size() == 1);

    Cube actual, expected;
    expected = {l0, negate(l1), negate(l2), l3};
    actual = cex[0].state;
    std::sort(expected.begin(), expected.end());
    std::sort(actual.begin(), actual.end());
    BOOST_CHECK(expected == actual);
}

BOOST_AUTO_TEST_CASE(unsafe)
{
    IC3Fixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.setInit(l0, ID_FALSE);
    f.setInit(l1, ID_TRUE);
    f.setInit(l2, ID_TRUE);
    f.setInit(l3, ID_FALSE);

    f.prepareSolver();

    IC3Result result = f.solver->prove();
    BOOST_CHECK_EQUAL(result.result, UNSAFE);

    std::vector<Cube> expected_states;
    expected_states.push_back({negate(l0), l1, l2, negate(l3)});
    expected_states.push_back({negate(l0), negate(l1), l2, l3});
    // Last state is the property violation and is empty
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
        BOOST_CHECK(expected == actual);
    }
}

BOOST_AUTO_TEST_CASE(lemma_access)
{
    IC3Fixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    f.solver->addLemma({negate(l0)}, 1);
    f.solver->addLemma({negate(l1)}, 2);
    f.solver->addLemma({negate(l2)}, 2);
    f.solver->addLemma({negate(l3)}, 2);

    Frame f1 = f.solver->getFrame(1);
    Frame f2 = f.solver->getFrame(2);
    Frame f3 = f.solver->getFrame(3);
    Frame finf = f.solver->getFrame(LEVEL_INF);

    BOOST_CHECK_EQUAL(f1.size(), 1);
    BOOST_CHECK_EQUAL(f2.size(), 3);
    BOOST_CHECK_EQUAL(f3.size(), 0);
    BOOST_CHECK_EQUAL(finf.size(), 0);

    f.solver->addLemma({negate(l0)}, LEVEL_INF);
    f.solver->addLemma({negate(l1)}, LEVEL_INF);
    f.solver->addLemma({negate(l2)}, LEVEL_INF);
    f.solver->addLemma({negate(l3)}, LEVEL_INF);

    f1 = f.solver->getFrame(1);
    f2 = f.solver->getFrame(2);
    f3 = f.solver->getFrame(3);
    finf = f.solver->getFrame(LEVEL_INF);

    BOOST_CHECK_EQUAL(f1.size(), 0);
    BOOST_CHECK_EQUAL(f2.size(), 0);
    BOOST_CHECK_EQUAL(f3.size(), 0);
    BOOST_CHECK_EQUAL(finf.size(), 4);
}

BOOST_AUTO_TEST_CASE(complex_init_state)
{
    IC3Fixture f;

    f.debug_tr->setCardinality(1);
    f.prepareSolver(*f.debug_tr);

    IC3Result result = f.solver->prove();
    BOOST_CHECK_EQUAL(result.result, UNSAFE);
}

BOOST_AUTO_TEST_CASE(complex_init_state_incrementality)
{
    IC3Fixture f;

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);

    std::set<ID> debug_latches(f.debug_tr->begin_debug_latches(),
                               f.debug_tr->end_debug_latches());
    BOOST_REQUIRE(!debug_latches.empty());

    // All zero initial state, 0 cardinality = SAFE
    f.debug_tr->setCardinality(0);

    f.prepareSolver(*f.debug_tr);
    IC3Result result = f.solver->prove();
    BOOST_CHECK_EQUAL(result.result, SAFE);

    // a2 is a solution of cardinality 1
    f.debug_tr->setCardinality(1);
    f.solver->initialStatesExpanded();

    result = f.solver->prove();
    BOOST_REQUIRE_EQUAL(result.result, UNSAFE);
    // Find which debug latch was active, block, reset
    SafetyCounterExample cex = result.cex;
    BOOST_REQUIRE(cex.size() > 0);
    Cube init = cex.at(0).state;

    ID soln_latch = ID_NULL;
    for (ID latch : init)
    {
        if (debug_latches.count(strip(latch)) > 0)
        {
            if (!is_negated(latch))
            {
                BOOST_REQUIRE_EQUAL(soln_latch, ID_NULL);
                soln_latch = latch;
            }
        }
    }

    BOOST_REQUIRE(soln_latch != ID_NULL);
    ID soln_gate = f.debug_tr->gateForDebugLatch(soln_latch);
    BOOST_CHECK_EQUAL(soln_gate, a2);

    f.solver->restrictInitialStates({negate(soln_latch)});
    f.solver->initialStatesRestricted();

    // Should be safe after blocking a2
    result = f.solver->prove();
    BOOST_CHECK_EQUAL(result.result, SAFE);

    // a1 and a0 are a solution of cardinality 2
    f.debug_tr->setCardinality(2);
    f.solver->initialStatesExpanded();

    result = f.solver->prove();
    BOOST_CHECK_EQUAL(result.result, UNSAFE);

    cex = result.cex;
    BOOST_REQUIRE(cex.size() > 0);
    init = cex.at(0).state;

    std::set<ID> soln_latches;
    for (ID latch : init)
    {
        if (debug_latches.count(strip(latch)) > 0)
        {
            if (!is_negated(latch))
            {
                ID soln_gate = f.debug_tr->gateForDebugLatch(latch);
                BOOST_CHECK(soln_gate == a1 || soln_gate == a0);
                soln_latches.insert(latch);
            }
        }
    }

    BOOST_CHECK_EQUAL(soln_latches.size(), 2);

    Clause block(soln_latches.begin(), soln_latches.end());
    block = negateVec(block);

    f.solver->restrictInitialStates(block);
    f.solver->initialStatesRestricted();

    // Now it should be safe at any cardinality
    result = f.solver->prove();
    BOOST_CHECK_EQUAL(result.result, SAFE);

    f.debug_tr->clearCardinality();
    f.solver->initialStatesExpanded();

    result = f.solver->prove();
    BOOST_CHECK_EQUAL(result.result, SAFE);

    f.debug_tr->setCardinality(3);
    f.solver->initialStatesRestricted();

    result = f.solver->prove();
    BOOST_CHECK_EQUAL(result.result, SAFE);
}

