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

#ifndef DEBUGGER_H_INCLUDED
#define DEBUGGER_H_INCLUDED

#include "pme/pme.h"

namespace PME {

    class Debugger
    {
        public:
            typedef std::pair<bool, std::vector<ID>> Result;

            virtual ~Debugger() { }

            virtual void setCardinality(unsigned n) = 0;
            virtual void clearCardinality() = 0;

            virtual Result debug() = 0;
            virtual Result debugOverGates(const std::vector<ID> & gates) = 0;
            virtual Result debugAndBlockOverGates(const std::vector<ID> & gates);
            virtual Result debugAndBlock();

            virtual void blockSolution(const std::vector<ID> & soln) = 0;
    };

}

#endif

