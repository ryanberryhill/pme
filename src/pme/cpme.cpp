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

#include <cassert>
#include <iostream>
#include <algorithm>

const char * cpme_version()
{
    return PME::pme_version().c_str();
}

void * cpme_init(aiger * aig, void * proof)
{
    PME::ExternalClauseVec * p = nullptr;

    if (proof)
    {
        p = static_cast<PME::ExternalClauseVec *>(proof);
        if (!p) { return NULL; }
    }

    try
    {
        auto engine = new PME::Engine(aig);
        if (p) { engine->setProof(*p); }
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

void * cpme_copy_proof(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    assert(eng);

    PME::ExternalClauseVec * copy = NULL;
    try
    {
        copy = new PME::ExternalClauseVec;
    }
    catch (...)
    {
        return NULL;
    }

    *copy = eng->getOriginalProofExternal();

    return copy;
}

void * cpme_copy_cex(void * pme)
{
    using namespace PME;
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    assert(eng);

    ExternalCounterExample * copy = NULL;
    try
    {
        copy = new ExternalCounterExample;
    }
    catch (...)
    {
        return NULL;
    }

    *copy = eng->getExternalCounterExample();

    return copy;
}

size_t cpme_cex_num_steps(void * cex)
{
    using namespace PME;
    ExternalCounterExample * c = static_cast<ExternalCounterExample *>(cex);
    assert(c);

    return c->size();
}

size_t cpme_cex_step_input_size(void * cex, size_t i)
{
    using namespace PME;
    ExternalCounterExample * c = static_cast<ExternalCounterExample *>(cex);
    assert(c);

    size_t n = 0;
    try
    {
        n = c->at(i).inputs.size();
    }
    catch (...)
    {
        assert(false);
    }

    return n;
}

size_t cpme_cex_step_state_size(void * cex, size_t i)
{
    using namespace PME;
    ExternalCounterExample * c = static_cast<ExternalCounterExample *>(cex);
    assert(c);

    size_t n = 0;
    try
    {
        n = c->at(i).state.size();
    }
    catch (...)
    {
        assert(false);
    }

    return n;
}

// Result returned in dst is guaranteed to be sorted
void cpme_cex_get_inputs(void * cex, size_t i, unsigned * dst)
{
    using namespace PME;
    ExternalCounterExample * c = static_cast<ExternalCounterExample *>(cex);
    assert(c);

    const ExternalCube & inputs = c->at(i).inputs;
    assert(std::is_sorted(inputs.begin(), inputs.end()));

    for (size_t j = 0; j < inputs.size(); ++j)
    {
        dst[j] = inputs.at(j);
    }
}

// Result returned in dst is guaranteed to be sorted
void cpme_cex_get_state(void * cex, size_t i, unsigned * dst)
{
    using namespace PME;
    ExternalCounterExample * c = static_cast<ExternalCounterExample *>(cex);
    assert(c);

    const ExternalCube & state = c->at(i).state;
    assert(std::is_sorted(state.begin(), state.end()));

    for (size_t j = 0; j < state.size(); ++j)
    {
        dst[j] = state.at(j);
    }
}

void cpme_free_cex(void * cex)
{
    using namespace PME;
    ExternalCounterExample * c = static_cast<ExternalCounterExample *>(cex);
    assert(c);

    try
    {
        delete c;
    }
    catch (...)
    {
        assert(false);
    }
}

void cpme_log_to_stdout(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    assert(eng);
    eng->setLogStream(std::cout);
}

void cpme_set_verbosity(void * pme, int v)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    assert(eng);
    eng->setVerbosity(v);
}

void cpme_set_channel_verbosity(void * pme, LogChannelID channel, int v)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    assert(eng);
    assert(channel < NUM_LOG_CHANNELS);
    eng->setChannelVerbosity(channel, v);
}

void cpme_add_clause(void * proof, const unsigned * cls, size_t n)
{
    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    assert(p);

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
        assert(false);
    }
}

void cpme_free_proof(void * proof)
{
    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    assert(p);

    try
    {
        delete p;
    }
    catch (...)
    {
        assert(false);
    }
}

void cpme_free(void * pme)
{
    try
    {
        PME::Engine * eng = static_cast<PME::Engine *>(pme);
        assert(eng);
        delete eng;
    }
    catch(...)
    {
        assert(false);
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

int cpme_run_marco(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->minimize(PME_MINIMIZATION_MARCO);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_camsis(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->minimize(PME_MINIMIZATION_CAMSIS);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_sisi(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->minimize(PME_MINIMIZATION_SISI);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_bfmin(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->minimize(PME_MINIMIZATION_BRUTEFORCE);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_simplemin(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->minimize(PME_MINIMIZATION_SIMPLE);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_caivc(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->findIVCs(PME_IVC_CAIVC);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_cbvc(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->findIVCs(PME_IVC_CBVC);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_marcoivc(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->findIVCs(PME_IVC_MARCO);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_ivcbf(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->findIVCs(PME_IVC_BF);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_ivcucbf(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        eng->findIVCs(PME_IVC_UCBF);
        return 0;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_ic3(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        bool safe = eng->runIC3();
        return safe;
    }
    catch(...)
    {
        return -1;
    }
}

int cpme_run_bmc(void * pme, unsigned k_max)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return -1; }

    try
    {
        bool safe = eng->runBMC(k_max);
        return safe;
    }
    catch(...)
    {
        return -1;
    }
}

size_t cpme_num_proofs(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return 0; }

    return eng->getNumProofs();
}

void * cpme_get_proof(void * pme, size_t i)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return NULL; }

    assert(i < eng->getNumProofs());
    PME::ExternalClauseVec * proof = new PME::ExternalClauseVec;
    *proof = eng->getProofExternal(i);

    return proof;
}

void cpme_set_proof(void * pme, void * proof)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    assert(eng);

    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    assert(p);

    eng->setProof(*p);
}

size_t cpme_num_ivcs(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return 0; }

    return eng->getNumIVCs();
}

void * cpme_get_ivc(void * pme, size_t i)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return NULL; }

    assert(i < eng->getNumIVCs());
    PME::ExternalIVC * proof = new PME::ExternalIVC;
    *proof = eng->getIVCExternal(i);

    return proof;
}

size_t cpme_ivc_num_gates(void * ivc)
{
    PME::ExternalIVC * p = static_cast<PME::ExternalIVC *>(ivc);
    assert(p);

    return p->size();
}

unsigned cpme_ivc_get_gate(void * ivc, size_t i)
{
    PME::ExternalIVC * p = static_cast<PME::ExternalIVC *>(ivc);
    assert(p);

    return p->at(i);
}

void cpme_free_ivc(void * ivc)
{
    PME::ExternalIVC * p = static_cast<PME::ExternalIVC *>(ivc);
    assert(p);

    try
    {
        delete p;
    }
    catch (...)
    {
        assert(false);
    }
}

size_t cpme_bvc_bound(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return 0; }

    return eng->getBVCBound();
}

size_t cpme_num_bvcs(void * pme, size_t bound)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return 0; }

    return eng->getNumBVCs(bound);
}

void * cpme_get_bvc(void * pme, size_t bound, size_t i)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    if (!eng) { return NULL; }

    assert(bound < eng->getBVCBound());
    assert(i < eng->getNumBVCs(bound));
    PME::ExternalIVC * bvc = new PME::ExternalIVC;
    *bvc = eng->getBVCExternal(bound, i);

    return bvc;
}

size_t cpme_bvc_num_gates(void * bvc)
{
    return cpme_ivc_num_gates(bvc);
}

unsigned cpme_bvc_get_gate(void * bvc, size_t i)
{
    return cpme_ivc_get_gate(bvc, i);
}

void cpme_free_bvc(void * bvc)
{
    cpme_free_ivc(bvc);
}

size_t cpme_proof_num_clauses(void * proof)
{
    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    assert(p);

    return p->size();
}

size_t cpme_proof_clause_size(void * proof, size_t n)
{
    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    assert(p);
    assert(n < p->size());

    return p->at(n).size();
}

unsigned cpme_proof_lit(void * proof, size_t cls, size_t n)
{
    PME::ExternalClauseVec * p = static_cast<PME::ExternalClauseVec *>(proof);
    assert(p);
    assert(cls < p->size());

    const PME::ExternalClause & clause = p->at(cls);
    assert(n < clause.size());
    return clause.at(n);
}

void cpme_print_stats(void * pme)
{
    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    assert(eng);
    eng->printStats();
    fflush(stdout);
}

const char * cpme_set_option(void * pme, const char * option)
{
    static std::string error_msg;

    PME::Engine * eng = static_cast<PME::Engine *>(pme);
    assert(eng);

    try
    {
        eng->parseOption(option);
    }
    catch (std::exception& e)
    {
        error_msg = e.what();
        return error_msg.c_str();
    }

    return NULL;
}

