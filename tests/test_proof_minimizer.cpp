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
#include "pme/minimization/sisi.h"
#include "pme/minimization/brute_force.h"

extern "C" {
#include "aiger/aiger.h"
}

#define BOOST_TEST_MODULE ProofMinimizerTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <set>
#include <limits>

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
    MinimizationFixture() { }
    virtual ~MinimizationFixture() { }
    virtual TransitionRelation & tr() = 0;
    virtual ClauseVec & proof() = 0;
};

struct TrivialProofFixture : public MinimizationFixture
{
    aiger * aig;
    ExternalID i0, l0, l1, l2, l3, o0, a0, a1, a2, a3;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> m_tr;
    ClauseVec m_proof;

    TrivialProofFixture()
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

        m_tr.reset(new TransitionRelation(vars, aig));

        // The minimal proof here is just ~Bad - it's an inductive property
        ExternalClauseVec eproof;

        eproof.push_back({aiger_not(l3), aiger_not(l2)});
        eproof.push_back({l3, aiger_not(l2), aiger_not(l1)});
        // A redundant clause found by resolving the above two on l3
        eproof.push_back({aiger_not(l2), aiger_not(l1)});

        m_proof = m_tr->makeInternal(eproof);

        m_proof.push_back({negate(m_tr->bad())});
        sortClauseVec(m_proof);
    }

    ~TrivialProofFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    TransitionRelation & tr() override { return *m_tr; }
    ClauseVec & proof() override { return m_proof; }
};

struct NonTrivialProofFixture : public MinimizationFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, o0, a0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> m_tr;
    ClauseVec m_proof;

    NonTrivialProofFixture()
    {
        aig = aiger_init();

        l0 = 2;
        l1 = 4;
        l2 = 6;
        l3 = 8;
        a0 = 10;

        // l0' = l3
        aiger_add_latch(aig, l0, l3, "l0");

        // l1' = l0
        aiger_add_latch(aig, l1, l0, "l1");

        // l2' = l1
        aiger_add_latch(aig, l2, l1, "l2");

        // l3' = l2
        aiger_add_latch(aig, l3, l2, "l3");

        // a0 = l3 & l2
        aiger_add_and(aig, a0, l3, l2);

        // o0 = a0 = l3 & l2
        aiger_add_output(aig, a0, "o0");
        o0 = a0;

        // Initial state 0001
        aiger_add_reset(aig, l0, 1);

        m_tr.reset(new TransitionRelation(vars, aig));

        ExternalClauseVec eproof;

        // Build the proof
        ExternalID nl0 = aiger_not(l0);
        ExternalID nl1 = aiger_not(l1);
        ExternalID nl2 = aiger_not(l2);
        ExternalID nl3 = aiger_not(l3);

        // The property itself (~l2 V ~l3) is not inductive.
        // The negation of pairs of "adjacent" registers is sufficient as a
        // proof, that is (~l0 V ~l1)(~l1 V ~l2)(~l2 V ~l3)(~l3 V ~l0).
        // We can also add extra pairs (or triples, etc.) to get redundant
        // clauses. For instance, adding (~l1 V ~l3)(~l0 V ~l2) gives a
        // stronger proof.

        // Minimal proof (note that nl2, nl3 shadows the property so it should
        // not appear in the minimal proof)
        eproof.push_back({nl0, nl1});
        eproof.push_back({nl1, nl2});
        eproof.push_back({nl2, nl3});
        eproof.push_back({nl3, nl0});

        // Stronger proof
        eproof.push_back({nl0, nl2});
        eproof.push_back({nl1, nl3});

        // Even stronger (the above requires at most one register is 1,
        // with the below we require also at least one is 1).
        eproof.push_back({l0, l1, l2, l3});

        // Subsumed clause
        eproof.push_back({nl1, nl2, nl3});

        m_proof = m_tr->makeInternal(eproof);

        m_proof.push_back({negate(m_tr->bad())});
        sortClauseVec(m_proof);
    }

    ~NonTrivialProofFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }

    TransitionRelation & tr() override { return *m_tr; }
    ClauseVec & proof() override { return m_proof; }
};

void test_minimize(ProofMinimizer & minimizer, MinimizationFixture & f, bool minimal, bool minimum)
{
    minimizer.minimize();

    BOOST_REQUIRE(minimizer.numProofs() > 0);
    size_t smallest_size = std::numeric_limits<size_t>::max();

    for (size_t i = 0; i < minimizer.numProofs(); ++i)
    {
        ClauseVec minproof = minimizer.getProof(i);
        sortClauseVec(minproof);
        smallest_size = std::min(smallest_size, minproof.size());

        std::set<Clause> orig_clauses(f.proof().begin(), f.proof().end());

        // For now we'll check that the returned clauses are a subset of the
        // original ones (unless it's the property, which the minimizer may add).
        // In principle we could minimize in other ways such that this won't be the
        // case anymore.
        Clause property = {negate(f.tr().bad())};
        for (const Clause & cls : minproof)
        {
            if (cls != property)
            {
                BOOST_CHECK(orig_clauses.count(cls) >= 1);
            }
        }

        // Check that the result is a proof
        ProofChecker pc(f.tr(), minproof);
        BOOST_CHECK(pc.checkProof());

        // if minimal is true, check that the result is a minimal proof
        if (minimal && minproof.size() > 1)
        {
            for (Clause cls : minproof)
            {
                if (cls == property) { continue; }
                ClauseVec test_proof;
                std::remove_copy(minproof.begin(), minproof.end(),
                                 std::back_inserter(test_proof), cls);
                ProofChecker pc_fail(f.tr(), test_proof);
                BOOST_CHECK(!pc_fail.checkProof());
            }
        }
    }

    if (minimum)
    {
        BOOST_REQUIRE(minimizer.minimumProofKnown());
        BOOST_CHECK_EQUAL(minimizer.getMinimumProof().size(), smallest_size);
    }
}

void test_findall(ProofMinimizer & minimizer, MinimizationFixture & f)
{
    test_minimize(minimizer, f, true, true);
}

void test_findminimal(ProofMinimizer & minimizer, MinimizationFixture & f)
{
    test_minimize(minimizer, f, true, false);
}

void test_shrink(ProofMinimizer & minimizer, MinimizationFixture & f)
{
    test_minimize(minimizer, f, false, false);
}

BOOST_AUTO_TEST_CASE(should_add_negbad)
{
    TrivialProofFixture f;

    // Delete ~Bad from the proof
    bool found = false;
    Clause property = {negate(f.tr().bad())};
    for (unsigned i = 0; i < f.proof().size(); ++i)
    {
        if (f.proof()[i] == property)
        {
            found = true;
            f.proof().erase(f.proof().begin() + i);
            break;
        }
    }
    BOOST_REQUIRE(found);

    // Everything should still work
    DummyMinimizer minimizer(f.vars, f.tr(), f.proof());
    test_shrink(minimizer, f);
}

BOOST_AUTO_TEST_CASE(dummy_minimizer_trivial)
{
    // This test will be the only one to check that the proof in the fixture is
    // actually a proof
    TrivialProofFixture f;
    ProofChecker pc(f.tr(), f.proof());
    BOOST_REQUIRE(pc.checkProof());

    DummyMinimizer dm(f.vars, f.tr(), f.proof());
    test_shrink(dm, f);
}

BOOST_AUTO_TEST_CASE(dummy_minimizer_nontrivial)
{
    // This test will be the only one to check that the proof in the fixture is
    // actually a proof
    NonTrivialProofFixture f;
    ProofChecker pc(f.tr(), f.proof());
    BOOST_REQUIRE(pc.checkProof());

    DummyMinimizer dm(f.vars, f.tr(), f.proof());
    test_shrink(dm, f);
}

BOOST_AUTO_TEST_CASE(marco_minimizer_trivial)
{
    TrivialProofFixture f;
    MARCOMinimizer minimizer(f.vars, f.tr(), f.proof());
    test_findall(minimizer, f);
}

BOOST_AUTO_TEST_CASE(marco_minimizer_nontrivial)
{
    NonTrivialProofFixture f;
    MARCOMinimizer minimizer(f.vars, f.tr(), f.proof());
    test_findall(minimizer, f);
}

BOOST_AUTO_TEST_CASE(sisi_minimizer_trivial)
{
    TrivialProofFixture f;
    SISIMinimizer minimizer(f.vars, f.tr(), f.proof());
    test_findminimal(minimizer, f);
}

BOOST_AUTO_TEST_CASE(sisi_minimizer_nontrivial)
{
    NonTrivialProofFixture f;
    SISIMinimizer minimizer(f.vars, f.tr(), f.proof());
    test_findminimal(minimizer, f);
}

BOOST_AUTO_TEST_CASE(brute_force_minimizer_trivial)
{
    TrivialProofFixture f;
    BruteForceMinimizer minimizer(f.vars, f.tr(), f.proof());
    test_findminimal(minimizer, f);
}

BOOST_AUTO_TEST_CASE(brute_force_minimizer_nontrivial)
{
    NonTrivialProofFixture f;
    BruteForceMinimizer minimizer(f.vars, f.tr(), f.proof());
    test_findminimal(minimizer, f);
}

