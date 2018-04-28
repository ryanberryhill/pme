#include "sat/sat.h"

namespace SAT
{
    Literal negate(Literal lit)
    {
        return -lit;
    }

    bool is_negated(Literal lit)
    {
        return lit < 0;
    }

    Variable strip(Literal lit)
    {
        return is_negated(lit) ? -lit : lit;
    }

    void Solver::addClause(const Clause & cls)
    {
        m_clauses.push_back(cls);
        sendClauseToSolver(cls);
    }

    auto Solver::begin_clauses() const -> decltype(m_clauses.cbegin())
    {
        return m_clauses.cbegin();
    }

    auto Solver::end_clauses() const -> decltype(m_clauses.cbegin())
    {
        return m_clauses.cend();
    }

    void SimplifyingSolver::clearClauses()
    {
        m_clauses.clear();
    }

    void SimplifyingSolver::addStoredClause(const Clause & cls)
    {
        m_clauses.push_back(cls);
    }
}

