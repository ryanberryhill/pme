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

#ifndef SAT_H_INCLUDED
#define SAT_H_INCLUDED

#include <memory>
#include <vector>
#include <unordered_map>

/*
 * This is a very thin wrapper around a SAT solver. The main reason for its
 * existence (as opposed to just having wrappers for different solvers in PME)
 * is to avoid having to include third-party headers (like those of glucose) in
 * the pme library. Glucose's and Minisat's headers generate warnings so they
 * should not be included in this file.
 */

namespace Glucose
{
    class Solver;
    struct Lit;
    typedef int Var;
}

namespace Minisat
{
    class Solver; class SimpSolver; struct Lit;
    typedef int Var;
}

namespace SAT
{
    typedef int Variable;
    typedef int Literal;
    typedef std::vector<Literal> Clause;
    typedef std::vector<Literal> Cube;

    enum ModelValue   { TRUE, FALSE, UNDEF };
    enum SolverResult { SAT, UNSAT, UNKNOWN };

    Literal negate(Literal lit);
    bool is_negated(Literal lit);
    Variable strip(Literal lit);

    template <typename Lit, typename Var> Lit mkLit(Var v, bool neg);
    template<typename Lit> bool sign(Lit lit);
    template<typename Lit, typename Var> Var toVar(Lit lit);

    class Solver
    {
        public:
            typedef std::vector<Clause>::const_iterator clause_iterator;
            typedef std::vector<Literal>::const_iterator literal_iterator;
            virtual ~Solver() { }
            virtual Variable newVariable() = 0;
            virtual void addClause(const Clause & cls) = 0;
            virtual bool solve(const Cube & assumps) = 0;
            virtual ModelValue getAssignment(Variable v) const = 0;
            virtual bool isSAT() const = 0;
            virtual clause_iterator begin_clauses() const = 0;
            virtual clause_iterator end_clauses() const = 0;
            virtual literal_iterator begin_trail() const = 0;
            virtual literal_iterator end_trail() const = 0;
            virtual void freeze(Variable v) = 0;
            virtual void eliminate() = 0;
    };

    template<typename MLit, typename MVar>
    class MinisatStyleSolver : public Solver
    {
        public:
            MinisatStyleSolver();
            virtual ~MinisatStyleSolver() { };
            Variable newVariable() override;
            void addClause(const Clause & cls) override;
            bool solve(const Cube & assumps) override;
            bool isSAT() const override;

            clause_iterator begin_clauses() const override;
            clause_iterator end_clauses() const override;
            literal_iterator begin_trail() const override;
            literal_iterator end_trail() const override;

        protected:
            virtual void sendClauseToSolver(const Clause& cls) = 0;
            virtual Variable createSolverVariable() = 0;
            virtual bool doSolve(const Cube & assumps) = 0;

            MLit toMinisat(Literal lit) const;
            Literal fromMinisat(const MLit & lit) const;
            Variable getNextVar();

            void clearClauses();
            void addStoredClause(const Clause & cls);
            void clearTrail();
            void addToTrail(Literal lit);

        private:
            // Minisat's SolverTypes generates warnings, but the ints below
            // should be Minisat::Var ideally
            std::unordered_map<Variable, int> m_varMap;
            std::unordered_map<int, Variable> m_reverseVarMap;

            Variable m_nextVar;
            std::vector<Clause> m_clauses;
            std::vector<Literal> m_trail;
            SolverResult m_lastResult;
    };

    class MinisatSolver : public MinisatStyleSolver<Minisat::Lit, Minisat::Var>
    {
        public:
            MinisatSolver();
            ~MinisatSolver();
            ModelValue getAssignment(Variable v) const override;
            void freeze(Variable v) override { }
            void eliminate() override { }

        protected:
            void sendClauseToSolver(const Clause& cls) override;
            Variable createSolverVariable() override;
            bool doSolve(const Cube & assumps) override;

        private:
            std::unique_ptr<Minisat::Solver> m_solver;
    };

    class MinisatSimplifyingSolver : public MinisatStyleSolver<Minisat::Lit, Minisat::Var>
    {
        public:
            MinisatSimplifyingSolver();
            ~MinisatSimplifyingSolver();
            ModelValue getAssignment(Variable v) const override;
            void freeze(Variable v) override;
            void eliminate() override;

        protected:
            void sendClauseToSolver(const Clause& cls) override;
            Variable createSolverVariable() override;
            bool doSolve(const Cube & assumps) override;

        private:
            std::unique_ptr<Minisat::SimpSolver> m_solver;
    };

    class GlucoseSolver : public MinisatStyleSolver<Glucose::Lit, Glucose::Var>
    {
        public:
            GlucoseSolver();
            ~GlucoseSolver();
            ModelValue getAssignment(Variable v) const override;
            void freeze(Variable v) override { }
            void eliminate() override { }

        protected:
            void sendClauseToSolver(const Clause& cls) override;
            Variable createSolverVariable() override;
            bool doSolve(const Cube & assumps) override;

        private:
            std::unique_ptr<Glucose::Solver> m_solver;
    };
}

#endif

