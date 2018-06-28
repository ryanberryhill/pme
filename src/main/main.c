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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <limits.h>

#include "pme/pme.h"
#include "aiger/aiger.h"

void print_usage(char ** argv)
{
    const char * name = argv[0];
    fprintf(stderr, "Usage: %s [OPTION]... AIG PROOF\n", name);
}

void parse_proof(void * proof, FILE * proof_file)
{
    char line [1024];
    unsigned cls[256];
    size_t lineno = 0;
    while (fgets(line, sizeof(line), proof_file)) {
        lineno++;

        // Check that the whole line was read in
        size_t len = strnlen(line, sizeof(line));
        if (line[len-1] != '\n')
        {
            fprintf(stderr,
                    "Error parsing proof: line %lu is too long\n",
                    lineno);
            exit(EXIT_FAILURE);
        }

        // Remove the newline
        line[len-1] = 0;

        // Parse line
        size_t n = 0;
        char * token = strtok(line, " ");
        while (token != NULL)
        {
            if (n >= sizeof(cls) / sizeof(unsigned))
            {
                fprintf(stderr,
                        "Error parsing proof: too many literals on line %lu\n",
                        lineno);
                exit(EXIT_FAILURE);
            }

            unsigned aig_id = atol(token);

            if (aig_id == 0)
            {
                fprintf(stderr,
                        "Error parsing proof: invalid token %s on line %lu\n",
                        token, lineno);
                exit(EXIT_FAILURE);
            }
            cls[n] = aig_id;
            n++;
            token = strtok(NULL, " ");
        }

        cpme_add_clause(proof, cls, n);
    }
}

void report_run(void * pme, const char * name)
{
    size_t num_proofs = cpme_num_proofs(pme);
    size_t largest = 0;
    size_t smallest = UINT_MAX;
    for (size_t i = 0; i < num_proofs; ++i)
    {
        void * min_proof = cpme_get_proof(pme, i);
        size_t current = cpme_proof_num_clauses(min_proof);

        if (current > largest) { largest = current; }
        if (current < smallest) { smallest = current; }

        cpme_free_proof(min_proof);
    }

    if (num_proofs > 0)
    {
        printf("Found %lu minimal proof(s) of size %lu-%lu with %s\n",
                num_proofs, smallest, largest, name);
    }
    else
    {
        printf("Found no proofs with %s\n", name);
    }
}

void save_proof(void * pme, size_t pindex, const char * filepath)
{
    FILE * fp = fopen(filepath, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Couldn't open %s for writing\n", filepath);
        return;
    }

    void * proof = cpme_get_proof(pme, pindex);
    size_t size = cpme_proof_num_clauses(proof);
    for (size_t i = 0; i < size; ++i)
    {
        size_t cls_size = cpme_proof_clause_size(proof, i);
        for (size_t j = 0; j < cls_size; ++j)
        {
            char last = j == (cls_size - 1);
            unsigned lit = cpme_proof_lit(proof, i, j);
            fprintf(fp, "%u%s", lit, last ? "" : " ");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    cpme_free_proof(proof);
}

char g_save_path[512];

void save_proofs(void * pme, char * name)
{
    char filepath[1024];

    unsigned num_proofs = cpme_num_proofs(pme);
    for (size_t i = 0; i < num_proofs; ++i)
    {
        size_t len = snprintf(filepath, sizeof(filepath),
                              "%s.%s%lu.pme", g_save_path, name, i);
        if (len >= sizeof(filepath))
        {
            fprintf(stderr, "Filepath ``%s...'' is too long\n", filepath);
            continue;
        }

        save_proof(pme, i, filepath);
    }
}

// These need to be global so they can be used in the initializer for
// long_opts in main() below
int g_ic3 = 0, g_check = 0, g_marco = 0, g_camsis = 0, g_bfmin = 0, g_sisi = 0, g_checkmin = 0;
int g_saveproofs = 0;

int main(int argc, char ** argv)
{
    const char * opts = "hv";
    char * aig_path = NULL;
    char * proof_path = NULL;
    int verbosity = 0;
    int option = 0;
    int option_index = 0;

    static struct option long_opts[] = {
            {"ic3",             no_argument,        &g_ic3,        1 },
            {"check",           no_argument,        &g_check,      1 },
            {"check-minimal",   no_argument,        &g_checkmin,   1 },
            {"save-proofs",     required_argument,  &g_saveproofs, 1 },
            {"marco",           no_argument,        &g_marco,      1 },
            {"camsis",          no_argument,        &g_camsis,     1 },
            {"sisi",            no_argument,        &g_sisi,       1 },
            {"bfmin",           no_argument,        &g_bfmin,      1 },
            {"help",            no_argument,        0,            'h'},
            {0,                 0,                  0,             0 }
    };

    // Parse flags
    while ((option = getopt_long(argc, argv, opts, long_opts, &option_index)) != -1)
    {
        switch(option)
        {
            case 0:
                if (strcmp(long_opts[option_index].name, "save-proofs") == 0)
                {
                    if (strlen(optarg) >= sizeof(g_save_path))
                    {
                        fprintf(stderr, "argument to save-proofs is too long\n");
                        return EXIT_FAILURE;
                    }
                    else
                    {
                        strncpy(g_save_path, optarg, sizeof(g_save_path));
                    }
                }
                break;
            case 'v':
                verbosity++;
                break;
            case 'h':
                print_usage(argv);
                return 0;
            default:
                print_usage(argv);
                return EXIT_FAILURE;
        }
    }

    // Parse positional argument(s)
    if (argc == optind + 2)
    {
        // Given an AIG and a proof
        aig_path = argv[optind];
        proof_path = argv[optind + 1];
    }
    else if (argc == optind + 1 && g_ic3)
    {
        // Given an AIG and --ic3
        aig_path = argv[optind];
    }
    else
    {
        print_usage(argv);
        return EXIT_FAILURE;
    }

    printf("pme version %s\n", cpme_version());
    printf("Input AIG: %s\n", aig_path);
    if (proof_path)
    {
        printf("Input proof: %s\n", proof_path);
    }

    // Check if the AIG file is readable
    if (access(aig_path, R_OK) == -1)
    {
        perror("Cannot open AIG for reading");
        return EXIT_FAILURE;
    }

    // Check if the proof file is readable
    if (proof_path && access(proof_path, R_OK) == -1)
    {
        perror("Cannot open proof for reading");
        return EXIT_FAILURE;
    }

    // Open the AIG file
    FILE * aig_file = (FILE *) fopen(aig_path, "r");
    if (aig_file == NULL)
    {
        perror("Failed to open AIG");
        return EXIT_FAILURE;
    }

    aiger * aig = aiger_init();

    // Read the AIG file
    const char * msg = aiger_read_from_file(aig, aig_file);
    fclose(aig_file);

    if (msg != NULL)
    {
        fprintf(stderr, "%s: %s\n", aig_path, msg);
        aiger_reset(aig);
        return EXIT_FAILURE;
    }

    // Open the proof file
    void * proof = NULL;
    if (proof_path)
    {
        FILE * proof_file = (FILE *) fopen(proof_path, "r");
        if (proof_file == NULL)
        {
            perror("Failed to open proof");
            return EXIT_FAILURE;
        }

        // Create the proof
        proof = cpme_alloc_proof();
        assert(proof);
        parse_proof(proof, proof_file);
        fclose(proof_file);
    }

    // Initialize the PME library
    void * pme = cpme_init(aig, proof);
    assert(pme);

    // Clean up the AIG
    aiger_reset(aig);
    aig = NULL;

    // Do things in the PME library
    cpme_log_to_stdout(pme);
    cpme_set_verbosity(pme, verbosity);

    // Run IC3
    if (g_ic3)
    {
        int safe = cpme_run_ic3(pme);
        if (safe < 0)
        {
            fprintf(stderr, "Error running IC3\n");
            return EXIT_FAILURE;
        }
        else if (safe == 0)
        {
            printf("The instance is unsafe, terminating\n");
            return EXIT_SUCCESS;
        }

        // Copy the proof and tell PME to use it for minimization
        assert(!proof);
        proof = cpme_copy_proof(pme);
        cpme_set_proof(pme, proof);
    }

    size_t proof_size = cpme_proof_num_clauses(proof);
    printf("The proof has %lu clauses\n", proof_size);

    // Clean up the proof
    cpme_free_proof(proof);
    proof = NULL;

    if (g_check)
    {
        int proof_ok = cpme_check_proof(pme);
        if (proof_ok < 0)
        {
            fprintf(stderr, "Error checking proof\n");
            return EXIT_FAILURE;
        }

        printf("The proof is %sa safe inductive invariant\n", proof_ok ? "" : "not ");
        if (!proof_ok)
        {
            return EXIT_FAILURE;
        }
    }

    if (g_checkmin)
    {
        int bfmin_ok = cpme_run_bfmin(pme);

        if (bfmin_ok < 0)
        {
            fprintf(stderr, "Error running brute force minimization\n");
            return EXIT_FAILURE;
        }

        size_t num_proofs = cpme_num_proofs(pme);
        assert(num_proofs == 1);

        void * min_proof = cpme_get_proof(pme, 0);
        size_t min_size = cpme_proof_num_clauses(min_proof);

        assert(min_size <= proof_size);

        if (min_size < proof_size)
        {
            printf("The proof (size %lu) is non-minimal. "
                   "A proof with %lu clauses was found.\n", proof_size, min_size);
            return EXIT_FAILURE;
        }
        else
        {
            printf("The proof (size %lu) is minimal.\n", proof_size);
            return EXIT_SUCCESS;
        }

        cpme_free_proof(min_proof);
    }

    if (g_bfmin)
    {
        int bfmin_ok = cpme_run_bfmin(pme);
        if (bfmin_ok < 0)
        {
            fprintf(stderr, "Error running brute force minimization\n");
            return EXIT_FAILURE;
        }

        report_run(pme, "BFMIN");

        if (g_saveproofs)
        {
            save_proofs(pme, "bfmin");
        }
    }

    if (g_sisi)
    {
        int sisi_ok = cpme_run_sisi(pme);
        if (sisi_ok < 0)
        {
            fprintf(stderr, "Error running SISI\n");
            return EXIT_FAILURE;
        }

        report_run(pme, "SISI");

        if (g_saveproofs)
        {
            save_proofs(pme, "sisi");
        }
    }

    if (g_marco)
    {
        int marco_ok = cpme_run_marco(pme);
        if (marco_ok < 0)
        {
            fprintf(stderr, "Error running MARCO\n");
            return EXIT_FAILURE;
        }

        report_run(pme, "MARCO");

        if (g_saveproofs)
        {
            save_proofs(pme, "marco");
        }
    }

    if (g_camsis)
    {
        int camsis_ok = cpme_run_camsis(pme);
        if (camsis_ok < 0)
        {
            fprintf(stderr, "Error running CAMSIS\n");
            return EXIT_FAILURE;
        }

        report_run(pme, "CAMSIS");

        if (g_saveproofs)
        {
            save_proofs(pme, "camsis");
        }
    }

    // Clean up the PME library
    cpme_free(pme);
    pme = NULL;

    return EXIT_SUCCESS;
}

