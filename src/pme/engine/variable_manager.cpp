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

#include "pme/engine/variable_manager.h"
#include <sstream>
#include <cassert>

namespace PME
{
    VariableManager::VariableManager()
        : m_nextID(MIN_ID)
    {
        m_vars[ID_FALSE] = Variable(ID_FALSE, 0, "false");
        m_internalToExternal[ID_FALSE] = 0;
        m_externalToInternal[0] = ID_FALSE;
    }

    ID VariableManager::getNewID(std::string name, ExternalID external)
    {
        assert(m_nextID <= MAX_ID);
        ID id = m_nextID;
        m_nextID += ID_INCR;

        std::string newName = name.empty() ? defaultName(id) : name;
        m_vars[id] = Variable(id, external, newName);

        if (external != 0)
        {
            assert(m_internalToExternal.find(id) == m_internalToExternal.end());
            assert(m_externalToInternal.find(external) == m_externalToInternal.end());
            m_internalToExternal[id] = external;
            m_externalToInternal[external] = id;
        }

        return id;
    }

    bool VariableManager::isKnown(ID id) const
    {
        return m_vars.find(id) != m_vars.end();
    }

    bool VariableManager::isKnownExternal(ExternalID external) const
    {
        return m_externalToInternal.find(external) != m_externalToInternal.end();
    }

    std::string VariableManager::defaultName(ID id) const
    {
        std::ostringstream ss;
        ss << "ID_" << id;
        return ss.str();
    }

    ID VariableManager::toInternal(ExternalID external_id) const
    {
        bool neg = aiger_sign(external_id);
        auto it = m_externalToInternal.find(aiger_strip(external_id));
        if (it == m_externalToInternal.end())
        {
            throw std::invalid_argument("External ID not found");
        }
        return neg ? negate(it->second) : it->second;
    }

    ExternalID VariableManager::toExternal(ID pme_id) const
    {
        assert(is_valid_id(pme_id));
        bool neg = is_negated(pme_id);
        pme_id = strip(pme_id);
        assert(is_valid_id(pme_id));

        auto it = m_vars.find(pme_id);
        if (it == m_vars.end())
        {
            throw std::invalid_argument("Internal ID not found");
        }
        ExternalID external_id = it->second.m_externalID;
        assert(!aiger_sign(external_id));

        return neg ? aiger_not(external_id) : external_id;
    }

    const Variable & VariableManager::varOf(ID id) const
    {
        assert(is_valid_id(id));
        ID stripped = strip(id);
        return m_vars.at(stripped);
    }

    Clause VariableManager::makeInternal(ExternalClause cls) const
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

    ClauseVec VariableManager::makeInternal(ExternalClauseVec vec) const
    {
        ClauseVec internal_vec;
        for (const ExternalClause & cls : vec)
        {
            internal_vec.push_back(makeInternal(cls));
        }

        return internal_vec;
    }
}

