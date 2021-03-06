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
#include "pme/util/timer.h"
#include "pme/engine/global_state.h"

#include <cassert>
#include <algorithm>

namespace PME
{
    const GroupID GROUP_NULL = 0;

    SAT::Solver * newSolver(SATBackend backend)
    {
        switch (backend)
        {
            case MINISAT:
                return new SAT::MinisatSolver;
            case GLUCOSE:
                return new SAT::GlucoseSolver;
            case MINISATSIMP:
                return new SAT::MinisatSimplifyingSolver;
            default:
                throw std::logic_error("Unknown SAT backend");
        }
    }

    SATAdaptor::SATAdaptor(SATBackend backend)
        : m_backend(backend)
    {
        reset();
    }

    bool SATAdaptor::s(ID a) { return solve({a}); }
    bool SATAdaptor::s(ID a, ID b) { return solve({a, b}); }
    bool SATAdaptor::s(ID a, ID b, ID c) { return solve({a, b, c}); }

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
        SAT::Clause satcls = toSAT(cls);
        m_solver->addClause(satcls);
    }

    void SATAdaptor::addClauses(const ClauseVec & vec)
    {
        for (const Clause & cls : vec)
        {
            addClause(cls);
        }
    }

    bool SATAdaptor::hasSAT(ID id) const
    {
        ID stripped = strip(id);
        return m_IDToSATMap.count(stripped) > 0;
    }

    SAT::Literal SATAdaptor::toSAT(ID id) const
    {
        bool neg = is_negated(id);
        ID stripped = strip(id);
        SAT::Variable satvar = SATVarOf(stripped);
        return neg ? SAT::negate(satvar) : satvar;
    }

    std::vector<SAT::Literal> SATAdaptor::toSAT(const std::vector<ID> & idvec)
    {
        std::vector<SAT::Literal> satvec;
        satvec.reserve(idvec.size());
        for (ID id : idvec)
        {
            introduceVariable(id);
            satvec.push_back(toSAT(id));
        }
        return satvec;
    }

    ID SATAdaptor::fromSAT(SAT::Literal lit) const
    {
        bool neg = SAT::is_negated(lit);
        SAT::Variable var = SAT::strip(lit);
        ID id = IDOf(var);
        return neg ? negate(id) : id;
    }

    std::vector<ID> SATAdaptor::fromSAT(const std::vector<SAT::Literal> & satvec) const
    {
        std::vector<ID> idvec;
        idvec.reserve(satvec.size());
        for (SAT::Literal lit : satvec)
        {
            idvec.push_back(fromSAT(lit));
        }
        return idvec;
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
        Cube empty_assumps;
        return groupSolve(GROUP_NULL, empty_assumps);
    }

    bool SATAdaptor::solve(const Cube & assumps, Cube * crits)
    {
        return groupSolve(GROUP_NULL, assumps, crits);
    }

    bool SATAdaptor::groupSolve(GroupID group)
    {
        Cube empty_assumps;
        return groupSolve(group, empty_assumps);
    }

    bool SATAdaptor::groupSolve(GroupID group, const Cube & assumps, Cube * crits)
    {
        GlobalState::stats().sat_calls++;
        AutoTimer timer(GlobalState::stats().sat_runtime);
        SAT::Cube satassumps;
        for (ID lit : assumps)
        {
            introduceVariable(lit);
            satassumps.push_back(toSAT(lit));
        }

        if (group != GROUP_NULL)
        {
            assert(m_groups.count(group) > 0);
            satassumps.push_back(group);
        }

        SAT::Cube satcrits;
        bool sat = m_solver->solve(satassumps, crits ? &satcrits : nullptr);

        if (!sat && crits != nullptr)
        {
            crits->reserve(satcrits.size());
            for (SAT::Literal satlit : satcrits)
            {
                if (m_groups.count(satlit) > 0) { continue; }
                ID lit = fromSAT(satlit);
                assert(std::find(assumps.begin(), assumps.end(), lit) != assumps.end());
                crits->push_back(lit);
            }
        }

        return sat;
    }

    bool SATAdaptor::isSAT() const
    {
        return m_solver->isSAT();
    }

    ModelValue SATAdaptor::safeGetAssignment(ID lit) const
    {
        assert(isSAT());
        if (!hasSAT(lit)) { return SAT::UNDEF; }
        SAT::Literal satlit = toSAT(lit);
        return m_solver->getAssignment(satlit);
    }

    ModelValue SATAdaptor::safeGetAssignmentToVar(ID var) const
    {
        assert(!is_negated(var));
        return safeGetAssignment(var);
    }

    ModelValue SATAdaptor::getAssignment(ID lit) const
    {
        ModelValue asgn = safeGetAssignment(lit);
        assert(asgn != SAT::UNDEF);
        return asgn;
    }

    ModelValue SATAdaptor::getAssignmentToVar(ID var) const
    {
        assert(!is_negated(var));
        return getAssignment(var);
    }

    void SATAdaptor::freeze(ID id)
    {
        introduceVariable(id);
        m_solver->freeze(toSAT(id));
    }

    ClauseVec SATAdaptor::simplify()
    {
        ClauseVec simplified;

        m_solver->eliminate();

        for (auto it = m_solver->begin_clauses(); it != m_solver->end_clauses(); ++it)
        {
            const SAT::Clause & satcls = *it;
            Clause cls = fromSAT(satcls);
            simplified.push_back(cls);
        }

        for (auto it = m_solver->begin_trail(); it != m_solver->end_trail(); ++it)
        {
            SAT::Literal satlit = *it;
            ID lit = fromSAT(satlit);
            simplified.push_back({lit});
        }

        return simplified;
    }

    void SATAdaptor::reset()
    {
        m_solver.reset(newSolver(m_backend));
        m_groups.clear();
        m_IDToSATMap.clear();
        m_SATToIDMap.clear();

        // Add the clause for ID_TRUE
        introduceVariable(ID_TRUE);
        addClause({ID_TRUE});
    }

    GroupID SATAdaptor::createGroup()
    {
        GroupID gid = m_solver->newVariable();
        m_groups.insert(gid);
        return gid;
    }

    void SATAdaptor::addGroupClause(GroupID group, const Clause & cls)
    {
        assert(!cls.empty());
        SAT::Clause satcls = toSAT(cls);
        satcls.push_back(SAT::negate(group));
        m_solver->addClause(satcls);
    }

    void SATAdaptor::freeze(id_iterator begin, id_iterator end, bool primes)
    {
        for (auto it = begin; it != end; ++it)
        {
            freeze(*it);
            if (primes) { freeze(prime(*it)); }
        }
    }

    void ClauseDeduplicatingSATAdaptor::addClause(const Clause & cls)
    {
        Clause copy = cls;
        std::sort(copy.begin(), copy.end());

        if (m_clauses.count(copy) == 0)
        {
            m_clauses.insert(copy);
            SATAdaptor::addClause(copy);
        }
    }

    void ClauseDeduplicatingSATAdaptor::reset()
    {
        m_clauses.clear();
        SATAdaptor::reset();
    }
}

