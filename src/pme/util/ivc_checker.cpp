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

#include "pme/util/ivc_checker.h"

#include "pme/engine/debug_transition_relation.h"
#include "pme/util/ic3_debugger.h"
#include "pme/ic3/ic3_solver.h"

#include <cassert>

namespace PME {
    IVCChecker::IVCChecker(VariableManager & varman,
                           TransitionRelation & tr,
                           GlobalState & gs)
       : m_vars(varman),
         m_tr(tr),
         m_gs(gs)
    { }

    bool IVCChecker::checkSafe(const IVC & ivc)
    {
        TransitionRelation partial(m_tr, ivc);
        IC3::IC3Solver solver(m_vars, partial, m_gs);
        SafetyResult result = solver.prove();
        assert(result.result != UNKNOWN);
        return result.result == SAFE;
    }

    bool IVCChecker::checkMinimal(const IVC & ivc)
    {
        TransitionRelation partial(m_tr, ivc);
        DebugTransitionRelation debug_tr(partial);
        IC3Debugger debugger(m_vars, debug_tr, m_gs);
        debugger.setCardinality(1);

        unsigned soln_count = 0;
        bool unsafe = true;

        while(unsafe)
        {
            std::tie(unsafe, std::ignore) = debugger.debugAndBlock();
            if (unsafe) { soln_count++; }
        }

        return soln_count == ivc.size();
    }

    bool IVCChecker::checkMIVC(const IVC & ivc)
    {
        return checkSafe(ivc) && checkMinimal(ivc);
    }
}

