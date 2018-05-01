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

#include "pme/engine/transition_relation.h"
#include "pme/util/proof_checker.h"

extern "C" {
#include "aiger/aiger.h"
}

#define BOOST_TEST_MODULE SATAdaptorTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct InductionFixture
{
    aiger * aig;
    std::unique_ptr<TransitionRelation> tr;
    VariableManager vars;

    ExternalID i0, l0, l1, l2, o0, a0, a1;

    InductionFixture()
    {
        i0 = 2;
        l0 = 4;
        l1 = 6;
        l2 = 8;
        a0 = 10;
        a1 = 12;
        o0 = 14;
        aig = aiger_init();
        aiger_add_input(aig, i0, "i0");

        // l0' = i0
        aiger_add_latch(aig, l0, i0, "l0");

        // a0 = l0 & l1
        aiger_add_and(aig, a0, l0, l1);

        // l1' = a0 = l0 & l1
        aiger_add_latch(aig, l1, a0, "l1");

        // a1 = ~l0 & l1
        aiger_add_and(aig, a1, aiger_not(l0), l1);

        // l2 = a1 = ~l0 & l1
        aiger_add_latch(aig, l2, a1, "l2");

        // bad = l2
        o0 = l2;
        aiger_add_output(aig, o0, "bad");

        tr.reset(new TransitionRelation(vars, aig));
    }

    ~InductionFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

void check_proof_test(bool simp)
{
    InductionFixture f;
    ClauseVec proof;

    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);

    // Proof = (~l1) & (~l2)
    proof.push_back({negate(l1)});
    proof.push_back({negate(l2)});

    ProofChecker ind0(*f.tr, proof, simp);

    BOOST_CHECK(ind0.checkInduction());
    BOOST_CHECK(ind0.checkInitiation());
    BOOST_CHECK(ind0.checkSafety());
    BOOST_CHECK(ind0.checkInductiveStrengthening());
    BOOST_CHECK(ind0.checkProof());

    // Non-proof due to non-induction
    ClauseVec nonproof;
    nonproof.push_back({negate(l2)});
    ProofChecker ind1(*f.tr, nonproof, simp);
    BOOST_CHECK(!ind1.checkInduction());
    BOOST_CHECK(ind1.checkInitiation());
    BOOST_CHECK(ind1.checkSafety());
    BOOST_CHECK(!ind1.checkInductiveStrengthening());
    BOOST_CHECK(!ind1.checkProof());

    // Non-proof due to non-induction and non-initiation
    nonproof.clear();
    nonproof.push_back({negate(l2)});
    nonproof.push_back({l1});
    ProofChecker ind2(*f.tr, nonproof, simp);
    BOOST_CHECK(!ind2.checkInduction());
    BOOST_CHECK(!ind2.checkInitiation());
    BOOST_CHECK(ind2.checkSafety());
    BOOST_CHECK(!ind2.checkInductiveStrengthening());
    BOOST_CHECK(!ind2.checkProof());

    // Empty proof is inductive and initiated but not safet
    nonproof.clear();
    ProofChecker ind3(*f.tr, nonproof, simp);
    BOOST_CHECK(ind3.checkInduction());
    BOOST_CHECK(ind3.checkInitiation());
    BOOST_CHECK(!ind3.checkSafety());
    BOOST_CHECK(!ind3.checkInductiveStrengthening());
    BOOST_CHECK(!ind3.checkProof());
}

BOOST_AUTO_TEST_CASE(check_proof_nosimplify)
{
    check_proof_test(false);
}

BOOST_AUTO_TEST_CASE(check_proof_simplify)
{
    check_proof_test(true);
}

BOOST_AUTO_TEST_CASE(constraints)
{
}

