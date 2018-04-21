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

#include <vector>
#include <unordered_map>

namespace PME
{
    struct Variable
    {
        Variable(ID id, unsigned aig_id, std::string name)
            : m_ID(id),
              m_aigerID(aig_id),
              m_name(name)
        { }

        Variable() : m_ID(ID_NULL), m_aigerID(0), m_name("")
        { }

        bool isNull() const { return m_ID == ID_NULL; }

        ID m_ID;
        unsigned m_aigerID;
        std::string m_name;
    };

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
            TransitionRelation(aiger * aig);
            TransitionRelation(aiger * aig, unsigned property);

            ID fromAiger(unsigned aig_id) const;
            unsigned toAiger(ID pme_id) const;

            Clause makeInternal(ExternalClause cls);
            ClauseVec makeInternal(ExternalClauseVec vec);

            ClauseVec unroll(unsigned n = 1) const;
            ClauseVec unrollWithInit(unsigned n = 1) const;
            ClauseVec initState() const;

        private:
            ID m_property;
            ID m_nextID;

            std::unordered_map<ID, Variable> m_vars;
            std::unordered_map<unsigned, ID> m_aigerToID;
            std::unordered_map<ID, Latch> m_latches;
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
            const Variable& createVar(unsigned aig_id, std::string name);
            const Variable& getOrCreateVar(unsigned aig_id);
    };
}

#endif

