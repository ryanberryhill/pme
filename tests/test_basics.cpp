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
#include "pme/pme.h"

#define BOOST_TEST_MODULE BasicsTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

BOOST_AUTO_TEST_CASE(subsumption)
{
    Cube a = { 2, 4, 6, 8 };
    Cube b = { 2, 4, 6, 8 };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(subsumes(b, a));

    a = { 4, 6, 8 };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 2, 4, 6 };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 2, 4, 8 };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 10, 12, 14, 16 };
    BOOST_CHECK(!subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 2, 4, 6, 900 };
    BOOST_CHECK(!subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 4, 6 };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 4 };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 2 };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 8 };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { };
    BOOST_CHECK(subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 2, 6, 8, 10 };
    BOOST_CHECK(!subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));

    a = { 3, 4, 6, 8 };
    BOOST_CHECK(!subsumes(a, b));
    BOOST_CHECK(!subsumes(b, a));
}

BOOST_AUTO_TEST_CASE(true_prime_false_prime)
{
    BOOST_CHECK(prime(ID_TRUE) == ID_TRUE);
    BOOST_CHECK(prime(ID_FALSE) == ID_FALSE);
}

BOOST_AUTO_TEST_CASE(negate_vecs)
{
    Cube c1 = { 2, 4, 6 };
    Cube neg_c1 = negateVec(c1);

    BOOST_REQUIRE(neg_c1.size() == c1.size());

    for (size_t i = 0; i < c1.size(); ++i)
    {
        BOOST_CHECK_EQUAL(c1.at(i), negate(neg_c1.at(i)));
    }
}

BOOST_AUTO_TEST_CASE(prime_vecs)
{
    Cube c1 = { 2, 4, 6 };
    Cube c1p = primeVec(c1);
    Cube c1p3 = primeVec(c1, 3);

    BOOST_REQUIRE(c1p.size() == c1.size());
    BOOST_REQUIRE(c1p3.size() == c1.size());

    for (size_t i = 0; i < c1.size(); ++i)
    {
        BOOST_CHECK_EQUAL(c1p.at(i), prime(c1.at(i), 1));
        BOOST_CHECK_EQUAL(c1p3.at(i), prime(c1.at(i), 3));
    }
}

BOOST_AUTO_TEST_CASE(unprime_vecs)
{
    Cube c1 = { 2, 4, 6 };
    Cube c1p = primeVec(c1);
    Cube c1p3 = primeVec(c1, 3);
    Cube c1pu = unprimeVec(c1p);
    Cube c1p3u = unprimeVec(c1p3);

    BOOST_REQUIRE(c1p.size() == c1.size());
    BOOST_REQUIRE(c1p3.size() == c1.size());

    for (size_t i = 0; i < c1.size(); ++i)
    {
        BOOST_CHECK_EQUAL(c1p.at(i), prime(c1.at(i), 1));
        BOOST_CHECK_EQUAL(c1p3.at(i), prime(c1.at(i), 3));
        BOOST_CHECK_EQUAL(c1p3u.at(i), c1.at(i));
        BOOST_CHECK_EQUAL(c1pu.at(i), c1.at(i));
    }
}

