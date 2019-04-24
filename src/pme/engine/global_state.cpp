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

#include <limits>

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
        num_clauses(0),
        num_msis_found(0),
        smallest_msis_size(std::numeric_limits<size_t>::max()),
        largest_msis_size(0),
        marco_get_unexplored_calls(0),
        marco_issis_calls(0),
        marco_shrink_calls(0),
        marco_grow_calls(0),
        marco_find_collapse_calls(0),
        marco_get_unexplored_time(0.0),
        marco_issis_time(0.0),
        marco_shrink_time(0.0),
        marco_grow_time(0.0),
        marco_find_collapse_time(0.0),
        camsis_extract_calls(0),
        camsis_prep_time(0.0),
        camsis_extract_time(0.0),
        num_mivcs_found(0),
        smallest_mivc_size(std::numeric_limits<size_t>::max()),
        largest_mivc_size(0),
        num_gates(0),
        caivc_isivc_calls(0),
        caivc_find_candidate_calls(0),
        caivc_find_mcs_calls(0),
        caivc_more_mcs_calls(0),
        caivc_correction_sets_found(0),
        caivc_prep_time(0.0),
        caivc_isivc_time(0.0),
        caivc_find_candidate_time(0.0),
        caivc_find_mcs_time(0.0),
        caivc_more_mcs_time(0.0),
        marcoivc_get_unexplored_calls(0),
        marcoivc_issafe_calls(0),
        marcoivc_shrink_calls(0),
        marcoivc_grow_calls(0),
        marcoivc_get_unexplored_time(0.0),
        marcoivc_issafe_time(0.0),
        marcoivc_shrink_time(0.0),
        marcoivc_grow_time(0.0)
    { }

    void PMEStats::printAll(std::ostream & os) const
    {
        os << "+-------------------+" << std::endl;
        os << "|       STATS       |" << std::endl;
        os << "+-------------------+" << std::endl;

        // Global
        os << "Runtime in seconds: " << runtime << std::endl;
        os << "Gate Count: " << num_gates << std::endl;
        os << "IC3 Calls: " << ic3_calls << std::endl;
        os << "IC3 Runtime: " << ic3_runtime << std::endl;
        os << "BMC Calls: " << bmc_calls << std::endl;
        os << "BMC Runtime: " << bmc_runtime << std::endl;
        os << "MaxSAT Calls: " << maxsat_calls << std::endl;
        os << "MaxSAT Runtime: " << maxsat_runtime << std::endl;
        os << "SAT Calls: " << sat_calls << std::endl;
        os << "SAT Runtime: " << sat_runtime << std::endl;

        // Proof Minimization
        os << "Clauses in proof: " << num_clauses << std::endl;
        os << "MSISes Found: " << num_msis_found << std::endl;
        if (num_msis_found > 0)
        {
            os << "Smallest MSIS: " << smallest_msis_size << std::endl;
            os << "Largest MSIS: " << largest_msis_size << std::endl;
        }

        // MARCO-MSIS
        os << "MARCO-MSIS getUnexplored Calls: " << marco_get_unexplored_calls << std::endl;
        os << "MARCO-MSIS getUnexplored Time: " << marco_get_unexplored_time << std::endl;
        os << "MARCO-MSIS isSIS Calls: " << marco_issis_calls << std::endl;
        os << "MARCO-MSIS isSIS Time: " << marco_issis_time << std::endl;
        os << "MARCO-MSIS findSIS Calls: " << marco_findsis_calls << std::endl;
        os << "MARCO-MSIS findSIS Time: " << marco_findsis_time << std::endl;
        os << "MARCO-MSIS grow Calls: " << marco_grow_calls << std::endl;
        os << "MARCO-MSIS grow Time: " << marco_grow_time << std::endl;
        os << "MARCO-MSIS shrink Calls: " << marco_shrink_calls << std::endl;
        os << "MARCO-MSIS shrink Time: " << marco_shrink_time << std::endl;
        os << "MARCO-MSIS findCollapse Calls: " << marco_find_collapse_calls << std::endl;
        os << "MARCO-MSIS findCollapse Time: " << marco_find_collapse_time << std::endl;

        // CAMSIS
        os << "CAMSIS Preparation Time: " << camsis_prep_time << std::endl;
        os << "CAMSIS extractCandidate Calls: " << camsis_extract_calls << std::endl;
        os << "CAMSIS extractCandidate Time: " << camsis_extract_time << std::endl;
        os << "CAMSIS isSIS Calls: " << camsis_issis_calls << std::endl;
        os << "CAMSIS isSIS Time: " << camsis_issis_time << std::endl;
        os << "CAMSIS findCollapse Calls: " << camsis_find_collapse_calls << std::endl;
        os << "CAMSIS findCollapse Time: " << camsis_find_collapse_time << std::endl;

        // IVCs
        os << "MIVCs Found: " << num_mivcs_found << std::endl;
        if (num_mivcs_found > 0)
        {
            os << "Smallest MIVC: " << smallest_mivc_size << std::endl;
            os << "Largest MIVC: " << largest_mivc_size << std::endl;
        }

        // CAIVC specific
        os << "CAIVC Correction Sets Found: " << caivc_correction_sets_found << std::endl;
        os << "CAIVC Preparation Time: " << caivc_prep_time << std::endl;
        os << "CAIVC isIVC Calls: " << caivc_isivc_calls << std::endl;
        os << "CAIVC isIVC Time: " << caivc_isivc_time << std::endl;
        os << "CAIVC Find Candidate Calls: " << caivc_find_candidate_calls << std::endl;
        os << "CAIVC Find Candidate Time: " << caivc_find_candidate_time << std::endl;
        os << "CAIVC Find Correction Set Calls: " << caivc_find_mcs_calls << std::endl;
        os << "CAIVC Find Correction Set Time: " << caivc_find_mcs_time << std::endl;
        os << "CAIVC More Correction Sets Calls: " << caivc_more_mcs_calls << std::endl;
        os << "CAIVC More Correction Sets Time: " << caivc_more_mcs_time << std::endl;

        // MARCO-IVC specific
        os << "MARCO-IVC getUnexplored Calls: " << marcoivc_get_unexplored_calls << std::endl;
        os << "MARCO-IVC getUnexplored Time: " << marcoivc_get_unexplored_time << std::endl;
        os << "MARCO-IVC isSafe Calls: " << marcoivc_issafe_calls << std::endl;
        os << "MARCO-IVC isSafe Time: " << marcoivc_issafe_time << std::endl;
        os << "MARCO-IVC grow Calls: " << marcoivc_grow_calls << std::endl;
        os << "MARCO-IVC grow Time: " << marcoivc_grow_time << std::endl;
        os << "MARCO-IVC shrink Calls: " << marcoivc_shrink_calls << std::endl;
        os << "MARCO-IVC shrink Time: " << marcoivc_shrink_time << std::endl;
    }

    GlobalState& GlobalState::instance()
    {
        static GlobalState gs;
        return gs;
    }

    void GlobalState::resetOptions()
    {
        m_opts.reset(new PMEOptions());
    }

    PMEOptions& GlobalState::options()
    {
        return *(instance().m_opts);
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
