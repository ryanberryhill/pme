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

#include "pme/id.h"
#include "pme/engine/variable_manager.h"

#define BOOST_TEST_MODULE VariableManagerTest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <set>

using namespace PME;

BOOST_AUTO_TEST_CASE(new_ids_are_valid)
{
    VariableManager v;
    ID id0 = v.getNewID();
    BOOST_CHECK(v.isKnown(id0));
    BOOST_CHECK(is_valid_id(id0));
    ID id1 = v.getNewID();
    BOOST_CHECK(is_valid_id(id1));
    BOOST_CHECK(v.isKnown(id1));
}

BOOST_AUTO_TEST_CASE(ids_are_unique)
{
    std::set<ID> ids;
    VariableManager v;

    for (unsigned i = 0; i < 65536; ++i)
    {
        ID id = v.getNewID();
        BOOST_CHECK(ids.count(id) == 0);
        ids.insert(id);
    }
}

BOOST_AUTO_TEST_CASE(make_internal)
{
    VariableManager v;
    ExternalClause cls0 { 2, 4, 6 };

    ID id0 = v.getNewID("external_a", cls0[0]);
    ID id1 = v.getNewID("external_b", cls0[1]);
    ID id2 = v.getNewID("external_c", cls0[2]);

    Clause internal0 = v.makeInternal(cls0);
    std::sort(internal0.begin(), internal0.end());
    Clause expected = { id0, id1, id2 };
    std::sort(expected.begin(), expected.end());

    BOOST_CHECK(internal0 == expected);

    ExternalClause cls1 { cls0[0], cls0[1], 8 };
    ID id3 = v.getNewID("external_d", cls1[2]);

    Clause internal1 = v.makeInternal(cls1);
    std::sort(internal1.begin(), internal1.end());
    expected = { id0, id1, id3 };
    std::sort(expected.begin(), expected.end());

    BOOST_CHECK(internal1 == expected);

    ExternalClauseVec vec = { cls0, cls1 };
    ClauseVec internalVec = v.makeInternal(vec);
    for (Clause c : internalVec)
    {
        std::sort(c.begin(), c.end());
    }
    std::sort(internalVec.begin(), internalVec.end());
    ClauseVec expectedVec = { internal0, internal1 };
    std::sort(expectedVec.begin(), expectedVec.end());

    BOOST_CHECK(expectedVec == internalVec);
}

BOOST_AUTO_TEST_CASE(to_external_and_internal)
{
    VariableManager v;

    ID id0 = v.getNewID("external_a", 2);
    ID id1 = v.getNewID("external_a", 4);

    BOOST_CHECK(v.toExternal(id0) == 2);
    BOOST_CHECK(v.toExternal(id1) == 4);
    BOOST_CHECK(v.toInternal(2) == id0);
    BOOST_CHECK(v.toInternal(4) == id1);

    // id0 + id1 + 2 is guaranteed not to equal id0 or id1
    ID non_existent = id0 + id1 + 2;
    BOOST_CHECK_THROW(v.toExternal(non_existent), std::invalid_argument);
    BOOST_CHECK_THROW(v.toInternal(6), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(var_of)
{
    VariableManager v;
    ID id = v.getNewID("variable", 2);

    BOOST_CHECK_THROW(v.varOf(id + 2), std::exception);

    const Variable var = v.varOf(id);

    BOOST_CHECK_EQUAL(var.m_name, "variable");
    BOOST_CHECK_EQUAL(var.m_externalID, 2);
    BOOST_CHECK_EQUAL(var.m_ID, id);
}

