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

#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <iostream>
#include "pme/pme.h"

/*
 * Logging functionality
 *
 * This closely resembles the logging functionality I added to iimc for Truss.
 * Most objects in PME have a reference to a single global Logger object owned
 * by the Engine object. They can log to different channels at different
 * levels of verbosity.
 */

namespace PME
{
    // From StackOverflow (http://stackoverflow.com/a/7818394)
    // This is a stream buffer that will accept any input and do
    // nothing with it. When logging to a channel that is disabled or a
    // verbosity level that is too low, it goes to a null_out_stream,
    // which sends it to a null_out_buf.
    class null_out_buf : public std::streambuf {
      public:
        virtual std::streamsize xsputn (const char *, std::streamsize n) override {
            return n;
        }
        virtual int overflow (int) override {
            return EOF;
        }
    };

    class null_out_stream : public std::ostream {
      public:
        null_out_stream() : std::ostream (&buf) {}
      private:
        null_out_buf buf;
    };

    extern null_out_stream g_null_ostream;

    std::ostream& operator<< (std::ostream& stream, LogChannelID channel);

    class Logger
    {
        public:
            Logger();
            Logger(std::ostream & stream);
            Logger(std::ostream & stream, int default_verbosity);

            std::ostream & log(LogChannelID channel, int v);
            int verbosity(LogChannelID channel) const;
            void setVerbosity(LogChannelID channel, int v);
            void setAllVerbosities(int v);
            void setLogStream(std::ostream & stream);

        private:
            int m_verbosity[NUM_LOG_CHANNELS];
            std::ostream * m_log;
    };
}

#endif

