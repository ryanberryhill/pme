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

#include "clause_database.h"

#include <cassert>
#include <algorithm>

namespace PME
{
    template<typename T>
    ClauseDatabaseT<T>::ClauseDatabaseT()
    { }

    template<typename T>
    void ClauseDatabaseT<T>::addClause(ClauseID id, ID activation, const Clause & cls)
    {
        T data;
        addClause(id, activation, cls, data);
    }

    template<typename T>
    void ClauseDatabaseT<T>::addClause(ClauseID id,
                                      ID activation,
                                      const Clause & cls,
                                      const T & data)
    {
        assert(m_IDToClause.find(id) == m_IDToClause.end());
        assert(m_IDToActivation.find(id) == m_IDToActivation.end());
        assert(m_activationToID.find(activation) == m_activationToID.end());
        assert(m_IDToData.find(id) == m_IDToData.end());

        Clause cls_sorted = cls;
        std::sort(cls_sorted.begin(), cls_sorted.end());

        m_IDToClause[id] = cls_sorted;
        m_IDToActivation[id] = activation;
        m_activationToID[activation] = id;
        m_IDToData[id] = data;
    }

    template<typename T>
    ID ClauseDatabaseT<T>::activationOfID(ClauseID id) const
    {
        return m_IDToActivation.at(id);
    }

    template<typename T>
    ClauseID ClauseDatabaseT<T>::IDOfActivation(ID act) const
    {
        return m_activationToID.at(act);
    }

    template<typename T>
    const Clause & ClauseDatabaseT<T>::clauseOf(ClauseID id) const
    {
        return m_IDToClause.at(id);
    }

    template<typename T>
    bool ClauseDatabaseT<T>::isActivation(ID id) const
    {
        return m_activationToID.find(id) != m_activationToID.end();
    }

    template<typename T>
    const T & ClauseDatabaseT<T>::getData(ClauseID id) const
    {
        return m_IDToData.at(id);
    }

    template<typename T>
    auto ClauseDatabaseT<T>::begin() -> iterator
    {
        return m_IDToClause.cbegin();
    }

    template<typename T>
    auto ClauseDatabaseT<T>::end() -> iterator
    {
        return m_IDToClause.cend();
    }

    template class ClauseDatabaseT<bool>;
    template class ClauseDatabaseT<ID>;
}

