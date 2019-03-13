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

// CAMUS is presented in "On Finding All Minimally Unsatisfiable Subformulas"
// by Liffiton and Sakallah and CAMSIS is presented in "Finding all Minimal
// Safe Inductive Sets" by Berryhill, Ivrii, and Veneris

#ifndef CAMSIS_H_INCLUDED
#define CAMSIS_H_INCLUDED

#include "pme/minimization/minimization.h"
#include "pme/engine/consecution_checker.h"
#include "pme/engine/collapse_set_finder.h"

#include <vector>

namespace PME
{
    class CAMSISMinimizer : public ProofMinimizer
    {
        public:
            CAMSISMinimizer(VariableManager & vars,
                            const TransitionRelation & tr,
                            const ClauseVec & proof);
        protected:
            void doMinimize() override;
            std::ostream & log(int verbosity) const override;
        private:
            typedef std::vector<ClauseID> MSIS;
            typedef std::vector<ClauseID> MSISCandidate;

            ID createClauseSelect(ClauseID c);
            Clause collapseClause(ClauseID c, const CollapseSet & collapse) const;

            bool findCollapse(ClauseID c, CollapseSet & collapse);
            bool attemptRefinement(ClauseID c);
            bool extractCandidate(MSIS & msis);
            void blockMSIS(const MSIS & msis);
            bool isSIS(const MSISCandidate & candidate, std::vector<ClauseID> & unsupported);

            void recordMSIS(const MSIS & msis);

            void naiveMinimize();
            void abstractionRefinementMinimize();

            ConsecutionChecker m_cons;
            CollapseSetFinder m_collapse_finder;
            PBOMaxSATSolver m_solver;
            ClauseDatabase m_clausedb;
            VariableManager m_vars;
    };
}

#endif

