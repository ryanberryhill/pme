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

#include "pme/id.h"
#include "pme/ic3/frame_solver.h"

#include <sstream>
#include <cassert>

namespace PME { namespace IC3 {

    std::string activationName(unsigned level)
    {
        std::ostringstream ss;
        ss << "act_lvl_" << level;
        return ss.str();
    }

    FrameSolver::FrameSolver(VariableManager & varman,
                             const TransitionRelation & tr,
                             const InductiveTrace & trace,
                             GlobalState & gs)
        : m_vars(varman),
          m_tr(tr),
          m_trace(trace),
          m_gs(g_null_gs),
          m_solverInited(false)
    { }

    void FrameSolver::renewSAT()
    {
        // We haven't yet computed the simplified transition relation
        if (m_unrolled.empty())
        {
            computeSimplifiedTR();
        }

        m_solver.reset();
        sendTR();

        // Add lemmas
        sendFrame(LEVEL_INF);
        for (size_t i = 0; i < m_trace.numFrames(); ++i)
        {
            sendFrame(i);
        }

        m_solverInited = true;
    }

    void FrameSolver::addLemma(LemmaID id)
    {
        if (m_solverInited) { sendLemma(id); }
    }

    bool FrameSolver::consecution(unsigned level, const Cube & c, Cube * core)
    {
        ConsecutionOptions opts;
        opts.level = level;
        opts.c = &c;
        opts.core = core;
        return consecution(opts);
    }

    bool FrameSolver::consecution(ConsecutionOptions & opts)
    {
        if (!m_solverInited) { renewSAT(); }
        assert(m_solverInited);

        const Cube & c = *opts.c;
        unsigned level = opts.level;
        Cube * core = opts.core;

        assert(!c.empty());

        // Assume F_k
        Cube assumps = levelAssumps(level);

        // Assume !c and c' (!c will be assumed using a clause group)
        Clause negc;
        for (ID lit : c)
        {
            assert(nprimes(lit) == 0);
            assumps.push_back(prime(lit));
            negc.push_back(negate(lit));
        }

        GroupID gid = m_solver.createGroup();
        m_solver.addGroupClause(gid, negc);

        Cube crits;
        Cube * solver_crits = core ? & crits : nullptr;
        bool sat = m_solver.groupSolve(gid, assumps, solver_crits);

        if (core && !sat)
        {
            *core = extractCoreOf(c, crits);
        }

        return !sat;
    }

    Cube FrameSolver::extractCoreOf(const Cube & c, const Cube & crits) const
    {
        Cube core;
        std::set<ID> lits(c.begin(), c.end());
        for (ID lit : crits) {
            if (nprimes(lit) != 1) { continue; }
            ID unprimed = unprime(lit);
            if (lits.count(unprimed)) {
                core.push_back(unprimed);
            }
        }

        return core;
    }

    Cube FrameSolver::levelAssumps(unsigned level)
    {
        // Assume ~act_lvl for each level >= level. Lemmas in F_inf are added
        // without activation literals so we don't need to assume ~act_inf
        Cube assumps;
        for (size_t i = level ; i < m_trace.numFrames(); ++i)
        {
            ID act = levelAct(i);
            assumps.push_back(negate(act));
        }
        return assumps;
    }

    bool FrameSolver::intersection(unsigned level, const Cube & c)
    {
        assert(!c.empty());
        if (!m_solverInited) { renewSAT(); }
        assert(m_solverInited);

        Cube assumps;

        // Assume ~act_lvl for each level >= level. Lemmas in F_inf are added
        // without activation literals so we don't need to assume ~act_inf
        for (size_t i = level ; i < m_trace.numFrames(); ++i)
        {
            ID act = levelAct(i);
            assumps.push_back(negate(act));
        }

        assumps.insert(assumps.end(), c.begin(), c.end());

        // F_k & c & Tr. Tr seems pointless but it might contain constraints
        bool sat = m_solver.solve(assumps);

        return sat;
    }

    void FrameSolver::sendTR()
    {
        m_solver.addClauses(m_unrolled);
    }

    void FrameSolver::sendFrame(unsigned level)
    {
        for (LemmaID id : m_trace.getFrame(level))
        {
            sendLemma(id);
        }
    }

    void FrameSolver::sendLemma(LemmaID id)
    {
        Clause cls = activatedClauseOf(id);
        m_solver.addClause(cls);
    }

    void FrameSolver::computeSimplifiedTR()
    {
        m_unrolled.clear();

        // Unroll 2 so we get primed constraints
        ClauseVec unrolled = m_tr.unroll(2);

        if (m_gs.opts.simplify)
        {
            SATAdaptor simpSolver(MINISATSIMP);

            simpSolver.addClauses(unrolled);

            // Freeze latches and constraints (including primes)
            simpSolver.freeze(m_tr.begin_latches(), m_tr.end_latches(), true);
            simpSolver.freeze(m_tr.begin_constraints(), m_tr.end_constraints(), true);

            // Freeze bad and bad'
            simpSolver.freeze(m_tr.bad());
            simpSolver.freeze(prime(m_tr.bad()));

            m_unrolled = simpSolver.simplify();
        }
        else
        {
            m_unrolled = unrolled;
        }
    }

    Clause FrameSolver::activatedClauseOf(LemmaID id)
    {
        const LemmaData & lemma = m_trace.getLemma(id);
        unsigned level = lemma.level;

        Clause cls = negateVec(lemma.cube);

        if (level < LEVEL_INF)
        {
            cls.push_back(levelAct(level));
        }

        return cls;
    }

    ID FrameSolver::levelAct(unsigned level)
    {
        if (level == LEVEL_INF) { return ID_FALSE; }
        while (level >= m_activation.size())
        {
            std::string name = activationName(level);
            m_activation.push_back(m_vars.getNewID(name));
        }

        return m_activation.at(level);
    }
} }

