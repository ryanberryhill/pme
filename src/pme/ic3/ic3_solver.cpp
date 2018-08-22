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

#include "pme/ic3/ic3_solver.h"
#include "pme/util/timer.h"

#include <algorithm>
#include <sstream>
#include <cassert>

namespace PME { namespace IC3 {

    bool ObligationComparator::operator()(const ProofObligation * lhs,
                                          const ProofObligation * rhs) const
    {
        if (lhs->level != rhs->level) { return lhs->level > rhs->level; }

        size_t lhs_size = lhs->cti.size(), rhs_size = rhs->cti.size();
        if (lhs_size != rhs_size) { return lhs_size > rhs_size; }

        if (lhs->may_degree != rhs->may_degree) { return lhs->may_degree > rhs->may_degree; }

        // At this point choose arbitrarily
        return lhs->cti > rhs->cti;
    }

    std::string levelString(unsigned level)
    {
        std::ostringstream ss;
        if (level == LEVEL_INF) { ss << "inf"; }
        else { ss << level; }
        return ss.str();
    }

    Cube subtractLit(const Cube & s, Cube::const_iterator it)
    {
        Cube s_copy;
        s_copy.reserve(s.size() - 1);
        std::copy(s.begin(), it, std::back_inserter(s_copy));
        std::copy(it + 1, s.end(), std::back_inserter(s_copy));
        assert(s_copy.size() == s.size() - 1);
        return s_copy;
    }

    ProofObligation::ProofObligation(const Cube & cti,
                                     unsigned level,
                                     ProofObligation * parent,
                                     const Cube & concrete_state,
                                     const Cube & inputs,
                                     unsigned may_degree)
        : level(level),
          cti(cti),
          concrete_state(concrete_state),
          inputs(inputs),
          may_degree(may_degree),
          parent(parent)
    {
        assert(level < LEVEL_INF);
    }

    ProofObligation::ProofObligation(const ProofObligation & other)
        : level(other.level),
          cti(other.cti),
          concrete_state(other.concrete_state),
          inputs(other.inputs),
          may_degree(other.may_degree),
          parent(other.parent)
    {
        assert(level < LEVEL_INF);
    }

    IC3Solver::IC3Solver(VariableManager & varman,
                         const TransitionRelation & tr)
        : m_vars(varman),
          m_tr(tr),
          m_cons(nullptr),
          m_lift(nullptr)
    {
       initialize();
    }

    unsigned IC3Solver::numFrames() const
    {
        return m_trace.numFrames();
    }

    std::vector<Cube> IC3Solver::getFrameCubes(unsigned n) const
    {
        std::vector<Cube> cubes;

        if (n == LEVEL_INF || n < m_trace.numFrames())
        {
            Frame f = m_trace.getFrame(n);
            for (LemmaID lemma_id : f)
            {
                cubes.push_back(m_trace.cubeOf(lemma_id));
            }
        }

        return cubes;
    }

    void IC3Solver::resetSAT()
    {
        m_cons.reset(new FrameSolver(m_vars, m_tr, m_trace));
        m_lift.reset(new UNSATCoreLifter(m_vars, m_tr, m_trace));
    }

    void IC3Solver::initialize()
    {
        // Add initial states from TR
        ClauseVec init_state = m_tr.initState();
        for (const Clause & cls : init_state)
        {
            Cube cube = negateVec(cls);
            if (!m_trace.lemmaIsActive(cube))
            {
                m_trace.addLemma(cube, 0);
            }
        }

        // Add user provided initial state constraints (e.g., to block
        // debugging solutions)
        for (const Cube & cube : m_init_constraints)
        {
            if (!m_trace.lemmaIsActive(cube))
            {
                m_trace.addLemma(cube, 0);
            }
        }

        // Set up SAT solvers with new initial states
        resetSAT();
    }

    void IC3Solver::initialStatesExpanded()
    {
        // TODO: incrementality?
        m_trace.clear();

        // Set up SAT solvers again
        initialize();
    }

    void IC3Solver::initialStatesRestricted()
    {
        initialize();
    }

    void IC3Solver::clearRestrictions()
    {
        m_init_constraints.clear();
    }

    void IC3Solver::restrictInitialStates(const ClauseVec & vec)
    {
        for (const Clause & cls : vec)
        {
            restrictInitialStates(cls);
        }
    }

    void IC3Solver::restrictInitialStates(const Clause & cls)
    {
        assert(!cls.empty());
        m_init_constraints.push_back(negateVec(cls));
    }

    std::ostream & IC3Solver::log(int verbosity) const
    {
        return GlobalState::logger().log(LOG_IC3, verbosity);
    }

    SafetyResult IC3Solver::prove()
    {
        Cube target = { m_tr.bad() };
        return prove(target);
    }

    SafetyResult IC3Solver::prove(const Cube & target)
    {
        GlobalState::stats().ic3_calls++;
        AutoTimer timer(GlobalState::stats().ic3_runtime);
        SafetyResult result;

        bool blocked;
        SafetyCounterExample cex;
        std::tie(blocked, cex) = checkInitial(target);
        if (!blocked) {
            log(2) << "Trivial counter-example" << std::endl;
            result.result = UNSAFE;
            result.cex = cex;
            return result;
        }

        size_t k = 1;
        while (!isSafe(target))
        {
            log(2) << "Level " << k << std::endl;
            std::tie(blocked, cex) = recursiveBlock(target, k);
            if (blocked)
            {
                clearObligationPool();
                pushLemmas();

                // Find new level of the property
                size_t level = m_trace.levelOf(target);
                assert(level >= k);
                if (level == LEVEL_INF) { break; }
                k = level + 1;
            }
            else
            {
                result.result = UNSAFE;
                result.cex = cex;
                log(2) << "Counter-example of length " << cex.size() << std::endl;
                return result;
            }
        }

        recordProof(result);
        result.result = SAFE;
        return result;
    }

    void IC3Solver::recordProof(SafetyResult & result) const
    {
        const Frame & proof = m_trace.getFrame(LEVEL_INF);
        for (LemmaID lemma : proof)
        {
            const Cube & cube = m_trace.cubeOf(lemma);
            Clause cls = negateVec(cube);
            result.proof.push_back(cls);
        }
    }

    SafetyCounterExample IC3Solver::buildCex(const ProofObligation * obl) const
    {
        SafetyCounterExample cex;
        const ProofObligation * current = obl;

        // An obligation's inputs are the ones needed to reach the parent
        // obligation's concrete state. conc & inputs & Tr & parent->conc
        // should be SAT. The ~Bad obligation doesn't have a parent, so it
        // shouldn't have inputs by this definition. However, outputs can
        // be a function of inputs (they are Mealy outputs), so it may have
        // inputs that came from the primed inputs of F_k & Tr & ~Bad.
        while (current != nullptr)
        {
            Cube inputs = current->inputs;
            Cube state = current->concrete_state;
            std::sort(inputs.begin(), inputs.end());
            std::sort(state.begin(), state.end());

            cex.push_back(Step(inputs, state));

            current = current->parent;
        }

        return cex;
    }

    IC3Solver::RecBlockResult IC3Solver::recursiveBlock(const Cube & target, unsigned target_level)
    {
        ObligationQueue q;

        q.push(newObligation(target, target_level));

        while (!q.empty())
        {
            ProofObligation * obl = popObligation(q);
            // TODO handle may proofs
            assert(obl->may_degree == 0);

            Cube s = obl->cti;
            unsigned level = obl->level;
            assert(std::is_sorted(s.begin(), s.end()));
            assert(level < LEVEL_INF);

            if (level == 0)
            {
                SafetyCounterExample cex = buildCex(obl);
                return std::make_pair(false, cex);
            }

            if (syntacticBlock(s, level)) { continue; }

            Cube t, conc, inp, pinp;
            unsigned g;
            bool blocked;
            std::tie(blocked, g, t, conc, inp, pinp) = block(s, level);

            if (blocked)
            {
                assert(g >= level);
                addLemma(t, g);
                // TODO Quip-style may-proof re-enqueue
                if (g < target_level)
                {
                    obl->level = g + 1;
                    q.push(obl);
                }
            }
            else
            {
                std::sort(t.begin(), t.end());
                q.push(newObligation(t, level - 1, obl, conc, inp));
                if (obl->inputs.empty()) { obl->inputs = pinp; }
                q.push(obl);
            }
        }

        return std::make_pair(true, SafetyCounterExample());
    }

    void IC3Solver::pushLemmas()
    {
        for (size_t k = 1; k < m_trace.numFrames(); ++k)
        {
            Frame frame_copy = m_trace.getFrame(k);
            size_t pushed = 0;
            for (LemmaID lemma : frame_copy)
            {
                const Cube & c = m_trace.cubeOf(lemma);
                if (m_cons->consecution(k, c))
                {
                    pushLemma(lemma, k + 1);
                    pushed++;
                }
            }

            if (pushed == frame_copy.size())
            {
                // Empty frame, promote to infinity
                pushFrameToInf(k);
                return;
            }
        }
    }

    bool IC3Solver::frameBlocks(const Cube & target, unsigned level) const
    {
        const Frame & fk = m_trace.getFrame(level);
        for (LemmaID id : fk)
        {
            const Cube & cube = m_trace.cubeOf(id);
            if (subsumes(cube, target)) { return true; }
        }

        return false;
    }

    bool IC3Solver::syntacticBlock(const Cube & target, unsigned level) const
    {
        for (unsigned k = level; k < m_trace.numFrames(); ++k)
        {
            if (frameBlocks(target, k)) { return true; }
        }

        if (frameBlocks(target, LEVEL_INF)) { return true; }

        return false;
    }

    BlockResult IC3Solver::block(const Cube & target, unsigned level)
    {
        assert(level < LEVEL_INF);
        if (level == 0) { return BlockResult(false); }

        Cube s = target;
        Cube t, inp, pinp, core;
        bool blocked = false;
        std::tie(blocked, t, inp, pinp, core) = m_cons->consecutionFull(level - 1, s);

        if (blocked)
        {
            std::sort(core.begin(), core.end());

            initiate(core, s);
            generalize(core, level);

            assert(initiation(core));

            // TODO push core forward (or do so in generalize)
            return BlockResult(true, level, core);
        }
        else
        {
            assert(!t.empty());
            Cube lifted = m_lift->lift(t, s, inp, pinp);
            assert(!lifted.empty());
            return BlockResult(false, 0, lifted, t, inp, pinp);
        }
    }

    void IC3Solver::generalize(Cube & s, unsigned level)
    {
        assert(level > 0);
        std::sort(s.begin(), s.end());
        // The simpler generalization strategy from PDR
        while (true)
        {
            size_t old_size = s.size();
            generalizeIteration(s, level);
            assert(s.size() <= old_size);
            if (s.size() == old_size) { return; }
        }
    }

    void IC3Solver::generalizeIteration(Cube & s, unsigned level)
    {
        unsigned k = (level == LEVEL_INF ? level : level - 1);

        for (auto it = s.begin(); it != s.end() && s.size() > 1; )
        {
            ID lit = *it;

            // s_copy = s minus lit
            Cube s_copy = subtractLit(s, it);

            // Check if s_copy is initiated, if not continue
            if (!initiation(s_copy)) { it++; continue; }

            // Check for consecution
            Cube s_core;
            bool cons = m_cons->consecutionCore(k, s_copy, s_core);

            if (cons)
            {
                std::sort(s_core.begin(), s_core.end());

                // If the core is not initiated, make it initiated
                initiate(s_core, s);

                // s = (re-initiated) UNSAT core, (which is still sorted)
                s = s_core;
                assert(std::is_sorted(s.begin(), s.end()));

                // Set 'it' to point to the next thing in the new s after lit
                it = std::upper_bound(s.begin(), s.end(), lit);
            }
            else
            {
                it++;
            }
        }
    }

    void IC3Solver::initiate(Cube & s, const Cube & orig)
    {
        assert(std::is_sorted(s.begin(), s.end()));
        if (!initiation(s))
        {
            s = reinitiate(s, orig);
        }
        assert(initiation(s));
    }

    Cube IC3Solver::reinitiate(const Cube & s, const Cube & orig)
    {
        assert(s.size() < orig.size());
        assert(std::is_sorted(s.begin(), s.end()));
        assert(std::is_sorted(orig.begin(), orig.end()));
        assert(initiation(orig));

        // TODO: consider doing re-initiation the simple way when the
        // initial state is just a cube (add back a literal from orig
        // \intersect init)
        //if (m_simpleInit)
        //{
        //    return reinitiateSimple(s, orig);
        //}

        Cube t = orig;

        auto s_it = s.begin();
        auto t_it = t.begin();

        while (t_it != t.end())
        {
            assert(s_it == s.end() || *t_it <= *s_it);
            // Advance to a point where they disagree
            while (s_it != s.end() && t_it != t.end() && *s_it == *t_it)
            {
                s_it++;
                t_it++;
            }

            // The loop above may have made t_it = t.end()
            if (t_it == t.end()) { break; }

            // See if it can be dropped from t
            ID lit = *t_it;
            Cube t_copy = subtractLit(t, t_it);

            if (initiation(t_copy))
            {
                // Upate t and point t_it to the next element
                t = t_copy;
                t_it = std::upper_bound(t.begin(), t.end(), lit);
            }
            else
            {
                // it cannot be dropped, try the next one
                t_it++;
            }

            // Update s_it to point to the next thing that will match t_it
            if (s_it != s.end() && t_it != t.end() && *s_it < *t_it)
            {
                s_it = std::upper_bound(s.begin(), s.end(), lit);
            }
        }

        assert(initiation(t));
        return t;
    }

    bool IC3Solver::initiation(const Cube & s)
    {
        if (s.empty()) { return false; }
        return !m_cons->intersection(0, s);
    }

    bool IC3Solver::isSafe(const Cube & target)
    {
        if (m_trace.lemmaExists(target))
        {
            const LemmaData & lemma = m_trace.getLemma(target);
            if (lemma.level == LEVEL_INF) { return true; }
        }

        if (!m_cons->intersection(LEVEL_INF, target))
        {
            return true;
        }

        return false;
    }

    IC3Solver::RecBlockResult IC3Solver::checkInitial(const Cube & target)
    {
        bool is_initial;
        Cube init_state, inputs;
        SafetyCounterExample cex;
        std::tie(is_initial, init_state, inputs) = m_cons->intersectionFull(0, target);

        if (is_initial) { cex.push_back(Step(inputs, init_state)); }

        return std::make_pair(!is_initial, cex);
    }

    ProofObligation * IC3Solver::newObligation(const Cube & cti,
                                               unsigned level)
    {
        Cube empty;
        m_obls.push_front(ProofObligation(cti, level, nullptr, empty, empty, 0));
        return &m_obls.front();
    }

    ProofObligation * IC3Solver::newObligation(const Cube & cti,
                                               unsigned level,
                                               ProofObligation * parent,
                                               const Cube & concrete_state,
                                               const Cube & inputs,
                                               unsigned may_degree)
    {
        m_obls.push_front(ProofObligation(cti, level, parent,
                                          concrete_state, inputs,
                                          may_degree));
        return &m_obls.front();
    }

    ProofObligation * IC3Solver::popObligation(ObligationQueue & q)
    {
        // priority_queue provides no way to pop off a non-const element.
        // However, we'd like to be able to pop the obligation, modify it
        // (change the level) and re-enqueue it without making another
        // allocation. The const_cast below should be totally safe since we
        // immediately pop the element.
        ProofObligation * obl = const_cast<ProofObligation *>(q.top());
        q.pop();
        return obl;
    }

    void IC3Solver::clearObligationPool()
    {
        m_obls.clear();
    }

    void IC3Solver::pushLemma(LemmaID id, unsigned level)
    {
        m_trace.pushLemma(id, level);
        m_cons->addLemma(id);
        log(4) << "To " << levelString(level) << ": " << clauseStringOf(id) << std::endl;
    }

    void IC3Solver::pushFrameToInf(unsigned level)
    {
        assert(level > 0);
        assert(level < LEVEL_INF);
        for (unsigned i = m_trace.numFrames() - 1; i >= level; --i)
        {
            Frame frame_copy = m_trace.getFrame(i);
            for (LemmaID id : frame_copy)
            {
                pushLemma(id, LEVEL_INF);
            }
        }

        m_trace.clearUnusedFrames();
        assert(m_trace.numFrames() == level);
    }

    LemmaID IC3Solver::addClausalLemma(const Clause & c, unsigned level)
    {
        Cube cube = negateVec(c);
        return addLemma(cube, level);
    }

    void IC3Solver::addClausalLemmas(const ClauseVec & c, unsigned level)
    {
        for (const Clause & cls : c)
        {
            addClausalLemma(cls, level);
        }
    }

    LemmaID IC3Solver::addLemma(const Cube & c, unsigned level)
    {
        // TODO consider updating lifting solver
        LemmaID id = m_trace.addLemma(c, level);
        m_cons->addLemma(id);
        log(4) << "At " << levelString(level) << ": " << clauseStringOf(id) << std::endl;
        return id;
    }

    std::string IC3Solver::stringOf(LemmaID id) const
    {
        const Cube & c = m_trace.cubeOf(id);
        return stringOf(c);
    }

    std::string IC3Solver::stringOf(const Cube & c) const
    {
        std::ostringstream ss;
        ss << "(" << m_vars.stringOf(c, " & ") << ")";
        return ss.str();
    }

    std::string IC3Solver::clauseStringOf(LemmaID id) const
    {
        const Cube & c = m_trace.cubeOf(id);
        return clauseStringOf(c);
    }

    std::string IC3Solver::clauseStringOf(const Cube & c) const
    {
        std::ostringstream ss;
        Clause cls = negateVec(c);
        ss << "(" << m_vars.stringOf(cls, " V ") << ")";
        return ss.str();
    }
} }

