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

#include "sat/sat.h"

#define BOOST_TEST_MODULE SATTest

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <algorithm>

using namespace SAT;

typedef std::unique_ptr<SAT::Solver> SolverPointer;

struct SATFixture
{
    SATFixture()
    {
        SolverPointer minisat(new MinisatSolver);
        SolverPointer minisat_simp(new MinisatSimplifyingSolver);
        SolverPointer glucose(new GlucoseSolver);
        solvers.push_back(std::move(minisat));
        solvers.push_back(std::move(minisat_simp));
        solvers.push_back(std::move(glucose));
    }

    std::vector<SolverPointer> solvers;
};

void sortClauseVec(std::vector<Clause> & vec)
{
    for (Clause & cls : vec)
    {
        std::sort(cls.begin(), cls.end());
    }
    std::sort(vec.begin(), vec.end());
}

BOOST_AUTO_TEST_CASE(one_var)
{
    SATFixture f;

    for (auto & slv : f.solvers)
    {
        BOOST_CHECK(!slv->isSAT());
        Variable v = slv->newVariable();

        // (v)
        slv->addClause({v});
        BOOST_CHECK(slv->solve({}));
        BOOST_CHECK(slv->isSAT());

        // (v) & (-v)[assumption]
        BOOST_CHECK(!slv->solve({negate(v)}));
        BOOST_CHECK(!slv->isSAT());

        // (v) & (-v)
        slv->addClause({negate(v)});
        BOOST_CHECK(!slv->isSAT());
        BOOST_CHECK(!slv->solve({}));
    }
}

BOOST_AUTO_TEST_CASE(three_var)
{
    SATFixture f;

    for (auto & slv : f.solvers)
    {
        Variable a = slv->newVariable();
        Variable b = slv->newVariable();
        Variable c = slv->newVariable();

        // (a) & (b V c) & (-c)
        Cube empty_assumps;
        slv->addClause({a});
        slv->addClause({b, c});
        slv->addClause({negate(c)});
        BOOST_CHECK(slv->solve({}));

        BOOST_CHECK(!slv->solve({c}));

        BOOST_CHECK(slv->solve({b}));

        slv->addClause({b});
        BOOST_CHECK(slv->solve(empty_assumps));
        BOOST_CHECK(!slv->solve({negate(b)}));
    }
}

BOOST_AUTO_TEST_CASE(get_assignment)
{
    SATFixture f;

    for (auto & slv : f.solvers)
    {
        Variable a = slv->newVariable();
        Variable b = slv->newVariable();

        // (-a V -b)
        slv->addClause({negate(a), negate(b)});
        BOOST_CHECK(slv->solve({a}));
        BOOST_CHECK_EQUAL(slv->getAssignment(a), TRUE);
        BOOST_CHECK_EQUAL(slv->getAssignment(b), FALSE);

        BOOST_CHECK(slv->solve({b}));
        BOOST_CHECK_EQUAL(slv->getAssignment(a), FALSE);
        BOOST_CHECK_EQUAL(slv->getAssignment(b), TRUE);
    }
}

BOOST_AUTO_TEST_CASE(critical_assumptions)
{
    SATFixture f;
    for (auto & slv : f.solvers)
    {
        Variable a = slv->newVariable();
        Variable b = slv->newVariable();
        Variable c = slv->newVariable();
        Variable d = slv->newVariable();

        slv->addClause({a, b, c, d});
        slv->addClause({negate(a), negate(b), negate(c), negate(d)});
        slv->addClause({negate(c), d});

        Cube assumps = { a, b, c, d };
        Cube crits;
        BOOST_REQUIRE(!slv->solve(assumps, &crits));

        // Criticals are sufficient to prove UNSAT
        BOOST_CHECK(!crits.empty());
        BOOST_CHECK(!slv->solve(crits));

        // Each critical should be from the assumptions
        for (Literal l : crits)
        {
            BOOST_CHECK(std::find(assumps.begin(), assumps.end(), l) != assumps.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(clause_iterators)
{
    SATFixture f;
    Cube empty;

    for (auto & slv : f.solvers)
    {
        std::vector<Clause> clauses(slv->begin_clauses(), slv->end_clauses());
        BOOST_CHECK_EQUAL(clauses.size(), 0);

        Variable a = slv->newVariable();
        Variable b = slv->newVariable();

        // (-a V -b)
        Clause c0 = {negate(a), negate(b)};
        slv->addClause(c0);

        std::vector<Clause> expected = {c0};
        clauses = std::vector<Clause>(slv->begin_clauses(), slv->end_clauses());
        sortClauseVec(expected);
        sortClauseVec(clauses);

        BOOST_CHECK_EQUAL(clauses.size(), 1);
        BOOST_CHECK(clauses == expected);

        BOOST_CHECK(slv->solve(empty));

        // Calling solve didn't change anything
        clauses = std::vector<Clause>(slv->begin_clauses(), slv->end_clauses());
        sortClauseVec(clauses);

        BOOST_CHECK_EQUAL(clauses.size(), 1);
        BOOST_CHECK(clauses == expected);

        // (-a V -b) & (a V b)
        Clause c1 = {a, b};
        slv->addClause(c1);

        expected = {c0, c1};
        clauses = std::vector<Clause>(slv->begin_clauses(), slv->end_clauses());
        sortClauseVec(expected);
        sortClauseVec(clauses);

        BOOST_CHECK_EQUAL(clauses.size(), 2);
        BOOST_CHECK(clauses == expected);

        BOOST_CHECK(slv->solve(empty));
        BOOST_CHECK(slv->solve({a}));
        BOOST_CHECK(slv->solve({b}));
        BOOST_CHECK(slv->solve({negate(a)}));
        BOOST_CHECK(slv->solve({negate(b)}));
        BOOST_CHECK(!slv->solve({a, b}));
        BOOST_CHECK(!slv->solve({negate(a), negate(b)}));

        clauses = std::vector<Clause>(slv->begin_clauses(), slv->end_clauses());
        sortClauseVec(clauses);
        BOOST_CHECK_EQUAL(clauses.size(), 2);
        BOOST_CHECK(clauses == expected);
    }
}

BOOST_AUTO_TEST_CASE(simplifying_trivial)
{
    MinisatSimplifyingSolver slv;

    Variable a = slv.newVariable();
    Variable b = slv.newVariable();
    Variable c = slv.newVariable();

    slv.addClause({a, c});
    slv.addClause({negate(a), b, negate(c)});

    slv.freeze(a);
    slv.freeze(b);

    slv.eliminate();

    // Everything should be eliminated
    std::vector<Clause> clauses(slv.begin_clauses(), slv.end_clauses());
    BOOST_CHECK_EQUAL(clauses.size(), 0);

    BOOST_CHECK(slv.solve({a, b}));
    BOOST_CHECK_EQUAL(slv.getAssignment(a), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::TRUE);

    BOOST_CHECK(slv.solve({negate(a), negate(b)}));
    BOOST_CHECK_EQUAL(slv.getAssignment(a), SAT::FALSE);
    BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::FALSE);
}

BOOST_AUTO_TEST_CASE(simplifying_simple)
{
    MinisatSimplifyingSolver slv;

    Variable a = slv.newVariable();
    Variable b = slv.newVariable();
    Variable c = slv.newVariable();

    slv.addClause({a, b});
    slv.addClause({negate(a), c});
    slv.addClause({negate(b), negate(c)});

    slv.freeze(b);
    slv.freeze(c);

    slv.eliminate();

    std::vector<Clause> clauses(slv.begin_clauses(), slv.end_clauses());
    BOOST_CHECK_EQUAL(clauses.size(), 2);
    std::vector<Clause> expected;
    expected.push_back({b, c});
    expected.push_back({negate(b), negate(c)});

    sortClauseVec(clauses);
    sortClauseVec(expected);

    BOOST_CHECK(clauses == expected);

    BOOST_CHECK(slv.solve({b, negate(c)}));
    BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::FALSE);

    BOOST_CHECK(slv.solve({b}));
    BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::TRUE);
    BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::FALSE);

    BOOST_CHECK(slv.solve({c}));
    BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::FALSE);
    BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::TRUE);

    BOOST_CHECK(!slv.solve({negate(b), negate(c)}));
}

BOOST_AUTO_TEST_CASE(simplifying_trail)
{
    MinisatSimplifyingSolver slv;

    Variable a = slv.newVariable();
    Variable b = slv.newVariable();
    Variable c = slv.newVariable();
    Variable d = slv.newVariable();

    slv.freeze(a);
    slv.freeze(d);

    slv.addClause({a});
    slv.addClause({a, b});
    slv.addClause({negate(b), c});
    slv.addClause({negate(a), d});

    slv.eliminate();

    // This check mostly checks that the test does something - we
    // don't really want to test how the SAT solver works internally
    std::vector<Literal> trail(slv.begin_trail(), slv.end_trail());
    BOOST_CHECK(!trail.empty());

    BOOST_CHECK(slv.solve({d}));
    BOOST_CHECK(!slv.solve({negate(d)}));

    // Adding the literals on the trail should not change satisfiability
    for (Literal lit : trail)
    {
        Clause cls { lit };
        slv.addClause(cls);
    }

    BOOST_CHECK(slv.solve({d}));
    BOOST_CHECK(!slv.solve({negate(d)}));
}

BOOST_AUTO_TEST_CASE(trail)
{
    SATFixture f;
    Cube empty;

    for (auto & slv : f.solvers)
    {
        Variable a = slv->newVariable();
        Variable b = slv->newVariable();
        Variable c = slv->newVariable();
        Variable d = slv->newVariable();

        slv->addClause({a});
        slv->addClause({a, b});
        slv->addClause({negate(b), c});
        slv->addClause({negate(a), d});

        BOOST_CHECK(slv->solve(empty));

        // We can't check if the trail is empty here - for solvers like Glucose
        // that don't implement an interface to access the trail SAT::Solver
        // should leave it empty
        std::vector<Literal> trail(slv->begin_trail(), slv->end_trail());

        BOOST_CHECK(slv->solve({d}));
        BOOST_CHECK(!slv->solve({negate(d)}));

        // Adding the literals on the trail should not change satisfiability
        for (Literal lit : trail)
        {
            Clause cls { lit };
            slv->addClause(cls);
        }

        BOOST_CHECK(slv->solve({d}));
        BOOST_CHECK(!slv->solve({negate(d)}));
    }
}

