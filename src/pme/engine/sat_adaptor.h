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

#ifndef SAT_ADAPTOR_H_INCLUDED
#define SAT_ADAPTOR_H_INCLUDED

#include "pme/pme.h"
#include "sat/sat.h"

namespace PME
{
    typedef SAT::ModelValue ModelValue;

    class SATAdaptor
    {
        public:
            SATAdaptor();
            void introduceVariable(ID var);
            void addClause(const Clause & cls);
            void addClauses(const ClauseVec & vec);
            bool solve();
            bool solve(const Cube & assumps);
            bool isSAT() const;
            ModelValue getAssignment(ID lit) const;
            ModelValue getAssignmentToVar(ID var) const;

        private:
            std::unique_ptr<SAT::Solver> m_solver;
            std::unordered_map<ID, SAT::Variable> m_IDToSATMap;
            std::unordered_map<SAT::Variable, ID> m_SATToIDMap;

            SAT::Variable SATVarOf(ID id) const;
            ID IDOf(SAT::Variable var) const;
            SAT::Literal toSAT(ID id) const;
    };
}

#endif

