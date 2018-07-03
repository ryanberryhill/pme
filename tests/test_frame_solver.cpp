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
#include "pme/ic3/frame_solver.h"

#define BOOST_TEST_MODULE FrameSolverTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <algorithm>

using namespace PME;
using namespace PME::IC3;

struct FrameSolverFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, o0;
    VariableManager vars;
    InductiveTrace trace;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<FrameSolver> solver;
    GlobalState gs;

    FrameSolverFixture(bool simplify = true)
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
        solver.reset(new FrameSolver(vars, *tr, trace, gs));
    }

    ~FrameSolverFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    void addClauseToTrace(const Clause & cls, unsigned level)
    {
        Cube negc = negateVec(cls);
        trace.addLemma(negc, level);
    }

    void addClause(const Clause & cls, unsigned level)
    {
        Cube negc = negateVec(cls);
        LemmaID id = trace.addLemma(negc, level);
        solver->addLemma(id);
    }
};

BOOST_AUTO_TEST_CASE(f_inf)
{
    FrameSolverFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause c0 = {negate(l0)};
    Clause c1 = {negate(l1)};
    Clause c2 = {negate(l2)};
    Clause c3 = {negate(l3)};

    BOOST_CHECK(!f.solver->consecution(LEVEL_INF, negateVec(c0)));
    BOOST_CHECK(!f.solver->consecution(LEVEL_INF, negateVec(c1)));
    BOOST_CHECK(!f.solver->consecution(LEVEL_INF, negateVec(c2)));
    BOOST_CHECK(!f.solver->consecution(LEVEL_INF, negateVec(c3)));

    f.addClause(c0, LEVEL_INF);
    f.addClause(c1, LEVEL_INF);
    f.addClause(c2, LEVEL_INF);
    f.addClause(c3, LEVEL_INF);

    BOOST_CHECK(f.solver->consecution(LEVEL_INF, negateVec(c0)));
    BOOST_CHECK(f.solver->consecution(LEVEL_INF, negateVec(c1)));
    BOOST_CHECK(f.solver->consecution(LEVEL_INF, negateVec(c2)));
    BOOST_CHECK(f.solver->consecution(LEVEL_INF, negateVec(c3)));
}

BOOST_AUTO_TEST_CASE(consecution)
{
    FrameSolverFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause cn0 = {negate(l0)};
    Clause cn1 = {negate(l1)};
    Clause cn2 = {negate(l2)};
    Clause cn3 = {negate(l3)};

    Clause cp0 = {l0};
    Clause cp1 = {l1};
    Clause cp2 = {l2};
    Clause cp3 = {l3};

    // Initial state 0001
    f.addClause(cp0, 0);
    f.addClause(cn1, 0);
    f.addClause(cn2, 0);
    f.addClause(cn3, 0);

    BOOST_CHECK(f.solver->consecution(0, negateVec(cn3)));
    BOOST_CHECK(f.solver->consecution(0, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(0, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(0, negateVec(cp0)));

    f.addClause(cn2, 1);
    f.addClause(cn3, 1);

    BOOST_CHECK(f.solver->consecution(0, negateVec(cn3)));
    BOOST_CHECK(f.solver->consecution(0, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(0, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(0, negateVec(cp0)));

    BOOST_CHECK(f.solver->consecution(1, negateVec(cn3)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cp0)));

    f.addClause(cn3, 2);

    BOOST_CHECK(f.solver->consecution(0, negateVec(cn3)));
    BOOST_CHECK(f.solver->consecution(0, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(0, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(0, negateVec(cp0)));

    BOOST_CHECK(f.solver->consecution(1, negateVec(cn3)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cp0)));

    BOOST_CHECK(!f.solver->consecution(2, negateVec(cn3)));
    BOOST_CHECK(!f.solver->consecution(2, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(2, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(2, negateVec(cp0)));

    f.solver->renewSAT();

    BOOST_CHECK(f.solver->consecution(0, negateVec(cn3)));
    BOOST_CHECK(f.solver->consecution(0, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(0, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(0, negateVec(cp0)));

    BOOST_CHECK(f.solver->consecution(1, negateVec(cn3)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(1, negateVec(cp0)));

    BOOST_CHECK(!f.solver->consecution(2, negateVec(cn3)));
    BOOST_CHECK(!f.solver->consecution(2, negateVec(cn2)));
    BOOST_CHECK(!f.solver->consecution(2, negateVec(cn1)));
    BOOST_CHECK(!f.solver->consecution(2, negateVec(cp0)));
}

BOOST_AUTO_TEST_CASE(intersection)
{
    FrameSolverFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause cn0 = {negate(l0)};
    Clause cn1 = {negate(l1)};
    Clause cn2 = {negate(l2)};
    Clause cn3 = {negate(l3)};

    Clause cp0 = {l0};
    Clause cp1 = {l1};
    Clause cp2 = {l2};
    Clause cp3 = {l3};

    Cube init = {negate(l0), negate(l1), negate(l2), negate(l3)};

    // Initial state 0000
    f.addClause(cn0, 0);
    f.addClause(cn1, 0);
    f.addClause(cn2, 0);
    f.addClause(cn3, 0);

    BOOST_CHECK(f.solver->intersection(0, init));
    BOOST_CHECK(f.solver->intersection(0, cn0));
    BOOST_CHECK(f.solver->intersection(0, cn1));
    BOOST_CHECK(f.solver->intersection(0, cn2));
    BOOST_CHECK(f.solver->intersection(0, cn3));
    BOOST_CHECK(!f.solver->intersection(0, cp0));
    BOOST_CHECK(!f.solver->intersection(0, cp1));
    BOOST_CHECK(!f.solver->intersection(0, cp2));
    BOOST_CHECK(!f.solver->intersection(0, cp3));

    // State xxx0
    f.addClause(cn0, 1);

    BOOST_CHECK(f.solver->intersection(1, init));
    BOOST_CHECK(f.solver->intersection(1, cn0));
    BOOST_CHECK(f.solver->intersection(1, cn1));
    BOOST_CHECK(f.solver->intersection(1, cn2));
    BOOST_CHECK(f.solver->intersection(1, cn3));
    BOOST_CHECK(!f.solver->intersection(1, cp0));
    BOOST_CHECK(f.solver->intersection(1, cp1));
    BOOST_CHECK(f.solver->intersection(1, cp2));
    BOOST_CHECK(f.solver->intersection(1, cp3));
    BOOST_CHECK(f.solver->intersection(1, {l3, l2, l1}));
    BOOST_CHECK(!f.solver->intersection(1, {l3, l2, l1, l0}));

    // State xx00
    f.addClause(cn1, 1);

    BOOST_CHECK(f.solver->intersection(1, init));
    BOOST_CHECK(f.solver->intersection(1, cn0));
    BOOST_CHECK(f.solver->intersection(1, cn1));
    BOOST_CHECK(f.solver->intersection(1, cn2));
    BOOST_CHECK(f.solver->intersection(1, cn3));
    BOOST_CHECK(!f.solver->intersection(1, cp0));
    BOOST_CHECK(!f.solver->intersection(1, cp1));
    BOOST_CHECK(f.solver->intersection(1, cp2));
    BOOST_CHECK(f.solver->intersection(1, cp3));
    BOOST_CHECK(f.solver->intersection(1, {l3, l2}));
    BOOST_CHECK(f.solver->intersection(1, {l3, l2, negate(l1)}));
    BOOST_CHECK(f.solver->intersection(1, {l3, l2, negate(l1), negate(l0)}));
    BOOST_CHECK(!f.solver->intersection(1, {l3, l2, l1}));
    BOOST_CHECK(!f.solver->intersection(1, {l3, l2, l1, negate(l0)}));
    BOOST_CHECK(!f.solver->intersection(1, {l3, l2, l1, l0}));
}

BOOST_AUTO_TEST_CASE(consecution_predecessor)
{
    FrameSolverFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause cp0 = {l0};
    Clause cn1 = {negate(l1)};
    Clause cn2 = {negate(l2)};
    Clause cn3 = {negate(l3)};

    // Initial state 0001
    f.addClause(cp0, 0);
    f.addClause(cn1, 0);
    f.addClause(cn2, 0);
    f.addClause(cn3, 0);

    Cube c = {negate(l0), l1, negate(l2), negate(l3)};
    Cube pred;
    BOOST_REQUIRE(!f.solver->consecutionPred(0, c, pred));
    BOOST_REQUIRE(!pred.empty());

    Cube expected = {l0, negate(l1), negate(l2), negate(l3)};
    std::sort(expected.begin(), expected.end());
    std::sort(pred.begin(), pred.end());
    BOOST_CHECK(pred == expected);
}

BOOST_AUTO_TEST_CASE(consecution_unsat_core)
{
    FrameSolverFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause cp0 = {l0};
    Clause cn1 = {negate(l1)};
    Clause cn2 = {negate(l2)};
    Clause cn3 = {negate(l3)};

    // Initial state 0001
    f.addClause(cp0, 0);
    f.addClause(cn1, 0);
    f.addClause(cn2, 0);
    f.addClause(cn3, 0);

    Cube c = {negate(l0), negate(l1), negate(l2), negate(l3)};
    Cube core;
    BOOST_REQUIRE(f.solver->consecutionCore(0, c, core));
    BOOST_REQUIRE(!core.empty());

    std::sort(c.begin(), c.end());
    std::sort(core.begin(), core.end());

    BOOST_CHECK(std::includes(c.begin(), c.end(), core.begin(), core.end()));
    BOOST_CHECK(f.solver->consecution(0, core));

    c = {l0, l1, l2, l3};
    core.clear();
    BOOST_REQUIRE(f.solver->consecutionCore(0, c, core));
    BOOST_REQUIRE(!core.empty());

    std::sort(c.begin(), c.end());
    std::sort(core.begin(), core.end());

    BOOST_CHECK(std::includes(c.begin(), c.end(), core.begin(), core.end()));
    BOOST_CHECK(f.solver->consecution(0, core));

    f.addClause(cn2, 1);
    f.addClause(cn3, 1);

    c = {negate(l0), negate(l1), negate(l2), negate(l3)};
    core.clear();
    BOOST_REQUIRE(f.solver->consecutionCore(1, c, core));
    BOOST_REQUIRE(!core.empty());

    std::sort(c.begin(), c.end());
    std::sort(core.begin(), core.end());

    BOOST_CHECK(std::includes(c.begin(), c.end(), core.begin(), core.end()));
    BOOST_CHECK(f.solver->consecution(1, core));

    c = {l0, l1, l2, l3};
    core.clear();
    BOOST_REQUIRE(f.solver->consecutionCore(1, c, core));
    BOOST_REQUIRE(!core.empty());

    std::sort(c.begin(), c.end());
    std::sort(core.begin(), core.end());

    BOOST_CHECK(std::includes(c.begin(), c.end(), core.begin(), core.end()));
    BOOST_CHECK(f.solver->consecution(1, core));
}

BOOST_AUTO_TEST_CASE(intersection_state)
{
    FrameSolverFixture f;

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause cn0 = {negate(l0)};
    Clause cn1 = {negate(l1)};
    Clause cn2 = {negate(l2)};
    Clause cn3 = {negate(l3)};

    Clause cp0 = {l0};
    Clause cp1 = {l1};
    Clause cp2 = {l2};
    Clause cp3 = {l3};

    // State xxx1
    f.addClause(cp0, 0);

    bool intersect;
    Cube state, inputs;

    std::tie(intersect, state, inputs) = f.solver->intersectionFull(0, cn0);
    BOOST_CHECK(!intersect);
    std::tie(intersect, state, inputs) = f.solver->intersectionFull(0, {negate(l0), l1, l2});
    BOOST_CHECK(!intersect);
    std::tie(intersect, state, inputs) = f.solver->intersectionFull(0, {negate(l0), l1, l2, negate(l3)});
    BOOST_CHECK(!intersect);

    Cube expected = {l0, l1, l2, l3};
    std::tie(intersect, state, inputs) = f.solver->intersectionFull(0, {l0, l1, l2, l3});
    BOOST_CHECK(intersect);
    std::sort(expected.begin(), expected.end());
    std::sort(state.begin(), state.end());
    BOOST_CHECK(expected == state);
    BOOST_CHECK(inputs.empty());

    std::tie(intersect, state, inputs) = f.solver->intersectionFull(0, {l0, l1});
    BOOST_CHECK(intersect);
    BOOST_CHECK(std::find(state.begin(), state.end(), l0) != state.end());
    BOOST_CHECK(std::find(state.begin(), state.end(), l1) != state.end());
    BOOST_CHECK(inputs.empty());
}

