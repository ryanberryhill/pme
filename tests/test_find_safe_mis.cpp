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
#include "pme/engine/consecution_checker.h"
#include "pme/util/find_safe_mis.h"

#define BOOST_TEST_MODULE FindSafeMISTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct MISFixture
{
    aiger * aig;
    ExternalID l0, l1, l2, l3, o0, a0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<ConsecutionChecker> checker;

    MISFixture()
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

        // a0 = l2 & l3
        aiger_add_and(aig, a0, l2, l3);

        // o0 = a0 = l2 & l3
        aiger_add_output(aig, a0, "o0");
        o0 = a0;

        tr.reset(new TransitionRelation(vars, aig));
        checker.reset(new ConsecutionChecker(vars, *tr));

        // Add the property clause as ID 0
        checker->addClause(0, tr->propertyClause());
    }

    ~MISFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(trivial_mis)
{
    MISFixture f;
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    ID nl0 = negate(l0);
    ID nl1 = negate(l1);
    ID nl2 = negate(l2);
    ID nl3 = negate(l3);

    // Safe inductive formula should be its own MIS
    Clause c0 = {nl0, nl1};
    Clause c1 = {nl1, nl2};
    Clause c2 = {nl2, nl3};
    Clause c3 = {nl3, nl0};

    f.checker->addClause(1, c0);
    f.checker->addClause(2, c1);
    f.checker->addClause(3, c2);
    f.checker->addClause(4, c3);

    ClauseIDVec proof = { 0, 1, 2, 3, 4 };
    ClauseIDVec proof_copy = proof;

    BOOST_CHECK(findSafeMIS(*f.checker, proof_copy, 0));
    BOOST_CHECK(proof_copy == proof);
}

BOOST_AUTO_TEST_CASE(no_mis)
{
    MISFixture f;
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    Clause c0 = { l0, l1, l2 };
    Clause c1 = { l1, l2, l3 };

    f.checker->addClause(1, c0);
    f.checker->addClause(2, c1);

    ClauseIDVec proof = { 0, 1, 2 };
    ClauseIDVec proof_copy = proof;

    BOOST_CHECK(!findSafeMIS(*f.checker, proof_copy, 0));
    BOOST_CHECK(proof_copy == proof);
}

BOOST_AUTO_TEST_CASE(nontrivial_mis)
{
    MISFixture f;
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    ID nl0 = negate(l0);
    ID nl1 = negate(l1);
    ID nl2 = negate(l2);
    ID nl3 = negate(l3);

    // Safe inductive formula
    Clause c0 = {nl0, nl1};
    Clause c1 = {nl0, nl2};
    Clause c2 = {nl0, nl3};
    Clause c3 = {nl1, nl2};
    Clause c4 = {nl1, nl3};
    Clause c5 = {nl2, nl3};

    // Extra clauses
    Clause c6 = {l0, l1};
    Clause c7 = {l1, l2};

    f.checker->addClause(1, c0);
    f.checker->addClause(2, c1);
    f.checker->addClause(3, c2);
    f.checker->addClause(4, c3);
    f.checker->addClause(5, c4);
    f.checker->addClause(6, c5);
    f.checker->addClause(7, c6);
    f.checker->addClause(8, c7);

    ClauseIDVec proof = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    ClauseIDVec proof_copy = proof;

    BOOST_CHECK(findSafeMIS(*f.checker, proof_copy, 0));
    std::sort(proof_copy.begin(), proof_copy.end());
    ClauseIDVec expected = { 0, 1, 2, 3, 4, 5, 6 };
    BOOST_CHECK(proof_copy == expected);

    // Check that the result is indeed safe inductive
    proof = proof_copy;
    BOOST_CHECK(findSafeMIS(*f.checker, proof_copy, 0));
    BOOST_CHECK(proof_copy == proof);

    // Deleting 2 or 5 from the below yields a non-inductive formula
    // that contains a MIS of the same clauses as the trivial_mis case
    // proof = { 0, 1, 2, 3, 4, 5, 6 }
    proof = { 0, 1, 3, 4, 5, 6 };
    proof_copy = proof;
    BOOST_CHECK(findSafeMIS(*f.checker, proof_copy, 0));
    std::sort(proof_copy.begin(), proof_copy.end());
    expected = { 0, 1, 3, 4, 6 };
    BOOST_CHECK(proof_copy == expected);
}

BOOST_AUTO_TEST_CASE(multiple_necessary_clauses)
{
    MISFixture f;
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    ID nl0 = negate(l0);
    ID nl1 = negate(l1);
    ID nl2 = negate(l2);
    ID nl3 = negate(l3);

    // Safe inductive formula
    Clause c0 = {nl0, nl1};
    Clause c1 = {nl0, nl2};
    Clause c2 = {nl0, nl3};
    Clause c3 = {nl1, nl2};
    Clause c4 = {nl1, nl3};
    Clause c5 = {nl2, nl3};
    // Extra clauses
    Clause c6 = {l0, l1};
    Clause c7 = {l1, l2};

    f.checker->addClause(1, c0);
    f.checker->addClause(2, c1);
    f.checker->addClause(3, c2);
    f.checker->addClause(4, c3);
    f.checker->addClause(5, c4);
    f.checker->addClause(6, c5);
    f.checker->addClause(7, c6);
    f.checker->addClause(8, c7);

    ClauseIDVec proof = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    ClauseIDVec proof_copy = proof;
    ClauseIDVec expected;

    BOOST_CHECK(findSafeMIS(*f.checker, proof_copy, {0, 2}));
    std::sort(proof_copy.begin(), proof_copy.end());
    expected = { 0, 1, 2, 3, 4, 5, 6 };
    BOOST_CHECK(proof_copy == expected);

    proof_copy = proof;
    BOOST_CHECK(findSafeMIS(*f.checker, proof_copy, {0, 5}));
    std::sort(proof_copy.begin(), proof_copy.end());
    expected = { 0, 1, 2, 3, 4, 5, 6 };
    BOOST_CHECK(proof_copy == expected);
}

BOOST_AUTO_TEST_CASE(no_checker_interface)
{
    MISFixture f;
    ID l0 = f.tr->toInternal(f.l0);
    ID l1 = f.tr->toInternal(f.l1);
    ID l2 = f.tr->toInternal(f.l2);
    ID l3 = f.tr->toInternal(f.l3);

    ID nl0 = negate(l0);
    ID nl1 = negate(l1);
    ID nl2 = negate(l2);
    ID nl3 = negate(l3);

    SafetyProof safe_non_inductive = {
        // Safe inductive formula
        { negate(f.tr->bad()) },
        {nl0, nl1},
        {nl0, nl2},
        {nl0, nl3},
        {nl1, nl2},
        {nl1, nl3},
        {nl2, nl3},
        // Extra clauses
        {l0, l1},
        {l1, l2}
    };

    BOOST_CHECK(findSafeMIS(f.vars, *f.tr, safe_non_inductive));

    SafetyProof safe_inductive = {
        { negate(f.tr->bad()) },
        {nl0, nl1},
        {nl0, nl2},
        {nl0, nl3},
        {nl1, nl2},
        {nl1, nl3},
        {nl2, nl3}
    };

    BOOST_CHECK(findSafeMIS(f.vars, *f.tr, safe_inductive));

    SafetyProof minimal_safe_inductive = {
        { negate(f.tr->bad()) },
        {nl0, nl1},
        {nl1, nl2},
        {nl2, nl3},
        {nl3, nl0}
    };

    BOOST_CHECK(findSafeMIS(f.vars, *f.tr, minimal_safe_inductive));

    SafetyProof non_proof = {
        { negate(f.tr->bad()) },
        {nl0, nl1},
        {nl2, nl3},
        {nl3, nl0}
    };

    BOOST_CHECK(!findSafeMIS(f.vars, *f.tr, non_proof));
}

