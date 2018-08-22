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

#include "pme/minimization/brute_force.h"

namespace PME
{
    BruteForceMinimizer::BruteForceMinimizer(VariableManager & vars,
                                            const TransitionRelation & tr,
                                            const ClauseVec & proof)
        : ProofMinimizer(vars, tr, proof),
          m_indSolver(vars, tr),
          m_sisi(m_indSolver)
    {
        initSolver();
    }

    void BruteForceMinimizer::initSolver()
    {
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            const Clause & cls = clauseOf(id);
            m_indSolver.addClause(id, cls);
        }
    }

    std::ostream & BruteForceMinimizer::log(int verbosity) const
    {
        return ProofMinimizer::log(LOG_BFMIN, verbosity);
    }

    void BruteForceMinimizer::minimize()
    {
        // FEAS = { all }
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            m_sisi.addToFEAS(id);
            m_sisi.addClause(id);
        }
        // NEC = { ~Bad }
        m_sisi.addToNEC(propertyID());

        log(1) << "Proof size: " << numClauses() << std::endl;

        ClauseIDVec minimized = m_sisi.bruteForceMinimize();

        log(1) << "Minimized proof size: " << minimized.size() << std::endl;

        addMinimalProof(minimized);
    }
}

