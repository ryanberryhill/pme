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
#include "pme/ic3/inductive_trace.h"

#define BOOST_TEST_MODULE InductiveTraceTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <algorithm>

using namespace PME;
using namespace PME::IC3;

struct IDFixture {
    ID a, b, c, d;

    IDFixture()
    {
        a = MIN_ID;
        b = a + ID_INCR;
        c = b + ID_INCR;
        d = c + ID_INCR;
    }
};

BOOST_AUTO_TEST_CASE(get_cubes)
{
    IDFixture id;
    InductiveTrace trace;

    Cube c = {id.a, negate(id.b), id.c, negate(id.d)};
    LemmaID c1 = trace.addLemma(c, 1);

    BOOST_CHECK_EQUAL(trace.IDOf(c), c1);
    std::sort(c.begin(), c.end());
    BOOST_CHECK_EQUAL(trace.IDOf(c), c1);
    std::reverse(c.begin(), c.end());
    BOOST_CHECK_EQUAL(trace.IDOf(c), c1);
}

BOOST_AUTO_TEST_CASE(insert_lemmas)
{
    IDFixture id;
    InductiveTrace trace;

    LemmaID c1 = trace.addLemma({id.a}, 0);
    LemmaID c2 = trace.addLemma({id.b}, 0);
    LemmaID c3 = trace.addLemma({id.c}, 0);
    LemmaID c4 = trace.addLemma({id.d}, 0);
    LemmaID c5 = trace.addLemma({id.b, id.c}, 1);
    LemmaID c6 = trace.addLemma({id.a, id.b, id.c, id.d}, LEVEL_INF);

    Frame expected_f0 = {c1, c2, c3, c4};
    Frame expected_f1 = {c5};
    Frame expected_finf = {c6};

    Frame actual_f0 = trace.getFrame(0);
    Frame actual_f1 = trace.getFrame(1);
    Frame actual_finf = trace.getFrame(LEVEL_INF);

    BOOST_CHECK(expected_f0 == actual_f0);
    BOOST_CHECK(expected_f1 == actual_f1);
    BOOST_CHECK(expected_finf == actual_finf);
}

BOOST_AUTO_TEST_CASE(push_lemmas)
{
    IDFixture id;
    InductiveTrace trace;

    trace.addLemma({id.a}, 0);
    trace.addLemma({id.b}, 0);
    trace.addLemma({id.c}, 0);
    trace.addLemma({id.d}, 0);
    LemmaID c5 = trace.addLemma({id.b, id.c}, 1);
    LemmaID c6 = trace.addLemma({id.a, id.c}, 2);
    LemmaID c7 = trace.addLemma({id.a, id.b}, 1);

    trace.pushLemma(c5, 2);
    Frame expected = {c5, c6};
    Frame actual = trace.getFrame(2);
    BOOST_CHECK(actual == expected);

    expected = {c7};
    actual = trace.getFrame(1);
    BOOST_CHECK(actual == expected);

    trace.pushLemma(c7, LEVEL_INF);

    BOOST_CHECK(trace.getFrame(1).empty());
    expected = {c7};
    actual = trace.getFrame(LEVEL_INF);
    BOOST_CHECK(actual == expected);

    BOOST_CHECK_EQUAL(trace.numFrames(), 3);
    trace.clearUnusedFrames();
    BOOST_CHECK_EQUAL(trace.numFrames(), 3);

    trace.pushLemma(c5, LEVEL_INF);
    trace.pushLemma(c6, LEVEL_INF);
    trace.clearUnusedFrames();
    BOOST_CHECK_EQUAL(trace.numFrames(), 1);
}

BOOST_AUTO_TEST_CASE(duplicate_lemmas)
{
    IDFixture id;
    InductiveTrace trace;

    Cube cube = {id.a, id.b, id.c, id.d};

    trace.addLemma({id.a}, 0);
    trace.addLemma({id.b}, 0);
    trace.addLemma({id.c}, 0);
    trace.addLemma({id.d}, 0);
    LemmaID c5 = trace.addLemma({id.b, id.c}, 1);
    LemmaID c6 = trace.addLemma({id.a, id.c}, 2);
    LemmaID c7 = trace.addLemma(cube, 1);

    Frame expected_f1 = {c5, c7};
    Frame actual_f1 = trace.getFrame(1);
    BOOST_CHECK(expected_f1 == actual_f1);

    std::reverse(cube.begin(), cube.end());
    trace.addLemma(cube, 2);

    expected_f1 = {c5};
    actual_f1 = trace.getFrame(1);
    BOOST_CHECK(expected_f1 == actual_f1);

    Frame expected_f2 = {c6, c7};
    Frame actual_f2 = trace.getFrame(2);
    BOOST_CHECK(expected_f2 == actual_f2);

    trace.addLemma(cube, 3);
    Frame expected = {c6};
    BOOST_CHECK(trace.getFrame(2) == expected);
    expected = {c7};
    BOOST_CHECK(trace.getFrame(3) == expected);

    trace.addLemma(cube, LEVEL_INF);
    BOOST_CHECK(trace.getFrame(3).empty());
    expected = {c7};
    BOOST_CHECK(trace.getFrame(LEVEL_INF) == expected);
}

