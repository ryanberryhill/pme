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

#include "pme/ivc/unified_ivc_finder.h"

namespace PME {
    UnifiedIVCFinder::UnifiedIVCFinder(VariableManager & varman,
                                   const TransitionRelation & tr)
        : IVCFinder(varman, tr),
          m_debug_tr(tr),
          m_mcs_finder(varman, m_debug_tr)
    {
    }

    std::ostream & UnifiedIVCFinder::log(int verbosity) const
    {
        return IVCFinder::log(LOG_UNIFIED_IVC, verbosity);
    }

    void UnifiedIVCFinder::doFindIVCs()
    {
        // Check for constant output
        if (tr().bad() == ID_FALSE)
        {
            log(3) << "Output is a literal 0" << std::endl;
            IVC empty;
            addMIVC(empty);
            return;
        }
    }

}
