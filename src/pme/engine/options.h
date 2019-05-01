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

#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include <string>
#include <unordered_map>
#include <functional>

namespace PME {

    class PMEOptions;

    typedef std::function<bool(std::string)> OptionParser;

    template<typename T>
    class PMEOption
    {
        public:
            PMEOption(PMEOptions& opts, T value, std::string name);

            operator T() const { return m_value; }
            PMEOption& operator=(const T& v) { m_value = v; return *this; }

            OptionParser& parser();
            const std::string& name() { return m_name; }

        private:
            void setDefaultParser();
            T m_value;
            std::string m_name;
            OptionParser m_parser;
    };

    enum MCSFinderType { MCS_FINDER_BASIC, MCS_FINDER_BMC, MCS_FINDER_LAST = MCS_FINDER_BMC };
    enum MapSolverType { MAP_SOLVER_SAT, MAP_SOLVER_MSU4, MAP_SOLVER_LAST = MAP_SOLVER_MSU4 };

    class PMEOptions
    {
        private:
            std::pair<std::string, std::string> splitOption(const std::string& option);

            // Note: this needs to be initialized before the option objects
            // below, so the order is important here
            std::unordered_map<std::string, OptionParser> m_option_parsers;
        public:
            // --- Global ---
            PMEOption<bool> simplify;
            PMEOption<unsigned> hybrid_ic3_bmc_kmax;


            // --- MaxSAT --
            PMEOption<unsigned> msu4_reset_solver_period;
            PMEOption<unsigned> msu4_reset_all_period;
            PMEOption<bool> msu4_use_hint_clauses;

            // --- Proof Minimization ---

            // Simple Minimizer
            PMEOption<bool> simple_min_use_min_supp;

            // CAMSIS
            PMEOption<bool> camsis_abstraction_refinement;

            // MARCO-MSIS
            PMEOption<bool> marco_direction_down;
            PMEOption<bool> marco_direction_up;
            PMEOption<bool> marco_mcs;
            PMEOption<bool> marco_collapse;

            // --- IVC Extraction ---

            // MCS Finding
            PMEOption<unsigned> mcs_bmc_kmax;
            PMEOption<unsigned> mcs_bmc_kmin;
            PMEOption<unsigned> mcs_bmc_nmax;
            PMEOption<bool> mcs_try_to_exhaust;

            // Unified IVC
            PMEOption<MCSFinderType> uivc_mcs_finder_type;
            PMEOption<MapSolverType> uivc_map_solver_type;
            PMEOption<unsigned> uivc_upfront_nmax;
            PMEOption<bool> uivc_direction_down;
            PMEOption<bool> uivc_direction_up;
            PMEOption<bool> uivc_mcs_grow;
            PMEOption<bool> uivc_coi_hints;
            PMEOption<bool> uivc_check_map;

            // CAIVC
            PMEOption<bool> caivc_use_bmc;
            PMEOption<bool> caivc_abstraction_refinement;
            PMEOption<bool> caivc_approx_mcs;
            PMEOption<bool> caivc_grow_mcs;
            PMEOption<bool> caivc_simple_mcs;
            PMEOption<bool> caivc_check_with_debug;
            PMEOption<unsigned> caivc_ar_bmc_kmax;
            PMEOption<unsigned> caivc_ar_bmc_nmax;
            PMEOption<unsigned> caivc_ar_upfront_nmax;

            // CBVC
            PMEOption<unsigned> cbvc_upfront_nmax;
            PMEOption<bool> cbvc_upfront_approx_mcs;
            PMEOption<bool> cbvc_lift;
            PMEOption<bool> cbvc_reenq;

            // MARCO-IVC
            PMEOption<bool> marcoivc_use_ivcucbf;
            PMEOption<bool> marcoivc_incr_issafe;
            PMEOption<bool> marcoivc_hybrid_issafe;
            PMEOption<bool> marcoivc_explore_basic_hints;
            PMEOption<bool> marcoivc_explore_complex_hints;
            PMEOption<bool> marcoivc_debug_grow;

            // IVC_UCBF
            PMEOption<bool> ivc_ucbf_use_core;
            PMEOption<bool> ivc_ucbf_use_mus;
            PMEOption<bool> ivc_ucbf_use_simple_min;
            PMEOption<bool> ivc_ucbf_use_sisi;

            PMEOptions();

            template<typename T>
            void registerOption(PMEOption<T>& opt);
            void parseOption(const std::string& option);
    };
}

#endif

