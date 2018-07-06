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
#include "pme/minimization/camsis.h"
#include "pme/minimization/sisi.h"
#include "pme/minimization/brute_force.h"
#include "pme/ic3/ic3.h"
#include "pme/ic3/ic3_solver.h"

extern "C" {
#include "aiger/aiger.h"
}

#include <sstream>
#include <cassert>

namespace PME
{
    Engine::Engine(aiger * aig)
        : m_tr(m_vars, aig)
    { }

    void Engine::setProof(const ExternalClauseVec & proof)
    {
        m_proof = m_tr.makeInternal(proof);
        removeProperty(m_proof);
    }

    bool Engine::checkProof()
    {
        ProofChecker ind(m_tr, m_proof, m_gs);
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
                m_minimizer.reset(new CAMSISMinimizer(m_vars, m_tr, m_proof, m_gs));
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

    bool Engine::runIC3()
    {
        IC3::IC3Solver solver(m_vars, m_tr, m_gs);
        IC3::IC3Result result = solver.prove();

        if (result.result == IC3::SAFE)
        {
            m_proof = result.proof;
            removeProperty(m_proof);
        }
        else
        {
            assert(result.result == IC3::UNSAFE);
            m_cex = result.cex;
        }

        return result.result == IC3::SAFE;
    }

    IC3::SafetyCounterExample Engine::getCounterExample() const
    {
        return m_cex;
    }

    ExternalCounterExample Engine::getExternalCounterExample() const
    {
        using namespace PME::IC3;
        ExternalCounterExample cex;

        for (const Step & step : m_cex)
        {
            ExternalStep ext_step;
            ext_step.inputs = m_tr.makeExternal(step.inputs);
            ext_step.state = m_tr.makeExternal(step.state);
            cex.push_back(ext_step);
        }

        return cex;
    }

    ClauseVec Engine::getOriginalProof() const
    {
        return m_proof;
    }

    ExternalClauseVec Engine::getOriginalProofExternal() const
    {
        ClauseVec proof = getOriginalProof();
        return m_tr.makeExternal(proof);
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

