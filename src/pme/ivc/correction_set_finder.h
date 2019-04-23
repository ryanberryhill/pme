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

#ifndef CORRECTION_SET_FINDER_H_INCLUDED
#define CORRECTION_SET_FINDER_H_INCLUDED

#include "pme/util/hybrid_debugger.h"

namespace PME {

    typedef std::vector<ID> CorrectionSet;
    typedef std::pair<bool, CorrectionSet> FindMCSResult;

    class MCSFinder
    {
        public:
            MCSFinder(VariableManager & varman,
                      DebugTransitionRelation & tr);

            bool moreCorrectionSets();
            FindMCSResult findAndBlock();
            FindMCSResult findAndBlockOverGates(const std::vector<ID> & gates);
            void setCardinality(unsigned n);

            void blockSolution(const CorrectionSet & corr);

        private:
            unsigned m_cardinality;
            HybridDebugger m_solver;
            HybridDebugger m_solver_inf;
    };

    class ApproximateMCSFinder
    {
        public:
            ApproximateMCSFinder(VariableManager & varman,
                                 DebugTransitionRelation & tr);

            FindMCSResult findAndBlockWithBMC(unsigned n);
            FindMCSResult findAndBlockOverGatesWithBMC(const std::vector<ID> & gates, unsigned n);
            FindMCSResult findAndBlockOverGates(const std::vector<ID> & gates);
            FindMCSResult findAndBlockOverGates(const std::vector<ID> & gates, unsigned n_max);
            void blockSolution(const CorrectionSet & corr);

        private:
            FindMCSResult findFallback(const std::vector<ID> & gates);

            unsigned m_cardinality;
            IC3Debugger m_fallback;
            BMCDebugger m_solver;
    };

    class CorrectionSetFinder
    {
        public:
            CorrectionSetFinder(VariableManager & varman,
                                const DebugTransitionRelation & tr);
            virtual FindMCSResult findNext(const std::vector<ID> & gates, unsigned n) = 0;
            virtual FindMCSResult findNext(const std::vector<ID> & gates);
            virtual FindMCSResult findNext(unsigned n);
            virtual FindMCSResult findNext();

            virtual bool moreCorrectionSets(unsigned n) = 0;
            virtual bool moreCorrectionSets();

            virtual void block(const CorrectionSet & corr) = 0;

        protected:
            const DebugTransitionRelation & tr() const { return m_tr; }
            VariableManager & vars() { return m_vars; }

        private:
            VariableManager & m_vars;
            const DebugTransitionRelation & m_tr;
    };

    class BasicMCSFinder : public CorrectionSetFinder
    {
        public:
            BasicMCSFinder(VariableManager & varman, const DebugTransitionRelation & tr);

            using CorrectionSetFinder::findNext;
            virtual FindMCSResult findNext(const std::vector<ID> & gates, unsigned n) override;
            // Override this one to ensure it won't call the one that takes
            // gates as a parameter, as it might have significant performance
            // implications
            virtual FindMCSResult findNext(unsigned n) override;

            using CorrectionSetFinder::moreCorrectionSets;
            virtual bool moreCorrectionSets(unsigned n) override;

            virtual void block(const CorrectionSet & corr) override;
        private:
            FindMCSResult doFind(const std::vector<ID> & gates, unsigned n);
            FindMCSResult doDebug(const std::vector<ID> & gates);

            void setCardinality(unsigned n);

            HybridDebugger m_solver;
            unsigned m_cardinality;
    };

    class BMCCorrectionSetFinder : public CorrectionSetFinder
    {
        public:
            BMCCorrectionSetFinder(VariableManager & varman,
                                   const DebugTransitionRelation & tr);

            using CorrectionSetFinder::findNext;
            virtual FindMCSResult findNext(const std::vector<ID> & gates, unsigned n) override;
            // Override this to ensure it doesn't call the one above
            virtual FindMCSResult findNext(unsigned n) override;

            using CorrectionSetFinder::moreCorrectionSets;
            virtual bool moreCorrectionSets(unsigned n) override;

            virtual void block(const CorrectionSet & corr) override;

        private:
            bool moreCorrectionSetsBMC(unsigned n);
            bool moreCorrectionSetsIC3(unsigned n);
            void setBMCCardinality(unsigned n);

            BMCDebugger m_bmc;
            IC3Debugger m_ic3;
            unsigned m_cardinality;
            unsigned m_current_k;

    };
}

#endif

