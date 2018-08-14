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
        : TransitionRelationSolver(varman, tr, gs),
          m_trace(trace),
          m_solverInited(false)
    { }

    void FrameSolver::renewSAT()
    {
        TransitionRelationSolver::renewSAT();

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

    FrameSolver::ConsecutionResult
    FrameSolver::consecutionFull(unsigned level, const Cube & c)
    {
        Cube pred, inp, pinp, core;
        ConsecutionOptions opts;
        opts.level = level;
        opts.c = &c;
        opts.pred = &pred;
        opts.inp = &inp;
        opts.pinp = &pinp;
        opts.core = &core;

        bool cons = consecution(opts);

        return std::make_tuple(cons, pred, inp, pinp, core);
    }

    bool FrameSolver::consecutionCore(unsigned level, const Cube & c, Cube & core)
    {
        ConsecutionOptions opts;
        opts.level = level;
        opts.c = &c;
        opts.core = &core;
        return consecution(opts);
    }

    bool FrameSolver::consecutionPred(unsigned level, const Cube & c, Cube & pred)
    {
        ConsecutionOptions opts;
        opts.level = level;
        opts.c = &c;
        opts.pred = &pred;
        return consecution(opts);
    }

    bool FrameSolver::consecution(unsigned level, const Cube & c)
    {
        ConsecutionOptions opts;
        opts.level = level;
        opts.c = &c;
        return consecution(opts);
    }

    bool FrameSolver::consecution(ConsecutionOptions & opts)
    {
        if (!m_solverInited) { renewSAT(); }
        assert(m_solverInited);

        const Cube & c = *opts.c;
        unsigned level = opts.level;
        Cube * core = opts.core;
        Cube * pred = opts.pred;
        Cube * inp = opts.inp;
        Cube * pinp = opts.pinp;

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

        GroupID gid = solver().createGroup();
        solver().addGroupClause(gid, negc);

        Cube crits;
        Cube * solver_crits = core ? & crits : nullptr;
        bool sat = solver().groupSolve(gid, assumps, solver_crits);

        // Extract core or predecessor and inputs if requested
        if (core && !sat) { *core = extractCoreOf(c, crits); }
        if (pred && sat)  { *pred = extractPredecessor(); }
        if (inp && sat)   { *inp = extractInputs(); }
        if (pinp && sat)  { *pinp = extractPrimedInputs(); }

        return !sat;
    }

    Cube FrameSolver::extractCoreOf(const Cube & c, const Cube & crits) const
    {
        return extractCoreWithPrimes(c, crits);
    }

    Cube FrameSolver::extractPredecessor() const
    {
        std::vector<ID> latches(tr().begin_latches(), tr().end_latches());
        Cube result = extract(latches);
        assert(!result.empty() || latches.empty());
        return result;
    }

    Cube FrameSolver::extractInputs() const
    {
        std::vector<ID> inputs(tr().begin_inputs(), tr().end_inputs());
        return extract(inputs);
    }

    Cube FrameSolver::extractPrimedInputs() const
    {
        std::vector<ID> inputs(tr().begin_inputs(), tr().end_inputs());
        return extract(inputs, 1);
    }

    Cube FrameSolver::extract(std::vector<ID> vars, unsigned nprimes) const
    {
        assert(csolver().isSAT());
        Cube extracted;
        for (ID id : vars)
        {
            ID lit = prime(id, nprimes);
            ModelValue asgn = csolver().safeGetAssignmentToVar(lit);
            if (asgn != SAT::UNDEF)
            {
                ID asgn_id = (asgn == SAT::TRUE ? lit : negate(lit));
                asgn_id = unprime(asgn_id);
                extracted.push_back(asgn_id);
            }
        }

        return extracted;
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
        bool intersects;
        std::tie(intersects, std::ignore, std::ignore) = intersectionFull(level, c);
        return intersects;
    }

    FrameSolver::IntersectionResult
    FrameSolver::intersectionFull(unsigned level, const Cube & c)
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
        bool sat = solver().solve(assumps);

        Cube state, inputs;
        if (sat) { state = extractPredecessor(); }
        if (sat) { inputs = extractInputs(); }

        return std::make_tuple(sat, state, inputs);
    }

    void FrameSolver::sendFrame(unsigned level)
    {
        for (LemmaID id : m_trace.getFrame(level))
        {
            const LemmaData & lemma = m_trace.getLemma(id);
            if (lemma.deleted) { continue; }
            sendLemma(id);
        }
    }

    void FrameSolver::sendLemma(LemmaID id)
    {
        Clause cls = activatedClauseOf(id);
        solver().addClause(cls);
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
            m_activation.push_back(vars().getNewID(name));
        }

        return m_activation.at(level);
    }
} }

