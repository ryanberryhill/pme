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
#include "pme/ivc/bvc_frame_solver.h"
#include "pme/ic3/ic3_solver.h"

#define BOOST_TEST_MODULE BVCFrameSolverTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct BVCFrameSolverFixture
{
    aiger * aig;
    ExternalID i0, l0, l1, l2, l3, a0, a1, a2, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<BVCFrameSolver> solver;

    BVCFrameSolverFixture(unsigned level)
    {
        aig = aiger_init();

        i0 = 2;
        l0 = 4;
        l1 = 6;
        l2 = 8;
        l3 = 10;
        a0 = 12;
        a1 = 14;
        a2 = 16;

        aiger_add_input(aig, i0, "i0");

        // l0' = i0
        aiger_add_latch(aig, l0, i0, "l0");

        // l1' = l3 & l0 = a0
        aiger_add_latch(aig, l1, a0, "l1");

        // l2' = l3 & l1 = a1
        aiger_add_latch(aig, l2, a1, "l2");

        // l3' = l1 & l2 = a2
        aiger_add_latch(aig, l3, a2, "l3");

        aiger_add_and(aig, a0, l0, l3);
        aiger_add_and(aig, a1, l1, l3);
        aiger_add_and(aig, a2, l1, l2);

        // o0 = l3
        aiger_add_output(aig, l3, "o0");
        o0 = l3;

        tr.reset(new TransitionRelation(vars, aig));

        setInit(tr->toInternal(l0), ID_FALSE);
        setInit(tr->toInternal(l1), ID_FALSE);
        setInit(tr->toInternal(l2), ID_FALSE);
        setInit(tr->toInternal(l3), ID_FALSE);

        prepareSolver(level);
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    void prepareSolver(unsigned level)
    {
        solver.reset(new BVCFrameSolver(vars, *tr, level));
    }

    ~BVCFrameSolverFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(bvc_abstraction_initially_empty)
{
    BVCFrameSolverFixture f(5);

    // Check the initial abstraction is empty
    std::set<ID> empty;
    BOOST_CHECK(f.solver->getAbstraction() == empty);
}

BOOST_AUTO_TEST_CASE(bvc_extract_no_abs)
{
    BVCFrameSolverFixture f(0);

    ID i0 = f.tr->toInternal(f.i0);
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Cube test;
    BVCSolution expected;

    test = { negate(l0), negate(l1) };
    expected = { negate(l0), negate(l1), negate(l2), negate(l3) };
    std::sort(expected.begin(), expected.end());

    // There should be a predecessor (i.e., the initial state)
    BOOST_REQUIRE(f.solver->solutionExistsUnprimed(test));
    BOOST_REQUIRE(f.solver->predecessorExistsUnprimed(test));

    BVCBlockResult result = f.solver->solveUnprimed(0, test);

    BOOST_REQUIRE(result.sat);
    BOOST_CHECK(result.solution.empty());
    BOOST_REQUIRE(!result.predecessor.empty());

    BVCPredecessor pred = result.predecessor;
    std::sort(pred.begin(), pred.end());
    BOOST_CHECK(pred == expected);

    Cube state = result.state;
    std::sort(state.begin(), state.end());
    BOOST_CHECK(state == expected);

    // I think the primed inputs are don't cares
    Cube pinp = result.pinputs;
    BOOST_CHECK(pinp.size() == 1);

    // The input is a don't care here. Check for exactly one of i0 and !i0
    BOOST_CHECK_EQUAL(result.inputs.size(), 1);
    BOOST_CHECK_EQUAL(strip(result.inputs.at(0)), i0);
}

BOOST_AUTO_TEST_CASE(bvc_extract_abs)
{
    BVCFrameSolverFixture f(2);

    Cube bad = { f.tr->bad() };
    ID i0 = f.tr->toInternal(f.i0);
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);
    ID a2 = f.tr->toInternal(f.a2);

    f.solver->setAbs({a2});

    Cube test;

    test = { negate(l0), negate(l1), l2, negate(l3) };
    Cube expected_pred = { negate(l0), l1, negate(l2), l3 };
    std::sort(expected_pred.begin(), expected_pred.end());
    std::sort(test.begin(), test.end());

    // There should be a predecessor (i.e., the initial state)
    BOOST_REQUIRE(f.solver->solutionExists(test));
    BOOST_REQUIRE(f.solver->predecessorExists(test));

    BVCBlockResult result = f.solver->solve(0, test);

    BOOST_REQUIRE(result.sat);
    BOOST_CHECK(result.solution.empty());
    BOOST_REQUIRE(!result.predecessor.empty());

    BVCPredecessor pred = result.predecessor;
    std::sort(pred.begin(), pred.end());
    BOOST_CHECK(pred == expected_pred);

    Cube state = result.state;
    std::sort(state.begin(), state.end());
    BOOST_CHECK(state == test);

    // Primed input is a don't care
    Cube pinp = result.pinputs;
    BOOST_CHECK(pinp.size() == 1);

    // The input should be false
    BOOST_CHECK_EQUAL(result.inputs.size(), 1);
    BOOST_CHECK_EQUAL(result.inputs.at(0), negate(i0));
}

BOOST_AUTO_TEST_CASE(basic_bvc_k0_unprimed)
{
    BVCFrameSolverFixture f(0);

    Cube bad = { f.tr->bad() };
    ID a2 = f.tr->toInternal(f.a2);

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    // bad is a latch output, so unless the initial state is bad (it isn't),
    // there should be no solution or predecessor (i.e., bad initial state)
    BOOST_REQUIRE(!f.solver->solutionExistsUnprimed(bad));
    BOOST_REQUIRE(!f.solver->predecessorExistsUnprimed(bad));

    // For this case, there should be a solution (i.e., a2 itself)
    Cube test = { a2 };

    BOOST_REQUIRE(f.solver->solutionExistsUnprimed(test));
    BOOST_REQUIRE(!f.solver->predecessorExistsUnprimed(test));

    bool sat = false;
    BVCSolution soln;
    BVCPredecessor pred;

    std::tie(sat, soln, pred) = f.solver->solve(1, test);

    BOOST_REQUIRE(sat);
    BOOST_CHECK(pred.empty());
    BOOST_CHECK(soln == test);

    test = { negate(l0), negate(l1) };
    BVCSolution expected = { negate(l0), negate(l1), negate(l2), negate(l3) };
    std::sort(expected.begin(), expected.end());

    // For this case, there should be a predecessor (i.e., the initial state)
    BOOST_REQUIRE(f.solver->solutionExistsUnprimed(test));
    BOOST_REQUIRE(f.solver->predecessorExistsUnprimed(test));

    std::tie(sat, soln, pred) = f.solver->solve(0, test);
    std::sort(soln.begin(), soln.end());

    BOOST_REQUIRE(sat);
    BOOST_CHECK(soln.empty());
    BOOST_CHECK(!pred.empty());
    BOOST_CHECK(pred == expected);
}

BOOST_AUTO_TEST_CASE(basic_bvc_k1)
{
    BVCFrameSolverFixture f(0);

    ID a2 = f.tr->toInternal(f.a2);
    Cube bad = { f.tr->bad() };

    // There should be a correction set of {a2}
    BOOST_REQUIRE(f.solver->solutionExists(bad));

    bool sat = false;
    BVCSolution soln;
    BVCPredecessor pred;

    BOOST_REQUIRE(!f.solver->predecessorExists(bad));
    BOOST_CHECK(f.solver->solutionExists(bad));
    BOOST_CHECK(f.solver->solutionAtCardinality(1, bad));

    std::tie(sat, soln, pred) = f.solver->solve(1, bad);

    BOOST_REQUIRE(sat);
    BOOST_CHECK(pred.empty());
    BOOST_REQUIRE(!soln.empty());

    BVCSolution expected = {a2};
    BOOST_CHECK(soln == expected);

    f.solver->blockSolution(soln);

    // There shouldn't be any more correction sets for k = 1
    BOOST_CHECK(!f.solver->solutionExists(bad));

    // Check the other interfaces
    BOOST_CHECK(!f.solver->predecessorExists(bad));
    BOOST_CHECK(!f.solver->solutionAtCardinality(0, bad));
    BOOST_CHECK(!f.solver->solutionAtCardinality(1, bad));
    BOOST_CHECK(!f.solver->solutionAtCardinality(2, bad));
    BOOST_CHECK(!f.solver->solutionAtCardinality(3, bad));
    BOOST_CHECK(!f.solver->solutionAtCardinality(50, bad));
}

BOOST_AUTO_TEST_CASE(basic_bvc_standard_usage)
{
    BVCFrameSolverFixture f0(0);
    BVCFrameSolverFixture f1(1);

    Cube bad = { f0.tr->bad() };

    ID a0 = f0.tr->toInternal(f0.a0);
    ID a1 = f0.tr->toInternal(f0.a1);
    ID a2 = f0.tr->toInternal(f0.a2);
    ID l2 = f0.tr->toInternal(f0.l2);
    ID l1 = f0.tr->toInternal(f0.l1);

    // There should be a correction set of {a2}
    BOOST_REQUIRE(f0.solver->solutionExists(bad));

    bool sat = false;
    BVCSolution soln;
    BVCPredecessor pred;

    std::tie(sat, soln, pred) = f0.solver->solve(0, bad);
    BOOST_REQUIRE(!sat);

    std::tie(sat, soln, pred) = f0.solver->solve(1, bad);

    BOOST_REQUIRE(sat);
    BOOST_CHECK(pred.empty());
    BOOST_REQUIRE(!soln.empty());

    BVCSolution expected = {a2};
    BOOST_CHECK(soln == expected);

    f0.solver->blockSolution(soln);
    f1.solver->blockSolution(soln);

    // There shouldn't be any more correction sets for k = 1
    BOOST_CHECK(!f0.solver->solutionExists(bad));

    // Setting the abstraction shouldn't change that
    f0.solver->setAbs({a2});
    BOOST_CHECK(!f0.solver->solutionExists(bad));

    // Check the abstraction is as expected
    std::set<ID> expected_abs = {a2};
    BOOST_CHECK(f0.solver->getAbstraction() == expected_abs);

    // Set the abstraction to {a2} (which is the hitting set
    // of the currently-known correction sets {{a2}}).
    // Also, move on to one frame of abstraction.
    f1.solver->setAbs({a2});

    // A predecessor should exist
    BOOST_REQUIRE(f1.solver->predecessorExists(bad));

    std::tie(sat, soln, pred) = f1.solver->solve(0, bad);
    BOOST_REQUIRE(sat);
    BOOST_CHECK(soln.empty());

    // It should be l1 & l2 (l0 and l3 are don't cares)
    std::set<ID> p(pred.begin(), pred.end());
    BOOST_CHECK(p.count(l1) > 0);
    BOOST_CHECK(p.count(l2) > 0);
    BOOST_CHECK(p.count(negate(l1)) == 0);
    BOOST_CHECK(p.count(negate(l2)) == 0);

    // "Generalize" magically
    pred = {l1, l2};

    // Block the predecessor back at level 0
    // This should lead to a correction set of {a0, a1} as the predecessor
    // is l1 & l2 and l1' = a0, l2' = a1, so keeping either one ends up making
    // l1 & l2 1-step unreachable.
    BOOST_REQUIRE(f0.solver->solutionExists(pred));
    BOOST_REQUIRE(!f0.solver->predecessorExists(pred));
    BOOST_CHECK(!f0.solver->solutionAtCardinality(0, pred));
    BOOST_CHECK(!f0.solver->solutionAtCardinality(1, pred));
    BOOST_CHECK(f0.solver->solutionAtCardinality(2, pred));

    BVCPredecessor old_pred = pred;
    std::tie(sat, soln, pred) = f0.solver->solve(2, pred);

    BOOST_REQUIRE(sat);
    BOOST_CHECK(pred.empty());
    BOOST_CHECK_EQUAL(soln.size(), 2);

    expected = {a0, a1};
    std::sort(expected.begin(), expected.end());
    std::sort(soln.begin(), soln.end());

    BOOST_REQUIRE(soln == expected);

    f0.solver->blockSolution(soln);
    f1.solver->blockSolution(soln);

    BOOST_CHECK(!f0.solver->predecessorExists(old_pred));
    BOOST_CHECK(!f0.solver->solutionExists(old_pred));

    // Our correction sets are now {a2}, {a0, a1}
    // Choose hitting set {a1, a2}
    // It turns out this is an IVC
    std::vector<ID> abstraction = {a1, a2};
    f0.solver->setAbs(abstraction);
    f1.solver->setAbs(abstraction);

    BOOST_CHECK(!f0.solver->predecessorExists(bad));
    BOOST_CHECK(!f0.solver->solutionExists(bad));

    BOOST_CHECK(!f1.solver->predecessorExists(bad));
    BOOST_CHECK(!f1.solver->solutionExists(bad));

    // Check it's an IVC
    TransitionRelation tr(*f0.tr, abstraction);
    IC3::IC3Solver ic3(f0.vars, tr);
    SafetyResult safe = ic3.prove();
    BOOST_CHECK(safe.safe());
}

