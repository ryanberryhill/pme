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

#include "pme/ivc/ivc_bf.h"
#include "pme/ic3/ic3_solver.h"

#include <algorithm>

namespace PME {
    IVCBFFinder::IVCBFFinder(VariableManager & varman,
                             const TransitionRelation & tr)
        : IVCFinder(varman, tr)
    { }

    std::ostream & IVCBFFinder::log(int verbosity) const
    {
        return IVCFinder::log(LOG_IVCBF, verbosity);
    }

    void IVCBFFinder::shrink(Seed & seed, MapSolver * map)
    {
        // TODO: incrementality

        if (GlobalState::isRandom())
        {
            std::shuffle(seed.begin(), seed.end(), GlobalState::random());
        }

        for (size_t i = 0; i < seed.size(); )
        {
            Seed seed_copy = seed;
            seed_copy.erase(seed_copy.begin() + i);
            if (map && !map->checkSeed(seed_copy))
            {
                log(4) << "Cannot remove " << seed[i] << std::endl;
                ++i;
                // Obviously it's not good to touch UIVC stats here, but we
                // do it anyway
                stats().uivc_map_checks++;
            }
            else if (isSafe(seed_copy))
            {
                log(4) << "Successfully removed " << seed[i] << std::endl;
                seed.erase(seed.begin() + i);
            }
            else
            {
                log(4) << "Cannot remove " << seed[i] << std::endl;
                ++i;
            }
        }
    }

    bool IVCBFFinder::isSafe(const Seed & seed)
    {
        SafetyProof proof;
        return isSafe(seed, proof);
    }

    bool IVCBFFinder::isSafe(const Seed & seed, SafetyProof & proof)
    {
        // TODO: incremental debugging-based version
        // TODO: non-incremental hybrid BMC/IC3 version
        TransitionRelation partial(tr(), seed);
        IC3::IC3Solver ic3(vars(), partial);

        SafetyResult safe = ic3.prove();

        if (safe.result == SAFE)
        {
            proof = safe.proof;
            return true;
        }
        else
        {
            return false;
        }
    }

    void IVCBFFinder::doFindIVCs()
    {
        Seed seed(tr().begin_gate_ids(), tr().end_gate_ids());
        shrink(seed);
        addMIVC(seed);
    }
}

