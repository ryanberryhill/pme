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

#ifndef CLAUSE_DB_H_INCLUDED
#define CLAUSE_DB_H_INCLUDED

#include "pme/pme.h"

#include <unordered_map>

namespace PME
{
    template<typename T>
    class ClauseDatabaseT
    {
        public:
            typedef std::unordered_map<ClauseID, Clause>::const_iterator iterator;
            ClauseDatabaseT();
            void addClause(ClauseID id, ID activation, const Clause & cls);
            void addClause(ClauseID id, ID activation, const Clause & cls, const T & data);
            ID activationOfID(ClauseID id) const;
            ClauseID IDOfActivation(ID act) const;
            const Clause & clauseOf(ClauseID id) const;
            const T & getData(ClauseID id) const;
            bool isActivation(ID id) const;

            iterator begin();
            iterator end();

        private:
            std::unordered_map<ClauseID, Clause> m_IDToClause;
            std::unordered_map<ClauseID, ID> m_IDToActivation;
            std::unordered_map<ClauseID, ID> m_activationToID;
            std::unordered_map<ClauseID, T> m_IDToData;
    };

    typedef ClauseDatabaseT<bool> ClauseDatabase;
    typedef ClauseDatabaseT<ID> DualActivationClauseDatabase;
}

#endif

