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

// The MSIS algorithm implemented here was first described in "Small Inductive
// Safe Invariants" by Ivrii, Gurfinkel, and Belov

#ifndef SISI_H_INCLUDED
#define SISI_H_INCLUDED

#include "pme/minimization/minimization.h"
#include "pme/engine/consecution_checker.h"

#include <unordered_map>

namespace PME
{
    class SISI
    {
        public:
            SISI(ConsecutionChecker & solver);
            bool findSafeMIS(ClauseIDVec & vec);
            void refineNEC();
            void refineFEAS();
            ClauseIDVec bruteForceMinimize();
            void minimizeSupport(ClauseIDVec & vec, ClauseID cls);
            void addToFEAS(ClauseID id);
            void addToNEC(ClauseID id);

        private:
            ConsecutionChecker & m_indSolver;
            std::set<ClauseID> m_nec, m_feas;
    };

    class SISIMinimizer : public ProofMinimizer
    {
        public:
            SISIMinimizer(VariableManager & vars,
                          const TransitionRelation & tr,
                          const ClauseVec & proof);
            void minimize() override;

        private:
            void initSolver();
            ConsecutionChecker m_indSolver;
            SISI m_sisi;
    };
}

#endif

