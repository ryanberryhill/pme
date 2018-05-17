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

#include "sat/sat.h"
#include "sat/solvers/minisat/minisat/core/SolverTypes.h"
#include "sat/solvers/minisat/minisat/core/Solver.h"
#include "sat/solvers/minisat/minisat/simp/SimpSolver.h"

#include <memory>
#include <cassert>
#include <set>

namespace SAT
{
    template <> Minisat::Lit mkLit(Minisat::Var v, bool neg)
    {
        return Minisat::mkLit(v, neg);
    }

    template<> bool sign(Minisat::Lit lit)
    {
        return Minisat::sign(lit);
    }

    template<> Minisat::Var toVar(Minisat::Lit lit)
    {
        return Minisat::var(lit);
    }

    //
    // MinisatSolver
    //

    MinisatSolver::MinisatSolver()
        : m_solver(new Minisat::Solver)
    { }

    MinisatSolver::~MinisatSolver()
    { }

    Variable MinisatSolver::createSolverVariable()
    {
        return m_solver->newVar();
    }

    void MinisatSolver::sendClauseToSolver(const Clause & cls)
    {
        Minisat::vec<Minisat::Lit> mcls;
        for (Literal lit : cls)
        {
            mcls.push(toMinisat(lit));
        }
        m_solver->addClause(mcls);
    }

    bool MinisatSolver::doSolve(const Cube & assumps, Cube * crits)
    {
        Minisat::vec<Minisat::Lit> mvec;
        for (Literal lit : assumps)
        {
            mvec.push(toMinisat(lit));
        }

        bool result = m_solver->solve(mvec);

        clearTrail();
        for (auto it = m_solver->trailBegin(); it != m_solver->trailEnd(); ++it)
        {
            const Minisat::Lit & lit = *it;
            Literal l = fromMinisat(lit);
            addToTrail(l);
        }

        if (!result && crits != nullptr)
        {
            crits->clear();
            std::set<Literal> assump_set(assumps.begin(), assumps.end());
            const Minisat::LSet & conflict = m_solver->conflict;
            for (int i = 0; i < conflict.size(); ++i)
            {
                Minisat::Lit mlit = conflict[i];
                Literal lit = fromMinisat(mlit);

                // We need to negate the literal to get the original assumption
                // because it came from the conflict clause
                lit = negate(lit);
                if (assump_set.count(lit) > 0)
                {
                    crits->push_back(lit);
                }
            }
        }

        return result;
    }

    ModelValue MinisatSolver::getAssignment(Variable v) const
    {
        assert(isSAT());
        Minisat::Lit lit = toMinisat(v);
        Minisat::lbool value = m_solver->modelValue(lit);
        if (value == Minisat::l_True) { return TRUE; }
        else if (value == Minisat::l_False) { return FALSE; }
        else { assert(value == Minisat::l_Undef); return UNDEF; }
    }

    //
    // MinisatSimplifyingSolver
    //

    MinisatSimplifyingSolver::MinisatSimplifyingSolver()
        : m_solver(new Minisat::SimpSolver)
    { }

    MinisatSimplifyingSolver::~MinisatSimplifyingSolver()
    { }

    Variable MinisatSimplifyingSolver::createSolverVariable()
    {
        return m_solver->newVar();
    }

    void MinisatSimplifyingSolver::sendClauseToSolver(const Clause & cls)
    {
        Minisat::vec<Minisat::Lit> mcls;
        for (Literal lit : cls)
        {
            mcls.push(toMinisat(lit));
        }
        m_solver->addClause(mcls);
    }

    bool MinisatSimplifyingSolver::doSolve(const Cube & assumps, Cube * crits)
    {
        Minisat::vec<Minisat::Lit> mvec;
        for (Literal lit : assumps)
        {
            mvec.push(toMinisat(lit));
        }

        // Don't do simplification - only eliminate() should do it
        bool result = m_solver->solve(mvec, false);

        clearTrail();
        for (auto it = m_solver->trailBegin(); it != m_solver->trailEnd(); ++it)
        {
            const Minisat::Lit & lit = *it;
            Literal l = fromMinisat(lit);
            addToTrail(l);
        }

        if (!result && crits != nullptr)
        {
            crits->clear();
            std::set<Literal> assump_set(assumps.begin(), assumps.end());
            const Minisat::LSet & conflict = m_solver->conflict;
            for (int i = 0; i < conflict.size(); ++i)
            {
                Minisat::Lit mlit = conflict[i];
                Literal lit = fromMinisat(mlit);

                // We need to negate the literal to get the original assumption
                // because it came from the conflict clause
                lit = negate(lit);
                if (assump_set.count(lit) > 0)
                {
                    crits->push_back(lit);
                }
            }
        }

        return result;
    }

    ModelValue MinisatSimplifyingSolver::getAssignment(Variable v) const
    {
        assert(isSAT());
        Minisat::Lit lit = toMinisat(v);
        Minisat::lbool value = m_solver->modelValue(lit);
        if (value == Minisat::l_True) { return TRUE; }
        else if (value == Minisat::l_False) { return FALSE; }
        else { assert(value == Minisat::l_Undef); return UNDEF; }
    }

    void MinisatSimplifyingSolver::freeze(Variable v)
    {
        Minisat::Lit lit = toMinisat(v);
        Minisat::Var mv = Minisat::var(lit);
        m_solver->freezeVar(mv);
    }

    void MinisatSimplifyingSolver::eliminate()
    {
        bool ok = m_solver->eliminate();
        assert(ok);

        clearClauses();
        for (auto it = m_solver->clausesBegin(); it != m_solver->clausesEnd(); ++it)
        {
            Clause internal_cls;

            const Minisat::Clause & cls = *it;
            for (size_t i = 0; i < (size_t) cls.size(); ++i)
            {
                const Minisat::Lit & lit = cls[i];
                Literal l = fromMinisat(lit);
                internal_cls.push_back(l);
            }

            addStoredClause(internal_cls);
        }

        clearTrail();
        for (auto it = m_solver->trailBegin(); it != m_solver->trailEnd(); ++it)
        {
            const Minisat::Lit & lit = *it;
            Literal l = fromMinisat(lit);
            addToTrail(l);
        }
    }
}

