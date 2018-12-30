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
#include "pme/ivc/bvc_solver.h"
#include "pme/util/hitting_set_finder.h"
#include "pme/ic3/ic3_solver.h"
#include "pme/util/ivc_checker.h"
#include "pme/util/check_cex.h"

#define BOOST_TEST_MODULE BVCSolverTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

bool isCEX(VariableManager & vars,
           const TransitionRelation & tr,
           const SafetyCounterExample & cex)
{
    return checkCounterExample(vars, tr, cex);
}

bool isIVC(VariableManager & vars,
           const TransitionRelation & tr,
           const std::vector<ID> & abstraction)
{
    IVCChecker checker(vars, tr);
    return checker.checkSafe(abstraction);
}

// Primitive implementation of CBVC for testing the block function
bool runBVC(VariableManager & vars, const TransitionRelation & tr, BVCSolver & solver)
{
    typedef std::pair<unsigned, Cube> Obligation;

    bool sat;
    BVCSolution soln;
    BVCPredecessor pred;

    HittingSetFinder hs(vars);

    std::set<BVCSolution> lemmas;
    std::vector<Obligation> obls;
    std::vector<ID> abstraction;
    Cube bad = {tr.bad()};
    unsigned level = 0;
    unsigned last_abs_size = 0;

    // Impose a limit on levels in case there's a bug that causes an infinite loop
    // (at the end we require isIVC is true, so the bug will be detected)
    // Also limit iterations
    while (!isIVC(vars, tr, abstraction) && level < 256)
    {
#define MAX_ITERS 1024
        unsigned iters = 0;
        obls.push_back(std::make_pair(level, bad));
        while (!obls.empty() && iters < MAX_ITERS)
        {
            iters++;
            unsigned level;
            Cube obl;

            std::tie(level, obl) = obls.back(); obls.pop_back();

            std::tie(sat, soln, pred) = solver.block(obl, level);

            if (sat && !pred.empty())
            {
                // Obligation at level below (or counter-example)
                if (level == 0) { return false; }
                else
                {
                    obls.push_back(std::make_pair(level, obl));
                    obls.push_back(std::make_pair(level - 1, pred));
                }
            }
            else if (sat && !soln.empty())
            {
                // New "lemma" (i.e., correction set).
                // Add, block, update abstraction, re-add obligation
                std::sort(soln.begin(), soln.end());
                BOOST_CHECK(lemmas.find(soln) == lemmas.end());
                lemmas.insert(soln);

                solver.blockSolution(soln);

                hs.addSet(soln);
                abstraction = hs.solve();
                BOOST_REQUIRE(!abstraction.empty());
                BOOST_REQUIRE(abstraction.size() >= last_abs_size);
                last_abs_size = abstraction.size();

                solver.setAbstraction(abstraction);

                obls.push_back(std::make_pair(level, obl));
            }
            else
            {
                // Should be done this level
                BOOST_CHECK(!sat);
            }
        }

        BOOST_REQUIRE(iters < MAX_ITERS);

        level++;
    }

    BOOST_REQUIRE(isIVC(vars, tr, abstraction));

    return true;
}

struct BVCSolverSafeFixture
{
    aiger * aig;
    ExternalID i0, l0, l1, l2, l3, a0, a1, a2, o0;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<BVCSolver> solver;

    BVCSolverSafeFixture()
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

        aiger_add_input(aig, i0, "i0");

        // l0' = i0
        aiger_add_latch(aig, l0, i0, "l0");

        // l1' = l3 & l0 = a0
        aiger_add_latch(aig, l1, a0, "l1");

        // l2' = l3 & l1 = a1
        aiger_add_latch(aig, l2, a1, "l2");

        // l3' = l1 & l2 = a2
        aiger_add_latch(aig, l3, a2, "l3");

        aiger_add_and(aig, a0, l0, l3);
        aiger_add_and(aig, a1, l1, l3);
        aiger_add_and(aig, a2, l1, l2);

        // o0 = l3
        aiger_add_output(aig, l3, "o0");
        o0 = l3;

        tr.reset(new TransitionRelation(vars, aig));

        setInit(tr->toInternal(l0), ID_FALSE);
        setInit(tr->toInternal(l1), ID_FALSE);
        setInit(tr->toInternal(l2), ID_FALSE);
        setInit(tr->toInternal(l3), ID_FALSE);

        solver.reset(new BVCSolver(vars, *tr));
    }

    void setInit(ID latch, ID val)
    {
        tr->setInit(latch, val);
    }

    ~BVCSolverSafeFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

struct BVCSolverUnsafeFixture
{
    aiger * aig;
    ExternalID i0, o0;
    std::vector<ExternalID> latches;
    std::vector<ExternalID> ands;
    VariableManager vars;
    std::unique_ptr<TransitionRelation> tr;
    std::unique_ptr<BVCSolver> solver;

    BVCSolverUnsafeFixture(unsigned n)
    {
        assert(n > 1);
        aig = aiger_init();

        i0 = 2;
        aiger_add_input(aig, i0, NULL);

        latches.reserve(n);
        ands.reserve(n - 1);

        // l0' = i0
        latches.push_back(4);
        aiger_add_latch(aig, latches[0], i0, NULL);

        // li' = l_{i-1}
        // i = 1 to n - 1
        for (unsigned i = 1; i < n; ++i)
        {
            unsigned li = 4 + i * 2;
            unsigned li_prev = li - 2;
            latches.push_back(li);
            aiger_add_latch(aig, li, li_prev, NULL);
        }

        // ai = li & a_{i+1}, i = 0 to n - 2
        for (unsigned i = 0; i < (n - 1); ++i)
        {
            // one input, n latches, i ands preceeding this one
            unsigned ai = 2 + 2 * n + 2 * i + 2;
            ands.push_back(ai);
            aiger_add_and(aig, ai, latches.at(i), ai + 2);
        }

        // Last and a_{n-1} = l_{n-2} & l_{n-1}
        // 1 input, n latches, n - 1 ands
        unsigned a_last = 2 + 2 * n + 2 * (n - 1) + 2;
        ands.push_back(a_last);
        aiger_add_and(aig, a_last, latches.at(n - 1), latches.at(n - 2));

        // bad = o0 = a0
        o0 = ands.at(0);
        aiger_add_output(aig, o0, NULL);

        tr.reset(new TransitionRelation(vars, aig));
        solver.reset(new BVCSolver(vars, *tr));
    }

    ~BVCSolverUnsafeFixture()
    {
        aiger_reset(aig);
        aig = nullptr;
    }
};

BOOST_AUTO_TEST_CASE(bvc_single_block_safe)
{
    BVCSolverSafeFixture f;

    ID a2 = f.tr->toInternal(f.a2);

    bool sat;
    BVCSolution soln;
    BVCPredecessor pred;

    std::tie(sat, soln, pred) = f.solver->block(0);
    BOOST_REQUIRE(!sat);

    std::tie(sat, soln, pred) = f.solver->block(1);

    BOOST_REQUIRE(sat);
    BOOST_CHECK(pred.empty());

    BVCSolution expected = {a2};
    BOOST_CHECK(soln == expected);
}

BOOST_AUTO_TEST_CASE(bvc_single_block_unsafe)
{
    BVCSolverUnsafeFixture f(8);

    ID a0 = f.tr->toInternal(f.ands.at(0));

    bool sat;
    BVCSolution soln;
    BVCPredecessor pred;

    std::tie(sat, soln, pred) = f.solver->block(0);

    BOOST_REQUIRE(sat);
    BOOST_CHECK(pred.empty());

    BVCSolution expected = {a0};
    BOOST_CHECK(soln == expected);
}

BOOST_AUTO_TEST_CASE(block_safe)
{
    BVCSolverSafeFixture f;
    bool safe = runBVC(f.vars, *f.tr, *f.solver);
    BOOST_CHECK(safe);
}

BOOST_AUTO_TEST_CASE(block_unsafe_small)
{
    BVCSolverUnsafeFixture f(3);
    bool safe = runBVC(f.vars, *f.tr, *f.solver);
    BOOST_CHECK(!safe);
}

BOOST_AUTO_TEST_CASE(block_unsafe_big)
{
    BVCSolverUnsafeFixture f(16);
    bool safe = runBVC(f.vars, *f.tr, *f.solver);
    BOOST_CHECK(!safe);
}

BOOST_AUTO_TEST_CASE(rec_block_safe)
{
    BVCSolverSafeFixture f;

    Cube bad = {f.tr->bad()};
    unsigned level = 0;
    std::vector<ID> abstraction;

    while (!isIVC(f.vars, *f.tr, abstraction) && level < 256)
    {
        BVCRecBlockResult result = f.solver->recursiveBlock(bad, level);
        abstraction = f.solver->getAbstraction();
        level++;

        if (!result.safe) { break; }
    }

    BOOST_CHECK(isIVC(f.vars, *f.tr, abstraction));
}

BOOST_AUTO_TEST_CASE(rec_block_unsafe_small)
{
    unsigned n = 3;
    BVCSolverUnsafeFixture f(n);

    Cube bad = {f.tr->bad()};
    unsigned level = 0;
    std::vector<ID> abstraction;

    BVCRecBlockResult result;

    while (!isIVC(f.vars, *f.tr, abstraction) && level < 256)
    {
        result = f.solver->recursiveBlock(bad, level);
        abstraction = f.solver->getAbstraction();

        if (!result.safe) { break; }

        level++;
    }

    BOOST_CHECK(!result.safe);
    BOOST_CHECK(result.cex_obl != nullptr);
    BOOST_CHECK(level <= n);
}

BOOST_AUTO_TEST_CASE(rec_block_unsafe_big)
{
    unsigned n = 16;
    BVCSolverUnsafeFixture f(n);

    Cube bad = {f.tr->bad()};
    unsigned level = 0;
    std::vector<ID> abstraction;

    BVCRecBlockResult result;

    while (!isIVC(f.vars, *f.tr, abstraction) && level < 256)
    {
        result = f.solver->recursiveBlock(bad, level);
        abstraction = f.solver->getAbstraction();

        if (!result.safe) { break; }

        level++;
    }

    BOOST_CHECK(!result.safe);
    BOOST_CHECK(result.cex_obl != nullptr);
    BOOST_CHECK(level <= n);
}

BOOST_AUTO_TEST_CASE(prove_safe)
{
    BVCSolverSafeFixture f;
    BVCResult result = f.solver->prove();
    BOOST_CHECK(result.safe());
    // TODO: check IVC and proof
}

BOOST_AUTO_TEST_CASE(prove_unsafe_small)
{
    BVCSolverUnsafeFixture f(3);
    BVCResult result = f.solver->prove();
    BOOST_CHECK(result.unsafe());

    BOOST_CHECK(isCEX(f.vars, *f.tr, result.cex()));
}

BOOST_AUTO_TEST_CASE(prove_unsafe_large)
{
    BVCSolverUnsafeFixture f(16);
    BVCResult result = f.solver->prove();
    BOOST_CHECK(result.unsafe());

    BOOST_CHECK(isCEX(f.vars, *f.tr, result.cex()));
}


