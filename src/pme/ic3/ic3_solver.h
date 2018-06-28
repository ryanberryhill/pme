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

#ifndef IC3_SOLVER_H_INCLUDED
#define IC3_SOLVER_H_INCLUDED

#include "pme/pme.h"
#include "pme/ic3/ic3.h"
#include "pme/ic3/inductive_trace.h"
#include "pme/ic3/frame_solver.h"
#include "pme/ic3/unsat_core_lifter.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/global_state.h"

#include <set>
#include <tuple>
#include <forward_list>
#include <queue>

namespace PME { namespace IC3 {

    struct ProofObligation {
        unsigned level;
        Cube cti;
        unsigned may_degree;

        ProofObligation(const Cube & cti, unsigned level, unsigned may_degree = 0);
        ProofObligation(const ProofObligation & other);

        bool isMust() const { return may_degree == 0; }
    };

    struct ObligationComparator {
        bool operator()(const ProofObligation * lhs, const ProofObligation * rhs) const
        {
            if (lhs->level != rhs->level) { return lhs->level > rhs->level; }

            size_t lhs_size = lhs->cti.size(), rhs_size = rhs->cti.size();
            if (lhs_size != rhs_size) { return lhs_size > rhs_size; }

            if (lhs->may_degree != rhs->may_degree) { return lhs->may_degree > rhs->may_degree; }

            // At this point choose arbitrarily
            return lhs->cti > rhs->cti;
        }
    };

    struct BlockResult {
        typedef std::tuple<bool &, Cube &, unsigned &> BlockResultTuple;
        bool blocked;
        Cube cti;
        unsigned level;

        BlockResult(bool blocked, Cube cti, unsigned level)
            : blocked(blocked), cti(cti), level(level)
        { }

        BlockResult(bool blocked)
            : blocked(blocked), level(0)
        { }

        BlockResult() : BlockResult(false)
        { }

        operator BlockResultTuple() { return BlockResultTuple(blocked, cti, level); }
    };

    class IC3Solver {
        public:
            IC3Solver(VariableManager & varman,
                      const TransitionRelation & tr,
                      GlobalState & gs = g_null_gs);

            IC3Result prove();
            IC3Result prove(const Cube & target);

        private:
            typedef std::forward_list<ProofObligation> ObligationPool;
            typedef std::priority_queue<const ProofObligation *,
                                        std::vector<const ProofObligation *>,
                                        ObligationComparator> ObligationQueue;

            ProofObligation * newObligation(const Cube & cti,
                                            unsigned level,
                                            unsigned may_degree = 0);
            void clearObligationPool();

            bool recursiveBlock(const Cube & target, unsigned target_level);
            BlockResult block(const Cube & target, unsigned level);
            void pushLemmas();

            void generalize(Cube & s, unsigned level);
            Cube reinitiate(const Cube & s, const Cube & orig);
            Cube reinitiateSimple(const Cube & s, const Cube & orig);
            bool initiation(const Cube & s);

            bool isSafe(const Cube & target);
            bool isInitial(const Cube & target);

            void pushLemma(LemmaID id, unsigned level);
            void pushFrameToInf(unsigned level);
            LemmaID addLemma(const Cube & c, unsigned level);

            void initialize();

            void recordProof(IC3Result & result) const;

            std::ostream & log(int verbosity) const;

            std::string stringOf(LemmaID id) const;
            std::string stringOf(const Cube & c) const;
            std::string clauseStringOf(LemmaID id) const;
            std::string clauseStringOf(const Cube & c) const;

            VariableManager & m_vars;
            const TransitionRelation & m_tr;
            GlobalState & m_gs;

            InductiveTrace m_trace;
            ObligationPool m_obls;

            FrameSolver m_cons;
            UNSATCoreLifter m_lift;

            bool m_simpleInit;
    };
} }

#endif

