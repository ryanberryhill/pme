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
#include <algorithm>

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

    TransitionRelation::TransitionRelation(const TransitionRelation & other)
        : m_vars(other.m_vars),
          m_bad(other.m_bad),
          m_latches(other.m_latches),
          m_latchIDs(other.m_latchIDs),
          m_inputIDs(other.m_inputIDs),
          m_gateIDs(other.m_gateIDs),
          m_constraints(other.m_constraints),
          m_gates(other.m_gates)
    { }

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
        processAnds(aig);
        processLatches(aig);
        processInputs(aig);
        processConstraints(aig);
        // TODO: bad states
    }

    void TransitionRelation::processAnds(aiger * aig)
    {
        m_gates.reserve(aig->num_ands);
        m_gateIDs.reserve(aig->num_ands);

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

            m_gates.push_back(AndGate(l, r0, r1));
            m_gateIDs.push_back(l);
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
            getOrCreateVar(external, name);
        }
    }

    void TransitionRelation::processLatches(aiger * aig)
    {
        for (size_t i = 0; i < aig->num_latches; ++i)
        {
            ExternalID external = aig->latches[i].lit;
            assert(!aiger_sign(external));
            ExternalID next = aig->latches[i].next;
            ExternalID next_var = aiger_strip(next);
            ExternalID reset = aig->latches[i].reset;
            ID reset_id = (reset == 0) ? (ID_FALSE) :
                          (reset == 1) ? (ID_TRUE)  :
                                         (ID_NULL);

            bool neg = aiger_sign(next);

            const Variable& varp = getOrCreateVar(next_var);
            ID next_id = neg ? negate(varp.id) : varp.id;
            ID latch_id = toInternal(external);

            createLatch(latch_id, next_id, reset_id);
        }
    }

    void TransitionRelation::processInputs(aiger * aig)
    {
        for (size_t i = 0; i < aig->num_inputs; ++i)
        {
            ExternalID external = aig->inputs[i].lit;
            assert(!aiger_sign(external));
            ID input_id = toInternal(external);
            createInput(input_id);
        }
    }

    void TransitionRelation::createInput(ID id)
    {
        assert(std::find(m_inputIDs.begin(), m_inputIDs.end(), id) == m_inputIDs.end());
        m_inputIDs.push_back(id);
    }

    void TransitionRelation::createLatch(ID id, ID next, ID reset)
    {
        assert(m_latches.find(id) == m_latches.end());
        m_latches[id] = Latch(id, next, reset);
        m_latchIDs.push_back(id);
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

    const Variable& TransitionRelation::createInternalVar(std::string name)
    {
        return createVar(0, name);
    }

    const Variable& TransitionRelation::getOrCreateVar(ExternalID external, std::string name)
    {
        if (m_vars.isKnownExternal(external))
        {
            ID id = toInternal(external);
            return varOf(id);
        }
        else
        {
            if (name.empty()) { name = default_name("aig", external); }
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

    ClauseVec TransitionRelation::toCNF() const
    {
        ClauseVec clauses;

        for (const AndGate & gate : m_gates)
        {
            ClauseVec c = toCNF(gate);
            clauses.insert(clauses.end(), c.begin(), c.end());
        }

        return clauses;
    }

    ClauseVec TransitionRelation::toCNF(const AndGate & gate) const
    {
        ClauseVec clauses;
        clauses.push_back({negate(gate.lhs), gate.rhs0});
        clauses.push_back({negate(gate.lhs), gate.rhs1});
        clauses.push_back({gate.lhs, negate(gate.rhs0), negate(gate.rhs1)});
        return clauses;
    }

    void TransitionRelation::addTimeFrame(unsigned n,
                                          const ClauseVec & tr,
                                          ClauseVec & unrolled) const
    {
        ClauseVec frame = primeClauses(tr, n);
        unrolled.insert(unrolled.end(), frame.begin(), frame.end());

        // Connect latch next-states
        for (const auto & p : m_latches)
        {
            ID latchp = prime(p.first, n + 1);
            const Latch & latch = p.second;
            ID next = prime(latch.next, n);

            // Add clauses enforcing latch' = next
            make_equal(unrolled, next, latchp);
        }
    }

    void TransitionRelation::constrainTimeFrame(unsigned n, ClauseVec & unrolled) const
    {
        for (ID lit : m_constraints)
        {
            unrolled.push_back({prime(lit, n)});
        }
    }

    ClauseVec TransitionRelation::unroll(unsigned n) const
    {
        ClauseVec clauses;
        ClauseVec unrolled;

        // Do the Tseitin transformation
        clauses = toCNF();

        // Add time frames
        for (unsigned i = 0; i < n; ++i)
        {
            addTimeFrame(i, clauses, unrolled);
            constrainTimeFrame(i, unrolled);
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

