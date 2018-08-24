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

#include "pme/engine/variable_manager.h"
#include "pme/engine/debug_transition_relation.h"
#include "pme/engine/sat_adaptor.h"
#include "pme/util/cardinality_constraint.h"
extern "C" {
#include "aiger/aiger.h"
}

#define BOOST_TEST_MODULE DebugTransitionRelationTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <memory>

using namespace PME;

struct CombinationalAigFixture
{
    aiger * aig;
    ExternalID i1, o1;
    VariableManager vars;
    std::unique_ptr<DebugTransitionRelation> tr;
    std::unique_ptr<TotalizerCardinalityConstraint> cardinality;

    CombinationalAigFixture()
    {
        aig = aiger_init();

        i1 = 2;
        o1 = i1 + 2;

        // o1 = i1 & ~i1
        aiger_add_input(aig, i1, "i1");
        aiger_add_and(aig, o1, i1, aiger_not(i1));
        aiger_add_output(aig, o1, "o1");
    }

    ~CombinationalAigFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    ID debugLatch()
    {
        std::vector<ID> debug_latches(tr->begin_debug_latches(),
                                      tr->end_debug_latches());
        BOOST_REQUIRE_EQUAL(debug_latches.size(), 1);
        return debug_latches[0];
    }

    void buildTR()
    {
        tr.reset(new DebugTransitionRelation(vars, aig));

        cardinality.reset(new TotalizerCardinalityConstraint(vars));

        cardinality->addInput(debugLatch());
    }

    ClauseVec getCardinality(unsigned n)
    {
        cardinality->setCardinality(n + 1);
        cardinality->clearIncrementality();

        ClauseVec vec = cardinality->CNFize();
        Cube assumps = cardinality->assumeLEq(n);
        for (ID id : assumps) { vec.push_back({id}); }

        return vec;
    }
};

struct AigFixture
{
    aiger * aig;
    ExternalID i0, l0, l1, l2, l3, a0, a1, a2, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> ndtr;
    std::unique_ptr<DebugTransitionRelation> tr;
    std::unique_ptr<TotalizerCardinalityConstraint> cardinality;

    AigFixture()
    {
        aig = aiger_init();
        i0 = 2;
        l0 = i0 + 2;
        l1 = l0 + 2;
        l2 = l1 + 2;
        l3 = l2 + 2;
        a0 = l3 + 2;
        a1 = a0 + 2;
        a2 = a1 + 2;

        // l0' = i0
        // l1' = l0 & i0
        // l2' = l1 & l0
        // l3' = l2 & l1 & l0
        // o0 = l3
        aiger_add_input(aig, i0, "i0");
        aiger_add_latch(aig, l0, i0, "l0");
        aiger_add_and(aig, a0, l0, i0);
        aiger_add_latch(aig, l1, a0, "l1");
        aiger_add_and(aig, a1, l0, l1);
        aiger_add_latch(aig, l2, a1, "l2");
        aiger_add_and(aig, a2, a1, l2);
        aiger_add_latch(aig, l3, a2, "l3");
        aiger_add_output(aig, l3, "o0");

        // Leave initial state unconstrained
        aiger_add_reset(aig, l0, l0);
        aiger_add_reset(aig, l1, l1);
        aiger_add_reset(aig, l2, l2);
        aiger_add_reset(aig, l3, l3);

        o0 = l3;
    }

    void initZero()
    {
        aiger_add_reset(aig, l0, 0);
        aiger_add_reset(aig, l1, 0);
        aiger_add_reset(aig, l2, 0);
        aiger_add_reset(aig, l3, 0);
    }

    std::vector<ID> debugLatches()
    {
        std::vector<ID> dl(tr->begin_latches(), tr->end_latches());
        return dl;
    }

    ~AigFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    void buildTR()
    {
        ndtr.reset(new TransitionRelation(vars, aig));
        tr.reset(new DebugTransitionRelation(*ndtr));
        cardinality.reset(new TotalizerCardinalityConstraint(vars));

        std::vector<ID> dl = debugLatches();
        for (ID id : dl)
        {
            cardinality->addInput(id);
        }
    }

    ClauseVec getCardinality(unsigned n)
    {
        cardinality->setCardinality(n + 1);
        cardinality->clearIncrementality();

        ClauseVec vec = cardinality->CNFize();
        Cube assumps = cardinality->assumeLEq(n);
        for (ID id : assumps) { vec.push_back({id}); }

        return vec;
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

BOOST_AUTO_TEST_CASE(latch_iterators)
{
    AigFixture f;
    f.buildTR();

    // 4 real latches + 3 debug latches
    std::vector<ID> latches(f.tr->begin_latches(), f.tr->end_latches());
    BOOST_CHECK_EQUAL(latches.size(), 7);

    std::vector<ID> debug_latches(f.tr->begin_debug_latches(),
                                  f.tr->end_debug_latches());
    BOOST_CHECK_EQUAL(debug_latches.size(), 3);
}

BOOST_AUTO_TEST_CASE(input_iterators)
{
    AigFixture f;
    f.buildTR();

    // 1 real input + 3 debug
    std::vector<ID> inputs(f.tr->begin_inputs(), f.tr->end_inputs());
    BOOST_CHECK_EQUAL(inputs.size(), 4);

    std::vector<ID> debug_inputs(f.tr->begin_debug_inputs(),
                                  f.tr->end_debug_inputs());
    BOOST_CHECK_EQUAL(debug_inputs.size(), 3);
}

BOOST_AUTO_TEST_CASE(latch_gate_mappings)
{
    AigFixture f;
    f.buildTR();

    std::set<ID> gates(f.tr->begin_gate_ids(), f.tr->end_gate_ids());
    std::vector<ID> debug_latches(f.tr->begin_debug_latches(),
                                  f.tr->end_debug_latches());

    BOOST_CHECK_EQUAL(gates.size(), 3);
    BOOST_CHECK_EQUAL(debug_latches.size(), 3);

    for (ID id : debug_latches)
    {
        ID gate_id = f.tr->gateForDebugLatch(id);
        ID dl_id = f.tr->debugLatchForGate(gate_id);
        BOOST_CHECK_EQUAL(dl_id, id);
        BOOST_CHECK(gates.count(gate_id) > 0);
        gates.erase(gate_id);
    }

    BOOST_CHECK(gates.empty());
}

BOOST_AUTO_TEST_CASE(combinational_cardinality_0)
{
    CombinationalAigFixture f;
    f.buildTR();

    ID o1 = f.tr->toInternal(f.o1);
    ID es = f.debugLatch();

    SATAdaptor sat;
    sat.addClauses(f.tr->unrollWithInit(1));

    // Add cardinality
    sat.addClauses(f.getCardinality(0));

    BOOST_CHECK(sat.solve());
    BOOST_CHECK_EQUAL(sat.getAssignment(es), SAT::FALSE);
    BOOST_CHECK(sat.solve({negate(o1)}));
    BOOST_CHECK_EQUAL(sat.getAssignment(es), SAT::FALSE);
    BOOST_CHECK(!sat.solve({o1}));
}

BOOST_AUTO_TEST_CASE(combinational_cardinality_1)
{
    CombinationalAigFixture f;
    f.buildTR();

    ID o1 = f.tr->toInternal(f.o1);
    ID es = f.debugLatch();

    SATAdaptor sat;
    sat.addClauses(f.tr->unrollWithInit(1));

    // Add cardinality
    sat.addClauses(f.getCardinality(1));

    BOOST_CHECK(sat.solve());
    BOOST_CHECK(sat.solve({negate(o1)}));

    BOOST_CHECK(sat.solve({o1}));
    BOOST_CHECK_EQUAL(sat.getAssignment(es), SAT::TRUE);
}


BOOST_AUTO_TEST_CASE(basic_cardinality_0)
{
    AigFixture f;
    f.initZero();
    f.buildTR();

    ID o0 = f.tr->toInternal(f.o0);
    ID o0_3 = prime(o0, 3);
    std::vector<ID> debug_latches = f.debugLatches();

    SATAdaptor sat;
    sat.addClauses(f.tr->unrollWithInit(4));

    // Add cardinality
    sat.addClauses(f.getCardinality(0));

    // Should be SAT with no assumptions
    BOOST_REQUIRE(sat.solve());
    for (unsigned i = 0; i < 4; ++i)
    {
        for (ID dl : debug_latches)
        {
            BOOST_CHECK_EQUAL(sat.getAssignment(prime(dl, i)), SAT::FALSE);
        }
    }

    // Should be UNSAT with 0 cardinality and output = 1
    BOOST_REQUIRE(!sat.solve({o0_3}));
}

BOOST_AUTO_TEST_CASE(basic_debugging)
{
    AigFixture f;
    f.initZero();
    f.buildTR();

    ID o0 = f.tr->toInternal(f.o0);
    ID o0_3 = prime(o0, 3);
    std::vector<ID> debug_latches = f.debugLatches();


    // Should be SAT with 1 cardinality and output = 1.
    // There are three solutions (each gate is a solution)
    std::set<ID> solutions;
    for (unsigned i = 0; i < 3; ++i)
    {
        SATAdaptor sat;
        sat.addClauses(f.tr->unrollWithInit(4));

        // Add cardinality
        sat.addClauses(f.getCardinality(1));

        BOOST_REQUIRE(sat.solve({o0_3}));

        ID solution = ID_NULL;
        unsigned count = 0;
        // One debug latch should be 1 (initially and in all cycles)
        for (ID id : debug_latches)
        {
            if (sat.getAssignment(id) == SAT::TRUE)
            {
                count++;
                solution = id;
                BOOST_CHECK_EQUAL(solutions.count(solution), 0);
                solutions.insert(solution);
            }
        }

        BOOST_REQUIRE(solution != ID_NULL);
        BOOST_CHECK_EQUAL(count, 1);

        for (unsigned i = 0; i < 4; ++i)
        {
            ID primed = prime(solution, i);
            BOOST_CHECK_EQUAL(sat.getAssignment(primed), SAT::TRUE);
        }

        // Block the solution
        f.tr->setInit(solution, ID_FALSE);
    }

    SATAdaptor sat;
    sat.addClauses(f.tr->unrollWithInit(4));

    // Add cardinality
    sat.addClauses(f.getCardinality(1));

    BOOST_CHECK(!sat.solve({o0_3}));
}

