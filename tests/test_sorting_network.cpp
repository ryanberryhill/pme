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

#include "pme/util/sorting_network.h"
#include "pme/engine/sat_adaptor.h"
#include "pme/engine/variable_manager.h"

#define BOOST_TEST_MODULE SortingNetworkTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

std::vector<ID> createVars(VariableManager & vars, size_t n)
{
    std::vector<ID> vec;
    for (size_t i = 0; i < n; ++i)
    {
        vec.push_back(vars.getNewID());
    }
    return vec;
}

Cube firstNTrue(const Cube & lits, size_t n)
{
    Cube result;
    result.reserve(lits.size());

    assert(lits.size() >= n);
    for (size_t i = 0; i < lits.size(); ++i)
    {
        ID orig = lits.at(i);
        ID lit = i < n ? orig : negate(orig);
        result.push_back(lit);
    }

    return result;
}

BOOST_AUTO_TEST_CASE(half_comparator)
{
    VariableManager vars;
    SATAdaptor sat;

    ID x_1 = vars.getNewID();
    ID x_2 = vars.getNewID();
    ID y_1 = vars.getNewID();
    ID y_2 = vars.getNewID();

    ClauseVec half_comp = compHalf(x_1, x_2, y_1, y_2);
    sat.addClauses(half_comp);

    BOOST_REQUIRE(sat.solve());

    BOOST_CHECK(sat.solve({x_1, x_2, y_1, y_2}));
    BOOST_CHECK(sat.solve({x_1, y_1}));
    BOOST_CHECK(sat.solve({x_2, y_1}));

    BOOST_CHECK(!sat.solve({x_1, negate(y_1)}));
    BOOST_CHECK(!sat.solve({x_2, negate(y_1)}));
    BOOST_CHECK(!sat.solve({x_1, x_2, negate(y_1)}));
    BOOST_CHECK(!sat.solve({x_1, x_2, negate(y_2)}));

    BOOST_REQUIRE(sat.solve({x_1, negate(x_2)}));
    BOOST_CHECK(sat.getAssignment(y_1) == SAT::TRUE);

    BOOST_REQUIRE(sat.solve({negate(x_1), x_2}));
    BOOST_CHECK(sat.getAssignment(y_1) == SAT::TRUE);

    BOOST_REQUIRE(sat.solve({x_1, x_2}));
    BOOST_CHECK(sat.getAssignment(y_1) == SAT::TRUE);
    BOOST_CHECK(sat.getAssignment(y_2) == SAT::TRUE);
}

BOOST_AUTO_TEST_CASE(full_comparator)
{
    VariableManager vars;
    SATAdaptor sat;

    ID x_1 = vars.getNewID();
    ID x_2 = vars.getNewID();
    ID y_1 = vars.getNewID();
    ID y_2 = vars.getNewID();

    ClauseVec full_comp = compFull(x_1, x_2, y_1, y_2);
    sat.addClauses(full_comp);

    BOOST_REQUIRE(sat.solve());

    BOOST_CHECK(sat.solve({x_1, x_2, y_1, y_2}));
    BOOST_CHECK(sat.solve({x_1, y_1}));
    BOOST_CHECK(sat.solve({x_2, y_1}));

    BOOST_CHECK(!sat.solve({x_1, negate(y_1)}));
    BOOST_CHECK(!sat.solve({x_2, negate(y_1)}));
    BOOST_CHECK(!sat.solve({x_1, x_2, negate(y_1)}));
    BOOST_CHECK(!sat.solve({x_1, x_2, negate(y_2)}));

    BOOST_CHECK(!sat.solve({negate(x_1), negate(x_2), y_1}));
    BOOST_CHECK(!sat.solve({negate(x_1), negate(x_2), y_2}));
    BOOST_CHECK(!sat.solve({negate(x_1), y_2}));
    BOOST_CHECK(!sat.solve({negate(x_2), y_2}));

    BOOST_CHECK(sat.solve({negate(x_1), negate(x_2), negate(y_1), negate(y_2)}));
    BOOST_CHECK(sat.solve({negate(x_1), x_2, y_1, negate(y_2)}));
    BOOST_CHECK(sat.solve({x_1, negate(x_2), y_1, negate(y_2)}));

    BOOST_REQUIRE(sat.solve({x_1, negate(x_2)}));
    BOOST_CHECK(sat.getAssignment(y_1) == SAT::TRUE);
    BOOST_CHECK(sat.getAssignment(y_2) == SAT::FALSE);

    BOOST_REQUIRE(sat.solve({negate(x_1), x_2}));
    BOOST_CHECK(sat.getAssignment(y_1) == SAT::TRUE);
    BOOST_CHECK(sat.getAssignment(y_2) == SAT::FALSE);

    BOOST_REQUIRE(sat.solve({negate(x_1), negate(x_2)}));
    BOOST_CHECK(sat.getAssignment(y_1) == SAT::FALSE);
    BOOST_CHECK(sat.getAssignment(y_2) == SAT::FALSE);

    BOOST_REQUIRE(sat.solve({x_1, x_2}));
    BOOST_CHECK(sat.getAssignment(y_1) == SAT::TRUE);
    BOOST_CHECK(sat.getAssignment(y_2) == SAT::TRUE);
}

void testMergeNetworks(bool le, bool ge)
{
    for (size_t a = 0; a < 6; ++a)
    {
        for (size_t b = 0; b < 6; ++b)
        {
            if (a == 0 && b == 0) { continue; }

            // Construct an a X b merge network
            VariableManager vars;
            SATAdaptor sat;

            std::vector<ID> inputs_a = createVars(vars, a);
            std::vector<ID> inputs_b = createVars(vars, b);

            std::vector<ID> outputs;
            ClauseVec cnf;
            std::tie(outputs, cnf) = mergeNetwork(vars, inputs_a, inputs_b, le, ge);

            BOOST_REQUIRE(outputs.size() == a + b);

            sat.addClauses(cnf);

            for (size_t a_true = 0; a_true <= a; ++a_true)
            {
                Cube assumps_a = firstNTrue(inputs_a, a_true);
                for (size_t b_true = 0; b_true <= b; ++b_true)
                {
                    // See that it outputs a_true + b_true sorted ones
                    // when it has the corresponding input
                    Cube assumps = firstNTrue(inputs_b, b_true);
                    assumps.insert(assumps.end(), assumps_a.begin(), assumps_a.end());

                    BOOST_REQUIRE(sat.solve(assumps));

                    // Check the first a_true + b_true are true, rest are false
                    for (size_t i = 0; i < a + b; ++i)
                    {
                        ID lit = outputs.at(i);
                        ModelValue asgn = sat.getAssignment(lit);
                        if (i < a_true + b_true)
                        {
                            if (le) { BOOST_CHECK(asgn == SAT::TRUE); }
                        }
                        else
                        {
                            if (ge) { BOOST_CHECK(asgn == SAT::FALSE); }
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(merge_networks)
{
    // Full merge
    testMergeNetworks(true, true);

    // <= only merge
    testMergeNetworks(true, false);

    // >= only merge
    testMergeNetworks(false, true);
}

void testSortingNetworkWithPattern(bool le, bool ge, std::vector<bool> pattern)
{
    VariableManager vars;
    SATAdaptor sat;

    size_t n = pattern.size();
    std::vector<ID> inputs = createVars(vars, n);
    std::vector<ID> assumps = inputs;

    size_t n_true = 0;

    for (size_t i = 0; i < n; ++i)
    {
        if (pattern.at(i))
        {
            n_true++;
        }
        else
        {
            assumps[i] = negate(assumps.at(i));
        }
    }

    std::vector<ID> outputs;
    ClauseVec cnf;

    std::tie(outputs, cnf) = sortingNetwork(vars, inputs, le, ge);

    BOOST_REQUIRE(outputs.size() == n);

    sat.addClauses(cnf);
    BOOST_REQUIRE(sat.solve(assumps));

    for (size_t i = 0; i < n; ++i)
    {
        ID lit = outputs.at(i);
        ModelValue asgn = sat.getAssignment(lit);

        if (i < n_true)
        {
            if (le) { BOOST_CHECK_EQUAL(asgn, SAT::TRUE); }
        }
        else
        {
            if (ge) { BOOST_CHECK_EQUAL(asgn, SAT::FALSE); }
        }
    }
}

void testSortingNetworks(bool le, bool ge)
{
    // For n up to 6, try every possible pattern
    for (size_t n = 1; n <= 6; ++n)
    {
        // For each possible number up to 2^n, use its binary representation
        // as a bitmask
        for (unsigned mask = 0; mask < (1u << n); ++mask)
        {
            // All 1s pattern
            std::vector<bool> pattern(n, true);

            // Flip the bits with 1s in mask
            for (unsigned i = 0; i < n; ++i)
            {
                unsigned pos = 1 << i;
                if ((mask & pos) == 0)
                {
                    pattern[i] = !pattern[i];
                }
            }

            testSortingNetworkWithPattern(le, ge, pattern);
        }
    }
}

BOOST_AUTO_TEST_CASE(sorting_networks)
{
    // Full sorting
    testSortingNetworks(true, true);

    // <= sorting
    testSortingNetworks(true, false);

    // >= sorting
    testSortingNetworks(false, true);
}

void testSimplifiedMergeNetworks(bool le, bool ge)
{
    for (size_t a = 0; a < 6; ++a)
    {
        for (size_t b = 0; b < 6; ++b)
        {
            if (a == 0 && b == 0) { continue; }

            for (size_t c = 1; c <= a + b + 1; ++c)
            {
                // Construct an a X b c-simplified merge network
                VariableManager vars;
                SATAdaptor sat;

                std::vector<ID> inputs_a = createVars(vars, a);
                std::vector<ID> inputs_b = createVars(vars, b);

                std::vector<ID> outputs;
                ClauseVec cnf;
                std::tie(outputs, cnf) = simpMergeNetwork(vars, inputs_a, inputs_b, c, le, ge);

                size_t expected_size = std::min(a + b, c);
                BOOST_REQUIRE(outputs.size() == expected_size);

                sat.addClauses(cnf);

                for (size_t a_true = 0; a_true <= a; ++a_true)
                {
                    Cube assumps_a = firstNTrue(inputs_a, a_true);
                    for (size_t b_true = 0; b_true <= b; ++b_true)
                    {
                        // See that it outputs a_true + b_true sorted ones
                        // when it has the corresponding input
                        Cube assumps = firstNTrue(inputs_b, b_true);
                        assumps.insert(assumps.end(), assumps_a.begin(), assumps_a.end());

                        BOOST_REQUIRE(sat.solve(assumps));

                        // Check the first a_true + b_true (up to c) are true, rest are false
                        for (size_t i = 0; i < expected_size; ++i)
                        {
                            ID lit = outputs.at(i);
                            ModelValue asgn = sat.getAssignment(lit);
                            if (i < a_true + b_true)
                            {
                                if (le) { BOOST_CHECK_EQUAL(asgn, SAT::TRUE); }
                            }
                            else
                            {
                                if (ge) { BOOST_CHECK_EQUAL(asgn, SAT::FALSE); }
                            }
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(simplified_merge_networks)
{
    // Full merge
    testSimplifiedMergeNetworks(true, true);

    // <= only merge
    testSimplifiedMergeNetworks(true, false);

    // >= only merge
    testSimplifiedMergeNetworks(false, true);
}

void testCardinalityNetworkWithPattern(bool le, bool ge, unsigned m, std::vector<bool> pattern)
{
    VariableManager vars;
    SATAdaptor sat;

    size_t n = pattern.size();
    std::vector<ID> inputs = createVars(vars, n);
    std::vector<ID> assumps = inputs;

    size_t n_true = 0;

    for (size_t i = 0; i < n; ++i)
    {
        if (pattern.at(i))
        {
            n_true++;
        }
        else
        {
            assumps[i] = negate(assumps.at(i));
        }
    }

    std::vector<ID> outputs;
    ClauseVec cnf;

    std::tie(outputs, cnf) = cardinalityNetwork(vars, inputs, m, le, ge);

    size_t expected_size = std::min(n, (size_t) m);
    BOOST_REQUIRE(outputs.size() == expected_size);

    sat.addClauses(cnf);
    BOOST_REQUIRE(sat.solve(assumps));

    for (size_t i = 0; i < expected_size; ++i)
    {
        ID lit = outputs.at(i);
        ModelValue asgn = sat.getAssignment(lit);

        if (i < n_true)
        {
            if (le) { BOOST_CHECK_EQUAL(asgn, SAT::TRUE); }
        }
        else
        {
            if (ge) { BOOST_CHECK_EQUAL(asgn, SAT::FALSE); }
        }
    }
}

void testCardinalityNetworks(bool le, bool ge)
{
    // For n up to 6, try every possible pattern
    for (size_t n = 1; n <= 6; ++n)
    {
        for (size_t m = 1; m <= n + 1; ++m)
        {
            // For each possible number up to 2^n, use its binary representation
            // as a bitmask
            for (unsigned mask = 0; mask < (1u << n); ++mask)
            {
                // All 1s pattern
                std::vector<bool> pattern(n, true);

                // Flip the bits with 1s in mask
                for (unsigned i = 0; i < n; ++i)
                {
                    unsigned pos = 1 << i;
                    if ((mask & pos) == 0)
                    {
                        pattern[i] = !pattern[i];
                    }
                }

                testCardinalityNetworkWithPattern(le, ge, m, pattern);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(cardinality_networks)
{
    // Full sorting
    testCardinalityNetworks(true, true);

    // <= sorting
    testCardinalityNetworks(true, false);

    // >= sorting
    testCardinalityNetworks(false, true);
}
