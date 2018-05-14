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

#ifndef MINIMIZATION_H_INCLUDED
#define MINIMIZATION_H_INCLUDED

#include "pme/pme.h"
#include "pme/id.h"
#include "pme/engine/transition_relation.h"

namespace PME
{
    typedef ClauseVec::const_iterator clause_iterator;

    class ProofMinimizer
    {
        public:
            ProofMinimizer(VariableManager & vars,
                           const TransitionRelation & tr,
                           const ClauseVec & proof);
            virtual ~ProofMinimizer() { }

            virtual void minimize() = 0;

            clause_iterator begin_minproof() const;
            clause_iterator end_minproof() const;

        protected:
            const TransitionRelation & tr() const;
            VariableManager & vars();

            const ClauseVec & proof() const;

            void clearMinimizedProof();
            void addToMinimizedProof(ClauseID id);
            size_t numClauses() const;
            const Clause & getClause(ClauseID id) const;

        private:
            const TransitionRelation & m_tr;
            VariableManager & m_vars;
            const ClauseVec & m_proof;
            ClauseVec m_minimizedProof;
    };

    class DummyMinimizer : public ProofMinimizer
    {
        public:
            DummyMinimizer(VariableManager & vars,
                           const TransitionRelation & tr,
                           const ClauseVec & proof);
            void minimize() override;
    };
}

#endif

