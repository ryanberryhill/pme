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
#include "pme/util/proof_checker.h"
#include "pme/minimization/marco.h"
#include "pme/minimization/sisi.h"
#include "pme/minimization/brute_force.h"

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
        ProofChecker ind(m_tr, m_proof);
        return ind.checkProof();
    }

    void Engine::minimize(PMEMinimizationAlgorithm algorithm)
    {
        switch (algorithm)
        {
            case PME_MINIMIZATION_MARCO:
                log(1) << "Starting MARCO" << std::endl;
                m_minimizer.reset(new MARCOMinimizer(m_vars, m_tr, m_proof, m_gs));
                break;
            case PME_MINIMIZATION_CAMSIS:
                log(1) << "Starting CAMSIS" << std::endl;
                throw std::logic_error("CAMSIS is not implemented");
                break;
            case PME_MINIMIZATION_SISI:
                log(1) << "Starting SISI" << std::endl;
                m_minimizer.reset(new SISIMinimizer(m_vars, m_tr, m_proof, m_gs));
                break;
            case PME_MINIMIZATION_BRUTEFORCE:
                log(1) << "Starting BFMIN" << std::endl;
                m_minimizer.reset(new BruteForceMinimizer(m_vars, m_tr, m_proof, m_gs));
                break;
            default:
                throw std::logic_error("Unknown minimization algorithm");
                break;
        }

        assert(m_minimizer);
        m_minimizer->minimize();
    }

    size_t Engine::getNumProofs() const
    {
        if (!m_minimizer) { return 0; }
        return m_minimizer->numProofs();
    }

    ClauseVec Engine::getProof(size_t i) const
    {
        ClauseVec empty;
        if (!m_minimizer) { return empty; }
        return m_minimizer->getProof(i);
    }

    ClauseVec Engine::getMinimumProof() const
    {
        ClauseVec empty;
        if (!m_minimizer) { return empty; }
        return m_minimizer->getMinimumProof();
    }

    ExternalClauseVec Engine::getProofExternal(size_t i) const
    {
        ClauseVec proof = getProof(i);

        removeProperty(proof);

        ExternalClauseVec vec = m_tr.makeExternal(proof);
        return vec;
    }

    ExternalClauseVec Engine::getMinimumProofExternal() const
    {
        ClauseVec proof = getMinimumProof();
        removeProperty(proof);
        ExternalClauseVec vec = m_tr.makeExternal(proof);
        return vec;
    }

    void Engine::removeProperty(ClauseVec & proof) const
    {
        for (unsigned i = 0; i < proof.size(); ++i)
        {
            const Clause & cls = proof.at(i);
            if (cls == m_tr.propertyClause())
            {
                proof.erase(proof.begin() + i);
                break;
            }
        }
    }

    void Engine::setLogStream(std::ostream & stream)
    {
        m_gs.logger.setLogStream(stream);
    }

    void Engine::setVerbosity(int v)
    {
        m_gs.logger.setAllVerbosities(v);
    }

    void Engine::setChannelVerbosity(LogChannelID channel, int v)
    {
        m_gs.logger.setVerbosity(channel, v);
    }

    std::ostream & Engine::log(int v) const
    {
        return m_gs.logger.log(LOG_PME, v);
    }
}

