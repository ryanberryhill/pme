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

    class MCSFinder
    {
        public:
            typedef std::pair<bool, CorrectionSet> FindResult;
            MCSFinder(VariableManager & varman,
                      DebugTransitionRelation & tr);

            bool moreCorrectionSets();
            FindResult findAndBlock();
            FindResult findAndBlockOverGates(const std::vector<ID> & gates);
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
            typedef std::pair<bool, CorrectionSet> FindResult;
            ApproximateMCSFinder(VariableManager & varman,
                                 DebugTransitionRelation & tr);

            FindResult findAndBlockWithBMC(unsigned n);
            FindResult findAndBlockOverGatesWithBMC(const std::vector<ID> & gates, unsigned n);
            FindResult findAndBlockOverGates(const std::vector<ID> & gates);
            FindResult findAndBlockOverGates(const std::vector<ID> & gates, unsigned n_max);
            void blockSolution(const CorrectionSet & corr);

        private:
            FindResult findFallback(const std::vector<ID> & gates);

            unsigned m_cardinality;
            IC3Debugger m_fallback;
            BMCDebugger m_solver;
    };
}

#endif

