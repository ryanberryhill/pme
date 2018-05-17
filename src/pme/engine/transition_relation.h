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

#ifndef TRANSITION_RELATION_H_INCLUDED
#define TRANSITION_RELATION_H_INCLUDED


#include "pme/pme.h"
#include "pme/id.h"
#include "pme/engine/variable_manager.h"

#include <vector>
#include <unordered_map>

namespace PME
{
    struct Latch
    {
        Latch(ID id, ID next, ID reset)
            : m_ID(id),
              m_next(next),
              m_reset(reset)
        { }

        Latch() : m_ID(ID_NULL), m_next(ID_NULL), m_reset(ID_NULL)
        { }

        bool isNull() const { return m_ID == ID_NULL; }

        ID m_ID;
        ID m_next;
        ID m_reset;
    };

    class TransitionRelation
    {
        public:
            TransitionRelation(VariableManager & varman, aiger * aig);
            TransitionRelation(VariableManager & varman, aiger * aig, unsigned property);

            ID toInternal(ExternalID aig_id) const;
            ExternalID toExternal(ID pme_id) const;

            Clause makeInternal(ExternalClause cls) const;
            ClauseVec makeInternal(ExternalClauseVec vec) const;
            ExternalClause makeExternal(Clause cls) const;
            ExternalClauseVec makeExternal(ClauseVec vec) const;

            ClauseVec unroll(unsigned n = 1) const;
            ClauseVec unrollWithInit(unsigned n = 1) const;
            ClauseVec initState() const;

            ID property() const { return negate(m_bad); }
            ID bad() const { return m_bad; }
            Clause propertyClause() const { return { negate(m_bad) }; }

            id_iterator begin_latches() const { return m_latchIDs.cbegin(); }
            id_iterator end_latches() const { return m_latchIDs.cend(); }
            id_iterator begin_constraints() const { return m_constraints.cbegin(); }
            id_iterator end_constraints() const { return m_constraints.cend(); }

        private:
            VariableManager & m_vars;
            ID m_bad;

            std::unordered_map<ID, Latch> m_latches;
            std::vector<ID> m_latchIDs;
            std::vector<ID> m_constraints;

            std::vector<Clause> m_clauses;

            ID getNewID();

            void buildModel(aiger * aig);
            void createAndProcessAnds(aiger * aig);
            void createInputs(aiger * aig);
            void createSymbols(aiger_symbol *syms,
                               size_t n,
                               std::string name_prefix);
            void createLatches(aiger * aig);
            void processConstraints(aiger * aig);
            void processLatches(aiger * aig);

            const Variable& varOf(ID id);
            const Variable& createVar(ExternalID external, std::string name);
            const Variable& getOrCreateVar(ExternalID external);
    };
}

#endif

