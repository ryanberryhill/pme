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

#include "pme/util/cardinality_constraint.h"
#include "pme/engine/sat_adaptor.h"
#include "pme/engine/variable_manager.h"

#define BOOST_TEST_MODULE CardinalityConstraintTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

template<typename T>
struct CardinalityFixture
{
    VariableManager v;
    std::unique_ptr<CardinalityConstraint> cardinality;
    std::vector<ID> ids;
    SATAdaptor sat;

    CardinalityFixture(unsigned n) : cardinality(new T(v))
    {
        for (unsigned i = 0; i < n; ++i)
        {
            ids.push_back(v.getNewID());
        }

        for (ID id : ids) { cardinality->addInput(id); }
    }

    unsigned countAssignedTo(ModelValue value)
    {
        unsigned count = 0;
        for (unsigned id : ids)
        {
            if (sat.getAssignment(id) == value) { count++; }
        }
        return count;
    }

    Cube assumeTrue(unsigned n)
    {
        Clause assumps;
        for (unsigned j = 0; j < ids.size(); ++j)
        {
            ID id = ids[j];
            if (j >= n) { id = negate(id); }
            assumps.push_back(id);
        }
        return assumps;
    }
};

template<typename T>
void testGetCardinality()
{
    VariableManager v;
    std::unique_ptr<CardinalityConstraint> cardinality(new T(v));

    ID a = v.getNewID("a");
    ID b = v.getNewID("b");
    ID c = v.getNewID("c");

    BOOST_CHECK_EQUAL(cardinality->getCardinality(), 0);
    BOOST_CHECK_EQUAL(cardinality->getOutputCardinality(), 0);

    cardinality->setCardinality(1);

    BOOST_CHECK_EQUAL(cardinality->getCardinality(), 1);
    BOOST_CHECK_EQUAL(cardinality->getOutputCardinality(), 0);

    cardinality->addInput(a);

    BOOST_CHECK_EQUAL(cardinality->getCardinality(), 1);
    BOOST_CHECK_EQUAL(cardinality->getOutputCardinality(), 1);
    cardinality->CNFize();
    BOOST_CHECK_EQUAL(cardinality->outputs().size(), 1);

    cardinality->addInput(b);

    BOOST_CHECK_EQUAL(cardinality->getCardinality(), 1);
    BOOST_CHECK_EQUAL(cardinality->getOutputCardinality(), 1);
    cardinality->CNFize();
    BOOST_CHECK_EQUAL(cardinality->outputs().size(), 1);

    cardinality->setCardinality(3);

    BOOST_CHECK_EQUAL(cardinality->getCardinality(), 3);
    BOOST_CHECK_EQUAL(cardinality->getOutputCardinality(), 2);
    cardinality->CNFize();
    BOOST_CHECK_EQUAL(cardinality->outputs().size(), 2);

    cardinality->addInput(c);

    BOOST_CHECK_EQUAL(cardinality->getCardinality(), 3);
    BOOST_CHECK_EQUAL(cardinality->getOutputCardinality(), 3);
    cardinality->CNFize();
    BOOST_CHECK_EQUAL(cardinality->outputs().size(), 3);
}

BOOST_AUTO_TEST_CASE(totalizer_get_cardinality)
{
    testGetCardinality<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_get_cardinality)
{
    testGetCardinality<SortingCardinalityConstraint>();
}

template<typename T>
void testCardinality1CNFization()
{
    CardinalityFixture<T> f(8);
    f.cardinality->setCardinality(1);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    f.sat.addClauses(cnf);

    BOOST_CHECK_EQUAL(f.cardinality->outputs().size(), 1);
    ID output = f.cardinality->outputs().at(0);

    // Cardinality should be SAT on its own
    BOOST_CHECK(f.sat.solve());

    // Should be SAT with output = 1
    BOOST_CHECK(f.sat.solve({output}));
    // And exactly one variable should be assigned to 1
    BOOST_CHECK_EQUAL(f.countAssignedTo(SAT::TRUE), 1);

    // Should be SAT with output = 0
    BOOST_CHECK(f.sat.solve({negate(output)}));
    // And none should be assigned to 1
    BOOST_CHECK_EQUAL(f.countAssignedTo(SAT::TRUE), 0);
}

BOOST_AUTO_TEST_CASE(totalizer_cnfization_cardinality_1)
{
    testCardinality1CNFization<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_cnfization_cardinality_1)
{
    testCardinality1CNFization<SortingCardinalityConstraint>();
}

template<typename T>
void testAssumeEq()
{
    CardinalityFixture<T> f(8);
    f.cardinality->setCardinality(8);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    f.sat.addClauses(cnf);

    for (unsigned i = 0; i <= 8; ++i)
    {
        for (unsigned j = 0; j <= 8; ++j)
        {
            Cube assumps = f.assumeTrue(i);
            Cube cassumps = f.cardinality->assumeEq(j);
            assumps.insert(assumps.end(), cassumps.begin(), cassumps.end());
            if (i == j)
            {
                BOOST_REQUIRE(f.sat.solve(assumps));
                BOOST_CHECK(f.countAssignedTo(SAT::TRUE) == j);
            }
            else
            {
                BOOST_CHECK(!f.sat.solve(assumps));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(totalizer_assume_eq)
{
    testAssumeEq<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_assume_eq)
{
    testAssumeEq<SortingCardinalityConstraint>();
}

template<typename T>
void testAssumeLEq()
{
    CardinalityFixture<T> f(8);
    f.cardinality->setCardinality(8);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    f.sat.addClauses(cnf);

    for (unsigned i = 0; i <= 8; ++i)
    {
        for (unsigned j = 0; j <= 8; ++j)
        {
            Cube assumps = f.assumeTrue(i);
            Cube cassumps = f.cardinality->assumeLEq(j);
            assumps.insert(assumps.end(), cassumps.begin(), cassumps.end());
            if (i <= j)
            {
                BOOST_REQUIRE(f.sat.solve(assumps));
                BOOST_CHECK(f.countAssignedTo(SAT::TRUE) <= j);
            }
            else
            {
                BOOST_CHECK(!f.sat.solve(assumps));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(totalizer_assume_leq)
{
    testAssumeLEq<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_assume_leq)
{
    testAssumeLEq<SortingCardinalityConstraint>();
}

template<typename T>
void testAssumeLT()
{
    CardinalityFixture<T> f(8);
    f.cardinality->setCardinality(8);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    f.sat.addClauses(cnf);

    for (unsigned i = 0; i <= 8; ++i)
    {
        for (unsigned j = 1; j < 8; ++j)
        {
            Cube assumps = f.assumeTrue(i);
            Cube cassumps = f.cardinality->assumeLT(j);
            assumps.insert(assumps.end(), cassumps.begin(), cassumps.end());
            if (i < j)
            {
                BOOST_REQUIRE(f.sat.solve(assumps));
                BOOST_CHECK(f.countAssignedTo(SAT::TRUE) < j);
            }
            else
            {
                BOOST_CHECK(!f.sat.solve(assumps));
            }
        }
    }

    BOOST_CHECK_THROW(f.cardinality->assumeLT(0), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(totalizer_assume_lt)
{
    testAssumeLT<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_assume_lt)
{
    testAssumeLT<SortingCardinalityConstraint>();
}

template<typename T>
void testAssumeGEq()
{
    CardinalityFixture<TotalizerCardinalityConstraint> f(8);
    f.cardinality->setCardinality(8);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    f.sat.addClauses(cnf);

    for (unsigned i = 0; i <= 8; ++i)
    {
        for (unsigned j = 0; j <= 8; ++j)
        {
            Cube assumps = f.assumeTrue(i);
            Cube cassumps = f.cardinality->assumeGEq(j);
            assumps.insert(assumps.end(), cassumps.begin(), cassumps.end());
            if (i >= j)
            {
                BOOST_REQUIRE(f.sat.solve(assumps));
                BOOST_CHECK(f.countAssignedTo(SAT::TRUE) >= j);
            }
            else
            {
                BOOST_CHECK(!f.sat.solve(assumps));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(totalizer_assume_geq)
{
    testAssumeGEq<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_assume_geq)
{
    testAssumeGEq<SortingCardinalityConstraint>();
}

template<typename T>
void testAssumeGT()
{
    CardinalityFixture<T> f(8);
    f.cardinality->setCardinality(8);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    f.sat.addClauses(cnf);

    for (unsigned i = 0; i <= 8; ++i)
    {
        for (unsigned j = 0; j < 8; ++j)
        {
            Cube assumps = f.assumeTrue(i);
            Cube cassumps = f.cardinality->assumeGT(j);
            assumps.insert(assumps.end(), cassumps.begin(), cassumps.end());
            if (i > j)
            {
                BOOST_REQUIRE(f.sat.solve(assumps));
                BOOST_CHECK(f.countAssignedTo(SAT::TRUE) > j);
            }
            else
            {
                BOOST_CHECK(!f.sat.solve(assumps));
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(totalizer_assume_gt)
{
    testAssumeGT<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_assume_gt)
{
    testAssumeGT<SortingCardinalityConstraint>();
}

template<typename T>
void testLargeCardinality()
{
    CardinalityFixture<T> f(100);
    f.cardinality->setCardinality(50);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    f.sat.addClauses(cnf);

    BOOST_CHECK_EQUAL(f.cardinality->outputs().size(), 50);

    // Cardinality should be SAT on its own
    BOOST_CHECK(f.sat.solve());

    // Should count correctly
    for (unsigned i = 0; i <= 50; ++i)
    {
        Clause assumps = f.assumeTrue(i);

        BOOST_CHECK(f.sat.solve(assumps));

        const std::vector<ID> & outputs = f.cardinality->outputs();
        for (unsigned j = 0; j < outputs.size(); ++j)
        {
            ID lit = outputs[j];
            ModelValue expected = j < i ? SAT::TRUE : SAT::FALSE;
            BOOST_CHECK_EQUAL(f.sat.getAssignment(lit), expected);
        }
    }

    // Should be SAT for all values
    for (unsigned i = 0; i < 50; ++i)
    {
        BOOST_REQUIRE(f.sat.solve(f.cardinality->assumeEq(i)));
        BOOST_CHECK_EQUAL(f.countAssignedTo(SAT::TRUE), i);
    }

    // Should throw for values that are too high
    BOOST_CHECK_THROW(f.cardinality->assumeEq(50), std::invalid_argument);
    BOOST_CHECK_THROW(f.cardinality->assumeEq(51), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(totalizer_large_cardinality)
{
    testLargeCardinality<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_large_cardinality)
{
    testLargeCardinality<SortingCardinalityConstraint>();
}

template<typename T>
void testFullCardinality()
{
    CardinalityFixture<T> f(50);
    f.cardinality->setCardinality(50);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    f.sat.addClauses(cnf);

    BOOST_CHECK_EQUAL(f.cardinality->outputs().size(), 50);

    // Cardinality should be SAT on its own
    BOOST_CHECK(f.sat.solve());

    // Should count correctly
    for (unsigned i = 0; i <= 50; ++i)
    {
        Clause assumps;
        for (unsigned j = 0; j < f.ids.size(); ++j)
        {
            ID id = f.ids[j];
            if (j >= i) { id = negate(id); }
            assumps.push_back(id);
        }

        BOOST_CHECK(f.sat.solve(assumps));

        const std::vector<ID> & outputs = f.cardinality->outputs();
        for (unsigned j = 0; j < outputs.size(); ++j)
        {
            ID lit = outputs[j];
            ModelValue expected = j < i ? SAT::TRUE : SAT::FALSE;
            BOOST_CHECK_EQUAL(f.sat.getAssignment(lit), expected);
        }
    }

    // Should be SAT for all values
    for (unsigned i = 0; i <= 50; ++i)
    {
        BOOST_REQUIRE(f.sat.solve(f.cardinality->assumeEq(i)));
        BOOST_CHECK_EQUAL(f.countAssignedTo(SAT::TRUE), i);
    }

    // Should throw for values that are too high
    BOOST_CHECK_THROW(f.cardinality->assumeEq(51), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(totalizer_full_cardinality)
{
    testFullCardinality<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_full_cardinality)
{
    testFullCardinality<SortingCardinalityConstraint>();
}

template<typename T>
void testDecreasingCardinality()
{
    CardinalityFixture<TotalizerCardinalityConstraint> f(3);
    f.cardinality->setCardinality(4);

    ClauseVec cnf = f.cardinality->CNFize();

    BOOST_REQUIRE(!cnf.empty());

    ID a = f.ids[0];
    ID b = f.ids[1];
    ID c = f.ids[2];

    f.sat.addClause({negate(a), negate(b), negate(c)});
    f.sat.addClauses(cnf);

    BOOST_REQUIRE(f.sat.solve());

    Cube assumps = f.cardinality->assumeGEq(3);
    BOOST_CHECK(!f.sat.solve(assumps));

    assumps = f.cardinality->assumeGEq(2);
    BOOST_CHECK(f.sat.solve(assumps));
    f.sat.addClause({negate(a), negate(c)});
    BOOST_CHECK(f.sat.solve(assumps));
    f.sat.addClause({negate(a), negate(b)});
    BOOST_CHECK(f.sat.solve(assumps));
    f.sat.addClause({negate(b), negate(c)});
    BOOST_CHECK(!f.sat.solve(assumps));

    assumps = f.cardinality->assumeGEq(1);
    BOOST_CHECK(f.sat.solve(assumps));
    f.sat.addClause({negate(a)});
    BOOST_CHECK(f.sat.solve(assumps));
    f.sat.addClause({negate(b)});
    BOOST_CHECK(f.sat.solve(assumps));
    f.sat.addClause({negate(c)});
    BOOST_CHECK(!f.sat.solve(assumps));

    assumps = f.cardinality->assumeGEq(0);
    BOOST_CHECK(f.sat.solve(assumps));
    BOOST_CHECK(f.sat.solve());
}

BOOST_AUTO_TEST_CASE(totalizer_decreasing_cardinality)
{
    testDecreasingCardinality<TotalizerCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(sn_decreasing_cardinality)
{
    testDecreasingCardinality<SortingCardinalityConstraint>();
}

BOOST_AUTO_TEST_CASE(incremental_cardinality)
{
    CardinalityFixture<TotalizerCardinalityConstraint> f(32);

    TotalizerCardinalityConstraint & cardinality =
        dynamic_cast<TotalizerCardinalityConstraint&>(*f.cardinality);

    std::set<Clause> knownClauses;

    for (unsigned current = 1; current < 32; ++current)
    {
        cardinality.setCardinality(current);
        ClauseVec cnf = cardinality.incrementalCNFize();

        // It should not generate duplicate clauses
        for (Clause cls : cnf)
        {
            std::sort(cls.begin(), cls.end());
            BOOST_CHECK(knownClauses.count(cls) == 0);
            knownClauses.insert(cls);
        }

        BOOST_REQUIRE(!cnf.empty());

        f.sat.addClauses(cnf);

        BOOST_CHECK_EQUAL(cardinality.outputs().size(), current);

        // Cardinality should be SAT on its own
        BOOST_CHECK(f.sat.solve());

        // Should count correctly
        for (unsigned i = 0; i <= current; ++i)
        {
            Clause assumps;
            for (unsigned j = 0; j < f.ids.size(); ++j)
            {
                ID id = f.ids[j];
                if (j >= i) { id = negate(id); }
                assumps.push_back(id);
            }

            BOOST_CHECK(f.sat.solve(assumps));

            const std::vector<ID> & outputs = cardinality.outputs();
            for (unsigned j = 0; j < outputs.size(); ++j)
            {
                ID lit = outputs[j];
                ModelValue expected = j < i ? SAT::TRUE : SAT::FALSE;
                BOOST_CHECK_EQUAL(f.sat.getAssignment(lit), expected);
            }
        }

        // Should be SAT for all values
        for (unsigned i = 0; i < current; ++i)
        {
            BOOST_REQUIRE(f.sat.solve(cardinality.assumeEq(i)));
            BOOST_CHECK_EQUAL(f.countAssignedTo(SAT::TRUE), i);
        }

        // Should throw for values that are too high
        if (current < 32)
        {
            BOOST_CHECK_THROW(cardinality.assumeEq(current), std::invalid_argument);
        }
        else
        {
            // But if we're assuming all variables it's okay to assume Eq
            BOOST_REQUIRE(f.sat.solve(cardinality.assumeEq(32)));
            BOOST_CHECK_EQUAL(f.countAssignedTo(SAT::TRUE), 32);
        }

        if (current + 1 < 32)
        {
            BOOST_CHECK_THROW(cardinality.assumeEq(current + 1), std::invalid_argument);
        }
    }
}

BOOST_AUTO_TEST_CASE(mixed_incremental)
{
    CardinalityFixture<TotalizerCardinalityConstraint> f(20);

    TotalizerCardinalityConstraint & cardinality =
        dynamic_cast<TotalizerCardinalityConstraint&>(*f.cardinality);
    cardinality.setCardinality(16);

    ClauseVec cnf_orig = cardinality.CNFize();
    ClauseVec should_be_empty = cardinality.incrementalCNFize();

    BOOST_CHECK(!cnf_orig.empty());
    BOOST_CHECK(should_be_empty.empty());

    ClauseVec cnf_again = cardinality.CNFize();

    std::sort(cnf_orig.begin(), cnf_orig.end());
    std::sort(cnf_again.begin(), cnf_again.end());

    BOOST_CHECK(cnf_again == cnf_orig);
}

