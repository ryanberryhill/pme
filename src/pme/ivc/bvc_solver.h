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

#ifndef BVC_SOLVER_H_INCLUDED
#define BVC_SOLVER_H_INCLUDED

#include "pme/safety.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/ivc/bvc_frame_solver.h"
#include "pme/util/hitting_set_finder.h"
#include "pme/engine/global_state.h"
#include "pme/ivc/correction_set_finder.h"

#include <forward_list>
#include <queue>

namespace PME {

    typedef std::vector<ID> BVC;

    struct BVCResult
    {
        SafetyResult safety;
        BVCSolution abstraction;

        bool safe() { return safety.safe(); }
        bool unsafe() { return safety.unsafe(); }
        bool unknown() { return safety.unknown(); }
        const SafetyCounterExample & cex() { return safety.cex; }
        const SafetyProof & proof() { return safety.proof; }
    };

    struct BVCProofObligation
    {
        unsigned level;
        Cube cti;
        BVCProofObligation * parent;
        Cube state, inputs, pinputs;

        BVCProofObligation(const Cube & cti, unsigned level) :
            level(level), cti(cti), parent(nullptr)
        { }

        BVCProofObligation(const Cube & cti, unsigned level, BVCProofObligation * parent) :
            level(level), cti(cti), parent(parent)
        { }
    };

    struct BVCRecBlockResult
    {
        bool safe;
        BVCProofObligation * cex_obl;

        BVCRecBlockResult() : safe(true), cex_obl(nullptr)
        { }

        BVCRecBlockResult(BVCProofObligation * obl) : safe(false), cex_obl(obl)
        { }
    };

    struct BVCObligationComparator {
        bool operator()(const BVCProofObligation * lhs, const BVCProofObligation * rhs) const;
    };

    class BVCSolver
    {
        public:
            BVCSolver(VariableManager & varman, const TransitionRelation & tr);

            BVCResult prove();

            BVCRecBlockResult recursiveBlock(const Cube & target, unsigned target_level);
            BVCBlockResult block(unsigned level);
            BVCBlockResult block(const Cube & target, unsigned level);

            void setAbstraction(const std::vector<ID> & gates);
            std::vector<ID> getAbstraction() const;

            void blockSolution(const BVCSolution & soln);

            unsigned numBVCs() const { return m_bvcs.size(); }
            const BVC & getBVC(unsigned i) const;

        private:
            typedef std::forward_list<BVCProofObligation> ObligationPool;
            typedef std::priority_queue<const BVCProofObligation *,
                                        std::vector<const BVCProofObligation *>,
                                        BVCObligationComparator> ObligationQueue;

            std::ostream & log(int verbosity) const;
            const PMEOptions & opts() const { return GlobalState::options(); }

            BVCFrameSolver & frameSolver(unsigned level);
            BVCBlockResult blockInitial(const Cube & target);

            BVCProofObligation * newObligation(const Cube & cti, unsigned level);
            BVCProofObligation * newObligation(const Cube & cti, unsigned level,
                                               BVCProofObligation * parent);
            BVCProofObligation * popObligation(ObligationQueue & q) const;
            void clearObligationPool();

            void refineAbstraction(const BVCSolution & correction_set);
            bool checkAbstraction(SafetyProof & proof) const;

            SafetyCounterExample buildCex(const BVCProofObligation * obl) const;
            BVCResult counterExampleResult(const SafetyCounterExample & cex) const;
            BVCResult safeResult(const SafetyProof & proof) const;

            void findUpfront();

            Cube lift(const Cube & pred, const Cube & succ, const Cube & inp, const Cube & pinp);

            void logObligation(const BVCProofObligation * obl) const;

            VariableManager & m_vars;
            const TransitionRelation & m_tr;
            std::vector<std::unique_ptr<BVCFrameSolver>> m_solvers;
            SATAdaptor m_initial_solver;
            std::set<ID> m_abstraction_gates;
            std::vector<BVCSolution> m_solutions;
            ObligationPool m_obls;
            HittingSetFinder m_hs_solver;
            std::vector<BVC> m_bvcs;
            SATAdaptor m_lift;

            DebugTransitionRelation m_debug_tr;
            MCSFinder m_mcs_finder;
            ApproximateMCSFinder m_approx_mcs_finder;
    };
}

#endif
