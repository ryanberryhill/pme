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

#ifndef GLOBAL_STATE_H_INCLUDED
#define GLOBAL_STATE_H_INCLUDED

#include "pme/engine/logger.h"

namespace PME
{
    struct PMEOptions
    {
        // Global
        bool simplify;
        unsigned hybrid_debug_bmc_frames;

        // CAMSIS
        bool camsis_abstraction_refinement;

        // CAIVC
        bool caivc_use_bmc;
        bool caivc_abstraction_refinement;
        unsigned caivc_ar_bmc_kmax;

        PMEOptions();
    };

    struct GlobalState
    {
        // The logger is mutable because changing it should not be considered
        // as conceptually changing the state of any object with a reference to
        // this GlobalState. i.e., const functions should be able to log
        mutable Logger logger;
        PMEOptions opts;
    };

    extern GlobalState g_null_gs;
}

#endif

