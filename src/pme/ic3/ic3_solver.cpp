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

#include <algorithm>
#include <sstream>
#include <cassert>

namespace PME { namespace IC3 {

    Cube subtractLit(const Cube & s, Cube::const_iterator it)
    {
        Cube s_copy;
        s_copy.reserve(s.size() - 1);
        std::copy(s.begin(), it, std::back_inserter(s_copy));
        std::copy(it + 1, s.end(), std::back_inserter(s_copy));
        assert(s_copy.size() == s.size() - 1);
        return s_copy;
    }

    ProofObligation::ProofObligation(const Cube & cti, unsigned level, unsigned may_degree)
        : level(level), cti(cti), may_degree(may_degree)
    {
        assert(level < LEVEL_INF);
    }

    ProofObligation::ProofObligation(const ProofObligation & other)
        : level(other.level), cti(other.cti), may_degree(other.may_degree)
    {
        assert(level < LEVEL_INF);
    }

    IC3Solver::IC3Solver(VariableManager & varman,
                         const TransitionRelation & tr,
                         GlobalState & gs)
        : m_vars(varman),
          m_tr(tr),
          m_gs(gs),
          m_cons(varman, tr, m_trace, gs),
          m_lift(varman, tr, m_trace, gs),
          m_simpleInit(true)
    {
       initialize();
    }

    void IC3Solver::initialize()
    {
        // TODO consider handling complex initial states
        for (auto it = m_tr.begin_latches(); it != m_tr.end_latches(); ++it)
        {
            ID latch = *it;
            ID init = m_tr.getInit(latch);
            if (init != ID_NULL)
            {
                assert(init == ID_TRUE || init == ID_FALSE);
                bool neg = init == ID_FALSE;
                // The cube for the lemma is negation of the clause, so
                // negate when neg is false to get the cube
                ID lit = neg ? latch : negate(latch);
                m_trace.addLemma({lit}, 0);
            }
        }
    }

    std::ostream & IC3Solver::log(int verbosity) const
    {
        return m_gs.logger.log(LOG_IC3, verbosity);
    }

    IC3Result IC3Solver::prove()
    {
        Cube target = { m_tr.bad() };
        return prove(target);
    }

    IC3Result IC3Solver::prove(const Cube & target)
    {
        IC3Result result;

        if (isInitial(target)) {
            // TODO counterexample
            result.result = UNSAFE;
            return result;
        }

        size_t k = 1;
        while (!isSafe(target))
        {
            log(2) << "Level " << k << std::endl;
            bool blocked = recursiveBlock(target, k);
            if (blocked)
            {
                clearObligationPool();
                pushLemmas();
                k++;
            }
            else
            {
                // TODO Return counter-example
                result.result = UNSAFE;
                return result;
            }
        }

        recordProof(result);
        result.result = SAFE;
        return result;
    }

    void IC3Solver::recordProof(IC3Result & result) const
    {
        const Frame & proof = m_trace.getFrame(LEVEL_INF);
        for (LemmaID lemma : proof)
        {
            const Cube & cube = m_trace.cubeOf(lemma);
            Clause cls = negateVec(cube);
            result.proof.push_back(cls);
        }
    }

    bool IC3Solver::recursiveBlock(const Cube & target, unsigned target_level)
    {
        ObligationQueue q;

        q.push(newObligation(target, target_level));

        while (!q.empty())
        {
            const ProofObligation * obl = q.top(); q.pop();
            // TODO make it Quip instead of IC3
            assert(obl->may_degree == 0);

            Cube s = obl->cti;
            unsigned level = obl->level;
            assert(level < LEVEL_INF);

            if (level == 0)
            {
                // TODO counter-example preparations
                return false;
            }

            Cube t;
            unsigned g;
            bool blocked;
            std::tie(blocked, t, g) = block(s, level);

            if (blocked)
            {
                assert(g >= level);
                log(4) << g << ": " << clauseStringOf(s) << std::endl;
                addLemma(s, g);
                // TODO Quip-style re-enqueue
                if (g < target_level)
                {
                    q.push(newObligation(obl->cti, g + 1));
                }
            }
            else
            {
                q.push(newObligation(t, level - 1));
                q.push(obl);
            }
        }
        return true;
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
                if (m_cons.consecution(k, c))
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

    BlockResult IC3Solver::block(const Cube & target, unsigned level)
    {
        assert(level < LEVEL_INF);
        if (level == 0) { return BlockResult(false); }

        // TODO syntactic checks
        Cube s = target;
        Cube t, inp, pinp;
        bool blocked = false;
        std::tie(blocked, t, inp, pinp, std::ignore) = m_cons.consecutionFull(level - 1, s);

        if (blocked)
        {
            generalize(s, level);
            // TODO push s forward
            return BlockResult(true, s, level);
        }
        else
        {
            assert(!t.empty());
            t = m_lift.lift(t, s, inp, pinp);
            assert(!t.empty());
            return BlockResult(false, t, 0);
        }
    }

    void IC3Solver::generalize(Cube & s, unsigned level)
    {
        // The simpler generalization strategy from PDR
        assert(level > 0);
        unsigned k = (level == LEVEL_INF ? level : level - 1);
        std::sort(s.begin(), s.end());

        for (auto it = s.begin(); it != s.end() && s.size() > 1; )
        {
            ID lit = *it;

            // s_copy = s minus lit
            Cube s_copy = subtractLit(s, it);

            // Check if s_copy is initiated, if not continue
            if (!initiation(s_copy)) { it++; continue; }

            // Check for consecution
            Cube s_core;
            bool cons = m_cons.consecutionCore(k, s_copy, s_core);

            if (cons)
            {
                std::sort(s_core.begin(), s_core.end());
                // If the core is not initiated, make it initiated
                if (!initiation(s_core))
                {
                    s_core = reinitiate(s_core, s_copy);
                }
                // s = UNSAT core, (which is still sorted)
                s = s_core;
                assert(std::is_sorted(s.begin(), s.end()));

                // Set it to point to the next thing in the new s after lit
                it = std::upper_bound(s.begin(), s.end(), lit);
            }
            else
            {
                it++;
            }
        }
    }

    Cube IC3Solver::reinitiateSimple(const Cube & s, const Cube & orig)
    {
        // In the simple case, just add back in a literal from the
        // orig \intersect (init - s)
        Cube init;
        const Frame & f0 = m_trace.getFrame(0);
        for (ID id : f0)
        {
            Cube i = m_trace.cubeOf(id);
            assert(i.size() == 1);
            init.push_back(i.at(0));
        }

        std::sort(init.begin(), init.end());

        auto i_it = init.begin();
        auto o_it = orig.begin();

        while (*i_it != *o_it)
        {
            assert(i_it != init.end());
            assert(o_it != orig.end());
            if (*i_it < *o_it) { i_it++; }
            else { o_it++; }
        }

        assert(*i_it == *o_it);
        ID lit = *i_it;
        assert(std::find(s.begin(), s.end(), lit) == s.end());

        Cube s_copy = s;
        s_copy.push_back(lit);
        assert(initiation(s_copy));
        std::sort(s_copy.begin(), s_copy.end());
        return s_copy;
    }

    Cube IC3Solver::reinitiate(const Cube & s, const Cube & orig)
    {
        assert(s.size() < orig.size());
        assert(std::is_sorted(s.begin(), s.end()));
        assert(std::is_sorted(orig.begin(), orig.end()));
        assert(initiation(orig));

        if (m_simpleInit)
        {
            return reinitiateSimple(s, orig);
        }

        Cube t = orig;

        log(4) << "s = " << stringOf(s) << std::endl;
        auto s_it = s.begin();
        auto t_it = t.begin();

        while (t_it != t.end())
        {
            assert(s_it == s.end() || *t_it <= *s_it);
            // Advance to a point where they disagree
            while (*s_it == *t_it && s_it != s.end() && t_it != t.end())
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
        return !m_cons.intersection(0, s);
    }

    bool IC3Solver::isSafe(const Cube & target)
    {
        if (m_trace.lemmaExists(target))
        {
            const LemmaData & lemma = m_trace.getLemma(target);
            if (lemma.level == LEVEL_INF) { return true; }
        }

        if (!m_cons.intersection(LEVEL_INF, target))
        {
            return true;
        }

        return false;
    }

    bool IC3Solver::isInitial(const Cube & target)
    {
        return m_cons.intersection(0, target);
    }

    ProofObligation * IC3Solver::newObligation(const Cube & cti,
                                               unsigned level,
                                               unsigned may_degree)
    {
        m_obls.push_front(ProofObligation(cti, level, may_degree));
        return &m_obls.front();
    }

    void IC3Solver::clearObligationPool()
    {
        m_obls.clear();
    }

    void IC3Solver::pushLemma(LemmaID id, unsigned level)
    {
        // TODO consider updating lifting solver
        m_trace.pushLemma(id, level);
        m_cons.addLemma(id);
    }

    void IC3Solver::pushFrameToInf(unsigned level)
    {
        // TODO consider updating lifting solver
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

    LemmaID IC3Solver::addLemma(const Cube & c, unsigned level)
    {
        // TODO consider updating lifting solver
        LemmaID id = m_trace.addLemma(c, level);
        m_cons.addLemma(id);
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

