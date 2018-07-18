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

#include "pme/util/simplify_tr.h"

#include "pme/engine/sat_adaptor.h"

namespace PME {
    ClauseVec simplifyTR(const TransitionRelation & tr)
    {
        // Unroll 2 so we get primed constraints
        ClauseVec unrolled = tr.unroll(2);

        SATAdaptor simpSolver(MINISATSIMP);

        simpSolver.addClauses(unrolled);

        // Freeze latches, inputs (optionally), and constraints (including primes)
        simpSolver.freeze(tr.begin_latches(), tr.end_latches(), true);
        simpSolver.freeze(tr.begin_constraints(), tr.end_constraints(), true);
        simpSolver.freeze(tr.begin_inputs(), tr.end_inputs(), true);

        // Freeze bad and bad'
        simpSolver.freeze(tr.bad());
        simpSolver.freeze(prime(tr.bad()));

        return simpSolver.simplify();
    }
}
