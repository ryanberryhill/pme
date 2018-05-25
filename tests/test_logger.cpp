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

#include "pme/engine/logger.h"

#define BOOST_TEST_MODULE LoggerTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <iostream>

using namespace PME;

BOOST_AUTO_TEST_CASE(null_logging)
{
    Logger log;
    LogChannelID channel = LOG_TEST;

    BOOST_CHECK_EQUAL(log.verbosity(channel), 0);

    BOOST_CHECK_NO_THROW(log.log(channel, 1) << "test" << std::endl);
}

BOOST_AUTO_TEST_CASE(logging_to_stream)
{
    std::ostringstream dst;
    Logger log(dst, 1);
    LogChannelID channel = LOG_TEST;

    BOOST_CHECK_EQUAL(log.verbosity(channel), 1);

    BOOST_CHECK_NO_THROW(log.log(channel, 2) << "test2" << std::endl);

    BOOST_CHECK_EQUAL(dst.str(), "");

    BOOST_CHECK_NO_THROW(log.log(channel, 1) << "test1" << std::endl);

    BOOST_CHECK_EQUAL(dst.str(), "TEST: test1\n");
    dst.str("");

    BOOST_CHECK_NO_THROW(log.log(channel, 0) << "test0" << std::endl);

    BOOST_CHECK_EQUAL(dst.str(), "TEST: test0\n");
}

BOOST_AUTO_TEST_CASE(set_stream)
{
    std::ostringstream dst1, dst2;
    Logger log(dst1, 1);

    BOOST_CHECK_NO_THROW(log.log(LOG_TEST, 1) << "test1");
    BOOST_CHECK_EQUAL(dst1.str(), "TEST: test1");
    BOOST_CHECK_EQUAL(dst2.str(), "");

    log.setLogStream(dst2);
    BOOST_CHECK_NO_THROW(log.log(LOG_TEST, 1) << "test2");
    BOOST_CHECK_EQUAL(dst1.str(), "TEST: test1");
    BOOST_CHECK_EQUAL(dst2.str(), "TEST: test2");
}

BOOST_AUTO_TEST_CASE(channel_verbosity)
{
    std::ostringstream dst;
    Logger log(dst, 1);
    LogChannelID channel1 = LOG_PME;
    LogChannelID channel2 = LOG_TEST;

    log.setVerbosity(channel1, 1);
    log.setVerbosity(channel2, 2);

    BOOST_CHECK_NO_THROW(log.log(channel1, 2) << "test12");
    BOOST_CHECK_NO_THROW(log.log(channel2, 2) << "test22");
    BOOST_CHECK_NO_THROW(log.log(channel1, 3) << "test13");
    BOOST_CHECK_NO_THROW(log.log(channel2, 3) << "test23");

    BOOST_CHECK_EQUAL(dst.str(), "TEST: test22");
}

