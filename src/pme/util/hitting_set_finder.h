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

#ifndef HITTING_SET_FINDER_H_INCLUDED
#define HITTING_SET_FINDER_H_INCLUDED

#include "pme/engine/variable_manager.h"
#include "pme/util/maxsat_solver.h"

#include <unordered_set>

namespace PME {
    class HittingSetFinder
    {
        public:
            HittingSetFinder(VariableManager & varman);
            void addSet(const std::vector<ID> & s);

            std::vector<ID> solve();
            void blockSolution(const std::vector<ID> & soln);

            void renew();

        private:
            void addVar(ID lit);
            bool checkSubsumption(const std::vector<ID> & s);

            VariableManager & m_vars;
            std::unordered_set<ID> m_known;
            std::vector<std::vector<ID>> m_sets;
            std::vector<std::vector<ID>> m_blocked;
            std::unique_ptr<MSU4MaxSATSolver> m_solver;
    };
}

#endif

