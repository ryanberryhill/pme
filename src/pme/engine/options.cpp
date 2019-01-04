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

#include "pme/engine/options.h"

#include <sstream>

namespace PME {

    template<typename T>
    PMEOption<T>::PMEOption(PMEOptions& opts, T value, std::string name)
        : m_value(value), m_name(name)
    {
        setDefaultParser();
        opts.registerOption(*this);
    }

    template<typename T>
    OptionParser& PMEOption<T>::parser()
    {
        return m_parser;
    }

    template<>
    void PMEOption<bool>::setDefaultParser()
    {
        m_parser = [this](std::string x) {
            if (x == "true") { this->m_value = true; return true; }
            else if (x == "false") { this->m_value = false; return true; }
            else { return false; }
        };
    }

    template<>
    void PMEOption<unsigned>::setDefaultParser()
    {
        m_parser = [this](std::string x) {
            try
            {
                this->m_value = std::stoul(x);
            }
            catch (...) { return false; }
            return true;
        };
    }

    PMEOptions::PMEOptions() :
        simplify(*this, true, "simplify"),
        hybrid_ic3_bmc_kmax(*this, 16, "hybrid_ic3_bmc_kmax"),
        msu4_reset_solver_period(*this, 8, "msu4_reset_solver_period"),
        msu4_reset_all_period(*this, 64, "msu4_reset_all_period"),
        msu4_use_hint_clauses(*this, true, "msu4_use_hint_clauses"),
        simple_min_use_min_supp(*this, true, "simple_min_use_min_supp"),
        camsis_abstraction_refinement(*this, true, "camsis_ar"),
        caivc_use_bmc(*this, true, "caivc_use_bmc"),
        caivc_abstraction_refinement(*this, true, "caivc_ar"),
        caivc_approx_mcs(*this, true, "caivc_approx_mcs"),
        caivc_check_with_debug(*this, true, "caivc_check_with_debug"),
        caivc_ar_bmc_kmax(*this, 24, "caivc_ar_bmc_kmax"),
        caivc_ar_bmc_nmax(*this, 8, "caivc_ar_bmc_nmax"),
        caivc_ar_upfront_nmax(*this, 1, "caivc_ar_upfront_nmax"),
        cbvc_upfront_nmax(*this, 0, "cbvc_upfront_nmax"),
        cbvc_upfront_approx_mcs(*this, true, "cbvc_upfront_approx_mcs"),
        cbvc_lift(*this, true, "cbvc_lift"),
        cbvc_reenq(*this, true, "cbvc_reenq"),
        marcoivc_use_ivcucbf(*this, true, "marcoivc_use_ivcucbf"),
        marcoivc_incr_issafe(*this, true, "marcoivc_incr_issafe"),
        ivc_ucbf_use_core(*this, true, "ivc_ucbf_use_core"),
        ivc_ucbf_use_mus(*this, false, "ivc_ucbf_use_mus"),
        ivc_ucbf_use_simple_min(*this, false, "ivc_ucbf_use_simple_min"),
        ivc_ucbf_use_sisi(*this, true, "ivc_ucbf_use_sisi")
    { }

    template<typename T>
    void PMEOptions::registerOption(PMEOption<T>& opt)
    {
        m_option_parsers[opt.name()] = opt.parser();
    }

    void PMEOptions::parseOption(const std::string& option)
    {
        if (option.find('=') == std::string::npos)
        {
            throw std::runtime_error("Option \"" + option + "\" unparseable (lacks =)");
        }

        std::string lhs, rhs;
        std::tie(lhs, rhs) = splitOption(option);

        if (m_option_parsers.count(lhs) == 0)
        {
            throw std::runtime_error("Option \"" + lhs + "\" is unknown");
        }

        OptionParser parser = m_option_parsers.at(lhs);
        bool success = parser(rhs);

        if (!success)
        {
            std::ostringstream oss;
            oss << "Option \"" << lhs << "\" has an invalid value: \"" << rhs << "\"";
            throw std::runtime_error(oss.str());
        }
    }

    std::pair<std::string, std::string>
    PMEOptions::splitOption(const std::string& option)
    {
        std::istringstream ss(option);
        std::string lhs, rhs;
        std::getline(ss, lhs, '=');
        std::getline(ss, rhs, '=');
        return std::make_pair(lhs, rhs);
    }
}

