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
#include "pme/engine/sat_adaptor.h"

#include <unordered_map>
#include <algorithm>

#define BOOST_TEST_MODULE MUSFinderTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using namespace PME;

struct MUSFixture
{
    VariableManager vars;
    MUSFinder finder;
    std::unordered_map<MUSGroupID, std::vector<Clause>> m_groups;
    std::vector<Clause> m_hardClauses;

    MUSFixture() : finder(vars)
    { }

    void addHardClauses(const ClauseVec& clauses)
    {
        m_hardClauses.insert(m_hardClauses.end(), clauses.begin(), clauses.end());
        finder.addHardClauses(clauses.begin(), clauses.end());
    }

    void addHardClause(const Clause & cls)
    {
        m_hardClauses.push_back(cls);
        finder.addHardClause(cls);
    }

    MUSGroupID addSoftClause(const Clause & cls)
    {
        MUSGroupID g = finder.addSoftClause(cls);
        m_groups[g].push_back(cls);
        return g;
    }

    MUSGroupID addSoftClauses(const ClauseVec& clauses)
    {
        MUSGroupID g = finder.addSoftClauses(clauses.begin(), clauses.end());
        m_groups[g] = clauses;
        return g;
    }

    bool isUNSAT(const std::vector<MUSGroupID> & core)
    {
        SATAdaptor solver;
        solver.addClauses(m_hardClauses);

        std::set<MUSGroupID> core_set(core.begin(), core.end());

        size_t found_groups = 0;
        for (auto p : m_groups)
        {
            MUSGroupID gid = p.first;

            if (core_set.count(gid) == 0) { continue; }

            // Add clauses from this group if they're in the core
            const std::vector<Clause> & clauses = p.second;
            assert(!clauses.empty());
            solver.addClauses(clauses);
            found_groups++;
        }

        assert(found_groups == core.size());

        return !solver.solve();
    }

    bool isMUS(const std::vector<MUSGroupID> & core)
    {
        // If we're not UNSAT, we're not a MUS
        if (!isUNSAT(core)) { return false; }

        for (auto it = core.begin(); it != core.end(); ++it)
        {
            MUSGroupID gid = *it;

            // sub = core - gid
            std::vector<MUSGroupID> sub;
            std::remove_copy(core.begin(), core.end(), std::back_inserter(sub), gid);

            // If a subset is UNSAT, we're not a MUS
            if (isUNSAT(sub)) { return false; }
        }

        return true;
    }
};

BOOST_AUTO_TEST_CASE(basic_mus_all_soft)
{
    MUSFixture f;

    ID a = f.vars.getNewID();
    ID b = f.vars.getNewID();
    ID c = f.vars.getNewID();

    f.addSoftClause({negate(a), negate(b), negate(c)});
    f.addSoftClause({a});
    f.addSoftClause({b});
    f.addSoftClause({c});

    std::vector<MUSGroupID> core = f.finder.findCore();
    std::vector<MUSGroupID> mus = f.finder.findMUS();

    BOOST_CHECK(f.isUNSAT(core));
    BOOST_CHECK(f.isMUS(mus));
}

BOOST_AUTO_TEST_CASE(basic_mus_hard_soft)
{
    MUSFixture f;

    ID a = f.vars.getNewID();
    ID b = f.vars.getNewID();
    ID c = f.vars.getNewID();

    f.addSoftClause({negate(a), negate(b), negate(c)});
    f.addSoftClause({a});
    f.addSoftClause({b, negate(c)});

    f.addHardClause({c});

    std::vector<MUSGroupID> core = f.finder.findCore();
    std::vector<MUSGroupID> mus = f.finder.findMUS();

    BOOST_CHECK(f.isUNSAT(core));
    BOOST_CHECK(f.isMUS(mus));
}

BOOST_AUTO_TEST_CASE(all_hard)
{
    MUSFixture f;

    ID a = f.vars.getNewID();
    ID b = f.vars.getNewID();
    ID c = f.vars.getNewID();

    f.addHardClause({negate(a), negate(b), negate(c)});
    f.addHardClause({a});
    f.addHardClause({b});
    f.addHardClause({c});

    std::vector<MUSGroupID> core = f.finder.findCore();
    std::vector<MUSGroupID> mus = f.finder.findMUS();

    BOOST_CHECK(f.isUNSAT(core));
    BOOST_CHECK(f.isMUS(mus));

    BOOST_CHECK(mus.empty());
}

BOOST_AUTO_TEST_CASE(group_mus)
{
    MUSFixture f;

    ID a = f.vars.getNewID();
    ID b = f.vars.getNewID();
    ID c = f.vars.getNewID();
    ID d = f.vars.getNewID();
    ID e = f.vars.getNewID();

    ClauseVec g1 = { {a, b, c}, {d, e} };
    ClauseVec g2 = { {negate(a)}, {negate(b)}, {negate(c)} };
    ClauseVec g3 = { {negate(d), negate(e)} };
    ClauseVec g4 = { {c, d} };
    ClauseVec g5 = { {negate(d)} };

    f.addHardClauses(g1);
    f.addSoftClauses(g2);
    f.addSoftClauses(g3);
    f.addSoftClauses(g4);
    f.addSoftClauses(g5);

    std::vector<MUSGroupID> core = f.finder.findCore();
    std::vector<MUSGroupID> mus = f.finder.findMUS();

    BOOST_CHECK(f.isUNSAT(core));
    BOOST_CHECK(f.isMUS(mus));
}

BOOST_AUTO_TEST_CASE(wrapper)
{
    VariableManager vars;
    MUSFinderWrapper finder(vars);

    ID a = vars.getNewID();
    ID b = vars.getNewID();
    ID c = vars.getNewID();
    ID d = vars.getNewID();

    // Aribitrary IDs, some of which may be actual variables
    ID g_1 = 4;
    ID g_2 = 30;
    ID g_3 = 20;
    ID g_4 = 100;

    // Group 1 (a V b V c)
    finder.addSoftClause(g_1, {a, b, c});

    // Group 2 (~a) (~b)
    finder.addSoftClause(g_2, {negate(a)});
    finder.addSoftClause(g_2, {negate(b)});

    // Group 3 (~c)
    finder.addSoftClause(g_3, {negate(c)});

    // Group 4 (a V d) (b V d) (c V d)
    finder.addSoftClause(g_4, {a, d});
    finder.addSoftClause(g_4, {b, d});
    finder.addSoftClause(g_4, {c, d});

    std::vector<ID> core = finder.findCore();
    std::vector<ID> mus = finder.findMUS();

    std::vector<ID> expected_mus = {g_1, g_2, g_3};
    std::sort(expected_mus.begin(), expected_mus.end());

    std::sort(core.begin(), core.end());
    std::sort(mus.begin(), mus.end());

    BOOST_CHECK(mus == expected_mus);

    // Check that the core contains the MUS
    for (ID group : expected_mus)
    {
        BOOST_CHECK(std::find(core.begin(), core.end(), group) != core.end());
    }
}

