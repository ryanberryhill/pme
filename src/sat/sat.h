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

namespace Glucose { class Solver; class Lit; }
namespace Minisat { class Solver; class Lit; }

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

    class Solver
    {
        public:
            virtual ~Solver() { };
            virtual Variable newVariable() = 0;
            virtual void addClause(const Clause& cls) = 0;
            virtual bool solve(const Cube& assumps) = 0;
            virtual ModelValue getAssignment(Variable v) const = 0;
            virtual bool isSAT() const = 0;
    };

    class GlucoseSolver : public Solver
    {
        public:
            GlucoseSolver();
            ~GlucoseSolver() override;
            Variable newVariable() override;
            void addClause(const Clause& cls) override;
            bool solve(const Cube& assumps) override;
            ModelValue getAssignment(Variable v) const override;
            bool isSAT() const override;

        private:
            std::unique_ptr<Glucose::Solver> m_solver;
            // I don't want to include glucose SolverTypes.h in here because
            // it induces warnings and this file will be included in pme.
            // So int should be Glucose::Var ideally in m_varMap below
            std::unordered_map<Variable, int> m_varMap;
            Variable m_nextVar;
            SolverResult m_lastResult;

            Glucose::Lit toGlucose(Literal lit) const;
            Variable getNextVar();
    };

    class MinisatSolver : public Solver
    {
        public:
            MinisatSolver();
            ~MinisatSolver() override;
            Variable newVariable() override;
            void addClause(const Clause& cls) override;
            bool solve(const Cube& assumps) override;
            ModelValue getAssignment(Variable v) const override;
            bool isSAT() const override;

        private:
            std::unique_ptr<Minisat::Solver> m_solver;
            // Minisat's SolverTypes generates warnings, but the int below
            // should be Minisat::Var ideally
            std::unordered_map<Variable, int> m_varMap;
            Variable m_nextVar;
            SolverResult m_lastResult;

            Minisat::Lit toMinisat(Literal lit) const;
            Variable getNextVar();
    };

    Literal negate(Literal lit);
    bool is_negated(Literal lit);
    Variable strip(Literal lit);
}

#endif

