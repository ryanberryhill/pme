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

#include "pme/util/mus_finder.h"

#include <sstream>
#include <cassert>
#include <algorithm>

namespace PME {

    std::string groupActivationName(MUSGroupID id)
    {
        std::ostringstream ss;
        ss << "act_musgroup_" << id;
        return ss.str();
    }

    MUSFinder::MUSFinder(VariableManager & vars)
        : m_vars(vars),
          m_nextGroup(0)
    { }


    void MUSFinder::addHardClause(const Clause & cls)
    {
        m_solver.addClause(cls);
    }

    MUSGroupID MUSFinder::createGroup()
    {
        MUSGroupID this_group = m_nextGroup;
        m_nextGroup++;

        ID act = m_vars.getNewID(groupActivationName(this_group));
        assert(m_groupToActivation.count(this_group) == 0);
        m_groupToActivation[this_group] = act;
        m_activationToGroup[act] = this_group;
        m_negActivation.push_back(negate(act));

        return this_group;
    }

    MUSGroupID MUSFinder::addSoftClause(const Clause & cls)
    {
        MUSGroupID gid = createGroup();

        addSoftClause(gid, cls);

        return gid;
    }

    void MUSFinder::addSoftClause(MUSGroupID group, const Clause & cls)
    {
        ID act = activationForGroup(group);
        Clause activated = cls;
        activated.push_back(negate(act));

        m_solver.addClause(activated);
    }

    ID MUSFinder::activationForGroup(MUSGroupID gid) const
    {
        return m_groupToActivation.at(gid);
    }

    MUSGroupID MUSFinder::groupForActivation(ID act) const
    {
        return m_activationToGroup.at(act);
    }

    std::vector<MUSGroupID> MUSFinder::findCore()
    {
        // Start with core = <all groups>
        std::vector<MUSGroupID> core;
        core.reserve(m_groupToActivation.size());
        for (auto p : m_groupToActivation) { core.push_back(p.first); }

        // Find UNSAT core
        bool unsat = isUNSAT(core);
        ((void)(unsat));
        assert(unsat);

        return core;
    }

    std::vector<MUSGroupID> MUSFinder::findMUS()
    {
        // Find an initial core of the whole formula
        std::vector<MUSGroupID> mus = findCore();

        // For each group in the core, try to remove it and see if the
        // result is still UNSAT. If so, replace mus by the UNSAT core.
        std::sort(mus.begin(), mus.end());
        for (auto it = mus.begin(); it != mus.end(); )
        {
            std::vector<MUSGroupID> candidate;
            candidate.reserve(mus.size() - 1);
            MUSGroupID gid = *it;

            // candidate = mus - gid
            std::remove_copy(mus.begin(), mus.end(), std::back_inserter(candidate), gid);

            if (isUNSAT(candidate))
            {
                // Replace mus with candidate, sort, and point it to the
                // correct spot (first element greater than gid)
                mus = candidate;
                std::sort(mus.begin(), mus.end());
                it = std::upper_bound(mus.begin(), mus.end(), gid);
            }
            else
            {
                // Try the next element
                ++it;
            }
        }

        return mus;
    }

    bool MUSFinder::isUNSAT(std::vector<MUSGroupID> & core)
    {
        Cube assumps;
        for (MUSGroupID gid : core)
        {
            ID act = activationForGroup(gid);
            assumps.push_back(act);
        }

        Cube crits;
        bool sat = m_solver.solve(assumps, &crits);

        if (sat) { return false; }

        core.clear();
        for (ID act : crits)
        {
            MUSGroupID gid = groupForActivation(act);
            core.push_back(gid);
        }

        return true;
    }

    std::vector<ID> MUSFinderWrapper::mapCore(const std::vector<MUSGroupID> & internal) const
    {
        std::vector<ID> external;
        external.reserve(internal.size());

        for (MUSGroupID gid : internal)
        {
            ID ext_id = m_internalToExternal.at(gid);
            external.push_back(ext_id);
        }

        return external;
    }

    MUSFinderWrapper::MUSFinderWrapper(VariableManager & vars)
        : m_finder(vars)
    { }

    void MUSFinderWrapper::addSoftClause(ID external_group, const Clause & cls)
    {
        if (m_externalToInternal.count(external_group) == 0)
        {
            MUSGroupID internal_group = m_finder.createGroup();
            m_externalToInternal[external_group] = internal_group;
            assert(m_internalToExternal.count(internal_group) == 0);
            m_internalToExternal[internal_group] = external_group;
        }

        MUSGroupID internal_group = m_externalToInternal.at(external_group);
        m_finder.addSoftClause(internal_group, cls);
    }

    std::vector<ID> MUSFinderWrapper::findCore()
    {
        std::vector<MUSGroupID> core = m_finder.findCore();
        return mapCore(core);
    }

    std::vector<ID> MUSFinderWrapper::findMUS()
    {
        std::vector<MUSGroupID> core = m_finder.findMUS();
        return mapCore(core);
    }
}

