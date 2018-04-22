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

#include "pme/pme.h"
#include "pme/engine/engine.h"

extern "C" {
#include "aiger/aiger.h"
}

#include <sstream>
#include <cassert>


namespace PME
{
    Engine::Engine(aiger * aig, const ExternalClauseVec & proof)
        : m_tr(m_vars, aig)
    {
        m_proof = m_tr.makeInternal(proof);
    }

    Engine::Engine(aiger * aig,
                   const ExternalClauseVec & proof,
                   unsigned property)
        : m_tr(m_vars, aig, property)
    {
        m_proof = m_tr.makeInternal(proof);
    }

    bool Engine::checkProof()
    {
        // Quick hack here
        SATAdaptor ind, init;

        // NOTE: if we're worried about primed constraints, we should do
        // unroll(2). Ideally we would also simplify after that.
        ind.addClauses(m_tr.unroll(1));
        ind.addClauses(m_proof);

        init.addClauses(m_tr.initState());

        // Check that each clause is inductive relative to the proof and
        // contains all initial states
        for (const Clause & c : m_proof)
        {
            Cube negc;
            for (ID lit : c) { negc.push_back(negate(lit)); }

            // relative induction
            if (ind.solve(negc)) { return false; }

            // initiation
            if (init.solve(negc)) { return false; }
        }


        return true;
    }
}

