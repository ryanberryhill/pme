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
        Cube concrete_state, inputs;
        unsigned may_degree;
        ProofObligation * parent;

        ProofObligation(const Cube & cti,
                        unsigned level,
                        ProofObligation * parent,
                        const Cube & concrete_state,
                        const Cube & inputs,
                        unsigned may_degree);
        ProofObligation(const ProofObligation & other);

        bool isMust() const { return may_degree == 0; }
    };

    struct ObligationComparator {
        bool operator()(const ProofObligation * lhs, const ProofObligation * rhs) const;
    };

    struct BlockResult {
        typedef std::tuple<bool &, unsigned &, Cube &, Cube &, Cube &, Cube &> BlockResultTuple;
        bool blocked;
        Cube cti, concrete_state, inputs, primed_inputs;
        unsigned level;

        BlockResult(bool blocked,
                    unsigned level,
                    Cube & cti,
                    Cube & concrete_state,
                    Cube & inputs,
                    Cube & primed_inputs)
            : blocked(blocked),
              cti(cti),
              concrete_state(concrete_state),
              inputs(inputs),
              primed_inputs(primed_inputs),
              level(level)
        { }

        BlockResult(bool blocked,
                    unsigned level,
                    Cube & cti)
            : blocked(blocked),
              cti(cti),
              level(level)
        { }

        BlockResult(bool blocked)
            : blocked(blocked), level(0)
        { }

        BlockResult() : BlockResult(false)
        { }

        operator BlockResultTuple()
        {
            return BlockResultTuple(blocked, level, cti, concrete_state, inputs, primed_inputs);
        }
    };

    class IC3Solver {
        public:
            IC3Solver(VariableManager & varman,
                      const TransitionRelation & tr,
                      GlobalState & gs);

            IC3Result prove();
            IC3Result prove(const Cube & target);

        private:
            typedef std::forward_list<ProofObligation> ObligationPool;
            typedef std::pair<bool, SafetyCounterExample> RecBlockResult;
            typedef std::priority_queue<const ProofObligation *,
                                        std::vector<const ProofObligation *>,
                                        ObligationComparator> ObligationQueue;

            ProofObligation * newObligation(const Cube & cti,
                                            unsigned level);
            ProofObligation * newObligation(const Cube & cti,
                                            unsigned level,
                                            ProofObligation * parent,
                                            const Cube & concrete_state,
                                            const Cube & inputs,
                                            unsigned may_degree = 0);
            ProofObligation * popObligation(ObligationQueue & q);
            void clearObligationPool();

            RecBlockResult recursiveBlock(const Cube & target, unsigned target_level);
            BlockResult block(const Cube & target, unsigned level);
            bool syntacticBlock(const Cube & target, unsigned level) const;
            bool frameBlocks(const Cube & target, unsigned level) const;
            void pushLemmas();

            void generalize(Cube & s, unsigned level);
            void generalizeIteration(Cube & s, unsigned level);

            void initiate(Cube & s, const Cube & orig);
            Cube reinitiate(const Cube & s, const Cube & orig);
            bool initiation(const Cube & s);

            bool isSafe(const Cube & target);
            RecBlockResult checkInitial(const Cube & target);

            void pushLemma(LemmaID id, unsigned level);
            void pushFrameToInf(unsigned level);
            LemmaID addLemma(const Cube & c, unsigned level);

            void initialize();

            void recordProof(IC3Result & result) const;
            SafetyCounterExample buildCex(const ProofObligation * obl) const;

            std::ostream & log(int verbosity) const;

            std::string stringOf(LemmaID id) const;
            std::string stringOf(const Cube & c) const;
            std::string clauseStringOf(LemmaID id) const;
            std::string clauseStringOf(const Cube & c) const;

            const PMEOptions & opts() const { return m_gs.opts; }

            VariableManager & m_vars;
            const TransitionRelation & m_tr;
            GlobalState & m_gs;

            InductiveTrace m_trace;
            ObligationPool m_obls;

            FrameSolver m_cons;
            UNSATCoreLifter m_lift;
    };
} }

#endif

