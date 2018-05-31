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

#include "pme/engine/collapse_set_finder.h"

#include <cassert>
#include <sstream>

namespace PME
{

    CollapseSetFinder::CollapseSetFinder(VariableManager & varman,
                                         const TransitionRelation & tr,
                                         GlobalState & gs)
        : m_vars(varman),
          m_tr(tr),
          m_gs(gs),
          m_solver(varman),
          m_solverInited(false)
    { }

    Clause CollapseSetFinder::getActivatedClause(ClauseID id) const
    {
        Clause cls = m_clausedb.clauseOf(id);
        ID act = activation(id);
        cls.push_back(negate(act));
        return cls;
    }

    void CollapseSetFinder::addClause(ClauseID id, const Clause & cls)
    {
        // Clauses can only be added at the beginning before solving
        assert(!m_solverInited);

        ID act = m_vars.getNewID(activationName(id));
        ID check = m_vars.getNewID(checkName(id));

        m_clausedb.addClause(id, act, cls, check);
    }

    std::string CollapseSetFinder::checkName(ClauseID id) const
    {
        std::ostringstream ss;
        ss << "check_cls_" << id;
        return ss.str();
    }

    std::string CollapseSetFinder::activationName(ClauseID id) const
    {
        std::ostringstream ss;
        ss << "act_cls_" << id;
        return ss.str();
    }

    void CollapseSetFinder::initSolver()
    {
        assert(!m_solverInited);
        m_solverInited = true;

        // Unroll 2 so we get primed constraints
        ClauseVec unrolled = m_tr.unroll(2);
        for (const auto & p : m_clausedb)
        {
            ClauseID id = p.first;
            Clause cls = getActivatedClause(id);

            unrolled.push_back(cls);
        }

        if (m_gs.opts.simplify)
        {
            SATAdaptor simpSolver(MINISATSIMP);

            simpSolver.addClauses(unrolled);

            for (const auto & p : m_clausedb)
            {
                ClauseID id = p.first;
                ID act = activation(id);

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
        else
        {
            m_solver.addClauses(unrolled);
        }

        // Add act for maxsat minimization.
        // Activated clause c is (c V ~act) so maximizing act maximizes the
        // number of clauses not removed (minimizes the number of clauses
        // removed). SAT assignments therefore correspond to minimal collapse
        // sets.
        for (const auto & p : m_clausedb)
        {
            ClauseID id = p.first;
            ID act = activation(id);
            m_solver.addForOptimization(act);
        }
    }

    bool CollapseSetFinder::find(ClauseID id)
    {
        CollapseSet collapse;
        return find(id, collapse);
    }

    bool CollapseSetFinder::find(ClauseID id, CollapseSet & collapse)
    {
        if (!m_solverInited) { initSolver(); }
        Clause c = m_clausedb.clauseOf(id);
        ID act = activation(id);
        ID check = checking(id);

        Cube negcprime = primeVec(negateVec(c));

        // Assume ~c'
        Cube assumps = negcprime;
        // and c
        assumps.push_back(act);
        // and checking(c) where checking(c) is a special per-clause variable
        // we assert whenever we're seeking a collapse set of c. When blocking
        // we add ~checking(c) to the blocking clause so we can still find the
        // same collapse set for another clause if applicable.
        assumps.push_back(check);

        bool sat = m_solver.solve(assumps);

        if (!sat) { return false; }

        collapse.clear();

        for (const auto & p : m_clausedb)
        {
            ClauseID id = p.first;

            if (isInactive(id))
            {
                collapse.push_back(id);
            }
        }

        assert(!collapse.empty());
        return true;
    }

    bool CollapseSetFinder::findAndBlock(ClauseID id, CollapseSet & collapse)
    {
        bool found = find(id, collapse);

        Clause block;
        if (found)
        {
            for (ClauseID id : collapse)
            {
                ID act = activation(id);
                block.push_back(act);
            }

            assert(block.size() == collapse.size());

            // The block should only apply to this clause, so we add the negation
            // of the check variable
            block.push_back(negate(checking(id)));
            m_solver.addClause(block);
        }

        return found;
    }

    bool CollapseSetFinder::isInactive(ClauseID id) const
    {
        assert(m_solver.isSAT());
        ID act = activation(id);

        return m_solver.getAssignment(act) == SAT::FALSE;
    }

    ID CollapseSetFinder::activation(ClauseID id) const
    {
        return m_clausedb.activationOfID(id);
    }

    ID CollapseSetFinder::checking(ClauseID id) const
    {
        return m_clausedb.getData(id);
    }
}

