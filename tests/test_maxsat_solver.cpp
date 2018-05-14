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

unsigned countAndBlock(const MaxSATSolver & solver, const std::vector<ID> & ids, Clause & block)
{
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

BOOST_AUTO_TEST_CASE(basic_maxsat)
{
    VariableManager vars;
    MaxSATSolver solver(vars);

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
    BOOST_CHECK(count == 2);
    solver.addClause(block);

    // The second one should also have 2
    BOOST_REQUIRE(solver.solve());
    block.clear();
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK(count == 2);
    solver.addClause(block);

    // And the third
    BOOST_REQUIRE(solver.solve());
    block.clear();
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK(count == 2);
    solver.addClause(block);

    // Then there should be three with one
    for (unsigned i = 0; i < 3; ++i)
    {
        BOOST_REQUIRE(solver.solve());
        block.clear();
        count = countAndBlock(solver, ids, block);
        BOOST_CHECK(count == 1);
        solver.addClause(block);
    }

    // Then it should be SAT with cardinality 0
    BOOST_CHECK(solver.solve());
    block.clear();
    count = countAndBlock(solver, ids, block);
    BOOST_CHECK(count == 0);

    // And finally UNSAT if we force a literal to be 1
    solver.addClause({a});
    BOOST_CHECK(!solver.solve());
}

