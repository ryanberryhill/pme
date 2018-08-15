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

#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

#include "pme/pme.h"
#include "pme/ic3/ic3.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/sat_adaptor.h"
#include "pme/engine/logger.h"
#include "pme/engine/global_state.h"
#include "pme/minimization/minimization.h"
#include "pme/ivc/ivc.h"

#include <vector>
#include <unordered_map>
#include <iostream>

namespace PME
{
    class Engine
    {
        public:
            Engine(aiger * aig);

            void setProof(const ExternalClauseVec & proof);

            bool checkProof();
            void minimize(PMEMinimizationAlgorithm algorithm);
            void findIVCs(PMEIVCAlgorithm algorithm);
            bool runIC3();
            bool runBMC(unsigned k_max);

            ClauseVec getOriginalProof() const;
            ExternalClauseVec getOriginalProofExternal() const;

            size_t getNumProofs() const;
            ClauseVec getProof(size_t i) const;
            ClauseVec getMinimumProof() const;

            ExternalClauseVec getProofExternal(size_t i) const;
            ExternalClauseVec getMinimumProofExternal() const;

            size_t getNumIVCs() const;
            IVC getIVC(size_t i) const;
            IVC getMinimumIVC() const;

            ExternalIVC getIVCExternal(size_t i) const;
            ExternalIVC getMinimumIVCExternal() const;

            SafetyCounterExample getCounterExample() const;
            ExternalCounterExample getExternalCounterExample() const;

            void setLogStream(std::ostream & stream);
            void setVerbosity(int v);
            void setChannelVerbosity(LogChannelID channel, int v);

            void printStats() const;

            void parseOption(const std::string& option);

        private:
            VariableManager m_vars;
            TransitionRelation m_tr;
            ClauseVec m_proof;
            SafetyCounterExample m_cex;
            std::unique_ptr<ProofMinimizer> m_minimizer;
            std::unique_ptr<IVCFinder> m_ivc_finder;
            GlobalState m_gs;

            std::ostream & log(int v) const;
            void removeProperty(ClauseVec & proof) const;
    };
}

#endif

