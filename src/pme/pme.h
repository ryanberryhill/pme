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

#ifndef PME_H_INCLUDED
#define PME_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "aiger/aiger.h"

const char * cpme_version();
void * cpme_init(aiger * aig, void * proof);
int cpme_free(void * pme);

void * cpme_alloc_proof();
int cpme_add_clause(void * proof, const unsigned * cls, size_t n);
int cpme_free_proof(void * proof);

int cpme_check_proof(void * pme);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus

#include <string>
#include <vector>
#include <cstdint>

namespace PME
{
    typedef uint64_t ID;
    typedef uint64_t ExternalID;

    typedef std::vector<unsigned> ExternalClause;
    typedef std::vector<ExternalClause> ExternalClauseVec;

    typedef std::vector<ID> Clause;
    typedef std::vector<ID> Cube;
    typedef std::vector<Clause> ClauseVec;

    const std::string& pme_version();
}

#endif // __cplusplus


#endif

