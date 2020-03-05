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
#include <set>

// HACK: for seed noise tests. This variable is in main.c and set
// by a command line option. If 0, no randomness.
extern unsigned g_sat_seed;

namespace SAT
{
    template <> Glucose::Lit mkLit(Glucose::Var v, bool neg)
    {
        return Glucose::mkLit(v, neg);
    }

    template<> bool sign(Glucose::Lit lit)
    {
        return Glucose::sign(lit);
    }

    template<> Glucose::Var toVar(Glucose::Lit lit)
    {
        return Glucose::var(lit);
    }

    GlucoseSolver::GlucoseSolver()
        : m_solver(new Glucose::Solver)
    {
        // HACK: for seed noise tests
        if (g_sat_seed != 0)
        {
            m_solver->random_seed = g_sat_seed;
            m_solver->rnd_pol = true;
            m_solver->rnd_init_act = true;
            m_solver->randomizeFirstDescent = true;
        }
    }

    GlucoseSolver::~GlucoseSolver()
    { }

    Variable GlucoseSolver::createSolverVariable()
    {
        return m_solver->newVar();
    }

    void GlucoseSolver::sendClauseToSolver(const Clause & cls)
    {
        Glucose::vec<Glucose::Lit> mcls;
        for (Literal lit : cls)
        {
            mcls.push(toMinisat(lit));
        }
        m_solver->addClause(mcls);
    }

    bool GlucoseSolver::doSolve(const Cube & assumps, Cube * crits)
    {
        Glucose::vec<Glucose::Lit> mvec;
        for (Literal lit : assumps)
        {
            mvec.push(toMinisat(lit));
        }

        bool result = m_solver->solve(mvec);

        if (!result && crits != nullptr)
        {
            crits->clear();
            std::set<Literal> assump_set(assumps.begin(), assumps.end());
            const Glucose::vec<Glucose::Lit> & conflict = m_solver->conflict;
            for (int i = 0; i < conflict.size(); ++i)
            {
                Glucose::Lit glit = conflict[i];
                Literal lit = fromMinisat(glit);

                // We need to negate the literal to get the original assumption
                // because it came from the conflict clause.
                lit = negate(lit);
                if (assump_set.count(lit) > 0)
                {
                    crits->push_back(lit);
                }
            }
        }

        return result;
    }

    ModelValue GlucoseSolver::getAssignment(Variable v) const
    {
        assert(isSAT());
        Glucose::Lit lit = toMinisat(v);
        Glucose::lbool value = m_solver->modelValue(lit);
        if (value == l_True) { return TRUE; }
        else if (value == l_False) { return FALSE; }
        else { assert(value == l_Undef); return UNDEF; }
    }
}

