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

#include <limits>

namespace PME {
    const unsigned CARDINALITY_INF = std::numeric_limits<unsigned>::max();

    BVCSolver::BVCSolver(const TransitionRelation & tr)
        : m_tr(tr),
          m_solver_inited(false),
          m_cardinality(CARDINALITY_INF),
          m_abstraction_frames(0)
    { }

    void BVCSolver::initSolver()
    {
        m_solver_inited = true;
    }

    void BVCSolver::setAbstraction(std::vector<ID> gates)
    {
        // TODO: if gates is a superset of m_abstraction_gates
        // we can just add the new gates to the solver.
        // Otherwise, we might have to clear and start over.
        m_abstraction_gates.clear();
        m_abstraction_gates.insert(gates.begin(), gates.end());
        // TODO: update solver
    }

    void BVCSolver::setAbstractionFrames(unsigned n)
    {
        m_abstraction_frames = n;
    }

    void BVCSolver::setCardinality(unsigned n)
    {
        m_cardinality = n;
        // TODO: update cardinality in solver if necessary
    }

    void BVCSolver::clearCardinality()
    {
        m_cardinality = CARDINALITY_INF;
    }

    void BVCSolver::blockSolution(const std::vector<ID> & soln)
    {
    }
}

