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

#include "pme/engine/transition_relation.h"

extern "C" {
#include "aiger/aiger.h"
}

#include <sstream>
#include <cassert>

namespace PME
{
    std::string default_name(std::string prefix, unsigned index)
    {
        std::ostringstream ss;
        ss << prefix << index;
        return ss.str();
    }

    std::string create_name(const char * sym, std::string defname)
    {
        return sym == nullptr ? defname : sym;
    }

    void make_equal(ClauseVec & vec, ID a, ID b)
    {
        if (b == ID_TRUE) { vec.push_back({a}); return; }
        if (a == ID_TRUE) { vec.push_back({b}); return; }
        if (b == ID_FALSE) { vec.push_back({negate(a)}); return; }
        if (a == ID_FALSE) { vec.push_back({negate(b)}); return; }
        vec.push_back({a, negate(b)});
        vec.push_back({negate(a), b});
    }

    TransitionRelation::TransitionRelation(VariableManager & varman, aiger * aig)
        : m_vars(varman),
          m_bad(ID_NULL)
    {
        assert(aig->num_outputs > 0);
        buildModel(aig);
        m_bad = toInternal(aig->outputs[0].lit);
    }

    TransitionRelation::TransitionRelation(VariableManager & varman,
                                           aiger * aig,
                                           unsigned property)
        : m_vars(varman),
          m_bad(ID_NULL)
    {
        assert(property <= aig->maxvar * 2 + 1);
        buildModel(aig);
        m_bad = toInternal(property);
    }

    ID TransitionRelation::toInternal(ExternalID external) const
    {
        return m_vars.toInternal(external);
    }

    ExternalID TransitionRelation::toExternal(ID pme_id) const
    {
        return m_vars.toExternal(pme_id);
    }

    void TransitionRelation::buildModel(aiger * aig)
    {
        createSymbols(aig->inputs, aig->num_inputs, "i");
        createSymbols(aig->latches, aig->num_latches, "l");
        // Could also consider doing justice and fairness but it doesn't
        // make sense in the context of (safety) proof minimization
        createAndProcessAnds(aig);
        processLatches(aig);
        processInputs(aig);
        processConstraints(aig);
        // TODO: bad states
    }

    void TransitionRelation::createAndProcessAnds(aiger * aig)
    {
        for (size_t i = 0; i < aig->num_ands; ++i)
        {
            unsigned lhs = aig->ands[i].lhs;
            unsigned rhs0 = aig->ands[i].rhs0;
            unsigned rhs1 = aig->ands[i].rhs1;

            getOrCreateVar(aiger_strip(lhs));
            getOrCreateVar(aiger_strip(rhs0));
            getOrCreateVar(aiger_strip(rhs1));

            ID l = toInternal(lhs);
            ID r0 = toInternal(rhs0);
            ID r1 = toInternal(rhs1);

            m_clauses.push_back({negate(l), r0});
            m_clauses.push_back({negate(l), r1});
            m_clauses.push_back({l, negate(r0), negate(r1)});
        }
    }

    void TransitionRelation::processConstraints(aiger * aig)
    {
        for (size_t i = 0; i < aig->num_constraints; ++i)
        {
            unsigned lit = aig->constraints[i].lit;
            ID l = toInternal(lit);
            m_constraints.push_back(l);
        }
    }

    void TransitionRelation::createSymbols(aiger_symbol *syms,
                                           size_t n,
                                           std::string name_prefix)
    {
        for (size_t i = 0; i < n; ++i)
        {
            ExternalID external = aiger_strip(syms[i].lit);
            const char * sym = syms[i].name;
            std::string name = create_name(sym, default_name(name_prefix, i));
            createVar(external, name);
        }
    }

    void TransitionRelation::processLatches(aiger * aig)
    {
        for (size_t i = 0; i < aig->num_latches; ++i)
        {
            ExternalID external = aig->latches[i].lit;
            assert(!aiger_sign(external));
            unsigned next = aig->latches[i].next;
            unsigned next_var = aiger_strip(next);
            unsigned reset = aig->latches[i].reset;
            ID reset_id = (reset == 0) ? (ID_FALSE) :
                          (reset == 1) ? (ID_TRUE)  :
                                         (ID_NULL);

            bool neg = aiger_sign(next);

            const Variable& varp = getOrCreateVar(next_var);
            ID next_id = neg ? negate(varp.id) : varp.id;
            ID latch_id = toInternal(external);

            assert(m_latches.find(latch_id) == m_latches.end());
            m_latches[latch_id] = Latch(latch_id, next_id, reset_id);
            m_latchIDs.push_back(latch_id);
        }
    }

    void TransitionRelation::processInputs(aiger * aig)
    {
        for (size_t i = 0; i < aig->num_inputs; ++i)
        {
            ExternalID external = aig->inputs[i].lit;
            assert(!aiger_sign(external));
            ID input_id = toInternal(external);
            m_inputIDs.push_back(input_id);
        }
    }

    const Variable& TransitionRelation::varOf(ID id)
    {
        return m_vars.varOf(id);
    }

    const Variable& TransitionRelation::createVar(ExternalID external,
                                                  std::string name)
    {
        assert(!aiger_sign(external));
        ID id = m_vars.getNewID(name, external);
        return varOf(id);
    }

    const Variable& TransitionRelation::getOrCreateVar(ExternalID external)
    {
        if (m_vars.isKnownExternal(external))
        {
            ID id = toInternal(external);
            return varOf(id);
        }
        else
        {
            std::string name = default_name("aig", external);
            return createVar(external, name);
        }
    }

    Clause TransitionRelation::makeInternal(ExternalClause cls) const
    {
        Clause internal_cls;
        internal_cls.reserve(cls.size());
        for (unsigned lit : cls)
        {
            ID internal_lit = toInternal(lit);
            internal_cls.push_back(internal_lit);
        }

        return internal_cls;
    }

    ClauseVec TransitionRelation::makeInternal(ExternalClauseVec vec) const
    {
        ClauseVec internal_vec;
        for (const ExternalClause & cls : vec)
        {
            internal_vec.push_back(makeInternal(cls));
        }

        return internal_vec;
    }

    ExternalClause TransitionRelation::makeExternal(Clause cls) const
    {
        ExternalClause external_cls;
        external_cls.reserve(cls.size());
        for (ID lit : cls)
        {
            ID external_lit = toExternal(lit);
            external_cls.push_back(external_lit);
        }

        return external_cls;
    }

    ExternalClauseVec TransitionRelation::makeExternal(ClauseVec vec) const
    {
        ExternalClauseVec external_vec;
        for (const Clause & cls : vec)
        {
            external_vec.push_back(makeExternal(cls));
        }

        return external_vec;
    }

    ClauseVec TransitionRelation::unrollWithInit(unsigned n) const
    {
        ClauseVec unrolled = unroll(n);
        ClauseVec init = initState();
        unrolled.insert(unrolled.end(), init.begin(), init.end());
        return unrolled;
    }

    ClauseVec TransitionRelation::unroll(unsigned n) const
    {
        ClauseVec unrolled;

        // Add latch' = next for each latch and constraints
        for (unsigned i = 1; i <= n; ++i)
        {
            ClauseVec next = primeClauses(m_clauses, i - 1);
            unrolled.insert(unrolled.end(), next.begin(), next.end());
            // Connect latch next-states
            for (const auto & p : m_latches)
            {
                ID latchp = prime(p.first, i);
                const Latch & latch = p.second;
                ID next = prime(latch.next, i - 1);

                // Add clauses enforcing latch' = next
                make_equal(unrolled, next, latchp);
            }

            // Add constraints
            for (ID lit : m_constraints)
            {
                unrolled.push_back({prime(lit, i - 1)});
            }
        }

        return unrolled;
    }

    ClauseVec TransitionRelation::initState() const
    {
        ClauseVec init;
        for (const auto & p : m_latches)
        {
            const Latch & latch = p.second;
            if (latch.reset != ID_NULL)
            {
                ID lit = latch.id;
                ID reset = latch.reset;
                assert(reset == ID_TRUE || reset == ID_FALSE);

                make_equal(init, lit, reset);
            }
        }
        return init;
    }

    void TransitionRelation::setInit(ID latch, ID val)
    {
        assert(m_latches.count(latch) > 0);
        assert(val == ID_TRUE || val == ID_FALSE || val == ID_NULL);
        m_latches[latch].reset = val;
    }

    ID TransitionRelation::getInit(ID latch) const
    {
        assert(!is_negated(latch));
        assert(nprimes(latch) == 0);
        assert(m_latches.count(latch) > 0);
        return m_latches.at(latch).reset;
    }
}

