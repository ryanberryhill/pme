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

#ifndef CONSECUTION_CHECKER_H_INCLUDED
#define CONSECUTION_CHECKER_H_INCLUDED

#include "pme/engine/global_state.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/sat_adaptor.h"
#include "pme/util/clause_database.h"

#include <unordered_map>

namespace PME
{
    class ConsecutionChecker
    {
        public:
            ConsecutionChecker(VariableManager & varman,
                               const TransitionRelation & tr);
            void addClause(ClauseID id, const Clause & cls);

            bool isInductive(const std::vector<ClauseID> & frame);

            // Solve relative to a subset of clauses and return a
            // (non-minimal) support set
            bool supportSolve(const std::vector<ClauseID> & frame,
                       const Clause & cls,
                       std::vector<ClauseID> & support);
            bool supportSolve(const std::vector<ClauseID> & frame,
                       const ClauseID id,
                       std::vector<ClauseID> & support);
            bool supportSolve(const ClauseID id, std::vector<ClauseID> & support);
            bool supportSolve(const Clause & cls, std::vector<ClauseID> & support);

            // Solve relative to a subset of clauses (frame)
            bool solve(const std::vector<ClauseID> & frame, const Clause & cls);
            bool solve(const std::vector<ClauseID> & frame, const ClauseID id);

            // With no frame we include every clause
            bool solve(const ClauseID id);
            bool solve(const Clause & cls);

            const Clause & clauseOf(ClauseID id) const;

            ModelValue safeGetAssignment(ID lit) const;
            ModelValue getAssignment(ID lit) const;

        private:
            std::string actName(ClauseID id) const;
            void initSolver();
            ID activation(ClauseID id) const;
            ClauseID IDOfActivation(ID act) const;
            bool isActivation(ID id) const;
            Clause getActivatedClause(ClauseID id) const;

            VariableManager & m_vars;
            const TransitionRelation & m_tr;
            bool m_solverInited;
            SATAdaptor m_solver;
            ClauseDatabase m_clausedb;
    };
}

#endif

