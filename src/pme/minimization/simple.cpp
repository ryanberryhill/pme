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

#include "pme/minimization/simple.h"
#include "pme/util/find_minimal_support.h"

#include <vector>
#include <queue>
#include <cassert>
#include <algorithm>

namespace PME {
    SimpleMinimizer::SimpleMinimizer(VariableManager & vars,
                                     const TransitionRelation & tr,
                                     const ClauseVec & proof)
        : ProofMinimizer(vars, tr, proof),
          m_solver(vars, tr)
    {
        initSolver();
    }

    std::ostream & SimpleMinimizer::log(int verbosity) const
    {
        return ProofMinimizer::log(LOG_SIMPLEMIN, verbosity);
    }

    void SimpleMinimizer::initSolver()
    {
        m_allClauses.reserve(numClauses());
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            m_allClauses.push_back(id);

            const Clause & cls = clauseOf(id);
            m_solver.addClause(id, cls);
        }
    }

    // Uses the solver to compute non-minimal support of the given clause
    std::vector<ClauseID> SimpleMinimizer::computeSupport(ClauseID id)
    {
        std::vector<ClauseID> support;

        if (opts().simple_min_use_min_supp)
        {
            support = findMinimalSupport(m_solver, allClauses(), id);
        }
        else
        {
            bool ind = m_solver.supportSolve(id, support);
            ((void)(ind));
            assert(ind);
        }

        log(4) << "Found support set of size " << support.size()
               << " for clause " << id << std::endl;
        log(4) << support << std::endl;


        return support;
    }

    // Starting from NEC = {(~Bad)}, recursively compute support of NEC and
    // add to NEC until a fixpoint is reached
    void SimpleMinimizer::doMinimize()
    {
        std::set<ClauseID> supported, nec;
        std::queue<ClauseID> to_process;

        to_process.push(propertyID());
        nec.insert(propertyID());

        while (!to_process.empty())
        {
            ClauseID id = to_process.front();
            to_process.pop();

            if (supported.count(id)) { continue; }

            std::vector<ClauseID> support = computeSupport(id);

            // Add each supporting clause to NEC and to the queue
            for (ClauseID s_id : support)
            {
                nec.insert(s_id);

                if (supported.count(s_id)) { continue; }

                to_process.push(s_id);
            }

            supported.insert(id);
        }

        std::vector<ClauseID> minimized(nec.begin(), nec.end());
        addMinimalProof(minimized);
    }
}

