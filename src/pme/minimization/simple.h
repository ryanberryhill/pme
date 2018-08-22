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

#ifndef SIMPLE_MINIMIZER_H_INCLUDED
#define SIMPLE_MINIMIZER_H_INCLUDED

#include "pme/minimization/minimization.h"
#include "pme/engine/consecution_checker.h"

#include <vector>

namespace PME {
    class SimpleMinimizer : public ProofMinimizer
    {
        public:
            SimpleMinimizer(VariableManager & vars,
                            const TransitionRelation & tr,
                            const ClauseVec & proof);
            void minimize() override;

        protected:
            std::ostream & log(int verbosity) const override;
        private:
            void initSolver();
            std::vector<ClauseID> allClauses() const { return m_allClauses; }
            std::vector<ClauseID> computeSupport(ClauseID id);

            ConsecutionChecker m_solver;
            std::vector<ClauseID> m_allClauses;
    };
}

#endif

