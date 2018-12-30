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

#include "pme/util/check_cex.h"
#include "pme/engine/sat_adaptor.h"

#include <cassert>

namespace PME {

    // Not the most efficient way of doing this, but it doesn't matter
    bool checkSimulation(VariableManager & varman,
                         const TransitionRelation & tr,
                         const SafetyCounterExample & cex,
                         const Cube & extra_assumps)
    {
        assert(!cex.empty());

        SATAdaptor solver;
        solver.addClauses(tr.unrollWithInit(cex.size()));

        Cube assumps;
        unsigned n = cex.size();
        assumps.reserve(n * (cex.front().inputs.size() + cex.front().state.size()) + 1);
        for (unsigned i = 0; i < n; ++i)
        {
            const Cube & inputs = cex.at(i).inputs;
            const Cube & state = cex.at(i).state;

            Cube pinputs = primeVec(inputs, i);
            Cube pstate = primeVec(state, i);

            assumps.insert(assumps.end(), pinputs.begin(), pinputs.end());
            assumps.insert(assumps.end(), pstate.begin(), pstate.end());
        }

        assumps.insert(assumps.end(), extra_assumps.begin(), extra_assumps.end());

        return solver.solve(assumps);
    }

    bool checkCounterExample(VariableManager & varman,
                             const TransitionRelation & tr,
                             const SafetyCounterExample & cex)
    {
        ID badp = prime(tr.bad(), cex.size() - 1);
        Cube extra_assumps = {badp};

        return checkSimulation(varman, tr, cex, extra_assumps);
    }

    bool checkSimulation(VariableManager & varman,
                         const TransitionRelation & tr,
                         const SafetyCounterExample & cex)
    {
        Cube empty;
        return checkSimulation(varman, tr, cex, empty);
    }

}

