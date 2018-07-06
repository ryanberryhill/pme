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
#ifndef PROOF_CHECKER_H_INCLUDED
#define PROOF_CHECKER_H_INCLUDED

#include "pme/pme.h"
#include "pme/engine/global_state.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/sat_adaptor.h"

namespace PME
{
    class ProofChecker
    {
        public:
            ProofChecker(const TransitionRelation & tr,
                         const ClauseVec & proof,
                         GlobalState & gs);
            bool checkInduction();
            bool checkInitiation();
            bool checkSafety();
            bool checkInductiveStrengthening();
            bool checkProof();

        private:
            SATAdaptor m_indSolver, m_initSolver;
            const TransitionRelation & m_tr;
            const ClauseVec & m_proof;
            GlobalState & m_gs;
    };
}

#endif

