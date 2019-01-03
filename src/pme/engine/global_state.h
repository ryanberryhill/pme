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
#include "pme/engine/options.h"

namespace PME
{
    struct PMEStats
    {
        // Global
        double runtime;

        size_t ic3_calls;
        double ic3_runtime;

        size_t bmc_calls;
        double bmc_runtime;

        size_t maxsat_calls;
        double maxsat_runtime;

        size_t sat_calls;
        double sat_runtime;

        // IVCs
        size_t num_mivcs_found;
        size_t smallest_mivc_size;
        size_t largest_mivc_size;

        // CAIVC
        unsigned caivc_isivc_calls;
        unsigned caivc_find_candidate_calls;
        unsigned caivc_find_mcs_calls;
        unsigned caivc_more_mcs_calls;
        double caivc_prep_time;
        double caivc_isivc_time;
        double caivc_find_candidate_time;
        double caivc_find_mcs_time;
        double caivc_more_mcs_time;

        // MARCO-IVC
        unsigned marcoivc_issafe_calls;
        unsigned marcoivc_shrink_calls;
        unsigned marcoivc_grow_calls;
        double marcoivc_issafe_time;
        double marcoivc_shrink_time;
        double marcoivc_grow_time;

        void printAll(std::ostream & os) const;

        PMEStats();
    };

    class GlobalState
    {
        private:
            // The logger is mutable because changing it should not be considered
            // as conceptually changing the state of any object with a reference to
            // this GlobalState. i.e., const functions should be able to log
            // Same goes for statistics
            mutable Logger m_logger;
            mutable PMEStats m_stats;
            PMEOptions m_opts;
        public:
            GlobalState() { }

            static GlobalState& instance();
            static PMEOptions& options();
            static PMEStats& stats();
            static Logger& logger();

            GlobalState(const GlobalState &)     = delete;
            void operator=(const GlobalState &)  = delete;
    };
}

#endif

