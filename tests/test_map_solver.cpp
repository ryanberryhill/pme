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

    void blockDown(const Seed & seed)
    {
        std::set<ID> seed_set(seed.begin(), seed.end());
        blocked_down.push_back(seed_set);

        solver->blockDown(seed);
    }

    void blockUp(const Seed & seed)
    {
        std::set<ID> seed_set(seed.begin(), seed.end());
        blocked_up.push_back(seed_set);

        solver->blockUp(seed);
    }

    bool isBlocked(const Seed & seed)
    {
        std::set<ID> s(seed.begin(), seed.end());

        for (const std::set<ID> blocked : blocked_down)
        {
            if (std::includes(blocked.begin(), blocked.end(), s.begin(), s.end()))
            {
                return true;
            }
        }

        for (const std::set<ID> blocked : blocked_up)
        {
            if (std::includes(s.begin(), s.end(), blocked.begin(), blocked.end()))
            {
                return true;
            }
        }

        return false;
    }

    VariableManager vars;
    ID g1, g2, g3, g4;

    std::unique_ptr<Solver> solver;
    std::vector<ID> gates;

    std::vector<std::set<ID>> blocked_down;
    std::vector<std::set<ID>> blocked_up;
};

std::set<Seed> powerSet(Seed seed)
{
    if (seed.size() == 0)
    {
        Seed empty;
        return {empty};
    }
    else
    {
        std::set<Seed> powerset;
        for (size_t i = 0; i < seed.size(); ++i)
        {
            ID deleted = seed.at(i);

            Seed copy = seed;
            copy.erase(copy.begin() + i);

            std::set<Seed> powerset_sub = powerSet(copy);
            for (const Seed & s : powerset_sub)
            {
                Seed pcopy = s;
                pcopy.push_back(deleted);

                assert(std::is_sorted(s.begin(), s.end()));
                std::sort(pcopy.begin(), pcopy.end());

                powerset.insert(s);
                powerset.insert(pcopy);
            }
        }

        return powerset;
    }
}

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

    BOOST_CHECK(f.solver->checkSeed(seed));
    BOOST_CHECK(f.solver->checkSeed({f.g1}));
    BOOST_CHECK(f.solver->checkSeed({f.g2}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g3}));

    // Block the whole search space
    f.solver->blockDown(seed);

    std::tie(sat, seed) = f.solver->findMaximalSeed();

    BOOST_CHECK(!sat);
    BOOST_CHECK(seed.empty());
    BOOST_CHECK(!f.solver->checkSeed(seed));
    BOOST_CHECK(!f.solver->checkSeed({f.g1}));
    BOOST_CHECK(!f.solver->checkSeed({f.g2}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g3}));
}

template<class Solver>
void testFindMaximalBlockUp()
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

    f.solver->blockUp({f.g1, f.g2, f.g3});

    std::tie(sat, seed) = f.solver->findMaximalSeed();

    // {g1, g2, g3, g4} should be blocked, as should {g1, g2, g3}
    BOOST_CHECK(!f.solver->checkSeed({f.gates}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2, f.g3}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g2, f.g4}));

    // So the only other possibility is {g1, g2, g4}
    BOOST_REQUIRE(sat);
    BOOST_REQUIRE(!seed.empty());
    BOOST_CHECK(f.solver->checkSeed(seed));
    BOOST_CHECK(containedIn(seed, f.gates));
    BOOST_CHECK_EQUAL(seed.size(), 3);
    BOOST_CHECK(containedIn(seed, {f.g1, f.g2, f.g4}));

    f.solver->blockUp({f.g2, f.g4});

    // Next should be {g1, g3, g4}
    std::tie(sat, seed) = f.solver->findMaximalSeed();
    BOOST_REQUIRE(sat);
    BOOST_REQUIRE(!seed.empty());
    BOOST_CHECK(f.solver->checkSeed(seed));
    BOOST_CHECK(containedIn(seed, f.gates));
    BOOST_CHECK_EQUAL(seed.size(), 3);
    BOOST_CHECK(containedIn(seed, {f.g1, f.g3, f.g4}));

    f.solver->blockUp({f.g3});
    f.solver->blockUp({f.g4});

    // Next should be {g1, g2}
    std::tie(sat, seed) = f.solver->findMaximalSeed();
    BOOST_REQUIRE(sat);
    BOOST_REQUIRE(!seed.empty());
    BOOST_CHECK(f.solver->checkSeed(seed));
    BOOST_CHECK(containedIn(seed, f.gates));
    BOOST_CHECK_EQUAL(seed.size(), 2);
    BOOST_CHECK(containedIn(seed, {f.g1, f.g2}));

    f.solver->blockUp({f.g1});

    // All that's left is {g2} and empty seed
    std::tie(sat, seed) = f.solver->findMaximalSeed();
    BOOST_REQUIRE(sat);
    BOOST_REQUIRE(!seed.empty());
    BOOST_CHECK(f.solver->checkSeed(seed));
    BOOST_CHECK(containedIn(seed, f.gates));
    BOOST_CHECK_EQUAL(seed.size(), 1);
    BOOST_CHECK(containedIn(seed, {f.g2}));

    f.solver->blockUp({f.g2});

    // Should be empty
    std::tie(sat, seed) = f.solver->findMaximalSeed();
    BOOST_REQUIRE(sat);
    BOOST_CHECK(seed.empty());

    Seed empty;
    f.solver->blockUp(empty);

    // Should be UNSAT
    std::tie(sat, seed) = f.solver->findMaximalSeed();
    BOOST_CHECK(!sat);
    BOOST_CHECK(seed.empty());
}

template<class Solver>
void testFindMaximalBlockDown()
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

    f.solver->blockDown({f.g1});
    f.solver->blockDown({f.g2});
    f.solver->blockDown({f.g3});
    f.solver->blockDown({f.g4});

    BOOST_CHECK(!f.solver->checkSeed({f.g1}));
    BOOST_CHECK(!f.solver->checkSeed({f.g2}));
    BOOST_CHECK(!f.solver->checkSeed({f.g3}));
    BOOST_CHECK(!f.solver->checkSeed({f.g4}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g4}));

    std::tie(sat, seed) = f.solver->findMaximalSeed();

    // Should still be the whole thing
    BOOST_REQUIRE(sat);
    BOOST_CHECK_EQUAL(seed.size(), f.gates.size());
    BOOST_CHECK(f.solver->checkSeed(seed));
    BOOST_CHECK(containedIn(seed, f.gates));

    f.solver->blockDown({f.g1, f.g2, f.g3});
    f.solver->blockDown({f.g1, f.g2, f.g4});
    f.solver->blockDown({f.g1, f.g3, f.g4});
    f.solver->blockDown({f.g2, f.g3, f.g4});

    BOOST_CHECK(!f.solver->checkSeed({f.g1}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2}));
    BOOST_CHECK(!f.solver->checkSeed({f.g2, f.g3}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g3, f.g4}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g2, f.g3, f.g4}));

    std::tie(sat, seed) = f.solver->findMaximalSeed();

    // Should still be the whole thing
    BOOST_REQUIRE(sat);
    BOOST_CHECK_EQUAL(seed.size(), f.gates.size());
    BOOST_CHECK(f.solver->checkSeed(seed));
    BOOST_CHECK(containedIn(seed, f.gates));

    f.solver->blockDown({f.g1, f.g2, f.g3, f.g4});

    BOOST_CHECK(!f.solver->checkSeed({f.g1}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2}));
    BOOST_CHECK(!f.solver->checkSeed({f.g2, f.g3}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g3, f.g4}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2, f.g3, f.g4}));

    std::tie(sat, seed) = f.solver->findMaximalSeed();

    // Should be UNSAT
    BOOST_CHECK(!sat);
    BOOST_CHECK(seed.empty());
}

template<class Solver>
void testFindMinimalBasic()
{
    MapSolverFixture<Solver> f;

    bool sat;
    Seed seed;

    std::tie(sat, seed) = f.solver->findMinimalSeed();

    // Must be SAT
    BOOST_REQUIRE(sat);
    // And empty
    BOOST_CHECK(seed.empty());
}

template<class Solver>
void testFindMinimalBlockEverything()
{
    MapSolverFixture<Solver> f;

    bool sat;
    Seed seed;

    std::tie(sat, seed) = f.solver->findMinimalSeed();

    // Must be SAT
    BOOST_REQUIRE(sat);
    // And empty
    BOOST_CHECK(seed.empty());

    BOOST_CHECK(f.solver->checkSeed(seed));
    BOOST_CHECK(f.solver->checkSeed({f.g1}));
    BOOST_CHECK(f.solver->checkSeed({f.g2}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g3}));

    // Block the whole search space
    f.solver->blockUp(seed);

    std::tie(sat, seed) = f.solver->findMinimalSeed();

    BOOST_CHECK(!sat);
    BOOST_CHECK(seed.empty());
    BOOST_CHECK(!f.solver->checkSeed(seed));
    BOOST_CHECK(!f.solver->checkSeed({f.g1}));
    BOOST_CHECK(!f.solver->checkSeed({f.g2}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g3}));
}

template<class Solver>
void testFindMinimalBlockUp()
{
    MapSolverFixture<Solver> f;

    bool sat;
    Seed seed;

    std::tie(sat, seed) = f.solver->findMinimalSeed();

    // Must be SAT
    BOOST_REQUIRE(sat);
    // And empty
    BOOST_CHECK(seed.empty());

    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g2, f.g3, f.g4}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g2, f.g3}));

    f.solver->blockUp({f.g1, f.g2, f.g3, f.g4});

    std::tie(sat, seed) = f.solver->findMinimalSeed();
    BOOST_REQUIRE(sat);
    BOOST_CHECK(seed.empty());

    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2, f.g3, f.g4}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g2, f.g3}));

    f.solver->blockUp({f.g1, f.g2, f.g3});
    f.solver->blockUp({f.g1, f.g2, f.g4});
    f.solver->blockUp({f.g1, f.g3, f.g4});
    f.solver->blockUp({f.g2, f.g3, f.g4});

    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2, f.g3}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g3}));

    std::tie(sat, seed) = f.solver->findMinimalSeed();
    BOOST_REQUIRE(sat);
    BOOST_CHECK(seed.empty());

    f.solver->blockUp({f.g1, f.g2});
    f.solver->blockUp({f.g1, f.g3});
    f.solver->blockUp({f.g1, f.g4});
    f.solver->blockUp({f.g2, f.g3});
    f.solver->blockUp({f.g2, f.g4});
    f.solver->blockUp({f.g3, f.g4});

    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g3}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2}));
    BOOST_CHECK(f.solver->checkSeed({f.g1}));
    BOOST_CHECK(f.solver->checkSeed({f.g2}));

    std::tie(sat, seed) = f.solver->findMinimalSeed();
    BOOST_REQUIRE(sat);
    BOOST_CHECK(seed.empty());

    f.solver->blockUp({f.g1});
    f.solver->blockUp({f.g2});
    f.solver->blockUp({f.g3});
    f.solver->blockUp({f.g4});

    std::tie(sat, seed) = f.solver->findMinimalSeed();
    BOOST_REQUIRE(sat);
    BOOST_CHECK(seed.empty());

    BOOST_CHECK(!f.solver->checkSeed({f.g1}));
    BOOST_CHECK(!f.solver->checkSeed({f.g2}));
    Seed empty;
    BOOST_CHECK(f.solver->checkSeed(empty));

    f.solver->blockUp(empty);
    BOOST_CHECK(!f.solver->checkSeed(empty));

    std::tie(sat, seed) = f.solver->findMinimalSeed();
    BOOST_CHECK(!sat);
    BOOST_CHECK(seed.empty());
}

template<class Solver>
void testFindMinimalBlockDown()
{
    MapSolverFixture<Solver> f;

    bool sat;
    Seed seed;

    std::tie(sat, seed) = f.solver->findMinimalSeed();

    // Must be SAT
    BOOST_REQUIRE(sat);
    // And empty
    BOOST_CHECK(seed.empty());

    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g2, f.g3, f.g4}));
    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g2, f.g3}));

    f.solver->blockDown({f.g1, f.g2, f.g3});

    BOOST_CHECK(f.solver->checkSeed({f.g1, f.g2, f.g3, f.g4}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2, f.g3}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g2}));
    BOOST_CHECK(!f.solver->checkSeed({f.g1, f.g3}));
    BOOST_CHECK(!f.solver->checkSeed({f.g3}));

    // Next should be {g4}
    std::tie(sat, seed) = f.solver->findMinimalSeed();

    BOOST_REQUIRE(sat);
    BOOST_CHECK_EQUAL(seed.size(), 1);
    BOOST_CHECK(containedIn(seed, {f.g4}));

    f.solver->blockDown({f.g1, f.g3, f.g4});

    // Next {g2, g4}
    std::tie(sat, seed) = f.solver->findMinimalSeed();

    BOOST_REQUIRE(sat);
    BOOST_CHECK_EQUAL(seed.size(), 2);
    BOOST_CHECK(containedIn(seed, {f.g2, f.g4}));

    f.solver->blockDown({f.g1, f.g2, f.g4});

    // Next {g2, g3, g4}
    std::tie(sat, seed) = f.solver->findMinimalSeed();

    BOOST_REQUIRE(sat);
    BOOST_CHECK_EQUAL(seed.size(), 3);
    BOOST_CHECK(containedIn(seed, {f.g2, f.g3, f.g4}));

    f.solver->blockDown({f.g2, f.g3, f.g4});

    // Next {g1, g2, g3, g4}
    std::tie(sat, seed) = f.solver->findMinimalSeed();

    BOOST_REQUIRE(sat);
    BOOST_CHECK_EQUAL(seed.size(), 4);
    BOOST_CHECK(containedIn(seed, {f.g1, f.g2, f.g3, f.g4}));

    f.solver->blockDown({f.g1, f.g2, f.g3, f.g4});

    // Now UNSAT
    std::tie(sat, seed) = f.solver->findMinimalSeed();

    BOOST_REQUIRE(!sat);
    BOOST_CHECK(seed.empty());
}

template<class Solver>
void testFindArbitraryBlockUp()
{
    MapSolverFixture<Solver> f;

    bool sat = true;
    Seed seed;


    while(sat)
    {
        std::tie(sat, seed) = f.solver->findSeed();

        // Must be SAT and not already blocked
        if (sat)
        {
            BOOST_CHECK(!f.isBlocked(seed));
            BOOST_CHECK(f.solver->checkSeed(seed));
            if (seed.empty())
            {
                f.blockDown(seed);
            }
            else
            {
                f.blockUp({seed.at(0)});
            }
        }
    }

    // Upon UNSAT, check the whole power set is blocked
    std::set<Seed> powerset = powerSet(f.gates);
    for (const Seed & s : powerset)
    {
        BOOST_CHECK(f.isBlocked(s));
    }
}

BOOST_AUTO_TEST_CASE(sat_arbitrary_map_solver)
{
    testFindArbitraryBlockUp<SATArbitraryMapSolver>();
    testFindMaximalBasic<SATArbitraryMapSolver>();
    testFindMaximalBlockEverything<SATArbitraryMapSolver>();
    testFindMaximalBlockUp<SATArbitraryMapSolver>();
    testFindMaximalBlockDown<SATArbitraryMapSolver>();
    testFindMinimalBasic<SATArbitraryMapSolver>();
    testFindMinimalBlockEverything<SATArbitraryMapSolver>();
    testFindMinimalBlockUp<SATArbitraryMapSolver>();
    testFindMinimalBlockDown<SATArbitraryMapSolver>();
}

BOOST_AUTO_TEST_CASE(msu4_arbitrary_map_solver)
{
    testFindArbitraryBlockUp<MSU4ArbitraryMapSolver>();
    testFindMaximalBasic<MSU4ArbitraryMapSolver>();
    testFindMaximalBlockEverything<MSU4ArbitraryMapSolver>();
    testFindMaximalBlockUp<MSU4ArbitraryMapSolver>();
    testFindMaximalBlockDown<MSU4ArbitraryMapSolver>();
    testFindMinimalBasic<MSU4ArbitraryMapSolver>();
    testFindMinimalBlockEverything<MSU4ArbitraryMapSolver>();
    testFindMinimalBlockUp<MSU4ArbitraryMapSolver>();
    testFindMinimalBlockDown<MSU4ArbitraryMapSolver>();
}

BOOST_AUTO_TEST_CASE(msu4_maximal_map_solver)
{
    testMinimalUnsupported<MSU4MaximalMapSolver>();
    testFindMaximalBasic<MSU4MaximalMapSolver>();
    testFindMaximalBlockEverything<MSU4MaximalMapSolver>();
    testFindMaximalBlockUp<MSU4MaximalMapSolver>();
    testFindMaximalBlockDown<MSU4MaximalMapSolver>();
    testFindArbitraryBlockUp<MSU4MaximalMapSolver>();
}

BOOST_AUTO_TEST_CASE(msu4_minimal_map_solver)
{
    testMaximalUnsupported<MSU4MinimalMapSolver>();
    testFindMinimalBasic<MSU4MinimalMapSolver>();
    testFindMinimalBlockEverything<MSU4MinimalMapSolver>();
    testFindMinimalBlockUp<MSU4MinimalMapSolver>();
    testFindMinimalBlockDown<MSU4MinimalMapSolver>();
    testFindArbitraryBlockUp<MSU4MinimalMapSolver>();
}

