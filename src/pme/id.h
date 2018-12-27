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

#ifndef ID_H_INCLUDED
#define ID_H_INCLUDED

#include "pme/pme.h"

namespace PME
{
    extern const ID ID_NULL;
    extern const ID ID_TRUE;
    extern const ID ID_FALSE;
    extern const ID MAX_ID;
    extern const ID MIN_ID;
    extern const size_t ID_INCR;

    ID unprime(ID id);
    ID prime(ID id, size_t n = 1);
    size_t nprimes(ID id);
    bool is_negated(ID id);
    bool is_valid_id(ID id);
    ID negate(ID id);
    std::vector<ID> negateVec(const std::vector<ID> & vec);
    ID strip(ID id);

    Clause primeClause(const Clause & cls, size_t n = 1);
    Clause unprimeClause(const Clause & cls);
    ClauseVec primeClauses(const ClauseVec & vec, size_t n = 1);

    std::vector<ID> primeVec(const std::vector<ID> & vec, size_t n = 1);
    std::vector<ID> unprimeVec(const std::vector<ID> & vec);
}

#endif

