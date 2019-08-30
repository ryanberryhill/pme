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
        mcs_fallbacks(0),
        uivc_get_unexplored_min_calls(0),
        uivc_get_unexplored_max_calls(0),
        uivc_get_unexplored_arb_calls(0),
        uivc_issafe_calls(0),
        uivc_shrink_calls(0),
        uivc_grow_calls(0),
        uivc_cs_found(0),
        uivc_map_checks(0),
        uivc_k_max(0),
        uivc_safe_cache_hits(0),
        uivc_safe_cache_misses(0),
        uivc_unsafe_cache_hits(0),
        uivc_unsafe_cache_misses(0),
        uivc_check_seed_time(0.0),
        uivc_phase2_time(0.0),
        uivc_safe_cache_time(0.0),
        uivc_unsafe_cache_time(0.0),
        uivc_prep_time(0.0),
        uivc_get_unexplored_min_time(0.0),
        uivc_get_unexplored_max_time(0.0),
        uivc_get_unexplored_arb_time(0.0),
        uivc_issafe_time(0.0),
        uivc_shrink_time(0.0),
        uivc_grow_time(0.0),
        uivc_shrink_cached_time(0.0),
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
        os << "MCS Fallbacks: " << mcs_fallbacks << std::endl;

        // UIVC specific
        os << "UIVC checkSeed Time: " << uivc_check_seed_time << std::endl;
        os << "UIVC Phase 2 Time: " << uivc_phase2_time << std::endl;
        os << "UIVC getUnexploredMin Calls: " << uivc_get_unexplored_min_calls << std::endl;
        os << "UIVC getUnexploredMin Time: " << uivc_get_unexplored_min_time << std::endl;
        os << "UIVC getUnexploredMax Calls: " << uivc_get_unexplored_max_calls << std::endl;
        os << "UIVC getUnexploredMax Time: " << uivc_get_unexplored_max_time << std::endl;
        os << "UIVC getUnexploredArb Calls: " << uivc_get_unexplored_arb_calls << std::endl;
        os << "UIVC getUnexploredArb Time: " << uivc_get_unexplored_arb_time << std::endl;
        os << "UIVC isSafe Calls: " << uivc_issafe_calls << std::endl;
        os << "UIVC isSafe Time: " << uivc_issafe_time << std::endl;
        os << "UIVC shrink Calls: " << uivc_shrink_calls << std::endl;
        os << "UIVC shrink Time: " << uivc_shrink_time << std::endl;
        os << "UIVC grow Calls: " << uivc_grow_calls << std::endl;
        os << "UIVC grow Time: " << uivc_grow_time << std::endl;
        os << "UIVC Shrink Cached Proof Time: " << uivc_shrink_cached_time << std::endl;
        os << "UIVC Preparation Time: " << uivc_prep_time << std::endl;
        os << "UIVC Correction Sets Found: " << uivc_cs_found << std::endl;
        os << "UIVC Successful Map Checks: " << uivc_map_checks << std::endl;
        os << "UIVC Adapted BMC k_max: " << uivc_k_max << std::endl;

        double safe_cache_ratio = 100.0;
        double unsafe_cache_ratio = 100.0;
        unsigned safe_cache_total = uivc_safe_cache_hits + uivc_safe_cache_misses;
        unsigned unsafe_cache_total = uivc_unsafe_cache_hits + uivc_unsafe_cache_misses;

        if (safe_cache_total > 0)
        {
            safe_cache_ratio = double(uivc_safe_cache_hits) / double(safe_cache_total);
        }

        if (unsafe_cache_total > 0)
        {
            unsafe_cache_ratio = double(uivc_unsafe_cache_hits) / double(unsafe_cache_total);
        }

        os << "UIVC Safe Cache Hits: " << uivc_safe_cache_hits << std::endl;
        os << "UIVC Safe Cache Misses: " << uivc_safe_cache_misses << std::endl;
        os << "UIVC Safe Cache Time: " << uivc_safe_cache_time << std::endl;
        os << "UIVC Safe Cache Ratio: " << safe_cache_ratio << std::endl;
        os << "UIVC Unsafe Cache Hits: " << uivc_unsafe_cache_hits << std::endl;
        os << "UIVC Unsafe Cache Misses: " << uivc_unsafe_cache_misses << std::endl;
        os << "UIVC Unsafe Cache Time: " << uivc_unsafe_cache_time << std::endl;
        os << "UIVC Unsafe Cache Ratio: " << unsafe_cache_ratio << std::endl;

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
