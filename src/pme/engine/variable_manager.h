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

#ifndef VARIABLE_MANAGER_H_INCLUDED
#define VARIABLE_MANAGER_H_INCLUDED

#include "pme/id.h"

#include <unordered_map>

namespace PME
{
    struct Variable
    {
        Variable(ID id, ExternalID external_id, std::string name)
            : id(id),
              externalID(external_id),
              name(name)
        { }

        Variable() : id(ID_NULL), externalID(0), name("")
        { }

        bool isNull() const { return id == ID_NULL; }

        ID id;
        ExternalID externalID;
        std::string name;
    };

    class VariableManager
    {
        public:
            VariableManager();
            VariableManager(const VariableManager&) = delete;

            ID getNewID(std::string name = "", ExternalID external = 0);

            bool isKnown(ID id) const;
            bool isKnownExternal(ExternalID external) const;

            Clause makeInternal(ExternalClause cls) const;
            ClauseVec makeInternal(ExternalClauseVec vec) const;

            ID toInternal(ExternalID external_id) const;
            ExternalID toExternal(ID pme_id) const;

            const Variable & varOf(ID id) const;

            std::string stringOf(ID id) const;
            std::string stringOf(const std::vector<ID> vec, std::string sep = " ") const;
            std::string stringOf(const std::vector<std::vector<ID>> vec) const;

        private:

            std::string defaultName(ID id) const;

            ID m_nextID;
            std::unordered_map<ID, Variable> m_vars;
            std::unordered_map<ID, ExternalID> m_internalToExternal;
            std::unordered_map<ExternalID, ID> m_externalToInternal;
    };
}

#endif

