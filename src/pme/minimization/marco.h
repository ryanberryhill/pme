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

// MARCO is presented in "Enumerating Infeasibility: Finding Multiple MUSes Quickly"
// by Liffiton and Malik

#ifndef MARCO_H_INCLUDED
#define MARCO_H_INCLUDED

#include "pme/minimization/minimization.h"
#include "pme/engine/consecution_checker.h"
#include "pme/util/maxsat_solver.h"

#include <unordered_map>

namespace PME
{
    class MARCOMinimizer : public ProofMinimizer
    {
        public:
            MARCOMinimizer(VariableManager & vars,
                           const TransitionRelation & tr,
                           const ClauseVec & proof,
                           GlobalState & gs = g_null_gs);
            void minimize() override;
        protected:
            std::ostream & log(int verbosity) const override;
        private:
            typedef ClauseIDVec Seed;
            typedef std::pair<bool, Seed> UnexploredResult;

            UnexploredResult getUnexplored();
            void initSolvers();
            ID seedVarOf(ClauseID cls) const;
            bool isSIS(const Seed & seed);
            void grow(Seed & seed);
            void shrink(Seed & seed);
            bool findSafeMIS(Seed & seed);
            void blockUp(const Seed & seed);
            void blockDown(const Seed & seed);

            void updateProofs(const Seed & seed);

            VariableManager m_vars;
            MaxSATSolver m_seedSolver;
            ConsecutionChecker m_indSolver;
            std::unordered_map<ClauseID, ID> m_clauseToSeedVar;

            Seed m_smallestProof;
    };
}

#endif

