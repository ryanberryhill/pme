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
#include <signal.h>

#include "pme/pme.h"
#include "aiger/aiger.h"

int g_verbosity = 0;

#define print(v, ...) if (g_verbosity >= v) { printf(__VA_ARGS__); }

#define MAX_PME_OPTS 256
unsigned g_pme_opt_index = 0;
char * g_pme_opts[MAX_PME_OPTS] = {NULL};

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

void report_ivc_run(void * pme, const char * name)
{
    size_t num_ivcs = cpme_num_ivcs(pme);
    size_t largest = 0;
    size_t smallest = UINT_MAX;

    for (size_t i = 0; i < num_ivcs; ++i)
    {
        void * mivc = cpme_get_ivc(pme, i);
        size_t current = cpme_ivc_num_gates(mivc);

        if (current > largest) { largest = current; }
        if (current < smallest) { smallest = current; }

        cpme_free_ivc(mivc);
    }

    if (num_ivcs > 0)
    {
        print(1, "Found %lu minimal IVC(s) of size %lu-%lu with %s\n",
                  num_ivcs, smallest, largest, name);
    }
    else
    {
        print(1, "Found no IVCs with %s\n", name);
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
        print(1, "Found %lu minimal proof(s) of size %lu-%lu with %s\n",
                  num_proofs, smallest, largest, name);
    }
    else
    {
        print(1, "Found no proofs with %s\n", name);
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

void save_subcircuit(aiger * ckt, void * subckt, size_t subckt_size, const char * filepath)
{
    FILE * fp = fopen(filepath, "w");
    if (fp == NULL)
    {
        fprintf(stderr, "Couldn't open %s for writing\n", filepath);
        return;
    }

    aiger * ivc_aig = aiger_init();

    size_t num_vars = ckt->maxvar + 1;
    unsigned char * relevant = calloc(num_vars, sizeof(unsigned char));

    for (size_t i = 0; i < subckt_size; ++i)
    {
        unsigned gate_id = cpme_ivc_get_gate(subckt, i);

        // Find the AND gate in the original aig
        // NOTE: and gates apparently are not sorted by lhs
        for (size_t and_index = 0; and_index < ckt->num_ands; ++and_index)
        {
            unsigned lhs = ckt->ands[and_index].lhs;

            if (lhs == gate_id)
            {
                // We've found it
                unsigned rhs0 = ckt->ands[and_index].rhs0;
                unsigned rhs1 = ckt->ands[and_index].rhs1;

                // Mark the variables as relevant
                unsigned l = aiger_lit2var(lhs);
                unsigned r0 = aiger_lit2var(rhs0);
                unsigned r1 = aiger_lit2var(rhs1);
                relevant[l] = 1;
                relevant[r0] = 1;
                relevant[r1] = 1;

                // Copy the gate
                aiger_add_and(ivc_aig, lhs, rhs0, rhs1);

                break;
            }
        }
    }

    // For all relevant latches, their next-states must also be relevant,
    // and their next-states, and so forth
    char fixpoint = 0;
    while (!fixpoint)
    {
        fixpoint = 1;
        for (unsigned i = 0; i < num_vars; ++i)
        {
            if (!relevant[i]) { continue; }

            unsigned lit = aiger_var2lit(i);

            aiger_symbol * sym = NULL;

            // If it's a latch, copy it and the reset
            sym = aiger_is_latch(ckt, lit);
            if (sym)
            {
                unsigned next = aiger_lit2var(sym->next);
                if (!relevant[next])
                {
                    relevant[next] = 1;
                    fixpoint = 0;
                }
            }
        }
    }

    // Iterate over all relevant variables
    // Start from i = 1 because 0 is the variable represeting constant 1
    for (unsigned i = 1; i < num_vars; ++i)
    {
        if (!relevant[i]) { continue; }

        unsigned lit = aiger_var2lit(i);

        aiger_symbol * sym = NULL;

        // If it's a latch, copy it and the reset
        sym = aiger_is_latch(ckt, lit);
        if (sym)
        {
            unsigned next = sym->next;
            unsigned reset = sym->reset;
            const char * name = sym->name;
            aiger_add_latch(ivc_aig, lit, next, name);
            aiger_add_reset(ivc_aig, lit, reset);
            continue;
        }

        // If it's an input, copy it with its name
        sym = aiger_is_input(ckt, lit);
        if (sym)
        {
            const char * name = sym->name;
            aiger_add_input(ivc_aig, lit, name);
            continue;
        }

        // If it's neither latch nor an input nor an AND gate of the IVC,
        // then it's a PPI created by removing gates
        if (!aiger_is_and(ivc_aig, lit))
        {
            aiger_add_input(ivc_aig, lit, NULL);
            continue;
        }
    }

    // Copy the first output
    assert(ckt->num_outputs == 1);
    unsigned o_lit = ckt->outputs[0].lit;
    const char * o_name = ckt->outputs[0].name;
    aiger_add_output(ivc_aig, o_lit, o_name);

    aiger_write_to_file(ivc_aig, aiger_binary_mode, fp);

    fclose(fp); fp = NULL;
    aiger_reset(ivc_aig); ivc_aig = NULL;


    free(relevant); relevant = NULL;
}

void save_ivc(aiger * aig, void * pme, size_t pindex, const char * filepath)
{
    void * ivc = cpme_get_ivc(pme, pindex);
    size_t size = cpme_ivc_num_gates(ivc);

    save_subcircuit(aig, ivc, size, filepath);

    cpme_free_ivc(ivc); ivc = NULL;
}

void save_bvc(aiger * aig, void * pme, size_t bound, size_t index, const char * filepath)
{
    void * bvc = cpme_get_bvc(pme, bound, index);
    size_t size = cpme_bvc_num_gates(bvc);

    save_subcircuit(aig, bvc, size, filepath);

    cpme_free_bvc(bvc); bvc = NULL;
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

void save_ivcs(aiger * aig, void * pme, char * name)
{
    char filepath[1024];

    unsigned num_ivcs = cpme_num_ivcs(pme);
    for (size_t i = 0; i < num_ivcs; ++i)
    {
        size_t len = snprintf(filepath, sizeof(filepath),
                              "%s.%s%lu.aig", g_save_path, name, i);

        if (len >= sizeof(filepath))
        {
            fprintf(stderr, "Filepath ``%s...'' is too long\n", filepath);
            continue;
        }

        save_ivc(aig, pme, i, filepath);
    }

    unsigned bvc_bound = cpme_bvc_bound(pme);
    for (size_t k = 0; k < bvc_bound; ++k)
    {
        size_t num_bvcs = cpme_num_bvcs(pme, k);
        for (size_t i = 0; i < num_bvcs; ++i)
        {
            size_t len = snprintf(filepath, sizeof(filepath), "%s.%s.bound%lu.%lu.aig",
                                  g_save_path, name, k, i);

            if (len >= sizeof(filepath))
            {
                fprintf(stderr, "Filepath ``%s...'' is too long\n", filepath);
                continue;
            }

            save_bvc(aig, pme, k, i, filepath);
        }
    }
}

void print_cex_step(unsigned * cex_vec, unsigned cex_size, aiger_symbol * syms, unsigned num_syms)
{
    for (size_t j = 0, k = 0; j < num_syms; ++j)
    {
        assert(k <= j);
        unsigned aig_var = syms[j].lit;
        assert(!aiger_sign(aig_var));
        unsigned cex_lit = 0;
        char neg = 0;
        unsigned cex_var = 0;

        if (k < cex_size)
        {
            cex_lit = cex_vec[k];
            neg = aiger_sign(cex_lit);
            cex_var = aiger_strip(cex_lit);
        }

        assert(cex_var >= aig_var || cex_var == 0);

        if (cex_var == aig_var)
        {
            printf("%c", neg ? '0' : '1');
            k++;
        }
        else
        {
            printf("x");
        }
    }
    printf("\n");
}

void print_cex(void * pme, aiger * aig)
{
    // We're assuming the inputs are sorted in the original AIG, which is true
    // if it's reencoded.
    assert(aiger_is_reencoded(aig));
    void * cex = cpme_copy_cex(pme);
    assert(cex);

    size_t n = cpme_cex_num_steps(cex);

    print(1, "Found a counter-example of length %lu\n", n);
    printf("1\n");
    printf("c witness\n");
    // TODO: handle cases other than single output properties
    printf("b0\n");

    // Print initial state
    assert(n > 0);
    size_t init_state_size = cpme_cex_step_state_size(cex, 0);
    unsigned * init_state = calloc(init_state_size, sizeof(unsigned));
    cpme_cex_get_state(cex, 0, init_state);

    print_cex_step(init_state, init_state_size, aig->latches, aig->num_latches);

    free(init_state); init_state = NULL;

    // Print inputs
    for (size_t i = 0; i < n; ++i)
    {
        size_t step_input_size = cpme_cex_step_input_size(cex, i);
        unsigned * cex_inputs = calloc(step_input_size, sizeof(unsigned));
        cpme_cex_get_inputs(cex, i, cex_inputs);

        print_cex_step(cex_inputs, step_input_size, aig->inputs, aig->num_inputs);

        free(cex_inputs); cex_inputs = NULL;
    }

    printf(".\n");
    printf("c end witness\n");

    cpme_free_cex(cex);
    cex = NULL;
}

// These need to be global so they can be used in the initializer for
// long_opts in main() below
int g_ic3 = 0, g_bmc = 0;
int g_checkproof = 0, g_checkmin = 0, g_checkmivc = 0;
int g_marco = 0, g_camsis = 0, g_bfmin = 0, g_sisi = 0, g_simplemin = 0;
int g_caivc = 0, g_cbvc = 0, g_marcoivc = 0, g_ivcbf = 0, g_ivcucbf = 0;
int g_saveproofs = 0, g_saveivcs = 0;
int g_printstats = 0, g_nocex = 0;

char * g_saveivc_name = NULL;
char * g_saveproof_name = NULL;

int needs_proof_arg()
{
    return !(g_ic3 || g_bmc ||
             g_caivc || g_cbvc || g_marcoivc || g_ivcbf || g_ivcucbf ||
             g_checkmivc);
}

int uses_proof()
{
    return !(g_bmc ||
             g_caivc || g_cbvc || g_marcoivc || g_ivcbf || g_ivcucbf ||
             g_checkmivc);
}

int ic3_should_be_quiet()
{
    return (g_caivc || g_cbvc || g_marcoivc || g_ivcbf || g_ivcucbf || g_checkmivc);
}

// I don't think this scheme for printing out stats on a failure is truly safe
// but it's good enough for now.
void * g_pme = NULL;
aiger * g_aig = NULL;
void term_int_handler(const int sig) {
    (void)(sig);
    exit(EXIT_FAILURE);
}

void exit_save()
{
    if (g_printstats && g_pme)
    {
        cpme_print_stats(g_pme);
    }

    if (g_saveproofs && g_pme)
    {
        save_proofs(g_pme, g_saveproof_name);
    }

    if (g_saveivcs && g_pme && g_aig)
    {
        save_ivcs(g_aig, g_pme, g_saveivc_name);
    }
}

int main(int argc, char ** argv)
{
    const char * opts = "ho:v";
    char * aig_path = NULL;
    char * proof_path = NULL;
    void * proof = NULL;
    int option = 0;
    int option_index = 0;
    unsigned bmc_kmax = 0;
    char failure = 0;
    aiger * aig = NULL;
    void * pme = NULL;

    static struct option long_opts[] = {
            {"ic3",                 no_argument,        &g_ic3,        1 },
            {"bmc",                 required_argument,  &g_bmc,        1 },
            {"check",               no_argument,        &g_checkproof, 1 },
            {"check-minimal",       no_argument,        &g_checkmin,   1 },
            {"check-minimal-ivc",   no_argument,        &g_checkmivc,  1 },
            {"save-proofs",         required_argument,  &g_saveproofs, 1 },
            {"save-ivcs",           required_argument,  &g_saveivcs,   1 },
            {"marco",               no_argument,        &g_marco,      1 },
            {"camsis",              no_argument,        &g_camsis,     1 },
            {"sisi",                no_argument,        &g_sisi,       1 },
            {"bfmin",               no_argument,        &g_bfmin,      1 },
            {"simplemin",           no_argument,        &g_simplemin,  1 },
            {"caivc",               no_argument,        &g_caivc,      1 },
            {"cbvc",                no_argument,        &g_cbvc,       1 },
            {"marco-ivc",           no_argument,        &g_marcoivc,   1 },
            {"ivcbf",               no_argument,        &g_ivcbf,      1 },
            {"ivcucbf",             no_argument,        &g_ivcucbf,    1 },
            {"stats",               no_argument,        &g_printstats, 1 },
            {"no-cex",              no_argument,        &g_nocex,      1 },
            {"opt",                 required_argument,  0,            'o'},
            {"help",                no_argument,        0,            'h'},
            {0,                     0,                  0,             0 }
    };

    // Parse flags
    while ((option = getopt_long(argc, argv, opts, long_opts, &option_index)) != -1)
    {
        switch(option)
        {
            case 0:
                if (strcmp(long_opts[option_index].name, "save-proofs") == 0)
                {
                    if (g_saveivcs)
                    {
                        fprintf(stderr, "--save-ivcs and save-proofs cannot be given together\n");
                        failure = 1; goto cleanup;
                    }

                    if (strlen(optarg) >= sizeof(g_save_path))
                    {
                        fprintf(stderr, "argument to save-proofs is too long\n");
                        failure = 1; goto cleanup;
                    }
                    else
                    {
                        strncpy(g_save_path, optarg, sizeof(g_save_path));
                    }
                }
                else if (strcmp(long_opts[option_index].name, "save-ivcs") == 0)
                {
                    if (g_saveproofs)
                    {
                        fprintf(stderr, "--save-ivcs and save-proofs cannot be given together\n");
                        failure = 1; goto cleanup;
                    }

                    if (strlen(optarg) >= sizeof(g_save_path))
                    {
                        fprintf(stderr, "argument to save-ivcs is too long\n");
                        failure = 1; goto cleanup;
                    }
                    else
                    {
                        strncpy(g_save_path, optarg, sizeof(g_save_path));
                    }
                }
                else if (strcmp(long_opts[option_index].name, "bmc") == 0)
                {
                    char *endptr = NULL;
                    // 0 base = decimal, hex, or octal depending on prefix
                    bmc_kmax = strtoul(optarg, &endptr, 0);
                    if (*endptr != '\0' && *optarg != '\0')
                    {
                        fprintf(stderr, "--bmc argument %s not understood\n", optarg);
                        print_usage(argv);
                        failure = 1; goto cleanup;
                    }
                }
                break;
            case 'v':
                g_verbosity++;
                break;
            case 'o':
                if (g_pme_opt_index < MAX_PME_OPTS)
                {
                    g_pme_opts[g_pme_opt_index] = strdup(optarg);
                    g_pme_opt_index++;
                }
                else
                {
                    fprintf(stderr, "Specified --opt too many times (max is %d)\n",
                                    MAX_PME_OPTS);
                    failure = 1; goto cleanup;
                }
                break;
            case 'h':
                print_usage(argv);
                goto cleanup;
            default:
                print_usage(argv);
                failure = 1; goto cleanup;
        }
    }

    // Parse positional argument(s)
    if (argc == optind + 2)
    {
        // Given an AIG and a proof
        aig_path = argv[optind];
        proof_path = argv[optind + 1];
    }
    else if (argc == optind + 1 && !needs_proof_arg())
    {
        // Given an AIG and --ic3 or --bmc
        aig_path = argv[optind];
    }
    else
    {
        print_usage(argv);
        failure = 1; goto cleanup;
    }

    // Set up exit handler to print stats on exit, signal handlers to call
    // exit on SIGTERM or SIGINT
    atexit(exit_save);
    struct sigaction sa;
    sa.sa_handler = term_int_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    int sigint_ok = sigaction(SIGINT, &sa, NULL);
    if (sigint_ok < 0)
    {
        perror("Failed to setup SIGINT handler");
        failure = 1; goto cleanup;
    }

    int sigterm_ok = sigaction(SIGTERM, &sa, NULL);
    if (sigterm_ok < 0)
    {
        perror("Failed to setup SIGTERM handler");
        failure = 1; goto cleanup;
    }

    print(2, "pme version %s\n", cpme_version());
    print(2, "Input AIG: %s\n", aig_path);
    if (proof_path)
    {
        print(2, "Input proof: %s\n", proof_path);
    }

    // Check if the AIG file is readable
    if (access(aig_path, R_OK) == -1)
    {
        perror("Cannot open AIG for reading");
        failure = 1; goto cleanup;
    }

    // Check if the proof file is readable
    if (proof_path && access(proof_path, R_OK) == -1)
    {
        perror("Cannot open proof for reading");
        failure = 1; goto cleanup;
    }

    // Open the AIG file
    FILE * aig_file = (FILE *) fopen(aig_path, "r");
    if (aig_file == NULL)
    {
        perror("Failed to open AIG");
        failure = 1; goto cleanup;
    }

    aig = aiger_init();

    // Read the AIG file
    const char * msg = aiger_read_from_file(aig, aig_file);
    fclose(aig_file);

    if (msg != NULL)
    {
        fprintf(stderr, "%s: %s\n", aig_path, msg);
        failure = 1; goto cleanup;
    }

    // Open the proof file
    if (proof_path)
    {
        FILE * proof_file = (FILE *) fopen(proof_path, "r");
        if (proof_file == NULL)
        {
            perror("Failed to open proof");
            failure = 1; goto cleanup;
        }

        // Create the proof
        proof = cpme_alloc_proof();
        assert(proof);
        parse_proof(proof, proof_file);
        fclose(proof_file);
    }

    // Initialize the PME library
    pme = cpme_init(aig, proof);
    assert(pme);

    // External pointer to the same PME instance, used by the atexit
    // handler to print stats
    g_pme = pme;
    // Same thing for the AIG, used by atexit to save IVCs, proofs, etc.
    g_aig = aig;

    // Set PME internal options if any were given
    for (size_t i = 0; i < g_pme_opt_index; ++i)
    {
        const char * error_msg = cpme_set_option(pme, g_pme_opts[i]);
        if (error_msg != NULL)
        {
            fprintf(stderr, "%s\n", error_msg);
            failure = 1; goto cleanup;
        }
    }

    // Do things in the PME library
    cpme_log_to_stdout(pme);
    cpme_set_verbosity(pme, g_verbosity);

    if (ic3_should_be_quiet())
    {
        cpme_set_channel_verbosity(pme, LOG_IC3, 0);
    }

    if (g_bmc)
    {
        int safe = cpme_run_bmc(pme, bmc_kmax);
        if (safe < 0)
        {
            fprintf(stderr, "Error running BMC\n");
            failure = 1; goto cleanup;
        }
        else if (safe == 0)
        {
            if (g_nocex) { print(0, "1\n"); }
            else { print_cex(pme, aig); }
            goto cleanup;
        }
        else
        {
            // Bounded safety i.e., we don't know the result: if we don't have
            // IC3 running or a proof, we need to quit now
            if (!g_ic3 && !proof)
            {
                print(0, "2\n");
                goto cleanup;
            }
        }
    }

    // Run IC3
    if (g_ic3)
    {
        int safe = cpme_run_ic3(pme);
        if (safe < 0)
        {
            fprintf(stderr, "Error running IC3\n");
            failure = 1; goto cleanup;
        }
        else if (safe == 0)
        {
            if (g_nocex) { print(0, "1\n"); }
            else { print_cex(pme, aig); }
            goto cleanup;
        }

        print(0, "0\n");

        // Copy the proof
        assert(!proof);
        proof = cpme_copy_proof(pme);
        // No need to call set_proof, run_ic3 will set the proof itself
        //cpme_set_proof(pme, proof);
    }

    size_t proof_size = 0;
    if (uses_proof())
    {
        assert(proof);
        proof_size = cpme_proof_num_clauses(proof);
        print(1, "The proof has %lu clauses\n", proof_size);
    }

    if (g_checkproof)
    {
        int proof_ok = cpme_check_proof(pme);
        if (proof_ok < 0)
        {
            fprintf(stderr, "Error checking proof\n");
            failure = 1; goto cleanup;
        }

        print(1, "The proof is %sa safe inductive invariant\n", proof_ok ? "" : "not ");
        if (!proof_ok)
        {
            failure = 1; goto cleanup;
        }
    }

    if (g_checkmin)
    {
        int bfmin_ok = cpme_run_bfmin(pme);

        if (bfmin_ok < 0)
        {
            fprintf(stderr, "Error running brute force minimization\n");
            failure = 1; goto cleanup;
        }

        size_t num_proofs = cpme_num_proofs(pme);
        ((void)(num_proofs));
        assert(num_proofs == 1);

        void * min_proof = cpme_get_proof(pme, 0);
        size_t min_size = cpme_proof_num_clauses(min_proof);

        cpme_free_proof(min_proof); min_proof = NULL;
        assert(min_size <= proof_size);

        if (min_size < proof_size)
        {
            print(1, "The proof (size %lu) is non-minimal. "
                     "A proof with %lu clauses was found.\n", proof_size, min_size);
            failure = 1; goto cleanup;
        }
        else
        {
            print(1, "The proof (size %lu) is minimal.\n", proof_size);
            goto cleanup;
        }
    }

    if (g_checkmivc)
    {
        int bf_ok = cpme_run_ivcbf(pme);
        if (bf_ok < 0)
        {
            fprintf(stderr, "Error checking IVC minimality\n");
            failure = 1; goto cleanup;
        }

        size_t ivc_size = aig->num_ands;

        size_t num_ivcs = cpme_num_ivcs(pme);

        if (num_ivcs == 0)
        {
            fprintf(stderr, "Error checking IVC minimality\n");
            failure = 1; goto cleanup;
        }

        void * min_ivc = cpme_get_ivc(pme, 0);
        size_t min_size = cpme_ivc_num_gates(min_ivc);

        cpme_free_ivc(min_ivc); min_ivc = NULL;
        assert(min_size <= ivc_size);

        if (min_size < ivc_size)
        {
            print(1, "The IVC (size %lu) is non-minimal. "
                     "An IVC with %lu gates was found.\n", ivc_size, min_size);
            failure = 1; goto cleanup;
        }
        else
        {
            assert(num_ivcs == 1);
            print(1, "The IVC (size %lu) is minimal.\n", ivc_size);
            goto cleanup;
        }
    }

    if (g_bfmin)
    {
        g_saveproof_name = "bfmin";
        int bfmin_ok = cpme_run_bfmin(pme);
        if (bfmin_ok < 0)
        {
            fprintf(stderr, "Error running brute force minimization\n");
            failure = 1; goto cleanup;
        }

        report_run(pme, "BFMIN");
    }

    if (g_sisi)
    {
        g_saveproof_name = "sisi";
        int sisi_ok = cpme_run_sisi(pme);
        if (sisi_ok < 0)
        {
            fprintf(stderr, "Error running SISI\n");
            failure = 1; goto cleanup;
        }

        report_run(pme, "SISI");
    }

    if (g_simplemin)
    {
        g_saveproof_name = "simplemin";
        int simplemin_ok = cpme_run_simplemin(pme);
        if (simplemin_ok < 0)
        {
            fprintf(stderr, "Error running simple minimization\n");
            failure = 1; goto cleanup;
        }

        report_run(pme, "SIMPLEMIN");
    }

    if (g_marco)
    {
        g_saveproof_name = "marco";
        int marco_ok = cpme_run_marco(pme);
        if (marco_ok < 0)
        {
            fprintf(stderr, "Error running MARCO\n");
            failure = 1; goto cleanup;
        }

        report_run(pme, "MARCO");
    }

    if (g_camsis)
    {
        g_saveproof_name = "camsis";
        int camsis_ok = cpme_run_camsis(pme);
        if (camsis_ok < 0)
        {
            fprintf(stderr, "Error running CAMSIS\n");
            failure = 1; goto cleanup;
        }

        report_run(pme, "CAMSIS");
    }

    if (g_marcoivc)
    {
        g_saveivc_name = "marcoivc";
        int marcoivc_ok = cpme_run_marcoivc(pme);
        if (marcoivc_ok < 0)
        {
            fprintf(stderr, "Error running MARCO-IVC\n");
            failure = 1; goto cleanup;
        }

        report_ivc_run(pme, "MARCOIVC");
    }

    if (g_ivcbf)
    {
        g_saveivc_name = "ivcbf";
        int ivcbf_ok = cpme_run_ivcbf(pme);
        if (ivcbf_ok < 0)
        {
            fprintf(stderr, "Error running IVC_BF\n");
            failure = 1; goto cleanup;
        }

        report_ivc_run(pme, "IVC_BF");
    }

    if (g_ivcucbf)
    {
        g_saveivc_name = "ivcucbf";
        int ivcucbf_ok = cpme_run_ivcucbf(pme);
        if (ivcucbf_ok < 0)
        {
            fprintf(stderr, "Error running IVC_UCBF\n");
            failure = 1; goto cleanup;
        }

        report_ivc_run(pme, "IVC_UCBF");
    }

    if (g_caivc)
    {
        g_saveivc_name = "caivc";
        int caivc_ok = cpme_run_caivc(pme);
        if (caivc_ok < 0)
        {
            fprintf(stderr, "Error running CAIVC\n");
            failure = 1; goto cleanup;
        }

        report_ivc_run(pme, "CAIVC");
    }

    if (g_cbvc)
    {
        g_saveivc_name = "cbvc";
        int cbvc_ok = cpme_run_cbvc(pme);
        if (cbvc_ok < 0)
        {
            fprintf(stderr, "Error running CBVC\n");
            failure = 1; goto cleanup;
        }

        report_ivc_run(pme, "CBVC");
    }

cleanup:
    if (g_printstats && pme != NULL)
    {
        cpme_print_stats(pme);
    }

    if (g_saveproofs && pme != NULL)
    {
        save_proofs(pme, g_saveproof_name);
    }

    if (g_saveivcs && pme != NULL && aig != NULL)
    {
        save_ivcs(aig, pme, g_saveivc_name);
    }

    // NULL out the outside pointer to PME, which is used by the atexit handler
    // to print out stats in cases where we get terminated.
    g_pme = NULL;

    // Clean up stored PME options
    if (g_pme_opt_index > 0)
    {
        for (size_t i = 0; i < g_pme_opt_index; ++i)
        {
            assert(g_pme_opts[i] != NULL);
            free(g_pme_opts[i]);
            g_pme_opts[i] = NULL;
        }
    }
    // Clean up the proof
    if (proof) { cpme_free_proof(proof); proof = NULL; }
    // Clean up the AIG
    if (aig) { aiger_reset(aig); aig = NULL; }
    // Clean up the PME library
    if (pme) { cpme_free(pme); pme = NULL; }

    return failure ? EXIT_FAILURE : EXIT_SUCCESS;
}

