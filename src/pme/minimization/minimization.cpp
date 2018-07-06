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

#include "pme/minimization/minimization.h"

#include <cassert>
#include <algorithm>

namespace PME
{
    void DummyMinimizer::minimize()
    {
        std::vector<ClauseID> proof;
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            proof.push_back(id);
        }
        addMinimalProof(proof);
    }

    DummyMinimizer::DummyMinimizer(VariableManager & vars,
                                   const TransitionRelation & tr,
                                   const ClauseVec & proof,
                                   GlobalState & gs)
        : ProofMinimizer(vars, tr, proof, gs)
    { }

    ProofMinimizer::ProofMinimizer(VariableManager & vars,
                                   const TransitionRelation & tr,
                                   const ClauseVec & proof,
                                   GlobalState & gs)
        : m_tr(tr),
          m_vars(vars),
          m_proof(proof),
          m_gs(gs)
    {
        addPropertyIfMissing();
    }

    std::ostream & ProofMinimizer::log(int verbosity) const
    {
        return log(LOG_MINIMIZATION, verbosity);
    }

    std::ostream & ProofMinimizer::log(LogChannelID channel, int verbosity) const
    {
        return m_gs.logger.log(channel, verbosity);
    }

    void ProofMinimizer::addPropertyIfMissing()
    {
        Clause property = {negate(m_tr.bad())};
        bool property_found = false;

        // Check if the given proof already contains a clause ~Bad
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            const Clause & cls = clauseOf(id);
            if (cls == property)
            {
                log(4) << "Proof contains the property, not adding it" << std::endl;
                property_found = true;
                m_property = id;
                break;
            }
        }

        // If it does not contain ~Bad, then add it
        if (!property_found)
        {
            log(3) << "Property not included in the proof, adding it" << std::endl;
            m_property = m_proof.size();
            m_proof.push_back(property);
        }
    }

    const TransitionRelation & ProofMinimizer::tr() const
    {
        return m_tr;
    }

    VariableManager & ProofMinimizer::vars()
    {
        return m_vars;
    }

    void ProofMinimizer::addMinimalProof(const std::vector<ClauseID> & proof)
    {
        std::vector<ClauseID> proof_copy = proof;
        std::sort(proof_copy.begin(), proof_copy.end());
        log(2) << "Minimal proof: " << proof_copy << std::endl;
        m_minimalProofs.push_back(proof_copy);
    }

    void ProofMinimizer::setMinimumProof(const std::vector<ClauseID> & proof)
    {
        m_minimumProof = proof;
    }

    size_t ProofMinimizer::numClauses() const
    {
        return m_proof.size();
    }

    const Clause & ProofMinimizer::clauseOf(ClauseID id) const
    {
        assert(id < numClauses());
        return m_proof.at(id);
    }

    const ClauseVec & ProofMinimizer::proof() const
    {
        return m_proof;
    }

    Clause ProofMinimizer::property() const
    {
        return clauseOf(propertyID());
    }

    ClauseID ProofMinimizer::propertyID() const
    {
        return m_property;
    }

    size_t ProofMinimizer::numProofs() const
    {
        return m_minimalProofs.size();
    }

    ClauseVec ProofMinimizer::getProof(size_t i) const
    {
        assert(i < numProofs());
        ClauseVec proof;

        const std::vector<ID> & proof_ids = m_minimalProofs.at(i);
        for (ClauseID id : proof_ids)
        {
            proof.push_back(m_proof.at(id));
        }

        return proof;
    }

    bool ProofMinimizer::minimumProofKnown() const
    {
        return !m_minimumProof.empty();
    }

    ClauseVec ProofMinimizer::getMinimumProof() const
    {
        ClauseVec proof;
        for (ClauseID id : m_minimumProof)
        {
            proof.push_back(m_proof.at(id));
        }

        return proof;
    }
}
