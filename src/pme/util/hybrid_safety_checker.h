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

#ifndef HYBRID_SAFETY_CHECKER_H_INCLUDED
#define HYBRID_SAFETY_CHECKER_H_INCLUDED

#include "pme/bmc/bmc_solver.h"
#include "pme/ic3/ic3_solver.h"

namespace PME {
    class HybridSafetyChecker
    {
        public:
            HybridSafetyChecker(VariableManager & varman,
                                const TransitionRelation & tr,
                                GlobalState & gs);
            void setKMax(unsigned k);
            SafetyResult prove();
        private:
            unsigned m_kmax;
            IC3::IC3Solver m_ic3;
            BMC::BMCSolver m_bmc;
            GlobalState & m_gs;
    };
}

#endif

