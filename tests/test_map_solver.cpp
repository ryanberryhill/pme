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

#include "pme/util/map_solver.h"

#define BOOST_TEST_MODULE MapSolverTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

template<class Solver>
struct MapSolverFixture
{
    MapSolverFixture()
    {
        g1 = vars.getNewID();
        g2 = vars.getNewID();
        g3 = vars.getNewID();
        g4 = vars.getNewID();

        gates.push_back(g1);
        gates.push_back(g2);
        gates.push_back(g3);
        gates.push_back(g4);

        solver.reset(new Solver(gates.begin(), gates.end(), vars));
    }

    VariableManager vars;
    ID g1, g2, g3, g4;

    std::unique_ptr<Solver> solver;
    std::vector<ID> gates;
};

bool containedIn(const Seed & s1, const Seed & s2)
{
    for (ID gate : s1)
    {
        auto it = std::find(s2.begin(), s2.end(), gate);
        if (it == s2.end()) { return false; }
    }

    return true;
}

template<class Solver>
void testMinimalUnsupported()
{
    MapSolverFixture<Solver> f;
    BOOST_CHECK_THROW(f.solver->findMinimalSeed(), std::logic_error);
}

template<class Solver>
void testMaximalUnsupported()
{
    MapSolverFixture<Solver> f;
    BOOST_CHECK_THROW(f.solver->findMaximalSeed(), std::logic_error);
}

template<class Solver>
void testFindMaximalBasic()
{
    MapSolverFixture<Solver> f;

    bool sat;
    Seed seed;

    std::tie(sat, seed) = f.solver->findMaximalSeed();

    // Must be SAT
    BOOST_REQUIRE(sat);
    // And equal to the whole thing
    BOOST_CHECK_EQUAL(seed.size(), f.gates.size());
    // Check the gates also match up
    BOOST_CHECK(containedIn(seed, f.gates));
}

template<class Solver>
void testFindMaximalBlockEverything()
{
    MapSolverFixture<Solver> f;

    bool sat;
    Seed seed;

    std::tie(sat, seed) = f.solver->findMaximalSeed();

    // Must be SAT
    BOOST_REQUIRE(sat);
    // And equal to the whole thing
    BOOST_CHECK_EQUAL(seed.size(), f.gates.size());
    // Check the gates also match up
    BOOST_CHECK(containedIn(seed, f.gates));

    // Block the whole search space
    f.solver->blockDown(seed);

    std::tie(sat, seed) = f.solver->findMaximalSeed();

    BOOST_CHECK(!sat);
    BOOST_CHECK(seed.empty());
}

BOOST_AUTO_TEST_CASE(maximal_map_solver)
{
    testMinimalUnsupported<MaximalMapSolver>();
    testFindMaximalBasic<MaximalMapSolver>();
    testFindMaximalBlockEverything<MaximalMapSolver>();
}

