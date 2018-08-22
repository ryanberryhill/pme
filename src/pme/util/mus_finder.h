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

#ifndef MUS_FINDER_H_INCLUDED
#define MUS_FINDER_H_INCLUDED

#include "pme/pme.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/global_state.h"
#include "pme/engine/sat_adaptor.h"

namespace PME {
    typedef unsigned MUSGroupID;

    class MUSFinder
    {
        public:
            MUSFinder(VariableManager & vars);

            void addHardClause(const Clause & cls);
            template<typename Iterator>
            void addHardClauses(Iterator begin, Iterator end);

            MUSGroupID createGroup();
            MUSGroupID addSoftClause(const Clause & cls);
            void addSoftClause(MUSGroupID group, const Clause & cls);
            template<typename Iterator>
            MUSGroupID addSoftClauses(Iterator begin, Iterator end);
            template<typename Iterator>
            void addSoftClauses(MUSGroupID group, Iterator begin, Iterator end);

            std::vector<MUSGroupID> findCore();
            std::vector<MUSGroupID> findMUS();

        private:
            ID activationForGroup(MUSGroupID gid) const;
            MUSGroupID groupForActivation(ID act) const;
            bool isUNSAT(std::vector<MUSGroupID> & core);

            VariableManager & m_vars;
            SATAdaptor m_solver;

            MUSGroupID m_nextGroup;
            std::unordered_map<MUSGroupID, ID> m_groupToActivation;
            std::unordered_map<ID, MUSGroupID> m_activationToGroup;
            std::vector<ID> m_negActivation;
    };

    // A wrapper around MUSFinder that lets the user provide the Group IDs
    // so that they don't need to maintain mappings from their IDs to the
    // internal IDs.
    class MUSFinderWrapper
    {
        public:
            MUSFinderWrapper(VariableManager & vars);

            void addHardClause(const Clause & cls) { m_finder.addHardClause(cls); }

            template<typename Iterator>
            void addHardClauses(Iterator begin, Iterator end)
            { m_finder.addHardClauses(begin, end); }

            void addSoftClause(ID external_group, const Clause & cls);

            template<typename Iterator>
            void addSoftClauses(ID external_group, Iterator begin, Iterator end)
            { for (auto it = begin; it != end; ++it) { addSoftClause(external_group, *it); } }

            std::vector<ID> findCore();
            std::vector<ID> findMUS();

        private:
            std::vector<ID> mapCore(const std::vector<MUSGroupID> & internal) const;
            MUSFinder m_finder;
            std::unordered_map<ID, MUSGroupID> m_externalToInternal;
            std::unordered_map<MUSGroupID, ID> m_internalToExternal;
    };

    // Function templates

    template<typename Iterator>
    void MUSFinder::addHardClauses(Iterator begin, Iterator end)
    {
        for (auto it = begin; it != end; ++it)
        {
            const Clause & cls = *it;
            addHardClause(cls);
        }
    }

    template<typename Iterator>
    MUSGroupID MUSFinder::addSoftClauses(Iterator begin, Iterator end)
    {
        MUSGroupID gid = createGroup();
        addSoftClauses(gid, begin, end);
        return gid;
    }

    template<typename Iterator>
    void MUSFinder::addSoftClauses(MUSGroupID group, Iterator begin, Iterator end)
    {
        for (auto it = begin; it != end; ++it)
        {
            const Clause & cls = *it;
            addSoftClause(group, cls);
        }
    }
}

#endif

