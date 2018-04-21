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
        vec.push_back({a, negate(b)});
        vec.push_back({negate(a), b});
    }

    TransitionRelation::TransitionRelation(aiger * aig)
        : m_property(ID_NULL),
          m_nextID(MIN_ID)
    {
        assert(aig->num_outputs > 0);
        buildModel(aig);
        m_property = fromAiger(aig->outputs[0].lit);
    }

    TransitionRelation::TransitionRelation(aiger * aig, unsigned property)
        : m_property(ID_NULL),
          m_nextID(MIN_ID)
    {
        assert(property <= aig->maxvar * 2 + 1);
        buildModel(aig);
        m_property = fromAiger(property);
    }

    ID TransitionRelation::getNewID()
    {
        assert(m_nextID <= MAX_ID);
        ID next = m_nextID;
        m_nextID += ID_INCR;
        return next;
    }

    ID TransitionRelation::fromAiger(unsigned aig_id) const
    {
        bool neg = aiger_sign(aig_id);
        auto it = m_aigerToID.find(aiger_strip(aig_id));
        assert(it != m_aigerToID.end());
        return neg ? negate(it->second) : it->second;
    }

    unsigned TransitionRelation::toAiger(ID pme_id) const
    {
        assert(is_valid_id(pme_id));
        bool neg = is_negated(pme_id);
        pme_id = strip(pme_id);
        assert(is_valid_id(pme_id));

        auto it = m_vars.find(pme_id);
        assert(it != m_vars.end());
        unsigned aig_id = it->second.m_aigerID;
        assert(!aiger_sign(aig_id));

        return neg ? aiger_not(aig_id) : aig_id;
    }

    void TransitionRelation::buildModel(aiger * aig)
    {
        createSymbols(aig->inputs, aig->num_inputs, "i");
        createSymbols(aig->latches, aig->num_latches, "l");
        // Could also consider doing justice and fairness but it doesn't
        // make sense in the context of (safety) proof minimization
        createAndProcessAnds(aig);
        processLatches(aig);
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

            ID l = fromAiger(lhs);
            ID r0 = fromAiger(rhs0);
            ID r1 = fromAiger(rhs1);

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
            ID l = fromAiger(lit);
            m_constraints.push_back(l);
        }
    }

    void TransitionRelation::createSymbols(aiger_symbol *syms,
                                           size_t n,
                                           std::string name_prefix)
    {
        for (size_t i = 0; i < n; ++i)
        {
            unsigned aig_id = aiger_strip(syms[i].lit);
            const char * sym = syms[i].name;
            std::string name = create_name(sym, default_name(name_prefix, i));
            createVar(aig_id, name);
        }
    }

    void TransitionRelation::processLatches(aiger * aig)
    {
        for (size_t i = 0; i < aig->num_latches; ++i)
        {
            unsigned aig_id = aig->latches[i].lit;
            assert(!aiger_sign(aig_id));
            unsigned next = aig->latches[i].next;
            unsigned next_var = aiger_strip(next);
            unsigned reset = aig->latches[i].reset;
            ID reset_id = (reset == 0) ? (ID_FALSE) :
                          (reset == 1) ? (ID_TRUE)  :
                                         (ID_NULL);

            bool neg = aiger_sign(next);

            const Variable& varp = getOrCreateVar(next_var);
            ID next_id = neg ? negate(varp.m_ID) : varp.m_ID;
            ID latch_id = fromAiger(aig_id);

            assert(m_latches.find(latch_id) == m_latches.end());
            m_latches[latch_id] = Latch(latch_id, next_id, reset_id);
        }
    }

    const Variable& TransitionRelation::varOf(ID id)
    {
        assert(is_valid_id(id));
        ID stripped = strip(id);
        assert(m_vars.find(stripped) != m_vars.end());
        return m_vars[stripped];
    }

    const Variable& TransitionRelation::createVar(unsigned aig_id,
                                                  std::string name)
    {
        assert(!aiger_sign(aig_id));
        assert(m_aigerToID.find(aig_id) == m_aigerToID.end());
        ID id = getNewID();
        m_vars[id] = Variable(id, aig_id, name);
        m_aigerToID[aig_id] = id;
        return varOf(id);
    }

    const Variable& TransitionRelation::getOrCreateVar(unsigned aig_id)
    {
        auto it = m_aigerToID.find(aig_id);
        if (it != m_aigerToID.end())
        {
            ID id = it->second;
            return varOf(id);
        }
        else
        {
            std::string name = default_name("aig", aig_id);
            return createVar(aig_id, name);
        }
    }


    Clause TransitionRelation::makeInternal(ExternalClause cls)
    {
        Clause internal_cls;
        internal_cls.reserve(cls.size());
        for (unsigned lit : cls)
        {
            ID internal_lit = fromAiger(lit);
            internal_cls.push_back(internal_lit);
        }

        return internal_cls;
    }

    ClauseVec TransitionRelation::makeInternal(ExternalClauseVec vec)
    {
        ClauseVec internal_vec;
        for (const ExternalClause & cls : vec)
        {
            internal_vec.push_back(makeInternal(cls));
        }

        return internal_vec;
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
        ClauseVec unrolled = m_clauses;

        // Add latch' = next for each latch and constraints
        for (unsigned i = 1; i <= n; ++i)
        {
            ClauseVec next = primeClauses(m_clauses, i);
            unrolled.insert(unrolled.end(), next.begin(), next.end());
            // Connect latch next-states
            for (const auto & p : m_latches)
            {
                ID latchp = prime(p.first, i);
                const Latch & latch = p.second;
                ID next = prime(latch.m_next, i - 1);

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
            if (latch.m_reset != ID_NULL)
            {
                ID lit = latch.m_ID;
                ID reset = latch.m_reset;
                assert(reset == ID_TRUE || reset == ID_FALSE);

                make_equal(init, lit, reset);
            }
        }
        return init;
    }
}

