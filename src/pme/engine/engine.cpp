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
#include "pme/minimization/simple.h"
#include "pme/ivc/unified_ivc.h"
#include "pme/ivc/caivc.h"
#include "pme/ivc/marco_ivc.h"
#include "pme/ivc/ivc_bf.h"
#include "pme/ivc/ivc_ucbf.h"
#include "pme/ic3/ic3.h"
#include "pme/ic3/ic3_solver.h"
#include "pme/bmc/bmc_solver.h"
#include "pme/util/timer.h"

extern "C" {
#include "aiger/aiger.h"
}

#include <sstream>
#include <cassert>

namespace PME
{
    Engine::Engine(aiger * aig)
        : m_tr(m_vars, aig)
    {
        GlobalState::stats().num_gates = m_tr.numGates();
    }

    void Engine::setProof(const ExternalClauseVec & proof)
    {
        m_proof = m_tr.makeInternal(proof);
        removeProperty(m_proof);
    }

    bool Engine::checkProof()
    {
        ProofChecker ind(m_tr, m_proof);
        return ind.checkProof();
    }

    bool Engine::proofIsMinimal()
    {
        BruteForceMinimizer checker(m_vars, m_tr, m_proof);
        return checker.isMinimal();
    }

    void Engine::minimize(PMEMinimizationAlgorithm algorithm)
    {
        AutoTimer timer(GlobalState::stats().runtime);
        switch (algorithm)
        {
            case PME_MINIMIZATION_MARCO:
                log(1) << "Starting MARCO" << std::endl;
                m_minimizer.reset(new MARCOMinimizer(m_vars, m_tr, m_proof));
                break;
            case PME_MINIMIZATION_CAMSIS:
                log(1) << "Starting CAMSIS" << std::endl;
                m_minimizer.reset(new CAMSISMinimizer(m_vars, m_tr, m_proof));
                break;
            case PME_MINIMIZATION_SISI:
                log(1) << "Starting SISI" << std::endl;
                m_minimizer.reset(new SISIMinimizer(m_vars, m_tr, m_proof));
                break;
            case PME_MINIMIZATION_BRUTEFORCE:
                log(1) << "Starting BFMIN" << std::endl;
                m_minimizer.reset(new BruteForceMinimizer(m_vars, m_tr, m_proof));
                break;
            case PME_MINIMIZATION_SIMPLE:
                log(1) << "Starting SIMPLEMIN" << std::endl;
                m_minimizer.reset(new SimpleMinimizer(m_vars, m_tr, m_proof));
                break;
            default:
                throw std::logic_error("Unknown minimization algorithm");
                break;
        }

        assert(m_minimizer);
        m_minimizer->minimize();
    }

    void Engine::findIVCs(PMEIVCAlgorithm algorithm)
    {
        AutoTimer timer(GlobalState::stats().runtime);
        switch (algorithm)
        {
            case PME_IVC_UIVC:
                log(1) << "Starting UIVC" << std::endl;
                m_ivc_finder.reset(new UnifiedIVCFinder(m_vars, m_tr));
                break;
            case PME_IVC_MARCO:
                log(1) << "Starting MARCOIVC" << std::endl;
                m_ivc_finder.reset(new MARCOIVCFinder(m_vars, m_tr));
                break;
            case PME_IVC_CAIVC:
                log(1) << "Starting CAIVC" << std::endl;
                m_ivc_finder.reset(new CAIVCFinder(m_vars, m_tr));
                break;
            case PME_IVC_BF:
                log(1) << "Starting IVC_BF" << std::endl;
                m_ivc_finder.reset(new IVCBFFinder(m_vars, m_tr));
                break;
            case PME_IVC_UCBF:
                log(1) << "Starting IVC_UCBF" << std::endl;
                m_ivc_finder.reset(new IVCUCBFFinder(m_vars, m_tr));
                break;
            default:
                throw std::logic_error("Unknown IVC algorithm");
                break;
        }

        assert(m_ivc_finder);
        m_ivc_finder->findIVCs();
    }

    bool Engine::runIC3()
    {
        AutoTimer timer(GlobalState::stats().runtime);
        IC3::IC3Solver solver(m_vars, m_tr);
        SafetyResult result = solver.prove();

        if (result.result == SAFE)
        {
            m_proof = result.proof;
            removeProperty(m_proof);
        }
        else
        {
            assert(result.result == UNSAFE);
            m_cex = result.cex;
        }

        return result.result == SAFE;
    }

    bool Engine::runBMC(unsigned k_max)
    {
        AutoTimer timer(GlobalState::stats().runtime);
        BMC::BMCSolver solver(m_vars, m_tr);
        SafetyResult result = solver.solve(k_max);

        if (result.result == UNSAFE)
        {
            m_cex = result.cex;
        }
        else
        {
            assert(result.result == UNKNOWN);
        }

        return result.result == UNKNOWN;
    }

    SafetyCounterExample Engine::getCounterExample() const
    {
        return m_cex;
    }

    ExternalCounterExample Engine::getExternalCounterExample() const
    {
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

    size_t Engine::getNumIVCs() const
    {
        if (!m_ivc_finder) { return 0; }
        return m_ivc_finder->numMIVCs();
    }

    IVC Engine::getIVC(size_t i) const
    {
        IVC empty;
        if (!m_ivc_finder) { return empty; }
        return m_ivc_finder->getMIVC(i);
    }

    IVC Engine::getMinimumIVC() const
    {
        IVC empty;
        if (!m_ivc_finder) { return empty; }
        return m_ivc_finder->getMinimumIVC();
    }

    ExternalIVC Engine::getIVCExternal(size_t i) const
    {
        IVC ivc = getIVC(i);
        ExternalIVC vec = m_tr.makeExternal(ivc);
        return vec;
    }

    ExternalIVC Engine::getMinimumIVCExternal() const
    {
        IVC ivc = getMinimumIVC();
        ExternalIVC vec = m_tr.makeExternal(ivc);
        return vec;
    }

    size_t Engine::getBVCBound() const
    {
        if (!m_ivc_finder) { return 0; }
        return m_ivc_finder->numBVCBounds();
    }

    size_t Engine::getNumBVCs(size_t bound) const
    {
        if (!m_ivc_finder) { return 0; }
        return m_ivc_finder->numBVCsAtBound(bound);
    }

    IVC Engine::getBVC(size_t bound, size_t i) const
    {
        IVC bvc;
        if (!m_ivc_finder) { return bvc; }
        return m_ivc_finder->getBVC(bound, i);
    }

    ExternalIVC Engine::getBVCExternal(size_t bound, size_t i) const
    {
        IVC bvc = getBVC(bound, i);
        ExternalIVC vec = m_tr.makeExternal(bvc);
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
        GlobalState::logger().setLogStream(stream);
    }

    void Engine::setVerbosity(int v)
    {
        GlobalState::logger().setAllVerbosities(v);
    }

    void Engine::setChannelVerbosity(LogChannelID channel, int v)
    {
        GlobalState::logger().setVerbosity(channel, v);
    }

    void Engine::printStats() const
    {
        GlobalState::stats().printAll(log(0));
    }

    std::ostream & Engine::log(int v) const
    {
        return GlobalState::logger().log(LOG_PME, v);
    }

    void Engine::parseOption(const std::string& option)
    {
        GlobalState::options().parseOption(option);
    }
}

