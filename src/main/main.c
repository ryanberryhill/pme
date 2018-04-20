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

int main(int argc, char ** argv)
{
    const char * opts = "hv";
    char * aig_path = NULL; 
    char * proof_path = NULL; 
    int verbosity = 0;
    int option = 0;

    // Parse flags
    while ((option = getopt(argc, argv, opts)) != -1)
    {
        switch(option)
        {
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
    if (argc > optind + 1)
    {
        aig_path = argv[optind];
        proof_path = argv[optind + 1];
    }
    else
    {
        print_usage(argv);
        return EXIT_FAILURE;
    }

    printf("pme version %s\n", cpme_version());
    printf("Input file: %s\n", aig_path);

    // Check if the AIG file is readable
    if (access(aig_path, R_OK) == -1)
    {
        perror("Cannot open AIG for reading");
        return EXIT_FAILURE;
    }

    // Check if the proof file is readable
    if (access(proof_path, R_OK) == -1)
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
    FILE * proof_file = (FILE *) fopen(proof_path, "r");
    if (proof_file == NULL)
    {
        perror("Failed to open proof");
        return EXIT_FAILURE;
    }

    // Create the proof
    void * proof = cpme_alloc_proof();
    assert(proof);
    parse_proof(proof, proof_file);
    fclose(proof_file);

    // Initialize the PME library
    void * pme = cpme_init(aig, proof);
    assert(pme);

    // Clean up the AIG and proof
    aiger_reset(aig);
    int free_ok = cpme_free_proof(proof);
    assert(free_ok == 0);
    aig = NULL; proof = NULL;

    // Do things in the PME library
    int proof_ok = cpme_check_proof(pme);
    if (proof_ok < 0)
    {
        fprintf(stderr, "Error checking proof\n");
        return EXIT_FAILURE;
    }

    printf("The proof is %san inductive invariant\n", proof_ok ? "" : "not ");

    // Clean up the PME library
    free_ok = cpme_free(pme);
    assert(free_ok == 0);
    pme = NULL;

    return EXIT_SUCCESS;
}

