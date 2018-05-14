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

#include "pme/util/proof_checker.h"
#include "pme/minimization/minimization.h"
#include "pme/minimization/marco.h"

extern "C" {
#include "aiger/aiger.h"
}

#define BOOST_TEST_MODULE ProofMinimizerTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <set>

using namespace PME;

void sortClauseVec(ClauseVec & vec)
{
    for (Clause & cls : vec)
    {
        std::sort(cls.begin(), cls.end());
    }

    std::sort(vec.begin(), vec.end());
}

struct MinimizationFixture
{
    aiger * aig;
    ExternalID i0, l0, l1, l2, l3, o0, a0, a1, a2, a3;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    ClauseVec proof;

    MinimizationFixture()
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

        aiger_add_input(aig, i0, "i0");

        // l0' = i0
        aiger_add_latch(aig, l0, i0, "l0");

        // l1' = l0
        aiger_add_latch(aig, l1, l0, "l1");

        // a0 = l1 & ~l0
        aiger_add_and(aig, a0, l1, aiger_not(l0));

        // l2' = a0 = l1 & ~l0
        aiger_add_latch(aig, l2, a0, "l2");

        // l3' = l2
        aiger_add_latch(aig, l3, l2, "l3");

        // The 1111 state is unreachable because this circuit is like a broken
        // shift register where l2 = 1 only happens when ~l0.

        aiger_add_and(aig, a1, l0, l1);
        aiger_add_and(aig, a2, l2, l3);
        aiger_add_and(aig, a3, a1, a2);

        // o0 = a3 = &(l0, l1, l2, l3)
        aiger_add_output(aig, a3, "o0");
        o0 = a3;

        tr.reset(new TransitionRelation(vars, aig));

        ExternalClauseVec eproof;

        // From manual calculation, this is the minimal proof
        eproof.push_back({aiger_not(l3), aiger_not(l2)});
        eproof.push_back({l3, aiger_not(l2), aiger_not(l1)});

        // A redundant clause found by resolving the above two on l3
        eproof.push_back({aiger_not(l2), aiger_not(l1)});

        proof = tr->makeInternal(eproof);

        proof.push_back({negate(tr->bad())});
        sortClauseVec(proof);
    }

    ~MinimizationFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

void test_minimize(ProofMinimizer & minimizer, MinimizationFixture & f, bool minimal)
{
    minimizer.minimize();
    ClauseVec minproof(minimizer.begin_minproof(), minimizer.end_minproof());
    sortClauseVec(minproof);

    std::set<Clause> orig_clauses(f.proof.begin(), f.proof.end());

    // For minimizers that return the smallest proof, also check that the
    // proof is minimized.
    // I believe the minimum has two clauses + the property here.
    if (minimal)
    {
        BOOST_CHECK(minproof.size() == 3);
    }
    else
    {
        BOOST_CHECK(minproof.size() <= f.proof.size());
    }

    // For now we'll check that the returned clauses are a subset of the
    // original ones (unless it's the property, which the minimizer may add).
    // In principle we could minimize in other ways such that this won't be the
    // case anymore.
    Clause property = {negate(f.tr->bad())};
    for (const Clause & cls : minproof)
    {
        if (cls != property)
        {
            BOOST_CHECK(orig_clauses.count(cls) >= 1);
        }
    }

    ProofChecker pc(*f.tr, minproof);
    BOOST_CHECK(pc.checkProof());

    // Could (probably should) also check that for each clause removed the
    // resulting set is not a proof if minimal is true.
}

void test_findmin(ProofMinimizer & minimizer, MinimizationFixture & f)
{
    test_minimize(minimizer, f, true);
}

void test_shrink(ProofMinimizer & minimizer, MinimizationFixture & f)
{
    test_minimize(minimizer, f, false);
}

BOOST_AUTO_TEST_CASE(should_add_negbad)
{
    MinimizationFixture f;

    // Delete ~Bad from the proof
    bool found = false;
    Clause property = {negate(f.tr->bad())};
    for (unsigned i = 0; i < f.proof.size(); ++i)
    {
        if (f.proof[i] == property)
        {
            found = true;
            f.proof.erase(f.proof.begin() + i);
            break;
        }
    }
    BOOST_REQUIRE(found);

    // Everything should still work
    MARCOMinimizer minimizer(f.vars, *f.tr, f.proof);
    test_findmin(minimizer, f);
}

BOOST_AUTO_TEST_CASE(dummy_minimizer)
{
    // This test will be the only one to check that the proof in the fixture is
    // actually a proof
    MinimizationFixture f;
    ProofChecker pc(*f.tr, f.proof);
    BOOST_REQUIRE(pc.checkProof());

    DummyMinimizer dm(f.vars, *f.tr, f.proof);
    test_shrink(dm, f);
}

BOOST_AUTO_TEST_CASE(marco_minimizer)
{
    MinimizationFixture f;
    MARCOMinimizer minimizer(f.vars, *f.tr, f.proof);
    test_findmin(minimizer, f);
}

