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

// Warning: glucose defines l_True (and False and Undef) as macros, which
// messes up the minisat header (which has Minisat::l_True as an lbool).
// So you must include minisat's header first.
#include "sat/solvers/minisat/minisat/core/SolverTypes.h"
#include "sat/solvers/glucose/src/core/SolverTypes.h"

#include <cassert>

// For safety, redefine these as errors. Put the semicolon so it still
// prints the message in assignments e.g., a = l_True
#define l_True  ;static_assert(false, "l_True should not be used here");
#define l_False ;static_assert(false, "l_False should not be used here");
#define l_Undef ;static_assert(false, "l_Undef should not be used here");

namespace SAT
{
    template<typename MLit, typename MVar>
    MinisatStyleSolver<MLit, MVar>::MinisatStyleSolver()
        : m_nextVar(1),
          m_lastResult(UNKNOWN)
    { }

    template<typename MLit, typename MVar>
    Variable MinisatStyleSolver<MLit, MVar>::newVariable()
    {
        Minisat::Var gvar = createSolverVariable();
        Variable var = getNextVar();
        assert(m_varMap.find(var) == m_varMap.end());
        m_varMap[var] = gvar;
        assert(m_reverseVarMap.find(gvar) == m_reverseVarMap.end());
        m_reverseVarMap[gvar] = var;
        return var;
    }

    template<typename MLit, typename MVar>
    Variable MinisatStyleSolver<MLit, MVar>::getNextVar()
    {
        Variable next = m_nextVar;
        m_nextVar++;
        return next;
    }

    template<typename MLit, typename MVar>
    MLit MinisatStyleSolver<MLit, MVar>::toMinisat(Literal lit) const
    {
        bool neg = is_negated(lit);
        Variable var = strip(lit);
        auto it = m_varMap.find(var);
        assert(it != m_varMap.end());
        MVar v = it->second;
        return mkLit<MLit, MVar>(v, neg);
    }

    template<typename MLit, typename MVar>
    Literal MinisatStyleSolver<MLit, MVar>::fromMinisat(const MLit & lit) const
    {
        bool neg = sign<MLit>(lit);
        Minisat::Var var = toVar<MLit, MVar>(lit);
        auto it = m_reverseVarMap.find(var);
        assert(it != m_reverseVarMap.end());
        Literal l = it->second;
        return neg ? negate(l) : l;
    }

    template<typename MLit, typename MVar>
    bool MinisatStyleSolver<MLit, MVar>::solve(const Cube & assumps, Cube * crits)
    {
        bool result = doSolve(assumps, crits);
        m_lastResult = result ? SAT : UNSAT;
        return result;
    }

    template<typename MLit, typename MVar>
    bool MinisatStyleSolver<MLit, MVar>::isSAT() const
    {
        return m_lastResult == SAT;
    }

    template<typename MLit, typename MVar>
    void MinisatStyleSolver<MLit, MVar>::addClause(const Clause & cls)
    {
        m_clauses.push_back(cls);
        sendClauseToSolver(cls);
    }

    template<typename MLit, typename MVar>
    Solver::clause_iterator MinisatStyleSolver<MLit, MVar>::begin_clauses() const
    {
        return m_clauses.begin();
    }

    template<typename MLit, typename MVar>
    Solver::clause_iterator MinisatStyleSolver<MLit, MVar>::end_clauses() const
    {
        return m_clauses.end();
    }

    template<typename MLit, typename MVar>
    Solver::literal_iterator MinisatStyleSolver<MLit, MVar>::begin_trail() const
    {
        return m_trail.begin();
    }

    template<typename MLit, typename MVar>
    Solver::literal_iterator MinisatStyleSolver<MLit, MVar>::end_trail() const
    {
        return m_trail.end();
    }

    template<typename MLit, typename MVar>
    void MinisatStyleSolver<MLit, MVar>::clearClauses()
    {
        m_clauses.clear();
    }

    template<typename MLit, typename MVar>
    void MinisatStyleSolver<MLit, MVar>::addStoredClause(const Clause & cls)
    {
        m_clauses.push_back(cls);
    }

    template<typename MLit, typename MVar>
    void MinisatStyleSolver<MLit, MVar>::clearTrail()
    {
        m_trail.clear();
    }

    template<typename MLit, typename MVar>
    void MinisatStyleSolver<MLit, MVar>::addToTrail(Literal lit)
    {
        m_trail.push_back(lit);
    }

    template class MinisatStyleSolver<Minisat::Lit, Minisat::Var>;
    template class MinisatStyleSolver<Glucose::Lit, Glucose::Var>;
}

