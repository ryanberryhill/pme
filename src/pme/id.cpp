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

#include "pme/pme.h"
#include "pme/id.h"

#include <limits>
#include <cassert>

namespace PME
{
    const ID ID_NULL = 0;
    const ID ID_FALSE = 2;
    const ID ID_TRUE = negate(ID_FALSE);

    static const size_t PRIME_BITS = 16;
    static const size_t PRIME_SHIFT = sizeof(ID) * 8 - PRIME_BITS;
    static const size_t MAX_PRIMES = (1 << PRIME_BITS) - 1;
    const ID MAX_ID = (std::numeric_limits<ID>::max() >> PRIME_BITS);
    const size_t ID_INCR = 2;
    const ID MIN_ID = ID_FALSE + ID_INCR;
    static const ID UNPRIME_MASK = MAX_ID;
    static const ID PRIMES_MASK = ~UNPRIME_MASK;

    ID unprime(ID id)
    {
        return id & UNPRIME_MASK;
    }

    ID prime(ID id, size_t n)
    {
        assert(n <= MAX_PRIMES);

        // Don't prime true or false
        if (id < MIN_ID) { return id; }

        return id | (n << PRIME_SHIFT);
    }

    size_t nprimes(ID id)
    {
        return (id & PRIMES_MASK) >> PRIME_SHIFT;
    }

    bool is_negated(ID id)
    {
        return (id & 1) != 0;
    }

    bool is_valid_id(ID id)
    {
        return id >= MIN_ID || id == ID_TRUE || id == ID_FALSE;
    }

    ID negate(ID id)
    {
        return id ^ 1;
    }

    ID strip(ID id)
    {
        return is_negated(id) ? negate(id) : id;
    }

    std::vector<ID> negateVec(const std::vector<ID> & vec)
    {
        std::vector<ID> negated;
        negated.reserve(vec.size());
        for (ID id : vec)
        {
            negated.push_back(negate(id));
        }
        return negated;
    }

    Clause primeClause(const Clause & cls, size_t n)
    {
        Clause pcls;
        pcls.reserve(cls.size());
        for (ID lit : cls)
        {
            pcls.push_back(prime(lit, n));
        }
        return pcls;
    }

    Clause unprimeClause(const Clause & cls)
    {
        Clause ucls;
        ucls.reserve(cls.size());
        for (ID lit : cls)
        {
            ucls.push_back(unprime(lit));
        }
        return ucls;
    }

    ClauseVec primeClauses(const ClauseVec & vec, size_t n)
    {
        ClauseVec pvec;
        pvec.reserve(vec.size());
        for (const Clause & cls : vec)
        {
            Clause pcls = primeClause(cls, n);
            pvec.push_back(pcls);
        }
        return pvec;
    }

    std::vector<ID> primeVec(const std::vector<ID> & vec, size_t n)
    {
        return primeClause(vec, n);
    }

    std::vector<ID> unprimeVec(const std::vector<ID> & vec)
    {
        return unprimeClause(vec);
    }
}
