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

#ifndef SORTING_NETWORK_H_INCLUDED
#define SORTING_NETWORK_H_INCLUDED

#include "pme/id.h"
#include "pme/engine/variable_manager.h"

namespace PME {

    ClauseVec compHalf(ID x_1, ID x_2, ID y_1, ID y_2);
    ClauseVec compFull(ID x_1, ID x_2, ID y_1, ID y_2);

    typedef std::pair<std::vector<ID>, ClauseVec> CNFNetwork;

    CNFNetwork mergeNetwork(VariableManager & varman,
                            const std::vector<ID> & inputs_a,
                            const std::vector<ID> & inputs_b,
                            bool le = true,
                            bool ge = false);

    CNFNetwork simpMergeNetwork(VariableManager & varman,
                                const std::vector<ID> & inputs_a,
                                const std::vector<ID> & inputs_b,
                                unsigned c,
                                bool le = true,
                                bool ge = false);

    CNFNetwork sortingNetwork(VariableManager & varman,
                              const std::vector<ID> & inputs,
                              bool le = true,
                              bool ge = false);

    CNFNetwork cardinalityNetwork(VariableManager & varman,
                                  const std::vector<ID> & inputs,
                                  unsigned m,
                                  bool le = true,
                                  bool ge = false);
}

#endif

