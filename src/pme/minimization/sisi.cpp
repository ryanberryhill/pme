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

#include "pme/minimization/sisi.h"
#include "pme/util/find_safe_mis.h"
#include "pme/util/minimal_support_finder.h"

#include <algorithm>
#include <cassert>

namespace PME
{
    //
    // Shared Functionality
    //

    SISI::SISI(ConsecutionChecker & solver) :
        m_indSolver(solver)
    { }

    void SISI::refineNEC()
    {
        // TODO optimization: instead of iterating over elements of feas, we
        // should ask if there exists and element of FEAS such that its removal
        // renders P or an element of NEC unsupported using a cardinality
        // constraint over activation variables
        assert(!m_feas.empty());
        // For each clause in FEAS, try to remove it. If removing it produces
        // a set that doesn't contain any proof, the clause is necessary
        for (ClauseID id : m_feas)
        {
            if (m_nec.count(id) > 0) { continue; }

            ClauseIDVec test_feas;
            test_feas.reserve(m_feas.size());
            // test_feas = m_feas - id
            std::remove_copy(m_feas.begin(), m_feas.end(),
                             std::back_inserter(test_feas), id);

            if (!findSIS(test_feas))
            {
                m_nec.insert(id);
            }
        }
    }

    void SISI::refineFEAS()
    {
        // Set FEAS = NEC. For each element in FEAS, compute a minimal support
        // set and add it to FEAS. Repeat until fixpoint.
        assert(!m_nec.empty());
        m_feas = m_nec;

        std::set<ClauseID> known_ind;

        std::vector<ClauseID> feas_vec(m_feas.begin(), m_feas.end());
        while (known_ind.size() < m_feas.size())
        {
            for (auto it = m_feas.begin(); it != m_feas.end(); ++it)
            {
                ClauseID cls = *it;
                if (known_ind.count(cls) > 0) { continue; }

                if (!m_indSolver.solve(feas_vec, cls))
                {
                    ClauseIDVec support;
                    m_indSolver.supportSolve(m_all, cls, support);
                    minimizeSupport(support, cls);
                    size_t old_size = m_feas.size();
                    m_feas.insert(support.begin(), support.end());
                    size_t new_size = m_feas.size();
                    assert(new_size > old_size);
                    it = m_feas.begin();

                    feas_vec.clear();
                    feas_vec.reserve(m_feas.size());
                    feas_vec.insert(feas_vec.end(), m_feas.begin(), m_feas.end());
                }

                known_ind.insert(cls);
            }
        }
    }

    ClauseIDVec SISI::bruteForceMinimize()
    {
        std::vector<ClauseID> feas(m_feas.begin(), m_feas.end());
        std::set<ClauseID> keep = m_nec;

        auto it = feas.begin();
        while (keep.size() < feas.size())
        {
            ClauseID id = *it;

            if (keep.count(id) > 0) { ++it; continue; }

            std::vector<ClauseID> test_feas;
            test_feas.reserve(feas.size());
            std::remove_copy(feas.begin(), feas.end(),
                             std::back_inserter(test_feas), id);

            if (findSIS(test_feas))
            {
                assert(test_feas.size() < feas.size());
                feas = test_feas;
                it = feas.begin();
            }
            else
            {
                keep.insert(id);
                ++it;
            }
        }

        std::vector<ClauseID> proof(keep.begin(), keep.end());
        return proof;
    }

    bool SISI::findSIS(ClauseIDVec & vec)
    {
        std::vector<ClauseID> nec_vec(m_nec.begin(), m_nec.end());
        return findSafeMIS(m_indSolver, vec, nec_vec);
    }

    void SISI::minimizeSupport(ClauseIDVec & vec, ClauseID cls)
    {
        MinimalSupportFinder finder(m_indSolver);
        ClauseIDVec minsupp = finder.findSupport(vec, cls);
        assert(minsupp.size() <= vec.size());
        vec = minsupp;
    }

    void SISI::addToFEAS(ClauseID id)
    {
        m_feas.insert(id);
    }

    void SISI::addToNEC(ClauseID id)
    {
        m_nec.insert(id);
    }

    void SISI::addClause(ClauseID id)
    {
        m_all.push_back(id);
    }

    //
    // Minimizer
    //

    SISIMinimizer::SISIMinimizer(VariableManager & vars,
                                 const TransitionRelation & tr,
                                 const ClauseVec & proof,
                                 GlobalState & gs)
        : ProofMinimizer(vars, tr, proof, gs),
          m_indSolver(vars, tr),
          m_sisi(m_indSolver)
    {
        initSolver();
    }

    void SISIMinimizer::initSolver()
    {
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            // Add clauses to m_indSolver
            const Clause & cls = clauseOf(id);
            m_indSolver.addClause(id, cls);
        }
    }

    std::ostream & SISIMinimizer::log(int verbosity) const
    {
        return ProofMinimizer::log(LOG_SISI, verbosity);
    }

    void SISIMinimizer::minimize()
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

        m_sisi.refineNEC();

        log(1) << "NEC size: " << m_sisi.sizeNEC() << std::endl;

        m_sisi.refineFEAS();

        log(1) << "FEAS size: " << m_sisi.sizeFEAS() << std::endl;

        m_sisi.refineNEC();

        log(1) << "Refined NEC size: " << m_sisi.sizeNEC() << std::endl;

        ClauseIDVec minimized = m_sisi.bruteForceMinimize();

        log(1) << "Minimized proof size: " << minimized.size() << std::endl;

        addMinimalProof(minimized);
    }

}

