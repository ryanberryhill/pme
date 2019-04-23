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

#ifndef MAP_SOLVER_H_INCLUDED
#define MAP_SOLVER_H_INCLUDED

#include "pme/engine/variable_manager.h"
#include "pme/util/maxsat_solver.h"

namespace PME {
    typedef std::vector<ID> Seed;
    typedef std::pair<bool, Seed> UnexploredResult;

    class MapSolver {
        public:
            template <class It>
            MapSolver(VariableManager & varman, It begin, It end)
                : m_ids(begin, end), m_vars(varman)
            { }

            virtual ~MapSolver() { };

            virtual void blockUp(const Seed & seed);
            virtual void blockDown(const Seed & seed);
            virtual void addClause(const Clause & cls);

            virtual UnexploredResult findMinimalSeed();
            virtual UnexploredResult findMaximalSeed();
            virtual UnexploredResult findSeed() = 0;

            virtual bool checkSeed(const Seed & seed) = 0;

        protected:
            VariableManager & vars() { return m_vars; }
            const std::set<ID> & ids() const { return m_ids; }

            auto begin_ids() const -> decltype(ids().cbegin())
            { return ids().cbegin(); }
            auto end_ids() const -> decltype(ids().cend())
            { return ids().cend(); }

            virtual void addClauseToSolver(const Clause & cls) = 0;

        private:
            std::set<ID> m_ids;
            VariableManager & m_vars;
    };

    class SATArbitraryMapSolver : public MapSolver
    {
        public:
            template <class It>
            SATArbitraryMapSolver(VariableManager & varman, It begin, It end)
                : MapSolver(varman, begin, end)
            { }
            virtual ~SATArbitraryMapSolver() { };

            virtual UnexploredResult findMinimalSeed() override;
            virtual UnexploredResult findMaximalSeed() override;
            virtual UnexploredResult findSeed() override;

            virtual bool checkSeed(const Seed & seed) override;

        protected:
            virtual void addClauseToSolver(const Clause & cls) override;

        private:
            Seed extractSeed() const;
            void grow(Seed & seed);
            void shrink(Seed & seed);

            SATAdaptor m_map;
    };

    class MSU4MapSolver : public MapSolver
    {
        public:
            template <class It>
            MSU4MapSolver(VariableManager & varman, It begin, It end)
                : MapSolver(varman, begin, end),
                  m_map(varman),
                  m_map_inited(false)
            { }
            virtual ~MSU4MapSolver() { };

            virtual UnexploredResult findMinimalSeed() override;
            virtual UnexploredResult findMaximalSeed() override;
            virtual UnexploredResult findSeed() override;

            virtual bool checkSeed(const Seed & seed) override;

        protected:
            virtual void addClauseToSolver(const Clause & cls) override;

            virtual void initSolver() = 0;
            virtual UnexploredResult doFindMinimalSeed();
            virtual UnexploredResult doFindMaximalSeed();
            virtual UnexploredResult doFindSeed() = 0;

            Seed extractSeed() const;

            MSU4MaxSATSolver & map() { return m_map; }
            const MSU4MaxSATSolver & map() const { return m_map; }

        private:
            void initIfNecessary();
            MSU4MaxSATSolver m_map;
            bool m_map_inited;
    };

    class MSU4ArbitraryMapSolver;

    class MSU4MaximalMapSolver : public MSU4MapSolver
    {
        friend class MSU4ArbitraryMapSolver;

        public:
            template <class It>
            MSU4MaximalMapSolver(VariableManager & varman, It begin, It end)
                : MSU4MapSolver(varman, begin, end)
            { }
            virtual ~MSU4MaximalMapSolver() { };

        protected:
            virtual void initSolver() override;
            virtual UnexploredResult doFindMaximalSeed() override;
            virtual UnexploredResult doFindSeed() override;
    };

    class MSU4MinimalMapSolver : public MSU4MapSolver
    {
        friend class MSU4ArbitraryMapSolver;

        public:
            template <class It>
            MSU4MinimalMapSolver(VariableManager & varman, It begin, It end)
                : MSU4MapSolver(varman, begin, end)
            { }
            virtual ~MSU4MinimalMapSolver() { };

        protected:
            virtual void initSolver() override;
            virtual UnexploredResult doFindMinimalSeed() override;
            virtual UnexploredResult doFindSeed() override;
    };

    class MSU4ArbitraryMapSolver : public MapSolver
    {
        public:
            template <class It>
            MSU4ArbitraryMapSolver(VariableManager & varman, It begin, It end)
                : MapSolver(varman, begin, end),
                  m_min(varman, begin, end),
                  m_max(varman, begin, end)
            { }
            virtual ~MSU4ArbitraryMapSolver() { };

            virtual UnexploredResult findMinimalSeed() override;
            virtual UnexploredResult findMaximalSeed() override;
            virtual UnexploredResult findSeed() override;

            virtual bool checkSeed(const Seed & seed) override;

        protected:
            virtual void addClauseToSolver(const Clause & cls) override;

        private:
            MSU4MinimalMapSolver m_min;
            MSU4MaximalMapSolver m_max;

    };
}

#endif

