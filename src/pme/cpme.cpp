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

#include "config.h"
#include "pme/pme.h"
#include "pme/engine/engine.h"

const char * cpme_version()
{
    return PME::pme_version().c_str();
}

void * cpme_init(aiger * aig, void * proof)
{
    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    if (!p) { return NULL; }

    try 
    {
        auto engine = new PME::Engine(aig, *p);
        return engine;
    }
    catch(...)
    {
        return NULL;
    }
}

void * cpme_alloc_proof()
{
    try
    {
        return new PME::ExternalClauseVec;
    }
    catch (...)
    {
        return NULL;
    }
}

int cpme_add_clause(void * proof, const unsigned * cls, size_t n)
{
    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    if (!p) { return 1; }

    try 
    {
        PME::ExternalClause real_cls;
        for (size_t i = 0; i < n; ++i)
        {
            unsigned aig_id = cls[i];
            real_cls.push_back(aig_id);
        }
        p->push_back(real_cls);
    }
    catch (...)
    {
        return 1;
    }
    return 0;
}

int cpme_free_proof(void * proof)
{
    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    if (!p) { return 1; }

    try 
    {
        delete p;
    }
    catch(...)
    {
        return 1;
    }

    return 0;
}

int cpme_free(void * pme)
{
    try 
    {
        PME::Engine * eng = static_cast<PME::Engine *>(pme);
        if (!eng) { return 1; }
        delete eng;
        return 0;
    }
    catch(...)
    {
        return 1;
    }
}

int cpme_check_proof(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        bool result = eng->checkProof();
        return result ? 1 : 0;
    }
    catch(...)
    {
        return -1;
    }
}

