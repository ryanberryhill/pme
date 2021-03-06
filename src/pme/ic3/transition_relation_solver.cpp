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
#include "pme/ic3/transition_relation_solver.h"
#include "pme/util/simplify_tr.h"

namespace PME { namespace IC3 {

    Cube extractCore(const Cube & c, const Cube & crits)
    {
        return extractCoreWithPrimes(c, crits, 0);
    }

    Cube extractCoreWithPrimes(const Cube & c, const Cube & crits, unsigned n)
    {
        Cube core;
        std::set<ID> lits(c.begin(), c.end());
        for (ID lit : crits) {
            if (nprimes(lit) != n) { continue; }
            ID unprimed = unprime(lit);
            if (lits.count(unprimed)) {
                core.push_back(unprimed);
            }
        }

        return core;
    }

    TransitionRelationSolver::TransitionRelationSolver(
                             VariableManager & varman,
                             const TransitionRelation & tr)
        : m_vars(varman),
          m_tr(tr)
    { }

    void TransitionRelationSolver::renewSAT()
    {
        // We haven't yet computed the simplified transition relation
        if (m_unrolled.empty())
        {
            computeSimplifiedTR();
        }

        solver().reset();
        sendTR();
    }

    void TransitionRelationSolver::sendTR()
    {
        solver().addClauses(m_unrolled);
    }

    void TransitionRelationSolver::computeSimplifiedTR()
    {
        m_unrolled.clear();

        if (GlobalState::options().simplify)
        {
            m_unrolled = simplifyTR(m_tr);
        }
        else
        {
            // Unroll 2 so we get primed constraints
            m_unrolled = m_tr.unroll(2);
        }
    }
} }

