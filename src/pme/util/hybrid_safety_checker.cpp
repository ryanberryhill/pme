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

#include "pme/util/hybrid_safety_checker.h"

#include <cassert>

namespace PME {
    HybridSafetyChecker::HybridSafetyChecker(VariableManager & varman,
                                             const TransitionRelation & tr,
                                             GlobalState & gs)
        : m_kmax(gs.opts.hybrid_ic3_bmc_kmax),
          m_ic3(varman, tr, gs),
          m_bmc(varman, tr, gs),
          m_gs(gs)
    { }

    void HybridSafetyChecker::setKMax(unsigned k)
    {
        m_kmax = k;
    }

    SafetyResult HybridSafetyChecker::prove()
    {
        SafetyResult result;

        if (m_kmax > 0)
        {
            result = m_bmc.solve(m_kmax);
            if (result.result == UNSAFE) { return result; }
        }

        assert(result.result == UNKNOWN);
        result = m_ic3.prove();

        return result;
    }
}

