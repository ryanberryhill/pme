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

typedef enum
{
    LOG_TEST = 0,
    LOG_PME,
    LOG_MINIMIZATION,
    LOG_MARCO,
    LOG_CAMSIS,
    LOG_SISI,
    LOG_BFMIN,
    LOG_IC3,
    LOG_IVC,
    LOG_CAIVC,
    LOG_MARCOIVC,
    LOG_INVALID,
    NUM_LOG_CHANNELS = LOG_INVALID
} LogChannelID;

typedef enum
{
    PME_MINIMIZATION_MARCO,
    PME_MINIMIZATION_CAMSIS,
    PME_MINIMIZATION_SISI,
    PME_MINIMIZATION_BRUTEFORCE
} PMEMinimizationAlgorithm;

typedef enum
{
    PME_IVC_MARCO,
    PME_IVC_CAIVC
} PMEIVCAlgorithm;

const char * cpme_version();
void * cpme_init(aiger * aig, void * proof);
void cpme_free(void * pme);

void cpme_log_to_stdout(void * pme);
void cpme_set_verbosity(void * pme, int v);
void cpme_set_channel_verbosity(void * pme, LogChannelID channel, int v);

void * cpme_alloc_proof();
void * cpme_copy_proof(void * pme);
void cpme_add_clause(void * proof, const unsigned * cls, size_t n);
void cpme_free_proof(void * proof);

void * cpme_copy_cex(void * pme);
size_t cpme_cex_num_steps(void * cex);
size_t cpme_cex_step_input_size(void * cex, size_t i);
size_t cpme_cex_step_state_size(void * cex, size_t i);
void cpme_cex_get_inputs(void * cex, size_t i, unsigned * dst);
void cpme_cex_get_state(void * cex, size_t i, unsigned * dst);
void cpme_free_cex(void * cex);

size_t cpme_proof_num_clauses(void * proof);
size_t cpme_proof_clause_size(void * proof, size_t n);
unsigned cpme_proof_lit(void * proof, size_t cls, size_t n);

size_t cpme_num_proofs(void * pme);
void * cpme_get_proof(void * pme, size_t i);
void cpme_set_proof(void * pme, void * proof);

size_t cpme_num_ivcs(void * pme);
void * cpme_get_ivc(void * pme, size_t i);
size_t cpme_ivc_num_gates(void * ivc);
unsigned cpme_ivc_get_gate(void * ivc, size_t i);
void cpme_free_ivc(void * ivc);

int cpme_check_proof(void * pme);
int cpme_run_marco(void * pme);
int cpme_run_camsis(void * pme);
int cpme_run_sisi(void * pme);
int cpme_run_bfmin(void * pme);
int cpme_run_caivc(void * pme);

int cpme_run_ic3(void * pme);
int cpme_run_bmc(void * pme, unsigned k_max);

#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

namespace PME
{
    typedef uint64_t ID;
    typedef uint64_t ClauseID;
    typedef unsigned ExternalID;

    typedef std::vector<unsigned> ExternalClause;
    typedef std::vector<ExternalClause> ExternalClauseVec;

    typedef std::vector<ID> Clause;
    typedef std::vector<ID> Cube;
    typedef std::vector<ID> IVC;
    typedef std::vector<Clause> ClauseVec;

    typedef std::vector<unsigned> ExternalCube;
    typedef std::vector<unsigned> ExternalIVC;

    struct ExternalStep {
        ExternalCube inputs;
        ExternalCube state;
    };
    typedef std::vector<ExternalStep> ExternalCounterExample;

    typedef std::vector<ClauseID> ClauseIDVec;

    const std::string& pme_version();

    typedef std::vector<ID>::const_iterator id_iterator;
    typedef ClauseVec::const_iterator clause_iterator;

    std::ostream & operator<<(std::ostream &, const ClauseIDVec &);
    bool subsumes(const Cube & a, const Cube & b);
}

namespace std {
    template<> struct hash<PME::Cube> {
        size_t operator()(const PME::Cube & vec) const noexcept
        {
            size_t seed = vec.size();
            for(auto& i : vec) {
              seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
}


#endif // __cplusplus


#endif

