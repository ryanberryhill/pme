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

namespace PME
{
    void DummyMinimizer::minimize()
    {
        clearMinimizedProof();
        for (ClauseID id = 0; id < numClauses(); ++id)
        {
            addToMinimizedProof(id);
        }
    }

    DummyMinimizer::DummyMinimizer(VariableManager & vars,
                                   const TransitionRelation & tr,
                                   const ClauseVec & proof)
        : ProofMinimizer(vars, tr, proof)
    { }

    ProofMinimizer::ProofMinimizer(VariableManager & vars,
                                   const TransitionRelation & tr,
                                   const ClauseVec & proof)
        : m_tr(tr),
          m_vars(vars),
          m_proof(proof)
    {
        addPropertyIfMissing();
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
                property_found = true;
                m_property = id;
                break;
            }
        }

        // If it does not contain ~Bad, then add it
        if (!property_found)
        {
            m_property = m_proof.size();
            m_proof.push_back(property);
        }
    }

    clause_iterator ProofMinimizer::begin_minproof() const
    {
        return m_minimizedProof.cbegin();
    }

    clause_iterator ProofMinimizer::end_minproof() const
    {
        return m_minimizedProof.cend();
    }

    const TransitionRelation & ProofMinimizer::tr() const
    {
        return m_tr;
    }

    VariableManager & ProofMinimizer::vars()
    {
        return m_vars;
    }

    void ProofMinimizer::clearMinimizedProof()
    {
        m_minimizedProof.clear();
    }

    void ProofMinimizer::addToMinimizedProof(ClauseID id)
    {
        m_minimizedProof.push_back(clauseOf(id));
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
}
