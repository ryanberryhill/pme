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

#include "pme/engine/global_state.h"

namespace PME
{
    PMEStats::PMEStats() :
        runtime(0.0),
        ic3_calls(0),
        ic3_runtime(0.0),
        bmc_calls(0),
        bmc_runtime(0.0),
        maxsat_calls(0),
        maxsat_runtime(0.0),
        sat_calls(0),
        sat_runtime(0.0),
        num_mivcs_found(0),
        smallest_mivc_size(0),
        largest_mivc_size(0)
    { }

    void PMEStats::printAll(std::ostream & os) const
    {
        os << "+-------------------+" << std::endl;
        os << "|       STATS       |" << std::endl;
        os << "+-------------------+" << std::endl;

        // Global
        os << "Runtime in seconds: " << runtime << std::endl;
        os << "IC3 Calls: " << ic3_calls << std::endl;
        os << "IC3 Runtime: " << ic3_runtime << std::endl;
        os << "BMC Calls: " << bmc_calls << std::endl;
        os << "BMC Runtime: " << bmc_runtime << std::endl;
        os << "MaxSAT Calls: " << maxsat_calls << std::endl;
        os << "MaxSAT Runtime: " << maxsat_runtime << std::endl;
        os << "SAT Calls: " << sat_calls << std::endl;
        os << "SAT Runtime: " << sat_runtime << std::endl;

        // IVCs
        os << "MIVCs Found: " << num_mivcs_found << std::endl;
        os << "Smallest MIVC: " << smallest_mivc_size << std::endl;
        os << "Largest MIVC: " << largest_mivc_size << std::endl;
    }

    GlobalState& GlobalState::instance()
    {
        static GlobalState gs;
        return gs;
    }

    PMEOptions& GlobalState::options()
    {
        return instance().m_opts;
    }

    PMEStats& GlobalState::stats()
    {
        return instance().m_stats;
    }

    Logger& GlobalState::logger()
    {
        return instance().m_logger;
    }
}
