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

#include "pme/minimization/camsis.h"

#include <cassert>
#include <sstream>

namespace PME
{
    CAMSISMinimizer::CAMSISMinimizer(VariableManager & vars,
                                     const TransitionRelation & tr,
                                     const ClauseVec & proof,
                                     GlobalState & gs)
        : ProofMinimizer(vars, tr, proof, gs),
          m_cons(vars, tr, gs),
          m_collapseFinder(vars, tr, gs),
          m_solver(vars)
    {
        // Set up collapse set finder and consecution checker and clause-select
        // variables for the MSIS solver
        for (ClauseID c = 0; c < numClauses(); ++c)
        {
            const Clause & cls = clauseOf(c);
            ID select = createClauseSelect(c);

            m_collapseFinder.addClause(c, cls);

            m_clausedb.addClause(c, select, cls);
            m_solver.addForOptimization(negate(select));

            m_cons.addClause(c, cls);
        }

        // Add a clause to the MSIS solver forcing it to select ~Bad
        ClauseID property = propertyID();
        ID selp = m_clausedb.activationOfID(property);
        m_solver.addClause({selp});
    }

    void CAMSISMinimizer::recordMSIS(const MSIS & msis)
    {
        if (numProofs() == 0)
        {
            setMinimumProof(msis);
        }
        addMinimalProof(msis);
    }

    void CAMSISMinimizer::minimize()
    {
        if (opts().camsis_abstraction_refinement)
        {
            abstractionRefinementMinimize();
        }
        else
        {
            naiveMinimize();
        }
    }

    void CAMSISMinimizer::abstractionRefinementMinimize()
    {
        MSISCandidate candidate;

        while (extractCandidate(candidate))
        {
            std::vector<ClauseID> unsupported;
            if (isSIS(candidate, unsupported))
            {
                log(1) << "Found MSIS of size " << candidate.size() << std::endl;
                blockMSIS(candidate);
                recordMSIS(candidate);
            }
            else
            {
                log(2) << "Found non-SIS of size " << candidate.size()
                       << " with " << unsupported.size()
                       << " unsupported clauses"
                       << std::endl;

                for (ClauseID c : unsupported)
                {
                    log(3) << "Attempting refinement with unsupported clause " << c
                           << std::endl;
                    bool refined = attemptRefinement(c);
                    assert(refined);
                }
            }
        }
    }

    void CAMSISMinimizer::naiveMinimize()
    {
        // The naive version of CAMSIS finds every collapse set of every clause
        for (ClauseID c = 0; c < numClauses(); ++c)
        {
            while (attemptRefinement(c)) { }
        }

        // And then extracts MSISes
        std::vector<ClauseID> msis;
        while (extractCandidate(msis))
        {
            log(1) << "Found MSIS of size " << msis.size() << std::endl;
            blockMSIS(msis);
            recordMSIS(msis);
        }
    }

    bool CAMSISMinimizer::isSIS(const MSISCandidate & candidate,
                                std::vector<ClauseID> & unsupported)
    {
        unsupported.clear();

        for (ClauseID c : candidate)
        {
            if (!m_cons.solve(candidate, c))
            {
                unsupported.push_back(c);
            }
        }

        return unsupported.empty();
    }

    bool CAMSISMinimizer::extractCandidate(MSISCandidate & msis)
    {
        msis.clear();
        bool sat = m_solver.solve();

        if (!sat) { return false; }

        for (ClauseID c = 0; c < numClauses(); ++c)
        {
            ID sel = m_clausedb.activationOfID(c);

            if (m_solver.getAssignmentToVar(sel) == SAT::TRUE)
            {
                msis.push_back(c);
            }
        }

        return true;
    }

    void CAMSISMinimizer::blockMSIS(const MSIS & msis)
    {
        Clause block;
        block.reserve(msis.size());
        for (ClauseID c : msis)
        {
            ID sel = m_clausedb.activationOfID(c);
            block.push_back(negate(sel));
        }

        m_solver.addClause(block);
    }

    bool CAMSISMinimizer::attemptRefinement(ClauseID c)
    {
        CollapseSet collapse;
        bool found = findCollapse(c, collapse);

        if (!found) { return false; }

        log(2) << "Found collapse set for clause " << c
               << " of size " << collapse.size()
               << std::endl;

        Clause cls = collapseClause(c, collapse);
        m_solver.addClause(cls);

        return true;
    }

    Clause CAMSISMinimizer::collapseClause(ClauseID c, const CollapseSet & collapse) const
    {
        // One element of collapse is selected or c is not select
        // i.e., ~c V (c_i for i in collapse)
        assert(!collapse.empty());

        ID selc = m_clausedb.activationOfID(c);
        Clause cls;
        cls.reserve(collapse.size() + 1);

        cls.push_back(negate(selc));
        for (ClauseID id : collapse)
        {
            ID sel = m_clausedb.activationOfID(id);
            cls.push_back(sel);
        }

        return cls;
    }

    ID CAMSISMinimizer::createClauseSelect(ClauseID c)
    {
        std::ostringstream ss;
        ss << "select_cls_" << c;
        std::string name = ss.str();
        return vars().getNewID(name);
    }

    bool CAMSISMinimizer::findCollapse(ClauseID c, CollapseSet & collapse)
    {
        return m_collapseFinder.findAndBlock(c, collapse);
    }

    std::ostream & CAMSISMinimizer::log(int verbosity) const
    {
        return ProofMinimizer::log(LOG_CAMSIS, verbosity);
    }
}

