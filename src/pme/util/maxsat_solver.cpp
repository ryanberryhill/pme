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

#include "pme/util/maxsat_solver.h"
#include <cassert>

namespace PME
{
    MaxSATSolver::MaxSATSolver(VariableManager & varman) :
        m_vars(varman),
        m_cardinality(varman),
        m_sat(false),
        m_dirty(false),
        m_lastCardinality(0)
    { }

    void MaxSATSolver::addClause(const Clause & cls)
    {
        m_sat = false;
        m_solver.addClause(cls);
    }

    void MaxSATSolver::addForOptimization(ID lit)
    {
        m_dirty = true;
        m_cardinality.addInput(lit);
        m_optimizationSet.insert(lit);
    }

    bool MaxSATSolver::solve()
    {
        unsigned cardinality = m_lastCardinality;

        if (!isSAT() && m_dirty)
        {
            cardinality = m_optimizationSet.size();
        }

        m_cardinality.setCardinality(cardinality + 1);
        m_solver.addClauses(m_cardinality.CNFize());

        bool sat = false;

        // Do a linear search for the maximum cardinality that is SAT
        while (!sat)
        {
            Cube assumps = m_cardinality.assumeGEq(cardinality);

            sat = m_solver.solve(assumps);

            if (!sat)
            {
                if (cardinality > 0) { cardinality--; }
                else if (cardinality == 0) { break; }
            }

        }

        m_dirty = false;
        m_sat = sat;
        m_lastCardinality = cardinality;

        return m_sat;
    }

    bool MaxSATSolver::isSAT() const
    {
        return m_sat;
    }

    ModelValue MaxSATSolver::getAssignment(ID lit) const
    {
        assert(isSAT());
        return m_solver.getAssignment(lit);
    }

    ModelValue MaxSATSolver::getAssignmentToVar(ID var) const
    {
        assert(isSAT());
        return m_solver.getAssignmentToVar(var);
    }
}

