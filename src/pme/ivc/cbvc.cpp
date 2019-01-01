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

#include "pme/ivc/cbvc.h"
#include "pme/ivc/bvc_solver.h"

#include <cassert>

namespace PME {
    CBVCFinder::CBVCFinder(VariableManager & varman,
                            const TransitionRelation & tr)
        : IVCFinder(varman, tr),
          m_vars(varman),
          m_tr(tr),
          m_gates(tr.begin_gate_ids(), tr.end_gate_ids())
    { }

    std::ostream & CBVCFinder::log(int verbosity) const
    {
        return IVCFinder::log(LOG_CBVC, verbosity);
    }

    void CBVCFinder::doFindIVCs()
    {
        log(2) << "Starting CBVC (there are " << m_gates.size() << " gates)" << std::endl;

        BVCSolver solver(m_vars, m_tr);

        for (unsigned k = 0; ; k++)
        {
            BVCResult br = solver.prove(k);

            if (br.safe())
            {
                log(2) << "The instance is safe (at " << k << ")" << std::endl;
                addMIVC(br.abstraction);
                break;
            }
            else if (br.unsafe())
            {
                log(2) << "The instance is unsafe (at " << k << ")" << std::endl;
                break;
            }
            else
            {
                const BVC & bvc = br.abstraction;
                log(2) << "Found BVC of size " << bvc.size() << " at " << k << std::endl;
                assert(br.unknown());
                addBVC(k, bvc);
            }
        }
    }
}

