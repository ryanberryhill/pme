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

namespace SAT
{
    //
    // MinisatSolver
    //

    MinisatSolver::MinisatSolver()
        : m_solver(new Minisat::Solver),
          m_nextVar(1),
          m_lastResult(UNKNOWN)
    { }

    MinisatSolver::~MinisatSolver()
    { }

    Variable MinisatSolver::newVariable()
    {
        Minisat::Var gvar = m_solver->newVar();
        Variable var = getNextVar();
        m_varMap[var] = gvar;
        return var;
    }

    Variable MinisatSolver::getNextVar()
    {
        Variable next = m_nextVar;
        m_nextVar++;
        return next;
    }

    void MinisatSolver::sendClauseToSolver(const Clause& cls)
    {
        Minisat::vec<Minisat::Lit> gcls;
        for (Literal lit : cls)
        {
            gcls.push(toMinisat(lit));
        }
        m_solver->addClause(gcls);
    }

    Minisat::Lit MinisatSolver::toMinisat(Literal lit) const
    {
        bool neg = is_negated(lit);
        Variable var = strip(lit);
        auto it = m_varMap.find(var);
        assert(it != m_varMap.end());
        Minisat::Var v = it->second;
        return Minisat::mkLit(v, neg);
    }

    bool MinisatSolver::solve(const Cube& assumps)
    {
        Minisat::vec<Minisat::Lit> gvec;
        for (Literal lit : assumps)
        {
            gvec.push(toMinisat(lit));
        }

        bool result = m_solver->solve(gvec);
        m_lastResult = result ? SAT : UNSAT;
        return result;
    }

    bool MinisatSolver::isSAT() const
    {
        return m_lastResult == SAT;
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
        : m_solver(new Minisat::SimpSolver),
          m_nextVar(1),
          m_lastResult(UNKNOWN)
    { }

    MinisatSimplifyingSolver::~MinisatSimplifyingSolver()
    { }

    Variable MinisatSimplifyingSolver::newVariable()
    {
        Minisat::Var gvar = m_solver->newVar();
        Variable var = getNextVar();
        m_varMap[var] = gvar;
        assert(m_reverseVarMap.find(gvar) == m_reverseVarMap.end());
        m_reverseVarMap[gvar] = var;
        return var;
    }

    Variable MinisatSimplifyingSolver::getNextVar()
    {
        Variable next = m_nextVar;
        m_nextVar++;
        return next;
    }

    void MinisatSimplifyingSolver::sendClauseToSolver(const Clause& cls)
    {
        Minisat::vec<Minisat::Lit> gcls;
        for (Literal lit : cls)
        {
            gcls.push(toMinisat(lit));
        }
        m_solver->addClause(gcls);
    }

    Minisat::Lit MinisatSimplifyingSolver::toMinisat(Literal lit) const
    {
        bool neg = is_negated(lit);
        Variable var = strip(lit);
        auto it = m_varMap.find(var);
        assert(it != m_varMap.end());
        Minisat::Var v = it->second;
        return Minisat::mkLit(v, neg);
    }

    Literal MinisatSimplifyingSolver::fromMinisat(const Minisat::Lit& lit) const
    {
        bool neg = Minisat::sign(lit);
        Minisat::Var var = Minisat::var(lit);
        auto it = m_reverseVarMap.find(var);
        assert(it != m_reverseVarMap.end());
        Literal l = it->second;
        return neg ? negate(l) : l;
    }

    bool MinisatSimplifyingSolver::solve(const Cube& assumps)
    {
        Minisat::vec<Minisat::Lit> gvec;
        for (Literal lit : assumps)
        {
            gvec.push(toMinisat(lit));
        }

        // Do not simplify
        // The current interface is such that eliminate() does simplification
        // but solve does not
        bool result = m_solver->solve(gvec, false);
        m_lastResult = result ? SAT : UNSAT;
        return result;
    }

    bool MinisatSimplifyingSolver::isSAT() const
    {
        return m_lastResult == SAT;
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
    }

}

