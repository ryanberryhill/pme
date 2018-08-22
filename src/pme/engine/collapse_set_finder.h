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

#ifndef COLLAPSE_SET_FINDER_H_INCLUDED
#define COLLAPSE_SET_FINDER_H_INCLUDED

#include "pme/engine/global_state.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/util/maxsat_solver.h"
#include "pme/util/clause_database.h"

namespace PME
{
    typedef std::vector<ClauseID> CollapseSet;

    class CollapseSetFinder
    {
        public:
            CollapseSetFinder(VariableManager & varman,
                              const TransitionRelation & tr);
            void initSolver();
            void addClause(ClauseID id, const Clause & cls);
            bool findAndBlock(ClauseID id, CollapseSet & collapse);
            bool find(ClauseID id, CollapseSet & collapse);
            bool find(ClauseID id);

        private:
            bool isInactive(ClauseID id) const;
            Clause getActivatedClause(ClauseID id) const;
            std::string activationName(ClauseID id) const;
            std::string checkName(ClauseID id) const;
            ID activation(ClauseID id) const;
            ID checking(ClauseID id) const;

            VariableManager & m_vars;
            const TransitionRelation & m_tr;
            DualActivationClauseDatabase m_clausedb;
            MaxSATSolver m_solver;
            bool m_solverInited;
    };
}

#endif

