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

#include "pme/ivc/bvc_solver.h"

#include <cassert>

namespace PME {

    BVCSolver::BVCSolver(VariableManager & varman, const TransitionRelation & tr)
        : m_vars(varman), m_tr(tr)
    { }

    void BVCSolver::setAbstraction(const std::vector<ID> & gates)
    {
        m_abstraction_gates.clear();
        m_abstraction_gates.insert(gates.begin(), gates.end());
    }

    void BVCSolver::blockSolution(const BVCSolution & soln)
    {
        for (auto & solver : m_solvers)
        {
            solver->blockSolution(soln);
        }
        m_solutions.push_back(soln);
    }

    BVCBlockResult BVCSolver::block(unsigned level)
    {
        Cube target = { m_tr.bad() };
        return block(target, level);
    }

    BVCBlockResult BVCSolver::block(const Cube & target, unsigned level)
    {
        BVCBlockResult result;
        BVCFrameSolver & solver = frameSolver(level);

        if (solver.solutionExists())
        {
            for (unsigned n = 0; n <= m_tr.numGates(); n++)
            {
                 result = solver.solve(n, target);
                 if (result.sat) { return result; }
            }

            // Getting here means a solution exists but we didn't find it
            assert(false);
        }

        return result;
    }

    BVCFrameSolver & BVCSolver::frameSolver(unsigned level)
    {
        // Construct new solvers if needed
        m_solvers.reserve(level + 1);
        for (unsigned i = m_solvers.size(); i <= level; ++i)
        {
            m_solvers.emplace_back(new BVCFrameSolver(m_vars, m_tr, i));
            for (const BVCSolution & soln : m_solutions)
            {
                m_solvers.at(i)->blockSolution(soln);
            }
        }

        // Set the abstraction for the relevant one
        m_solvers.at(level)->setAbstraction(m_abstraction_gates);
        return *m_solvers.at(level);
    }
}

