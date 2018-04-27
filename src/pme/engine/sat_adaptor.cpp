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
#include "pme/engine/sat_adaptor.h"

#include <cassert>

namespace PME
{
    SATAdaptor::SATAdaptor()
        : m_solver(new SAT::GlucoseSolver)
    {
       // Add the clause for ID_TRUE
       introduceVariable(ID_TRUE);
       addClause({ID_TRUE});
    }

    void SATAdaptor::introduceVariable(ID id)
    {
        ID stripped = strip(id);
        auto it = m_IDToSATMap.find(stripped);
        if (it == m_IDToSATMap.end())
        {
            SAT::Variable satvar = m_solver->newVariable();
            m_IDToSATMap[stripped] = satvar;
            assert(m_SATToIDMap.find(satvar) == m_SATToIDMap.end());
            m_SATToIDMap[satvar] = stripped;
        }
    }

    void SATAdaptor::addClause(const Clause & cls)
    {
        assert(!cls.empty());
        SAT::Clause satcls;
        for (ID id : cls)
        {
            introduceVariable(id);
            SAT::Literal satlit = toSAT(id);
            satcls.push_back(satlit);
        }
        m_solver->addClause(satcls);
    }

    void SATAdaptor::addClauses(const ClauseVec & vec)
    {
        for (const Clause & cls : vec)
        {
            addClause(cls);
        }
    }

    SAT::Literal SATAdaptor::toSAT(ID id) const
    {
        bool neg = is_negated(id);
        ID stripped = strip(id);
        SAT::Variable satvar = SATVarOf(stripped);
        return neg ? SAT::negate(satvar) : satvar;
    }

    SAT::Variable SATAdaptor::SATVarOf(ID id) const
    {
        return m_IDToSATMap.at(id);
    }

    ID SATAdaptor::IDOf(SAT::Variable var) const
    {
        return m_SATToIDMap.at(var);
    }

    bool SATAdaptor::solve()
    {
        Cube assumps;
        return solve(assumps);
    }

    bool SATAdaptor::solve(const Cube & assumps)
    {
        SAT::Cube satassumps;
        for (ID lit : assumps)
        {
            introduceVariable(lit);
            satassumps.push_back(toSAT(lit));
        }
        return m_solver->solve(satassumps);
    }

    bool SATAdaptor::isSAT() const
    {
        return m_solver->isSAT();
    }

    ModelValue SATAdaptor::getAssignment(ID lit) const
    {
        assert(isSAT());
        SAT::Literal satlit = toSAT(lit);
        return m_solver->getAssignment(satlit);
    }

    ModelValue SATAdaptor::getAssignmentToVar(ID var) const
    {
        assert(!is_negated(var));
        return getAssignment(var);
    }
}

