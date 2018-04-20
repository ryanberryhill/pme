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
#include "sat/solvers/glucose/src/core/SolverTypes.h"
#include "sat/solvers/glucose/src/core/Solver.h"

#include <memory>
#include <cassert>

namespace SAT
{
    Literal negate(Literal lit)
    {
        return -lit;
    }

    bool is_negated(Literal lit)
    {
        return lit < 0;
    }

    Variable strip(Literal lit)
    {
        return is_negated(lit) ? -lit : lit;
    }

    Solver::Solver() 
        : m_solver(new Glucose::Solver),
          m_nextVar(1),
          m_lastResult(UNKNOWN)
    { }

    Solver::~Solver()
    { }

    Variable Solver::newVariable() 
    {
        Glucose::Var gvar = m_solver->newVar();
        Variable var = getNextVar();
        m_varMap[var] = gvar;
        return var;
    }
            
    Variable Solver::getNextVar()
    {
        Variable next = m_nextVar;
        m_nextVar++;
        return next;
    }

    void Solver::addClause(const Clause& cls)
    {
        Glucose::vec<Glucose::Lit> gcls;
        for (Literal lit : cls)
        {
            gcls.push(toGlucose(lit));
        }
        m_solver->addClause(gcls);
    }

    Glucose::Lit Solver::toGlucose(Literal lit) const
    {
        bool neg = is_negated(lit);
        Variable var = strip(lit);
        auto it = m_varMap.find(var);
        assert(it != m_varMap.end());
        Glucose::Var v = it->second;
        return Glucose::mkLit(v, neg);
    }
            
    bool Solver::solve(const Cube& assumps) 
    {
        Glucose::vec<Glucose::Lit> gvec;
        for (Literal lit : assumps)
        {
            gvec.push(toGlucose(lit));
        }

        bool result = m_solver->solve(gvec);
        m_lastResult = result ? SAT : UNSAT;
        return result;
    }
            
    bool Solver::isSAT() const 
    {
        return m_lastResult == SAT;
    }

    ModelValue Solver::getAssignment(Variable v) const
    {
        assert(isSAT());
        Glucose::Lit lit = toGlucose(v);
        Glucose::lbool value = m_solver->modelValue(lit);
        if (value == l_True) { return TRUE; }
        else if (value == l_False) { return FALSE; }
        else { assert(value == l_Undef); return UNDEF; }
    }
}

