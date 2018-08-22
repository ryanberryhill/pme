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

#include "pme/util/proof_checker.h"
#include "pme/util/simplify_tr.h"

namespace PME
{
    ProofChecker::ProofChecker(
                    const TransitionRelation & tr,
                    const ClauseVec & proof)
        : m_tr(tr),
          m_proof(proof)
    {
        // Unroll 2 frames so we get primed constraints
        ClauseVec unrolled = m_tr.unroll(2);

        ClauseVec simp, simp_init;
        if (GlobalState::options().simplify)
        {
            // Simplify
            simp = simplifyTR(m_tr);
        }
        else
        {
            simp = unrolled;
        }

        // Copy simplified clauses over to indSolver
        m_indSolver.addClauses(simp);

        // Add the proof
        m_indSolver.addClauses(proof);

        // Set up initSolver with initial states and simplified clauses
        ClauseVec init = m_tr.initState();
        if (GlobalState::options().simplify)
        {
            SATAdaptor simpSolver(MINISATSIMP);
            simpSolver.addClauses(unrolled);
            simpSolver.addClauses(init);

            // Don't freeze primes except for constraints -- this means an
            // initial state that inherently leads to a constraint violation
            // on the next cycle doesn't count
            simpSolver.freeze(m_tr.begin_latches(), m_tr.end_latches(), false);
            simpSolver.freeze(m_tr.begin_constraints(), m_tr.end_constraints(), true);
            simpSolver.freeze(m_tr.bad());

            simp_init = simpSolver.simplify();
        }
        else
        {
            simp_init.reserve(init.size() + unrolled.size());
            simp_init.insert(simp_init.end(), init.begin(), init.end());
            simp_init.insert(simp_init.end(), unrolled.begin(), unrolled.end());
        }

        m_initSolver.addClauses(simp_init);
    }

    bool ProofChecker::checkInitiation()
    {
        for (const Clause & c : m_proof)
        {
            Cube negc = negateVec(c);

            // Check for initiation
            if (m_initSolver.solve(negc)) { return false; }
        }

        return true;
    }

    bool ProofChecker::checkInduction()
    {
        for (const Clause & c : m_proof)
        {
            Cube negc = negateVec(c);
            Cube assumps = primeVec(negc);
            assumps.push_back(negate(m_tr.bad()));

            // Check for relative induction
            if (m_indSolver.solve(assumps)) { return false; }
        }

        return true;
    }

    bool ProofChecker::checkSafety()
    {
        ID bad = m_tr.bad();
        return !m_indSolver.solve({bad});
    }

    bool ProofChecker::checkInductiveStrengthening()
    {
        ID bad = m_tr.bad();
        Cube assumps({negate(bad), prime(bad)});
        return !m_indSolver.solve(assumps);
    }

    bool ProofChecker::checkProof()
    {
        return checkInitiation() && checkInduction() &&
               (checkSafety() || checkInductiveStrengthening());
    }
}

