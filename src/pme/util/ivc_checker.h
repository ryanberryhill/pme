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

#ifndef IVC_CHECKER_H_INCLUDED
#define IVC_CHECKER_H_INCLUDED

#include "pme/engine/variable_manager.h"
#include "pme/engine/global_state.h"
#include "pme/engine/transition_relation.h"
#include "pme/ivc/ivc.h"

namespace PME {

    class IVCChecker
    {
        public:
            IVCChecker(VariableManager & varman,
                       TransitionRelation & tr,
                       GlobalState & gs);

            bool checkSafe(const IVC & ivc);
            bool checkMinimal(const IVC & ivc);
            bool checkMIVC(const IVC & ivc);

        private:
            VariableManager & m_vars;
            const TransitionRelation & m_tr;
            GlobalState & m_gs;
    };
}

#endif

