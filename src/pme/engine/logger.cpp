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

#include <cassert>

namespace PME
{
    null_out_stream g_null_ostream;

    Logger::Logger() : Logger(g_null_ostream)
    { }

    Logger::Logger(std::ostream & stream) : Logger(stream, 0)
    { }

    Logger::Logger(std::ostream & stream, int default_verbosity) : m_log(&stream)
    {
        setAllVerbosities(default_verbosity);
    }

    void Logger::setLogStream(std::ostream & stream)
    {
        m_log = &stream;
    }

    void Logger::setVerbosity(LogChannelID channel, int v)
    {
        assert(channel < NUM_LOG_CHANNELS);
        m_verbosity[channel] = v;
    }

    void Logger::setAllVerbosities(int v)
    {
        for (size_t i = 0; i < NUM_LOG_CHANNELS; ++i)
        {
            m_verbosity[i] = v;
        }
    }

    int Logger::verbosity(LogChannelID channel) const
    {
        assert(channel < NUM_LOG_CHANNELS);
        return m_verbosity[channel];
    }

    std::ostream & Logger::log(LogChannelID channel, int v)
    {
        assert(channel < NUM_LOG_CHANNELS);
        if (verbosity(channel) >= v)
        {
            return *m_log;
        }
        else
        {
            return g_null_ostream;
        }
    }
}

