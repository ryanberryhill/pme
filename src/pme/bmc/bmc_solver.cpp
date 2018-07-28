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

#include "pme/bmc/bmc_solver.h"

#include <cassert>

namespace PME { namespace BMC {

    BMCSolver::BMCSolver(VariableManager & varman,
                         const TransitionRelation & tr,
                         GlobalState & gs)
        : m_vars(varman),
          m_tr(tr),
          m_gs(gs),
          m_numFrames(0),
          m_solverInited(false)
    { }

    void BMCSolver::restrictInitialStates(const ClauseVec & vec)
    {
        for (const Clause & cls : vec)
        {
            restrictInitialStates(cls);
        }
    }

    void BMCSolver::restrictInitialStates(const Clause & cls)
    {
        m_init_constraints.push_back(cls);
        if (m_solverInited) { m_solver.addClause(cls); }
    }

    void BMCSolver::clearRestrictions()
    {
        m_init_constraints.clear();
        m_solver.reset();
        m_solverInited = false;
        m_numFrames = 0;
    }

    SafetyResult BMCSolver::solve(unsigned k_max)
    {
        Cube assumps;
        return solve(k_max, assumps);
    }

    SafetyResult BMCSolver::solve(unsigned k_max, const Cube & assumps)
    {
        for (unsigned k = 0; k <= k_max; ++k)
        {
            SafetyResult kresult = solveAtK(k, assumps);
            if (kresult.result == UNSAFE)
            {
                return kresult;
            }
        }

        SafetyResult result;
        result.result = UNKNOWN;
        return result;
    }

    SafetyResult BMCSolver::solveAtK(unsigned k)
    {
        Cube assumps;
        return solveAtK(k, assumps);
    }

    SafetyResult BMCSolver::solveAtK(unsigned k, const Cube & assumps)
    {
        SafetyResult result;

        ID bad = prime(m_tr.bad(), k);

        if (k >= m_numFrames) { unroll(k + 1); }

        Cube kassumps = assumps;
        kassumps.push_back({bad});
        bool sat = m_solver.solve(kassumps);
        if (sat)
        {
            result.result = UNSAFE;
            result.cex = extractTrace(k);
            return result;
        }

        result.result = UNKNOWN;
        return result;
    }

    void BMCSolver::unroll(unsigned n)
    {
        // TODO simplification
        if (!m_solverInited)
        {
            initSolver();
        }

        assert(n > m_numFrames);

        for (unsigned k = m_numFrames; k < n; ++k)
        {
            m_solver.addClauses(m_tr.unrollFrame(k));
        }

        m_numFrames = n;
    }

    void BMCSolver::initSolver()
    {
        // TODO simplification
        assert(!m_solverInited);

        m_solver.addClauses(m_tr.initState());
        m_solver.addClauses(m_tr.unrollFrame(0));
        m_solver.addClauses(m_init_constraints);

        m_numFrames = 0;
        m_solverInited = true;
    }

    SafetyCounterExample BMCSolver::extractTrace(unsigned k)
    {
        assert(m_solver.isSAT());
        SafetyCounterExample cex;

        Cube input_vars(m_tr.begin_inputs(), m_tr.end_inputs());
        Cube latch_vars(m_tr.begin_latches(), m_tr.end_latches());

        for (unsigned i = 0; i <=k; ++i)
        {
            Cube inputs = extract(input_vars, i);
            Cube latches = extract(latch_vars, i);

            cex.push_back(Step(inputs, latches));
        }

        return cex;
    }

    Cube BMCSolver::extract(Cube vars, unsigned k)
    {
        assert(m_solver.isSAT());

        Cube extracted;
        for (ID id : vars)
        {
            ID lit = prime(id, k);
            ModelValue asgn = m_solver.safeGetAssignmentToVar(lit);
            if (asgn != SAT::UNDEF)
            {
                ID asgn_id = (asgn == SAT::TRUE ? lit : negate(lit));
                asgn_id = unprime(asgn_id);
                extracted.push_back(asgn_id);
            }
        }

        return extracted;
    }
} }

