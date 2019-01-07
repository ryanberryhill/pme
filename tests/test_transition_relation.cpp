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

#include "pme/engine/transition_relation.h"
#include "pme/engine/sat_adaptor.h"
extern "C" {
#include "aiger/aiger.h"
}

#define BOOST_TEST_MODULE TransitionRelationTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <set>

using namespace PME;

struct CombinationalAigFixture
{
    aiger * aig;
    ExternalID i1, i2, o1, c0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;

    CombinationalAigFixture()
    {
        aig = aiger_init();

        i1 = 2;
        i2 = i1 + 2;
        o1 = i2 + 2;

        // o1 = i1 & i2
        aiger_add_input(aig, i1, "i1");
        aiger_add_input(aig, i2, "i2");
        aiger_add_and(aig, o1, i1, i2);
        aiger_add_output(aig, o1, "o1");

        c0 = i1;
    }

    ~CombinationalAigFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    void addConstraint()
    {
        aiger_add_constraint(aig, i1, "c0");
    }

    void buildTR()
    {
        tr.reset(new TransitionRelation(vars, aig));
    }
};

struct AigFixture
{
    aiger * aig;
    ExternalID i0, i1, l0, l1, l2, l3, a0, o0, c0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;

    AigFixture()
    {
        aig = aiger_init();
        i0 = 2;
        i1 = i0 + 2;
        l0 = i1 + 2;
        l1 = l0 + 2;
        l2 = l1 + 2;
        l3 = l2 + 2;
        a0 = l3 + 2;

        aiger_add_input(aig, i0, "i0");
        aiger_add_input(aig, i1, "i1");
        aiger_add_and(aig, a0, i0, i1);
        aiger_add_latch(aig, l0, a0, "l0");
        aiger_add_latch(aig, l1, l0, "l1");
        aiger_add_latch(aig, l2, l1, "l2");
        aiger_add_latch(aig, l3, l2, "l3");
        aiger_add_output(aig, l3, "o0");
        o0 = l3;
        c0 = a0;
    }

    ~AigFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    void addResets(unsigned value)
    {
        assert(value == 0 || value == 1);
        aiger_add_reset(aig, l0, value);
        aiger_add_reset(aig, l1, value);
        aiger_add_reset(aig, l2, value);
        aiger_add_reset(aig, l3, value);
    }

    void clearResets()
    {
        aiger_add_reset(aig, l0, l0);
        aiger_add_reset(aig, l1, l1);
        aiger_add_reset(aig, l2, l2);
        aiger_add_reset(aig, l3, l3);
    }

    void addConstraint()
    {
        aiger_add_constraint(aig, a0, "c0");
    }

    void buildTR()
    {
        tr.reset(new TransitionRelation(vars, aig));
    }
};

struct ManyGateAigFixture
{
    aiger * aig;
    ExternalID i0, l0, l1, l2, l3, a0, a1, a2, a3, a4, a5, a6, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;

    ManyGateAigFixture()
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
        a3 = 18;
        a4 = 20;
        a5 = 22;
        a6 = 24;

        aiger_add_input(aig, i0, "i0");

        // l0' = i0
        aiger_add_latch(aig, l0, i0, "l0");

        // l1' = l0
        aiger_add_latch(aig, l1, l0, "l1");

        // l2' = l1
        aiger_add_latch(aig, l2, l1, "l2");

        // l3' = l2
        aiger_add_latch(aig, l3, l2, "l3");

        // a0-a3 are ands of each pair of adjacent latches
        // a4 and a5 are ands of adjacent pairs of a0 and a3
        // a6 = a4 & a5 = <every adjacent pair is high>
        // a6 <=> ALL(l0-l3)
        aiger_add_and(aig, a0, l0, l1);
        aiger_add_and(aig, a1, l1, l2);
        aiger_add_and(aig, a2, l2, l3);
        aiger_add_and(aig, a3, l3, l0);
        aiger_add_and(aig, a4, a0, a1);
        aiger_add_and(aig, a5, a2, a3);
        aiger_add_and(aig, a6, a4, a5);
        aiger_add_output(aig, a6, "o0");
        o0 = a6;
    }

    ~ManyGateAigFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    void addResets(unsigned value)
    {
        assert(value == 0 || value == 1);
        aiger_add_reset(aig, l0, value);
        aiger_add_reset(aig, l1, value);
        aiger_add_reset(aig, l2, value);
        aiger_add_reset(aig, l3, value);
    }

    void clearResets()
    {
        aiger_add_reset(aig, l0, l0);
        aiger_add_reset(aig, l1, l1);
        aiger_add_reset(aig, l2, l2);
        aiger_add_reset(aig, l3, l3);
    }

    void addConstraint()
    {
        aiger_add_constraint(aig, a0, "c0");
    }

    void buildTR()
    {
        tr.reset(new TransitionRelation(vars, aig));
    }
};

void sortClauseVec(ClauseVec & vec)
{
    for (Clause & cls : vec)
    {
        std::sort(cls.begin(), cls.end());
    }

    std::sort(vec.begin(), vec.end());
}

BOOST_AUTO_TEST_CASE(internal_and_external)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID l0 = tr.toInternal(f.l0);
    ID l1 = tr.toInternal(f.l1);

    ExternalClause ext_cls = { f.l0, aiger_not(f.l1) };
    Clause cls = tr.makeInternal(ext_cls);
    Clause expected = { l0, negate(l1) };
    std::sort(cls.begin(), cls.end());
    std::sort(expected.begin(), expected.end());

    BOOST_CHECK(expected == cls);

    ExternalClause back_to_ext = tr.makeExternal(cls);
    std::sort(back_to_ext.begin(), back_to_ext.end());
    std::sort(ext_cls.begin(), ext_cls.end());

    BOOST_CHECK(back_to_ext == ext_cls);
}

BOOST_AUTO_TEST_CASE(combinational)
{
    CombinationalAigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ClauseVec unrolled = tr.unroll(1);

    SATAdaptor sat;
    sat.addClauses(unrolled);

    // TR on its own should be SAT
    BOOST_CHECK(sat.solve());

    // Output = 1 should be SAT
    ID o1 = tr.toInternal(f.o1);
    ID i1 = tr.toInternal(f.i1);
    ID i2 = tr.toInternal(f.i2);
    sat.addClause({o1});
    BOOST_CHECK(sat.solve());

    // And both inputs should be 1
    BOOST_CHECK_EQUAL(sat.getAssignment(i1), SAT::TRUE);
    BOOST_CHECK_EQUAL(sat.getAssignment(i2), SAT::TRUE);

    // Output = 1 and either input = 1 should be UNSAT
    BOOST_CHECK(!sat.solve({negate(i1)}));
    BOOST_CHECK(!sat.solve({negate(i2)}));
}

BOOST_AUTO_TEST_CASE(undefined_resets)
{
    AigFixture f_default, f_zero, f_none;
    f_zero.addResets(0);
    f_none.clearResets();

    f_default.buildTR();
    f_zero.buildTR();
    f_none.buildTR();

    // Default reset should be zero
    TransitionRelation & tr_default = *f_default.tr;
    TransitionRelation & tr_zero    = *f_zero.tr;
    TransitionRelation & tr_none    = *f_none.tr;

    for (unsigned i = 1; i < 8; ++i)
    {
        ClauseVec default_init = tr_default.unrollWithInit(i);
        ClauseVec zero_init = tr_zero.unrollWithInit(i);
        ClauseVec none_init = tr_none.unrollWithInit(i);

        // Technically I suppose we could check for equivalence with the SAT
        // solver, but we'll check for syntactic equivalence instead.
        // The initialized unrolled transition relations should be the same
        // (i.e., zero reset states are the default).
        sortClauseVec(default_init);
        sortClauseVec(zero_init);
        sortClauseVec(none_init);
        BOOST_CHECK(default_init == zero_init);

        // These three and none_init should be identical
        ClauseVec default_unrolled = tr_default.unroll(i);
        ClauseVec zero_unrolled = tr_zero.unroll(i);
        ClauseVec none_unrolled = tr_none.unroll(i);

        sortClauseVec(default_unrolled);
        sortClauseVec(zero_unrolled);
        sortClauseVec(none_unrolled);
        BOOST_CHECK(default_unrolled == zero_unrolled);
        BOOST_CHECK(zero_unrolled == none_unrolled);
        BOOST_CHECK(none_unrolled == none_init);
    }
}

BOOST_AUTO_TEST_CASE(sequential_nounroll)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    SATAdaptor sat;
    sat.addClauses(tr.unroll(1));

    // TR on its own should be SAT
    BOOST_CHECK(sat.solve());

    // Output = 0 or = 1 should be SAT
    ID o0 = tr.toInternal(f.o0);
    BOOST_CHECK(sat.solve({o0}));
    BOOST_CHECK(sat.solve({negate(o0)}));
}

BOOST_AUTO_TEST_CASE(basic_accessors)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID i0 = tr.toInternal(f.i0);
    ID l0 = tr.toInternal(f.l0);
    ID l1 = tr.toInternal(f.l1);
    ID l2 = tr.toInternal(f.l2);
    ID l3 = tr.toInternal(f.l3);
    ID a0 = tr.toInternal(f.a0);

    BOOST_CHECK(tr.isInput(i0));
    BOOST_CHECK(tr.isLatch(l0));
    BOOST_CHECK(tr.isLatch(l1));
    BOOST_CHECK(tr.isLatch(l2));
    BOOST_CHECK(tr.isLatch(l3));
    BOOST_CHECK(tr.isGate(a0));
}

BOOST_AUTO_TEST_CASE(sequential_nounroll_initzero)
{
    AigFixture f;
    f.addResets(0);
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    SATAdaptor sat;
    sat.addClauses(tr.unrollWithInit(1));

    // TR with zero init state should be SAT
    BOOST_CHECK(sat.solve());

    // Latches should be zero
    ID l0 = tr.toInternal(f.l0);
    ID l1 = tr.toInternal(f.l1);
    ID l2 = tr.toInternal(f.l2);
    ID l3 = tr.toInternal(f.l3);
    BOOST_CHECK_EQUAL(sat.getAssignment(l0), SAT::FALSE);
    BOOST_CHECK_EQUAL(sat.getAssignment(l1), SAT::FALSE);
    BOOST_CHECK_EQUAL(sat.getAssignment(l2), SAT::FALSE);
    BOOST_CHECK_EQUAL(sat.getAssignment(l3), SAT::FALSE);

    // Output is l3
    // l3 = 0 should be SAT
    ID o0 = tr.toInternal(f.o0);
    BOOST_CHECK(!sat.solve({o0}));
    BOOST_CHECK(sat.solve({negate(o0)}));
}

BOOST_AUTO_TEST_CASE(sequential_unroll)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID i0 = tr.toInternal(f.i0);
    ID i1 = tr.toInternal(f.i1);
    ID l0 = tr.toInternal(f.l0);
    ID l1 = tr.toInternal(f.l1);
    ID l2 = tr.toInternal(f.l2);
    ID l3 = tr.toInternal(f.l3);

    SATAdaptor sat;
    sat.addClauses(tr.initState());
    sat.addClauses(tr.unrollFrame(0));

    for (unsigned i = 1; i < 8; ++i)
    {
        sat.addClauses(tr.unrollFrame(i));

        // TR on its own should be SAT
        BOOST_CHECK(sat.solve());

        for (unsigned j = 0; j <= i; ++j)
        {
            // l0' = 1 should be reachable in the first cycle (and all others)
            if (j > 0) { BOOST_CHECK(sat.solve({prime(l0, j)})); }
            else       { BOOST_CHECK(!sat.solve({prime(l0, j)})); }

            // l1 = 1 should be reachable after the first cycle
            if (j > 1) { BOOST_CHECK(sat.solve({prime(l1, j)})); }
            else       { BOOST_CHECK(!sat.solve({prime(l1, j)})); }

            if (j > 2) { BOOST_CHECK(sat.solve({prime(l2, j)})); }
            else       { BOOST_CHECK(!sat.solve({prime(l2, j)})); }

            if (j > 3) { BOOST_CHECK(sat.solve({prime(l3, j)})); }
            else       { BOOST_CHECK(!sat.solve({prime(l3, j)})); }
        }

        // To make the output 0 at the end, inputs must be both 1 for every
        // cycle. Note we don't prime(i0, i) because it doesn't exist -- it
        // refers to the input on the next (unmodeled) cycle.
        if (i > 3)
        {
            BOOST_CHECK(sat.solve({prime(l3, i)}));
            for (unsigned j = 0; j < i; ++j)
            {
                BOOST_CHECK_EQUAL(sat.getAssignment(prime(i0, j)),
                                  SAT::TRUE);
                BOOST_CHECK_EQUAL(sat.getAssignment(prime(i1, j)),
                                  SAT::TRUE);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(incremental_unroll)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID i0 = tr.toInternal(f.i0);
    ID i1 = tr.toInternal(f.i1);
    ID l0 = tr.toInternal(f.l0);
    ID l1 = tr.toInternal(f.l1);
    ID l2 = tr.toInternal(f.l2);
    ID l3 = tr.toInternal(f.l3);

    for (unsigned i = 1; i < 8; ++i)
    {
        SATAdaptor sat;
        sat.addClauses(tr.unrollWithInit(i));

        // TR on its own should be SAT
        BOOST_CHECK(sat.solve());

        for (unsigned j = 0; j <= i; ++j)
        {
            // l0' = 1 should be reachable in the first cycle (and all others)
            if (j > 0) { BOOST_CHECK(sat.solve({prime(l0, j)})); }
            else       { BOOST_CHECK(!sat.solve({prime(l0, j)})); }

            // l1 = 1 should be reachable after the first cycle
            if (j > 1) { BOOST_CHECK(sat.solve({prime(l1, j)})); }
            else       { BOOST_CHECK(!sat.solve({prime(l1, j)})); }

            if (j > 2) { BOOST_CHECK(sat.solve({prime(l2, j)})); }
            else       { BOOST_CHECK(!sat.solve({prime(l2, j)})); }

            if (j > 3) { BOOST_CHECK(sat.solve({prime(l3, j)})); }
            else       { BOOST_CHECK(!sat.solve({prime(l3, j)})); }
        }

        // To make the output 0 at the end, inputs must be both 1 for every
        // cycle. Note we don't prime(i0, i) because it doesn't exist -- it
        // refers to the input on the next (unmodeled) cycle.
        if (i > 3)
        {
            BOOST_CHECK(sat.solve({prime(l3, i)}));
            for (unsigned j = 0; j < i; ++j)
            {
                BOOST_CHECK_EQUAL(sat.getAssignment(prime(i0, j)),
                                  SAT::TRUE);
                BOOST_CHECK_EQUAL(sat.getAssignment(prime(i1, j)),
                                  SAT::TRUE);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(combinational_constraints)
{
    CombinationalAigFixture f;
    f.addConstraint();
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID i1 = tr.toInternal(f.i1);
    ID i2 = tr.toInternal(f.i2);
    ID o1 = tr.toInternal(f.o1);

    SATAdaptor sat;
    sat.addClauses(tr.unrollWithInit(1));

    // Constrained TR should be SAT on its own
    BOOST_CHECK(sat.solve());
    // And with output = 1
    BOOST_CHECK(sat.solve({o1}));
    BOOST_CHECK_EQUAL(sat.getAssignment(i1), SAT::TRUE);
    BOOST_CHECK_EQUAL(sat.getAssignment(i2), SAT::TRUE);
    // And with output = 0
    BOOST_CHECK(sat.solve({negate(o1)}));
    // The constraint forces i1 to be 1
    BOOST_CHECK_EQUAL(sat.getAssignment(i1), SAT::TRUE);
    BOOST_CHECK_EQUAL(sat.getAssignment(i2), SAT::FALSE);
}

BOOST_AUTO_TEST_CASE(sequential_constraints)
{
    AigFixture f;
    f.addConstraint();
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID i0 = tr.toInternal(f.i0);
    ID i1 = tr.toInternal(f.i1);
    ID o0 = tr.toInternal(f.o0);
    ID o0_final = prime(o0, 4);

    SATAdaptor sat;
    sat.addClauses(tr.unrollWithInit(4));

    // With the constraint the output after 4 cycles must be 1
    BOOST_CHECK(!sat.solve({negate(o0_final)}));
    BOOST_CHECK(sat.solve({o0_final}));

    // Output is 1
    BOOST_CHECK_EQUAL(sat.getAssignment(o0_final), SAT::TRUE);
    // Inputs are 1 in each cycle
    for (unsigned i = 0; i < 4; ++i)
    {
        BOOST_CHECK_EQUAL(sat.getAssignment(prime(i0,i)), SAT::TRUE);
        BOOST_CHECK_EQUAL(sat.getAssignment(prime(i1,i)), SAT::TRUE);
    }
}

BOOST_AUTO_TEST_CASE(set_init_states)
{
    AigFixture f;
    f.clearResets();
    f.buildTR();

    TransitionRelation & tr = *f.tr;

    SATAdaptor sat;

    ID l0 = tr.toInternal(f.l0);
    ID l1 = tr.toInternal(f.l1);
    ID l2 = tr.toInternal(f.l2);
    ID l3 = tr.toInternal(f.l3);

    // X010
    tr.setInit(l0, ID_FALSE);
    tr.setInit(l1, ID_TRUE);
    tr.setInit(l2, ID_FALSE);
    tr.setInit(l3, ID_NULL);

    BOOST_CHECK_EQUAL(tr.getInit(l0), ID_FALSE);
    BOOST_CHECK_EQUAL(tr.getInit(l1), ID_TRUE);
    BOOST_CHECK_EQUAL(tr.getInit(l2), ID_FALSE);
    BOOST_CHECK_EQUAL(tr.getInit(l3), ID_NULL);

    sat.addClauses(tr.initState());

    BOOST_CHECK(!sat.solve({l0}));
    BOOST_CHECK(!sat.solve({negate(l1)}));
    BOOST_CHECK(!sat.solve({l2}));

    BOOST_CHECK(sat.solve({negate(l0), l1, negate(l2), l3}));
    BOOST_CHECK(sat.solve({negate(l0), l1, negate(l2), negate(l3)}));
    BOOST_CHECK(sat.solve({l1, negate(l2), negate(l3)}));
    BOOST_CHECK(!sat.solve({negate(l1), negate(l2), negate(l3)}));
    BOOST_CHECK(!sat.solve({negate(l1), l2, negate(l3)}));
}

BOOST_AUTO_TEST_CASE(init_states)
{
    AigFixture f0, f1;
    f0.addResets(0);
    f1.addResets(1);
    f0.buildTR();
    f1.buildTR();

    TransitionRelation & tr0 = *f0.tr;
    TransitionRelation & tr1 = *f1.tr;

    SATAdaptor sat0, sat1;

    ID l0_0 = tr0.toInternal(f0.l0);
    ID l1_0 = tr0.toInternal(f0.l1);
    ID l2_0 = tr0.toInternal(f0.l2);
    ID l3_0 = tr0.toInternal(f0.l3);

    ID l0_1 = tr1.toInternal(f1.l0);
    ID l1_1 = tr1.toInternal(f1.l1);
    ID l2_1 = tr1.toInternal(f1.l2);
    ID l3_1 = tr1.toInternal(f1.l3);

    sat0.addClauses(tr0.initState());
    sat1.addClauses(tr1.initState());

    BOOST_CHECK(sat0.solve(negateVec({l0_0, l1_0, l2_0, l3_0})));
    BOOST_CHECK(!sat1.solve(negateVec({l0_1, l1_1, l2_1, l3_1})));
    BOOST_CHECK(sat0.solve({negate(l0_0)}));
    BOOST_CHECK(!sat1.solve({negate(l0_1)}));

    BOOST_CHECK(!sat0.solve({l0_0,l1_0,l2_0,l3_0}));
    BOOST_CHECK(sat1.solve({l0_1,l1_1,l2_1,l3_1}));
    BOOST_CHECK(!sat0.solve({l0_0}));
    BOOST_CHECK(sat1.solve({l0_1}));

    for (auto it = tr0.begin_latches(); it != tr0.end_latches(); ++it)
    {
        ID latch = *it;
        BOOST_CHECK_EQUAL(tr0.getInit(latch), ID_FALSE);
    }

    for (auto it = tr1.begin_latches(); it != tr1.end_latches(); ++it)
    {
        ID latch = *it;
        BOOST_CHECK_EQUAL(tr1.getInit(latch), ID_TRUE);
    }
}

BOOST_AUTO_TEST_CASE(latches_iter)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID l0 = tr.toInternal(f.l0);
    ID l1 = tr.toInternal(f.l1);
    ID l2 = tr.toInternal(f.l2);
    ID l3 = tr.toInternal(f.l3);

    std::set<ID> latches(tr.begin_latches(), tr.end_latches());

    BOOST_CHECK_EQUAL(latches.size(), 4);
    BOOST_CHECK_EQUAL(latches.count(l0), 1);
    BOOST_CHECK_EQUAL(latches.count(l1), 1);
    BOOST_CHECK_EQUAL(latches.count(l2), 1);
    BOOST_CHECK_EQUAL(latches.count(l3), 1);

    CombinationalAigFixture fcomb;
    fcomb.buildTR();
    TransitionRelation & trcomb = *fcomb.tr;
    std::set<ID> latchescomb(trcomb.begin_latches(), trcomb.end_latches());
    BOOST_CHECK_EQUAL(latchescomb.size(), 0);
}

BOOST_AUTO_TEST_CASE(constraints_iter)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    std::set<ID> constraints(tr.begin_constraints(), tr.end_constraints());
    BOOST_CHECK_EQUAL(constraints.size(), 0);

    AigFixture f2;
    f2.addConstraint();
    f2.buildTR();
    TransitionRelation & tr2 = *f2.tr;
    ID c0 = tr2.toInternal(f2.c0);

    std::set<ID> constraints2(tr2.begin_constraints(), tr2.end_constraints());

    BOOST_CHECK_EQUAL(constraints2.size(), 1);
    BOOST_CHECK_EQUAL(constraints2.count(c0), 1);
}

BOOST_AUTO_TEST_CASE(inputs_iter)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID i0 = tr.toInternal(f.i0);
    ID i1 = tr.toInternal(f.i1);

    std::set<ID> inputs(tr.begin_inputs(), tr.end_inputs());

    BOOST_CHECK_EQUAL(inputs.size(), 2);
    BOOST_CHECK_EQUAL(inputs.count(i0), 1);
    BOOST_CHECK_EQUAL(inputs.count(i1), 1);
}

BOOST_AUTO_TEST_CASE(gate_ids_iter)
{
    AigFixture f;
    f.buildTR();
    TransitionRelation & tr = *f.tr;

    ID a0 = tr.toInternal(f.a0);

    std::vector<ID> gate_ids(tr.begin_gate_ids(), tr.end_gate_ids());

    BOOST_CHECK_EQUAL(gate_ids.size(), 1);
    BOOST_CHECK_EQUAL(gate_ids.at(0), a0);
}

BOOST_AUTO_TEST_CASE(copy_constructor)
{
    AigFixture f;
    f.buildTR();

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    // Set init state to X010
    f.tr->setInit(l0, ID_FALSE);
    f.tr->setInit(l1, ID_TRUE);
    f.tr->setInit(l2, ID_FALSE);
    f.tr->setInit(l3, ID_NULL);

    // Copy
    TransitionRelation tr(*f.tr);

    // Latch IDs
    BOOST_CHECK_EQUAL(tr.toInternal(f.l0), l0);
    BOOST_CHECK_EQUAL(tr.toInternal(f.l1), l1);
    BOOST_CHECK_EQUAL(tr.toInternal(f.l2), l2);
    BOOST_CHECK_EQUAL(tr.toInternal(f.l3), l3);

    // Latches
    std::set<ID> latches(tr.begin_latches(), tr.end_latches());

    BOOST_CHECK_EQUAL(latches.size(), 4);
    BOOST_CHECK_EQUAL(latches.count(l0), 1);
    BOOST_CHECK_EQUAL(latches.count(l1), 1);
    BOOST_CHECK_EQUAL(latches.count(l2), 1);
    BOOST_CHECK_EQUAL(latches.count(l3), 1);

    // Inputs
    ID i0 = tr.toInternal(f.i0);
    ID i1 = tr.toInternal(f.i1);

    std::set<ID> inputs(tr.begin_inputs(), tr.end_inputs());

    BOOST_CHECK_EQUAL(inputs.size(), 2);
    BOOST_CHECK_EQUAL(inputs.count(i0), 1);
    BOOST_CHECK_EQUAL(inputs.count(i1), 1);

    // Constraints
    std::set<ID> constraints(tr.begin_constraints(), tr.end_constraints());
    BOOST_CHECK_EQUAL(constraints.size(), 0);

    // Gate IDs
    ID a0 = tr.toInternal(f.a0);

    std::vector<ID> gate_ids(tr.begin_gate_ids(), tr.end_gate_ids());

    BOOST_CHECK_EQUAL(gate_ids.size(), 1);
    BOOST_CHECK_EQUAL(gate_ids.at(0), a0);

    // Initial states
    SATAdaptor sat;

    BOOST_CHECK_EQUAL(tr.getInit(l0), ID_FALSE);
    BOOST_CHECK_EQUAL(tr.getInit(l1), ID_TRUE);
    BOOST_CHECK_EQUAL(tr.getInit(l2), ID_FALSE);
    BOOST_CHECK_EQUAL(tr.getInit(l3), ID_NULL);

    sat.addClauses(tr.initState());

    BOOST_CHECK(!sat.solve({l0}));
    BOOST_CHECK(!sat.solve({negate(l1)}));
    BOOST_CHECK(!sat.solve({l2}));

    BOOST_CHECK(sat.solve({negate(l0), l1, negate(l2), l3}));
    BOOST_CHECK(sat.solve({negate(l0), l1, negate(l2), negate(l3)}));
    BOOST_CHECK(sat.solve({l1, negate(l2), negate(l3)}));
    BOOST_CHECK(!sat.solve({negate(l1), negate(l2), negate(l3)}));
    BOOST_CHECK(!sat.solve({negate(l1), l2, negate(l3)}));

    // Unrolling
    tr.setInit(l0, ID_FALSE);
    tr.setInit(l1, ID_FALSE);
    tr.setInit(l2, ID_FALSE);
    tr.setInit(l3, ID_FALSE);

    SATAdaptor sat1;
    sat1.addClauses(tr.unrollWithInit(1));

    // TR with zero init state should be SAT
    BOOST_CHECK(sat1.solve());

    // Latches should be zero
    BOOST_CHECK_EQUAL(sat1.getAssignment(l0), SAT::FALSE);
    BOOST_CHECK_EQUAL(sat1.getAssignment(l1), SAT::FALSE);
    BOOST_CHECK_EQUAL(sat1.getAssignment(l2), SAT::FALSE);
    BOOST_CHECK_EQUAL(sat1.getAssignment(l3), SAT::FALSE);

    // Output is l3
    // l3 = 0 should be SAT
    ID o0 = tr.toInternal(f.o0);
    BOOST_CHECK(!sat1.solve({o0}));
    BOOST_CHECK(sat1.solve({negate(o0)}));
}

BOOST_AUTO_TEST_CASE(partial_copy_constructor_full_copy)
{
    ManyGateAigFixture f;
    f.buildTR();

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    // Set init state to 0000
    f.tr->setInit(l0, ID_FALSE);
    f.tr->setInit(l1, ID_FALSE);
    f.tr->setInit(l2, ID_FALSE);
    f.tr->setInit(l3, ID_FALSE);

    ID a0 = f.tr->toInternal(f.a0);
    ID a1 = f.tr->toInternal(f.a1);
    ID a2 = f.tr->toInternal(f.a2);
    ID a3 = f.tr->toInternal(f.a3);
    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    ID o0 = f.tr->toInternal(f.o0);

    // Partial copy (but actually full)
    TransitionRelation tr(*f.tr, {a0, a1, a2, a3, a4, a5, a6});

    // Everything should be the same
    std::vector<ID> old_inputs(f.tr->begin_inputs(), f.tr->end_inputs());
    std::vector<ID> old_latches(f.tr->begin_latches(), f.tr->end_latches());
    std::vector<ID> old_constraints(f.tr->begin_constraints(), f.tr->end_constraints());
    std::vector<ID> old_gates(f.tr->begin_gate_ids(), f.tr->end_gate_ids());

    std::vector<ID> new_inputs(tr.begin_inputs(), tr.end_inputs());
    std::vector<ID> new_latches(tr.begin_latches(), tr.end_latches());
    std::vector<ID> new_constraints(tr.begin_constraints(), tr.end_constraints());
    std::vector<ID> new_gates(tr.begin_gate_ids(), tr.end_gate_ids());

    std::sort(old_inputs.begin(), old_inputs.end());
    std::sort(old_latches.begin(), old_latches.end());
    std::sort(old_constraints.begin(), old_constraints.end());
    std::sort(old_gates.begin(), old_gates.end());

    std::sort(new_inputs.begin(), new_inputs.end());
    std::sort(new_latches.begin(), new_latches.end());
    std::sort(new_constraints.begin(), new_constraints.end());
    std::sort(new_gates.begin(), new_gates.end());

    BOOST_CHECK(old_inputs == new_inputs);
    BOOST_CHECK(old_latches == new_latches);
    BOOST_CHECK(old_constraints == new_constraints);
    BOOST_CHECK(old_gates == new_gates);

    // Should be unreachable in 3 or fewer cycles
    SATAdaptor sat;
    sat.addClauses(tr.unrollWithInit(5));
    BOOST_CHECK(!sat.solve({o0}));
    BOOST_CHECK(!sat.solve({prime(o0, 3)}));
    BOOST_CHECK(sat.solve({negate(prime(o0, 3))}));
    BOOST_CHECK(sat.solve({prime(o0, 4)}));
}

BOOST_AUTO_TEST_CASE(partial_copy_1)
{
    ManyGateAigFixture f;
    f.buildTR();

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    // Set init state to 0000
    f.tr->setInit(l0, ID_FALSE);
    f.tr->setInit(l1, ID_FALSE);
    f.tr->setInit(l2, ID_FALSE);
    f.tr->setInit(l3, ID_FALSE);

    ID i0 = f.tr->toInternal(f.i0);

    ID a0 = f.tr->toInternal(f.a0);
    ID a4 = f.tr->toInternal(f.a4);
    ID a6 = f.tr->toInternal(f.a6);

    ID o0 = f.tr->toInternal(f.o0);

    // Partial copy (if i0 = 0, this should be SAFE)
    TransitionRelation tr(*f.tr, {a0, a4, a6});

    std::vector<ID> new_gates(tr.begin_gate_ids(), tr.end_gate_ids());
    std::sort(new_gates.begin(), new_gates.end());
    std::vector<ID> expected_gates = {a0, a4, a6};
    std::sort(expected_gates.begin(), expected_gates.end());

    BOOST_CHECK(new_gates == expected_gates);

    Cube input_0;
    for (unsigned i = 0; i <= 8; ++i)
    {
        input_0.push_back(negate(prime(i0, i)));
    }

    SATAdaptor sat;
    sat.addClauses(tr.unrollWithInit(8));
    for (unsigned i = 0; i < 8; ++i)
    {
        Cube assumps_safe = input_0;
        ID o = prime(o0, i);
        assumps_safe.push_back(o);
        BOOST_CHECK(!sat.solve(assumps_safe));

        BOOST_CHECK(sat.solve({negate(o)}));

        Cube assumps_unsafe = input_0;
        assumps_unsafe.push_back(negate(o));
        BOOST_CHECK(sat.solve(assumps_unsafe));
    }
}

BOOST_AUTO_TEST_CASE(partial_copy_2)
{
    ManyGateAigFixture f;
    f.buildTR();

    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    // Set init state to 0000
    f.tr->setInit(l0, ID_FALSE);
    f.tr->setInit(l1, ID_FALSE);
    f.tr->setInit(l2, ID_FALSE);
    f.tr->setInit(l3, ID_FALSE);

    ID a4 = f.tr->toInternal(f.a4);
    ID a5 = f.tr->toInternal(f.a5);
    ID a6 = f.tr->toInternal(f.a6);

    // Partial copy of just a6
    TransitionRelation tr(*f.tr, {a6});

    std::vector<ID> new_inputs(tr.begin_inputs(), tr.end_inputs());
    std::vector<ID> new_latches(tr.begin_latches(), tr.end_latches());
    std::vector<ID> new_constraints(tr.begin_constraints(), tr.end_constraints());
    std::vector<ID> new_gates(tr.begin_gate_ids(), tr.end_gate_ids());

    std::vector<ID> expected_inputs = { a4, a5 };

    BOOST_CHECK_EQUAL(new_inputs.size(), 2);
    BOOST_CHECK_EQUAL(new_latches.size(), 0);
    BOOST_CHECK_EQUAL(new_constraints.size(), 0);
    BOOST_CHECK_EQUAL(new_gates.size(), 1);
    BOOST_CHECK_EQUAL(new_gates.at(0), a6);

    std::sort(new_inputs.begin(), new_inputs.end());
    std::sort(expected_inputs.begin(), expected_inputs.end());

    BOOST_CHECK(new_inputs == expected_inputs);

    SATAdaptor sat;
    sat.addClauses(tr.unrollWithInit(1));

    BOOST_CHECK(sat.solve({a4, a5, a6}));
    BOOST_CHECK(sat.solve({a4, a6}));
    BOOST_CHECK(sat.solve({a5, a6}));
    BOOST_CHECK(!sat.solve({negate(a5), a6}));
    BOOST_CHECK(!sat.solve({a4, a5, negate(a6)}));
}

