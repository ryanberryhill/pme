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

