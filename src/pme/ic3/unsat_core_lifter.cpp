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
#include "pme/ic3/unsat_core_lifter.h"

#include <cassert>

namespace PME { namespace IC3 {

    UNSATCoreLifter::UNSATCoreLifter(VariableManager & varman,
                                     const TransitionRelation & tr,
                                     const InductiveTrace & trace)
        : TransitionRelationSolver(varman, tr),
          m_trace(trace),
          m_solverInited(false)
    { }

    void UNSATCoreLifter::renewSAT()
    {
        TransitionRelationSolver::renewSAT();

        // Add f_inf lemmas
        for (LemmaID id : m_trace.getFrame(LEVEL_INF))
        {
            const LemmaData & lemma = m_trace.getLemma(id);
            if (lemma.deleted) { continue; }
            sendLemma(id);
        }

        m_solverInited = true;
    }

    void UNSATCoreLifter::addLemma(LemmaID id)
    {
        sendLemma(id);
    }

    void UNSATCoreLifter::sendLemma(LemmaID id)
    {
        const LemmaData & lemma = m_trace.getLemma(id);
        Clause cls = negateVec(lemma.cube);
        solver().addClause(cls);
    }

    Cube UNSATCoreLifter::lift(const Cube & pred,
                               const Cube & succ,
                               const Cube & inp,
                               const Cube & pinp)
    {
        LiftOptions opts(pred, succ, inp, pinp);
        return lift(opts);
    }

    Cube UNSATCoreLifter::lift(const LiftOptions & opts)
    {
        const Cube & pred = opts.pred;
        const Cube & succ = opts.succ;
        const Cube & inp = opts.inp;
        const Cube & pinp = opts.pinp;
        assert(!succ.empty());

        if (!m_solverInited) { renewSAT(); }

        Cube assumps;
        assumps.reserve(pred.size() + inp.size() + pinp.size());
        Cube pinp_p = primeVec(pinp);
        Clause negsucc_p = negateVec(primeVec(succ));

        // pred & inp & pinp_p
        assumps.insert(assumps.end(), pred.begin(), pred.end());
        assumps.insert(assumps.end(), inp.begin(), inp.end());
        assumps.insert(assumps.end(), pinp_p.begin(), pinp_p.end());

        Cube crits;
        // Don't bother creating a group if we can just use assumptions
        if (succ.size() == 1)
        {
            assert(negsucc_p.size() == 1);
            assumps.push_back(negsucc_p.at(0));

            bool sat = solver().solve(assumps, &crits);
            ((void)(sat));
            assert(!sat);
        }
        else
        {
            // ~succ'
            GroupID gid = solver().createGroup();
            solver().addGroupClause(gid, negsucc_p);

            // F_inf & pred & inp & Tr & pinp' & ~succ'
            // Guaranteed to be UNSAT (if lift is called in the usual
            // circumstance) as pred & <inputs> & Tr => succ'
            // pred can be reduced using the conflicting assumptions to get a
            // smaller cube representing a set of states all of which can be
            // reach succ in one step
            bool sat = solver().groupSolve(gid, assumps, &crits);
            ((void)(sat));
            assert(!sat);
        }

        Cube lifted = extractCore(pred, crits);

        // The lifted cube could be empty if all states lead to succ under the
        // given input conditions. Just make lifted include an arbitrary
        // literal from pred in that case.
        if (lifted.empty())
        {
            lifted.push_back(pred[0]);
        }

        return lifted;
    }
} }

