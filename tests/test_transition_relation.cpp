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

using namespace PME;

struct CombinationalAigFixture
{
    aiger * aig;
    ExternalID i1, i2, o1, c0;

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
};

struct AigFixture
{
    aiger * aig;
    ExternalID i0, i1, l0, l1, l2, l3, a0, o0, c0;

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
};

void sortClauseVec(ClauseVec & vec)
{
    for (Clause & cls : vec)
    {
        std::sort(cls.begin(), cls.end());
    }

    std::sort(vec.begin(), vec.end());
}

BOOST_AUTO_TEST_CASE(combinational)
{
    CombinationalAigFixture f;

    VariableManager vars;
    TransitionRelation tr(vars, f.aig);
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

    // Default reset should be zero
    VariableManager vars_default, vars_zero, vars_none;
    TransitionRelation tr_default(vars_default, f_default.aig);
    TransitionRelation tr_zero(vars_zero, f_zero.aig);
    TransitionRelation tr_none(vars_none, f_none.aig);

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

    VariableManager vars;
    TransitionRelation tr(vars, f.aig);
    SATAdaptor sat;
    sat.addClauses(tr.unroll(1));

    // TR on its own should be SAT
    BOOST_CHECK(sat.solve());

    // Output = 0 or = 1 should be SAT
    ID o0 = tr.toInternal(f.o0);
    BOOST_CHECK(sat.solve({o0}));
    BOOST_CHECK(sat.solve({negate(o0)}));
}

BOOST_AUTO_TEST_CASE(sequential_nounroll_initzero)
{
    AigFixture f;
    f.addResets(0);

    VariableManager vars;
    TransitionRelation tr(vars, f.aig);
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

    VariableManager vars;
    TransitionRelation tr(vars, f.aig);
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

    VariableManager vars;
    TransitionRelation tr(vars, f.aig);
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

    VariableManager vars;
    TransitionRelation tr(vars, f.aig);
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

BOOST_AUTO_TEST_CASE(init_states)
{
    AigFixture f0, f1;
    f0.addResets(0);
    f1.addResets(1);

    VariableManager vars0, vars1;
    TransitionRelation tr0(vars0, f0.aig), tr1(vars1, f1.aig);
    SATAdaptor sat0, sat1;

    ID l0_0 = tr0.toInternal(f0.l0);
    ID l1_0 = tr0.toInternal(f0.l1);
    ID l2_0 = tr0.toInternal(f0.l2);
    ID l3_0 = tr0.toInternal(f0.l3);

    ID l0_1 = tr0.toInternal(f1.l0);
    ID l1_1 = tr0.toInternal(f1.l1);
    ID l2_1 = tr0.toInternal(f1.l2);
    ID l3_1 = tr0.toInternal(f1.l3);

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
}

