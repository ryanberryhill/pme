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
#include "pme/engine/sat_adaptor.h"

#define BOOST_TEST_MODULE SATAdaptorTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

std::vector<SATBackend> backends = { MINISAT, GLUCOSE, MINISATSIMP };

BOOST_AUTO_TEST_CASE(constants)
{
    for (SATBackend backend : backends)
    {
        SATAdaptor slv(backend);
        for (unsigned i = 0; i <= 1; ++i)
        {
            BOOST_CHECK(!slv.isSAT());

            BOOST_CHECK(slv.solve());
            BOOST_CHECK(slv.isSAT());

            BOOST_CHECK_EQUAL(slv.getAssignment(ID_TRUE), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignment(ID_FALSE), SAT::FALSE);

            slv.addClause({MIN_ID});
            BOOST_CHECK(slv.solve());
            BOOST_CHECK(slv.isSAT());

            BOOST_CHECK_EQUAL(slv.getAssignment(ID_TRUE), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignment(ID_FALSE), SAT::FALSE);

            if (i == 0) { slv.reset(); }
        }
    }
}

BOOST_AUTO_TEST_CASE(one_var)
{
    for (SATBackend backend : backends)
    {
        SATAdaptor slv(backend);

        ID a = MIN_ID;

        for (unsigned i = 0; i <= 1; ++i)
        {
            // (a)
            slv.addClause({a});
            BOOST_CHECK(slv.solve());
            BOOST_CHECK(slv.isSAT());

            // (a) & (-a)[assumption]
            BOOST_CHECK(!slv.solve({negate(a)}));
            BOOST_CHECK(!slv.isSAT());

            // (a) again
            BOOST_CHECK(slv.solve());
            BOOST_CHECK(slv.isSAT());

            // (a) & (-a)
            slv.addClause({negate(a)});
            BOOST_CHECK(!slv.solve());
            BOOST_CHECK(!slv.isSAT());

            if (i == 0) { slv.reset(); }
        }
    }
}

BOOST_AUTO_TEST_CASE(three_var)
{
    for (SATBackend backend : backends)
    {
        SATAdaptor slv(backend);

        ID a = MIN_ID, b = a + ID_INCR, c = b + ID_INCR;

        for (unsigned i = 0; i <= 1; ++i)
        {
            // (a) & (b V c) & (-b V -c)
            slv.addClause({a});
            slv.addClause({b, c});
            slv.addClause({negate(b), negate(c)});

            BOOST_CHECK(slv.solve());

            BOOST_CHECK(slv.solve({b}));
            BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignmentToVar(b), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignmentToVar(c), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignment(negate(b)), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignment(negate(c)), SAT::TRUE);

            BOOST_CHECK(slv.solve({negate(b)}));
            BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignmentToVar(b), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignmentToVar(c), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignment(negate(b)), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignment(negate(c)), SAT::FALSE);

            if (i == 0) { slv.reset(); }
        }
    }
}

BOOST_AUTO_TEST_CASE(critical_assumptions)
{
    for (SATBackend backend : backends)
    {
        SATAdaptor slv(backend);

        ID a = MIN_ID, b = a + ID_INCR, c = b + ID_INCR, d = c + ID_INCR;

        for (unsigned i = 0; i <= 1; ++i)
        {
            slv.addClause({a, b, c, d});
            slv.addClause({negate(a), negate(b), negate(c), negate(d)});
            slv.addClause({negate(c), d});

            Cube assumps = { a, b, c, d };
            Cube crits;
            BOOST_REQUIRE(!slv.solve(assumps, &crits));

            // Criticals are sufficient to prove UNSAT
            BOOST_CHECK(!crits.empty());
            BOOST_CHECK(!slv.solve(crits));

            // Each critical should be from the assumptions
            for (ID l : crits)
            {
                BOOST_CHECK(std::find(assumps.begin(), assumps.end(), l) != assumps.end());
            }

            if (i == 0) { slv.reset(); }
        }
    }
}

BOOST_AUTO_TEST_CASE(simplify)
{
    for (SATBackend backend : backends)
    {
        SATAdaptor slv(backend);
        ID a = MIN_ID, b = a + ID_INCR, c = b + ID_INCR, d = c + ID_INCR;

        for (unsigned i = 0; i <= 1; ++i)
        {
            slv.addClause({a});
            slv.addClause({a, b});
            slv.addClause({negate(b), c});
            slv.addClause({negate(a), d});

            BOOST_CHECK(slv.solve({d}));
            BOOST_CHECK(!slv.solve({negate(d)}));

            slv.freeze(a);
            slv.freeze(d);
            ClauseVec simplified = slv.simplify();
            BOOST_CHECK(!simplified.empty());

            // Same satisfiability after simplify
            BOOST_CHECK(slv.solve({d}));
            BOOST_CHECK(!slv.solve({negate(d)}));

            // And after taking the simplified clauses to a different solver
            // Note that for non-simplifying solvers, the clauses after
            // simplification are the same as those before
            SATAdaptor slv2(GLUCOSE);
            for (const Clause & cls : simplified)
            {
                slv2.addClause(cls);
            }

            BOOST_CHECK(slv2.solve({d}));
            BOOST_CHECK(!slv2.solve({negate(d)}));

            if (i == 0) { slv.reset(); }
        }
    }
}

BOOST_AUTO_TEST_CASE(groups)
{
    for (SATBackend backend : backends)
    {
        SATAdaptor slv(backend);
        ID a = MIN_ID, b = a + ID_INCR;

        for (unsigned i = 0; i <= 1; ++i)
        {
            slv.addClause({a});

            BOOST_CHECK(slv.solve({a}));
            BOOST_CHECK(!slv.solve({negate(a)}));

            GroupID gida = slv.createGroup();
            slv.addGroupClause(gida, {negate(a)});

            BOOST_CHECK(!slv.groupSolve(gida));
            BOOST_CHECK(slv.solve({a}));
            BOOST_CHECK(!slv.solve({negate(a)}));

            slv.addClause({a, b});
            GroupID gidb = slv.createGroup();
            slv.addGroupClause(gidb, {negate(b)});

            BOOST_CHECK(!slv.groupSolve(gida));
            BOOST_CHECK(slv.groupSolve(gidb));
            BOOST_CHECK(!slv.groupSolve(gidb, {negate(a), negate(b)}));
            BOOST_CHECK(slv.solve());

            if (i == 0) { slv.reset(); }
        }
    }
}

BOOST_AUTO_TEST_CASE(clause_dedup)
{
    for (SATBackend backend : backends)
    {
        ClauseDeduplicatingSATAdaptor slv(backend);

        ID a = MIN_ID, b = a + ID_INCR, c = b + ID_INCR;

        for (unsigned i = 0; i <= 1; ++i)
        {
            // (a) & (b V c) & (-b V -c)
            slv.addClause({a});
            slv.addClause({a});
            slv.addClause({a});
            slv.addClause({b, c});
            slv.addClause({c, b});
            slv.addClause({a});
            slv.addClause({negate(b), negate(c)});
            slv.addClause({b, c});
            slv.addClause({c, b});
            slv.addClause({negate(c), negate(b)});
            slv.addClause({negate(b), negate(c)});

            BOOST_CHECK(slv.solve());

            BOOST_CHECK(slv.solve({b}));
            BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignmentToVar(b), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignmentToVar(c), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignment(negate(b)), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignment(negate(c)), SAT::TRUE);

            BOOST_CHECK(slv.solve({negate(b)}));
            BOOST_CHECK_EQUAL(slv.getAssignment(b), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignment(c), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignmentToVar(b), SAT::FALSE);
            BOOST_CHECK_EQUAL(slv.getAssignmentToVar(c), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignment(negate(b)), SAT::TRUE);
            BOOST_CHECK_EQUAL(slv.getAssignment(negate(c)), SAT::FALSE);

            if (i == 0) { slv.reset(); }
        }
    }
}

