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
#include <algorithm>

namespace PME {

    bool BVCObligationComparator::operator()(const BVCProofObligation * lhs,
                                             const BVCProofObligation * rhs) const
    {
        // First sort by level
        if (lhs->level != rhs->level) { return lhs->level > rhs->level; }

        // Then by number of literals in the CTI
        size_t lhs_size = lhs->cti.size(), rhs_size = rhs->cti.size();
        if (lhs_size != rhs_size) { return lhs_size > rhs_size; }

        // Finally, break ties arbitrarily
        return lhs->cti > rhs->cti;
    }

    BVCSolver::BVCSolver(VariableManager & varman, const TransitionRelation & tr)
        : m_vars(varman), m_tr(tr), m_hs_solver(varman)
    { }

    BVCResult BVCSolver::prove()
    {
        BVCResult result;



        return result;
    }

    std::vector<ID> BVCSolver::getAbstraction() const
    {
        return std::vector<ID>(m_abstraction_gates.begin(), m_abstraction_gates.end());
    }

    void BVCSolver::setAbstraction(const std::vector<ID> & gates)
    {
        m_abstraction_gates.clear();
        m_abstraction_gates.insert(gates.begin(), gates.end());

        TransitionRelation abs_tr(m_tr, gates);
        m_initial_solver.reset();
        m_initial_solver.addClauses(abs_tr.unrollWithInit());
    }

    void BVCSolver::blockSolution(const BVCSolution & soln)
    {
        m_solutions.push_back(soln);
        for (auto & solver : m_solvers)
        {
            solver->blockSolution(soln);
        }
    }

    // Caller should check for unbounded safety or build the counter-example
    // if necessary
    BVCRecBlockResult BVCSolver::recursiveBlock(const Cube & target, unsigned target_level)
    {
        ObligationQueue q;

        q.push(newObligation(target, target_level));

        while(!q.empty())
        {
            BVCProofObligation * obl = popObligation(q);

            const Cube & s = obl->cti;
            unsigned level = obl->level;

            assert(std::is_sorted(s.begin(), s.end()));

            BVCBlockResult br = block(s, level);
            bool sat = br.sat;
            BVCSolution soln = br.solution;
            BVCPredecessor pred = br.predecessor;

            if (sat && !pred.empty())
            {
                assert(std::is_sorted(pred.begin(), pred.end()));
                if (level == 0)
                {
                    // Counter-example found
                    return BVCRecBlockResult(obl);
                }
                else
                {
                    // Predecessor found. Add new obligation.
                    q.push(obl);
                    q.push(newObligation(pred, level - 1));
                }
            }
            else if (sat && !soln.empty())
            {
                // Correction set found. Block, refine, re-enqueue
                assert(std::is_sorted(soln.begin(), soln.end()));

                blockSolution(soln);
                refineAbstraction(soln);

                q.push(obl);
            }
            else
            {
                // Obligation discharged
                assert(!sat);
            }
        }

        return BVCRecBlockResult();
    }

    BVCBlockResult BVCSolver::block(unsigned level)
    {
        Cube target = { m_tr.bad() };
        return block(target, level);
    }

    BVCBlockResult BVCSolver::block(const Cube & target, unsigned level)
    {
        BVCBlockResult result;

        // For level 0, we need to check if the given target is an initial
        // state (n = 0) or can be made one by a correction set (n > 0)
        if (level == 0) { return blockInitial(target); }

        BVCFrameSolver & solver = frameSolver(level);
        if (solver.solutionExists(target))
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

    BVCBlockResult BVCSolver::blockInitial(const Cube & target)
    {
        // TODO: this has high duplication with the level > 0 block function
        BVCBlockResult result;
        BVCFrameSolver & solver = frameSolver(0);

        if (solver.solutionExistsUnprimed(target))
        {
            for (unsigned n = 0; n <= m_tr.numGates(); n++)
            {
                 result = solver.solveUnprimed(n, target);
                 if (result.sat) { return result; }
            }

            // A solution exists but we didn't find it
            assert(false);
        }

        return result;
    }

    void BVCSolver::refineAbstraction(const BVCSolution & correction_set)
    {
        m_hs_solver.addSet(correction_set);
        std::vector<ID> abstraction = m_hs_solver.solve();
        assert(!abstraction.empty());

        setAbstraction(abstraction);
    }

    BVCProofObligation * BVCSolver::newObligation(const Cube & cti, unsigned level)
    {
        m_obls.emplace_front(cti, level);
        return &m_obls.front();
    }

    void BVCSolver::clearObligationPool()
    {
        m_obls.clear();
    }

    BVCProofObligation * BVCSolver::popObligation(ObligationQueue & q) const
    {
        // priority_queue only gives const access, but we want to be able
        // to modify the obligation and re-enqueue it without copies.
        // The const_cast below should be totally safe since we
        // immediately pop the element.
        BVCProofObligation * obl = const_cast<BVCProofObligation *>(q.top());
        q.pop();
        return obl;
    }

    BVCFrameSolver & BVCSolver::frameSolver(unsigned level)
    {
        // Levels 0 and 1 use the same solver with 0 abstracted frames
        unsigned abstraction_frames = level == 0 ? 0 : level - 1;

        // Construct new solvers if needed
        m_solvers.reserve(abstraction_frames + 1);
        for (unsigned i = m_solvers.size(); i <= abstraction_frames; ++i)
        {
            m_solvers.emplace_back(new BVCFrameSolver(m_vars, m_tr, i));

            for (const BVCSolution & soln : m_solutions)
            {
                m_solvers.back()->blockSolution(soln);
            }
        }

        // Set the abstraction for the relevant one
        m_solvers.at(abstraction_frames)->setAbstraction(m_abstraction_gates);
        return *m_solvers.at(abstraction_frames);
    }
}

