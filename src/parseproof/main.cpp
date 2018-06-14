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
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

extern "C" {
#include "aiger/aiger.h"
}

typedef std::vector<unsigned> ParsedClause;
typedef std::vector<ParsedClause> ParsedProof;

void print_usage(char ** argv)
{
    const char * name = argv[0];
    fprintf(stderr, "Usage: %s AIG PROOF\n", name);
}

std::string default_name(std::string prefix, unsigned index)
{
    std::ostringstream ss;
    ss << prefix << index;
    return ss.str();
}


void parse_symbols(aiger_symbol * syms,
                   size_t n,
                   std::string name_prefix,
                   std::unordered_map<std::string, unsigned> & name_to_id)
{
    for (size_t i = 0; i < n; ++i)
    {
        unsigned lit = syms[i].lit;
        assert(!aiger_sign(lit));
        const char * aig_name = syms[i].name;
        std::string name;
        if (aig_name == NULL)
        {
            name = default_name(name_prefix, i);
        }
        else
        {
            name = aig_name;
        }
        assert(!name.empty());
        name_to_id[name] = lit;
    }
}

ParsedProof parse_proof(aiger * aig, std::ifstream & file)
{
    ParsedProof parsed;
    std::unordered_map<std::string, unsigned> name_to_id;

    parse_symbols(aig->inputs, aig->num_inputs, "i", name_to_id);
    parse_symbols(aig->latches, aig->num_latches, "l", name_to_id);

    std::string ignorable = "One-step Inductive Strengthening of Property"
                            " (in CNF):";
    std::string remove_from_line("Clause");

    std::string line;
    size_t lineno = 0;
    while (std::getline(file, line))
    {
        lineno++;
        if (!line.compare(0, remove_from_line.size(), remove_from_line))
        {
            // Remove the "Clause n:" part
            assert(line.size() >= line.find(":") + 2);
            line = line.substr(line.find(":") + 2, line.size());
            ParsedClause cls;
            std::string disjunct;
            std::istringstream iss(line);

            bool ok = true;

            while (iss >> disjunct)
            {
                bool neg = false;
                if (disjunct[0] == '!')
                {
                    neg = true;
                    assert(disjunct.size() > 1);
                    disjunct.erase(0, 1);
                }

                unsigned id = name_to_id[disjunct];
                if (id == 0) {
                    std::cerr << "WARNING: line " << lineno << ": "
                              << "unmapped name ``" << disjunct << "''"
                              << ", skipping line"
                              << std::endl;
                    ok = false;
                    break;
                }
                if (neg) { id = aiger_not(id); }
                cls.push_back(id);
            }

            if (!ok) { continue; }
            parsed.push_back(cls);
        }
        else if (!line.empty() && line != ignorable)
        {
            std::cerr << "Error at line " << lineno << ": "
                      << line << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    return parsed;
}

int main(int argc, char ** argv)
{
    const char * opts = "h";
    char * aig_path = NULL;
    char * proof_path = NULL;
    int option = 0;

    // Parse flags
    while ((option = getopt(argc, argv, opts)) != -1)
    {
        switch(option)
        {
            case 'h':
                print_usage(argv);
                return 0;
            default:
                print_usage(argv);
                return EXIT_FAILURE;
        }
    }

    // Parse positional arguments
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
    std::ifstream proof_file(proof_path, std::ifstream::in);
    if (!proof_file.is_open())
    {
        perror("Failed to open proof");
        return EXIT_FAILURE;
    }

    // Read and parse the proof file
    ParsedProof pp = parse_proof(aig, proof_file);

    // Output the new proof to stdout
    for (const ParsedClause & pc : pp)
    {
        for (unsigned lit : pc)
        {
            std::cout << lit << " ";
        }
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}

