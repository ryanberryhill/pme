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
#include "pme/util/hybrid_safety_checker.h"

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
        : m_vars(varman),
          m_tr(tr),
          m_hs_solver(varman),
          m_debug_tr(tr),
          m_mcs_finder(varman, m_debug_tr),
          m_approx_mcs_finder(varman, m_debug_tr)
    {
       m_lift.addClauses(m_tr.unroll(2));
    }

    std::ostream & BVCSolver::log(int verbosity) const
    {
        return GlobalState::logger().log(LOG_CBVC, verbosity);
    }

    BVCResult BVCSolver::prove()
    {
        BVCResult result;

        // Find small MCSes upfront as an optimization (if enabled)
        findUpfront();

        Cube bad = { m_tr.bad() };
        unsigned level = 0;

        SafetyProof proof;
        while (!checkAbstraction(proof))
        {
            log(3) << "Level " << level << std::endl;
            clearObligationPool();
            BVCRecBlockResult br = recursiveBlock(bad, level);

            if (!br.safe)
            {
                SafetyCounterExample cex = buildCex(br.cex_obl);
                return counterExampleResult(cex);
            }
            else
            {
                m_bvcs.push_back(getAbstraction());
            }

            level++;
        }

        return safeResult(proof);
    }

    std::vector<ID> BVCSolver::getAbstraction() const
    {
        return std::vector<ID>(m_abstraction_gates.begin(), m_abstraction_gates.end());
    }

    const BVC & BVCSolver::getBVC(unsigned i) const
    {
        assert(i < numBVCs());
        return m_bvcs.at(i);
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
            logObligation(obl);

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

                obl->state = br.state;
                obl->inputs = br.inputs;
                obl->pinputs = br.pinputs;

                if (level == 0)
                {
                    // Counter-example found
                    return BVCRecBlockResult(obl);
                }
                else
                {
                    // Predecessor found. Add new (lifted) obligation.
                    pred = lift(pred, s, br.inputs, br.pinputs);

                    q.push(obl);
                    q.push(newObligation(pred, level - 1, obl));
                }
            }
            else if (sat && !soln.empty())
            {
                // Correction set found. Block, refine, re-enqueue
                assert(std::is_sorted(soln.begin(), soln.end()));

                blockSolution(soln);
                refineAbstraction(soln);

                q.push(obl);

                log(4) << "At " << level << ": " << soln << std::endl;
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

    void BVCSolver::findUpfront()
    {
       unsigned n_max = opts().cbvc_upfront_nmax;
       for (unsigned n = 1; n <= n_max; ++n)
       {
           while (true)
           {
                bool found = false;
                CorrectionSet corr;
                if (opts().cbvc_upfront_approx_mcs)
                {
                    std::tie(found, corr) = m_approx_mcs_finder.findAndBlockWithBMC(n);
                }
                else
                {
                    std::tie(found, corr) = m_mcs_finder.findAndBlock();
                }

               if (!found) { break; }
               if (corr.empty()) { log(3) << "Found unsafe early" << std::endl; break; }

               log(3) << "Upfront: " << corr << std::endl;

               blockSolution(corr);
               refineAbstraction(corr);
           }
       }
    }

    Cube BVCSolver::lift(const Cube & pred,
                         const Cube & succ,
                         const Cube & inp,
                         const Cube & pinp)
    {
        if (!opts().cbvc_lift) { return pred; }
        if (pred.size() == 1) { return pred; }
        // s = pred, t = succ
        // s & inp & Tr & t' is SAT, and s & inp & Tr & !s' is UNSAT. The core
        // of s with respect to the latter query is the lifted version.

        Cube assumps;
        Cube pinp_p = primeVec(pinp);
        Cube negsucc_p = negateVec(primeVec(succ));

        assumps.insert(assumps.end(), pred.begin(), pred.end());
        assumps.insert(assumps.end(), inp.begin(), inp.end());
        assumps.insert(assumps.end(), pinp_p.begin(), pinp_p.end());

        Cube crits;

        if (succ.size() == 1)
        {
            // ~succ' is just a literal, assume it
            assert(negsucc_p.size() == 1);
            assumps.push_back(negsucc_p.at(0));

            bool sat = m_lift.solve(assumps, &crits);
            assert(!sat);
        }
        else
        {
            GroupID gid = m_lift.createGroup();
            m_lift.addGroupClause(gid, negsucc_p);

            bool sat = m_lift.groupSolve(gid, assumps, &crits);
            assert(!sat);
        }

        std::sort(crits.begin(), crits.end());
        assert(std::is_sorted(pred.begin(), pred.end()));
        Cube lifted;

        std::set_intersection(pred.begin(), pred.end(),
                              crits.begin(), crits.end(),
                              std::back_inserter(lifted));

        assert(lifted.size() <= pred.size());

        if (lifted.empty()) { return pred; }
        else { return lifted; }
    }

    BVCResult BVCSolver::counterExampleResult(const SafetyCounterExample & cex) const
    {
        BVCResult result;

        result.safety.result = UNSAFE;
        result.safety.cex = cex;

        return result;
    }

    BVCResult BVCSolver::safeResult(const SafetyProof & proof) const
    {
        BVCResult result;

        result.safety.result = SAFE;
        result.safety.proof = proof;
        result.abstraction = getAbstraction();

        return result;
    }

    SafetyCounterExample BVCSolver::buildCex(const BVCProofObligation * obl) const
    {
        SafetyCounterExample cex;

        const BVCProofObligation * current = obl;
        while (current != nullptr)
        {
            Cube inputs = current->inputs;
            Cube state = current->state;

            assert(std::is_sorted(inputs.begin(), inputs.end()));
            assert(std::is_sorted(state.begin(), state.end()));

            cex.push_back(Step(inputs, state));

            current = current->parent;
        }

        return cex;
    }

    bool BVCSolver::checkAbstraction(SafetyProof & proof) const
    {
        TransitionRelation abs_tr(m_tr, getAbstraction());

        HybridSafetyChecker checker(m_vars, abs_tr);
        SafetyResult result = checker.prove();
        assert(!result.unknown());

        if (result.safe()) { proof = result.proof; }

        return result.safe();
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

    BVCProofObligation * BVCSolver::newObligation(const Cube & cti, unsigned level,
                                                  BVCProofObligation * parent)
    {
        m_obls.emplace_front(cti, level, parent);
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

    void BVCSolver::logObligation(const BVCProofObligation * obl) const
    {
        log(4) << "Obligation at " << obl->level << ": "
               << m_vars.stringOf(obl->cti)
               << std::endl;
    }
}

