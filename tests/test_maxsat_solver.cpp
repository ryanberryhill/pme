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

#include "pme/util/maxsat_solver.h"

#define BOOST_TEST_MODULE MaxSATSolverTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

unsigned countAndBlock(const MaxSATSolver & solver,
                       const std::vector<ID> & ids,
                       Clause & block)
{
    block.clear();
    BOOST_REQUIRE(solver.isSAT());
    unsigned count = 0;
    for (ID id : ids)
    {
        if (solver.getAssignment(id) == SAT::TRUE)
        {
            count++;
            block.push_back(negate(id));
        }
    }
    return count;
}

void testBasicMaxSAT(VariableManager & vars, MaxSATSolver & solver)
{
    ID a = vars.getNewID();
    ID b = vars.getNewID();
    ID c = vars.getNewID();

    std::vector<ID> ids = {a,b,c};

    solver.addForOptimization(a);
    solver.addForOptimization(b);
    solver.addForOptimization(c);

    Clause cls = {negate(a), negate(b), negate(c)};
    solver.addClause(cls);

    BOOST_REQUIRE(solver.solve());

    // First assignment should have two variables set to 1
    Clause block;
    unsigned count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    solver.addClause(block);

    // The second one should also have 2
    BOOST_REQUIRE(solver.solve());
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    solver.addClause(block);

    // And the third
    BOOST_REQUIRE(solver.solve());
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    solver.addClause(block);

    // Then there should be three with one
    for (unsigned i = 0; i < 3; ++i)
    {
        BOOST_REQUIRE(solver.solve());
        count = countAndBlock(solver, ids, block);
        BOOST_CHECK_EQUAL(count, 1);
        solver.addClause(block);
    }

    // Then it should be SAT with cardinality 0
    BOOST_REQUIRE(solver.solve());
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 0);

    // And finally UNSAT if we force a literal to be 1
    solver.addClause({a});
    BOOST_CHECK(!solver.solve());
}

BOOST_AUTO_TEST_CASE(basic_pbo_maxsat)
{
    VariableManager vars;
    PBOMaxSATSolver solver(vars);
    testBasicMaxSAT(vars, solver);
}

BOOST_AUTO_TEST_CASE(basic_msu4_maxsat)
{
    VariableManager vars;
    MSU4MaxSATSolver solver(vars);
    testBasicMaxSAT(vars, solver);
}

void testMaxSAT(VariableManager & vars, MaxSATSolver & solver)
{
    ID a = vars.getNewID();
    ID b = vars.getNewID();
    ID c = vars.getNewID();
    ID d = vars.getNewID();
    ID e = vars.getNewID();

    std::vector<ID> ids = {negate(a), negate(b), c, negate(d), negate(e)};

    solver.addForOptimization(negate(a));
    solver.addForOptimization(negate(b));
    solver.addForOptimization(c);
    solver.addForOptimization(negate(d));
    solver.addForOptimization(negate(e));

    // Any 3 of the above is a MaxSAT solution, but not 4 or 5
    solver.addClause({a, b, negate(c), d});
    solver.addClause({a, b, negate(c), e});
    solver.addClause({a, b, d, e});
    solver.addClause({a, negate(c), d, e});
    solver.addClause({b, negate(c), d, e});

    Clause block;
    // First (5 C 3) = 10 assignments should satisfy 3 variables
    for (unsigned i = 0; i < 10; ++i)
    {
        BOOST_REQUIRE(solver.solve());
        unsigned count = countAndBlock(solver, ids, block);
        BOOST_CHECK_EQUAL(count, 3);
        solver.addClause(block);
    }

    // Next (5 C 2) = 10 assignments should satisfy 2 variables
    for (unsigned i = 0; i < 10; ++i)
    {
        BOOST_REQUIRE(solver.solve());
        unsigned count = countAndBlock(solver, ids, block);
        BOOST_CHECK_EQUAL(count, 2);
        solver.addClause(block);
    }

    // Next (5 C 1) = 5 assignments should satisfy 1 variable
    for (unsigned i = 0; i < 5; ++i)
    {
        BOOST_REQUIRE(solver.solve());
        unsigned count = countAndBlock(solver, ids, block);
        BOOST_CHECK_EQUAL(count, 1);
        solver.addClause(block);
    }

    // Finally we should get a solution where none are satisfied
    BOOST_REQUIRE(solver.solve());
    unsigned count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 0);

    // Add a clause to force UNSAT
    solver.addClause({c});
    BOOST_CHECK(!solver.solve());
}

BOOST_AUTO_TEST_CASE(pbo_maxsat)
{
    VariableManager vars;
    PBOMaxSATSolver solver(vars);
    testMaxSAT(vars, solver);
}

BOOST_AUTO_TEST_CASE(msu4_maxsat)
{
    VariableManager vars;
    MSU4MaxSATSolver solver(vars);
    testMaxSAT(vars, solver);
}

BOOST_AUTO_TEST_CASE(maxsat_with_assumptions)
{
    VariableManager vars;
    PBOMaxSATSolver solver(vars);

    ID a = vars.getNewID();
    ID b = vars.getNewID();
    ID c = vars.getNewID();
    ID d = vars.getNewID();
    ID e = vars.getNewID();

    std::vector<ID> ids = {a, b, c};

    // Maximizing a, b, c
    solver.addForOptimization(a);
    solver.addForOptimization(b);
    solver.addForOptimization(c);

    // a V b V c
    solver.addClause({a, b, c});
    // And an extra constraint depending on d
    solver.addClause({negate(d), negate(e), negate(a), negate(b), negate(c)});

    BOOST_REQUIRE(solver.solve());

    // First assumption-less assignment should have all three set to 1
    Clause block;
    unsigned count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 3);
    // don't block it yet

    // First with assumption (d) should have all three
    BOOST_REQUIRE(solver.assumpSolve({d}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 3);
    // Same for (e)
    BOOST_REQUIRE(solver.assumpSolve({e}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 3);
    // With (e) and (d) it should be 2
    BOOST_REQUIRE(solver.assumpSolve({d, e}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    // Order shouldn't matter
    BOOST_REQUIRE(solver.assumpSolve({e, d}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    // Nor should calling it several times
    BOOST_REQUIRE(solver.assumpSolve({e, d}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);

    // Now go back to assumptionless and block it
    BOOST_REQUIRE(solver.solve());
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 3);
    solver.addClause(block);

    // Going back to assumption-less there should be three cardinality 2s
    BOOST_REQUIRE(solver.solve());
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    // Don't block it yet

    // Add some clauses depending on d
    solver.addClause({negate(d), negate(a)});
    solver.addClause({negate(d), negate(b)});
    solver.addClause({negate(e), negate(a)});

    // There should not be a two true assingment with (d)
    BOOST_REQUIRE(solver.assumpSolve({d}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 1);
    // Don't block it

    // With (e) there should be only 1 cardinality 2 solution
    BOOST_REQUIRE(solver.assumpSolve({e}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    solver.addClause(block);

    BOOST_REQUIRE(solver.assumpSolve({e}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 1);

    // Back to assumption-less there should be two more at cardinality 2
    BOOST_REQUIRE(solver.solve());
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    solver.addClause(block);

    BOOST_REQUIRE(solver.solve());
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 2);
    solver.addClause(block);

    // With (d) there should only be a single cardinality 1 solution
    BOOST_REQUIRE(solver.assumpSolve({d}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 1);
    BOOST_CHECK(solver.getAssignment(c) == SAT::TRUE);
    solver.addClause(block);

    // No more solutions under (d)
    BOOST_CHECK(!solver.assumpSolve({d}));

    // With (e) there should be one more
    BOOST_REQUIRE(solver.assumpSolve({e}));
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 1);
    BOOST_CHECK(solver.getAssignment(b) == SAT::TRUE);
    solver.addClause(block);

    // No more solutions under (e)
    BOOST_CHECK(!solver.assumpSolve({e}));

    // Assumptionless there should be one more solution
    BOOST_REQUIRE(solver.solve());
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK_EQUAL(count, 1);
    BOOST_CHECK(solver.getAssignment(a) == SAT::TRUE);
    solver.addClause(block);

    // And now UNSAT
    BOOST_CHECK(!solver.solve());
}

BOOST_AUTO_TEST_CASE(pbo_safe_get_assignment)
{
    VariableManager vars;
    PBOMaxSATSolver solver(vars);

    ID a = vars.getNewID();
    ID b = vars.getNewID();
    ID c = vars.getNewID();
    ID d = vars.getNewID();

    solver.addForOptimization(a);
    solver.addForOptimization(b);

    solver.addClause({negate(a), negate(b)});
    solver.addClause({c});

    solver.solve();

    BOOST_CHECK_EQUAL(solver.safeGetAssignment(c), SAT::TRUE);
    BOOST_CHECK_EQUAL(solver.safeGetAssignment(d), SAT::UNDEF);
}
