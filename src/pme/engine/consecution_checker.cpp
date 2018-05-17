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

#include "pme/engine/consecution_checker.h"

#include <cassert>
#include <sstream>
#include <algorithm>

namespace PME
{
    ConsecutionChecker::ConsecutionChecker(VariableManager & varman,
                                           const TransitionRelation & tr)
        : m_vars(varman),
          m_tr(tr),
          m_solverInited(false)
    { }

    void ConsecutionChecker::addClause(ClauseID id, const Clause & cls)
    {
        assert(m_IDToClause.find(id) == m_IDToClause.end());
        assert(m_IDToActivation.find(id) == m_IDToActivation.end());

        ID act = m_vars.getNewID(actName(id));

        assert(m_activationToID.find(act) == m_activationToID.end());

        Clause cls_sorted = cls;
        std::sort(cls_sorted.begin(), cls_sorted.end());
        m_IDToClause[id] = cls_sorted;
        m_IDToActivation[id] = act;
        m_activationToID[act] = id;

        // Add the clause to the already-initialized solver
        if (m_solverInited)
        {
            m_solver.addClause(getActivatedClause(id));
        }
    }

    std::string ConsecutionChecker::actName(ClauseID id) const
    {
        std::ostringstream ss;
        ss << "act_cls_" << id;
        return ss.str();
    }

    ID ConsecutionChecker::activation(ClauseID id) const
    {
        return m_IDToActivation.at(id);
    }

    ClauseID ConsecutionChecker::IDOfActivation(ID act) const
    {
        auto it = m_activationToID.find(act);
        assert(it != m_activationToID.end());
        return it->second;
    }

    bool ConsecutionChecker::isActivation(ID id) const
    {
        return m_activationToID.find(id) != m_activationToID.end();
    }

    bool ConsecutionChecker::supportSolve(const std::vector<ClauseID> & frame,
                                          const Clause & cls,
                                          std::vector<ClauseID> & support)
    {
        if (!m_solverInited) { initSolver(); }

        // Assume act for each clause to be included (unprimed)
        Cube assumps;
        Clause cls_sorted = cls;
        std::sort(cls_sorted.begin(), cls_sorted.end());

        bool cls_is_added = false;

        for (ClauseID id : frame)
        {
            ID act = activation(id);
            assumps.push_back(act);
            const Clause & cls_added = clauseOf(id);
            assert(std::is_sorted(cls_added.begin(), cls_added.end()));
            if (cls_added == cls_sorted) { cls_is_added = true; }
        }

        // If the clause isn't already added (because it is an element of
        // frame), then add it unprimed
        GroupID grp = GROUP_NULL;
        if (!cls_is_added)
        {
            grp = m_solver.createGroup();
            m_solver.addGroupClause(grp, cls);
        }

        // Assume ~c'
        for (ID id : cls)
        {
            assumps.push_back(negate(prime(id)));
        }

        Cube crits;
        bool sat = m_solver.groupSolve(grp, assumps, &crits);

        if (!sat)
        {
            support.clear();
            support.reserve(crits.size());
            for (ID crit : crits)
            {
                if (isActivation(crit))
                {
                    ClauseID support_cls = IDOfActivation(crit);
                    support.push_back(support_cls);
                }
            }
        }

        return !sat;
    }

    bool ConsecutionChecker::supportSolve(const std::vector<ClauseID> & frame,
                                          const ClauseID id,
                                          std::vector<ClauseID> & support)
    {
        const Clause & cls = clauseOf(id);
        return supportSolve(frame, cls, support);
    }

    bool ConsecutionChecker::supportSolve(const ClauseID id, std::vector<ClauseID> & support)
    {
        const Clause & cls = clauseOf(id);
        return supportSolve(cls, support);
    }

    bool ConsecutionChecker::supportSolve(const Clause & cls, std::vector<ClauseID> & support)
    {
        std::vector<ClauseID> frame;
        for (const auto & p : m_IDToClause)
        {
            ClauseID id = p.first;
            frame.push_back(id);
        }
        return supportSolve(frame, cls, support);
    }

    bool ConsecutionChecker::solve(const std::vector<ClauseID> & frame, const Clause & cls)
    {
        std::vector<ClauseID> support;
        return supportSolve(frame, cls, support);
    }

    bool ConsecutionChecker::solve(const std::vector<ClauseID> & frame, const ClauseID id)
    {
        const Clause & cls = clauseOf(id);
        return solve(frame, cls);
    }

    bool ConsecutionChecker::solve(const ClauseID id)
    {
        std::vector<ClauseID> support;
        return supportSolve(id, support);
    }

    bool ConsecutionChecker::solve(const Clause & cls)
    {
        std::vector<ClauseID> support;
        return supportSolve(cls, support);
    }

    const Clause & ConsecutionChecker::clauseOf(ClauseID id) const
    {
        return m_IDToClause.at(id);
    }

    void ConsecutionChecker::initSolver()
    {
        assert(!m_solverInited);
        m_solverInited = true;

        SATAdaptor simpSolver(MINISATSIMP);

        // Unroll 2 so we get primed constraints
        ClauseVec unrolled = m_tr.unroll(2);

        simpSolver.addClauses(unrolled);

        for (const auto & p : m_IDToClause)
        {
            ClauseID id = p.first;
            ID act = activation(id);
            Clause cls = getActivatedClause(id);

            simpSolver.addClause(cls);

            // Freeze activation literals
            simpSolver.freeze(act);
        }

        // Freeze latches and constraints (including primes)
        simpSolver.freeze(m_tr.begin_latches(), m_tr.end_latches(), true);
        simpSolver.freeze(m_tr.begin_constraints(), m_tr.end_constraints(), true);

        // Freeze bad and bad'
        simpSolver.freeze(m_tr.bad());
        simpSolver.freeze(prime(m_tr.bad()));

        ClauseVec simp = simpSolver.simplify();
        m_solver.addClauses(simp);
    }

    Clause ConsecutionChecker::getActivatedClause(ClauseID id) const
    {
        Clause cls = clauseOf(id);
        ID act = activation(id);
        cls.push_back(negate(act));
        return cls;
    }

    bool ConsecutionChecker::isInductive(const std::vector<ClauseID> & frame)
    {
        for (ClauseID id : frame)
        {
            if (!solve(frame, id)) { return false; }
        }

        return true;
    }
}
