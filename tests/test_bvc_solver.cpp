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
#include "pme/ivc/bvc_solver.h"

#define BOOST_TEST_MODULE BVCSolverTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct BVCSolverFixture
{
    aiger * aig;
    ExternalID i0, l0, l1, l2, l3, a0, a1, a2, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<BVCSolver> solver;

    BVCSolverFixture()
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

        prepareSolver();
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    void prepareSolver()
    {
        solver.reset(new BVCSolver(vars, *tr));
    }

    ~BVCSolverFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(basic_bvc_k1)
{
    BVCSolverFixture f;

    ID a2 = f.tr->toInternal(f.a2);

    // There should be a correction set of {a2}
    BOOST_REQUIRE(f.solver->solutionExists());

    bool sat = false;
    BVCSolution soln;
    BVCPredecessor pred;

    std::tie(sat, soln, pred) = f.solver->solve();

    BOOST_REQUIRE(sat);
    BOOST_CHECK(pred.empty());
    BOOST_REQUIRE(!soln.empty());

    BVCSolution expected = {a2};
    BOOST_CHECK(soln == expected);

    f.solver->blockSolution(soln);

    // There shouldn't be any more correction sets for k = 1
    BOOST_CHECK(!f.solver->solutionExists());
}

BOOST_AUTO_TEST_CASE(basic_bvc_abstraction)
{
    BVCSolverFixture f;

    ID a2 = f.tr->toInternal(f.a2);

    // There should be a correction set of {a2}
    BOOST_REQUIRE(f.solver->solutionExists());

    bool sat = false;
    BVCSolution soln;
    BVCPredecessor pred;

    std::tie(sat, soln, pred) = f.solver->solve();

    BOOST_REQUIRE(sat);
    BOOST_CHECK(pred.empty());
    BOOST_REQUIRE(!soln.empty());

    BVCSolution expected = {a2};
    BOOST_CHECK(soln == expected);

    f.solver->blockSolution(soln);

    // There shouldn't be any more correction sets for k = 1
    BOOST_CHECK(!f.solver->solutionExists());

    // Set the abstraction to {a2} (which is the hitting set
    // of the currently-known correction sets {{a2}}).
    // Also, one frame of abstraction.
    f.solver->setAbstraction({a2});
    f.solver->increaseLevel(1);

    // A predecessor should exist
    BOOST_REQUIRE(f.solver->predecessorExists());

    // TODO: finish test
}

