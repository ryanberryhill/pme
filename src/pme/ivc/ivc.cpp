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

#include "pme/ivc/ivc.h"

#include <cassert>

namespace PME {

    IVCFinder::IVCFinder(VariableManager & varman,
                         const TransitionRelation & tr)
        : m_vars(varman),
          m_tr(tr)
    { }

    std::ostream & IVCFinder::log(int verbosity) const
    {
        return log(LOG_IVC, verbosity);
    }

    std::ostream & IVCFinder::log(LogChannelID channel, int verbosity) const
    {
        return GlobalState::logger().log(channel, verbosity);
    }

    void IVCFinder::findIVCs()
    {
        m_timer.start();
        doFindIVCs();
    }

    size_t IVCFinder::numMIVCs() const
    {
        return m_mivcs.size();
    }

    const IVC & IVCFinder::getMIVC(size_t i) const
    {
        assert(i < numMIVCs());
        return m_mivcs.at(i);
    }

    bool IVCFinder::minimumIVCKnown() const
    {
        return !m_minimum_ivc.empty();
    }

    const IVC & IVCFinder::getMinimumIVC() const
    {
        return m_minimum_ivc;
    }

    void IVCFinder::addMIVC(const IVC & ivc)
    {
        double time = m_timer.elapsed();
        log(1) << "IVC #" << m_mivcs.size() + 1 << " found, time: " << time << std::endl;

        m_mivcs.push_back(ivc);
        stats().num_mivcs_found++;
        stats().largest_mivc_size = std::max(stats().largest_mivc_size, ivc.size());
    }

    void IVCFinder::setMinimumIVC(const IVC & ivc)
    {
        m_minimum_ivc = ivc;
        stats().smallest_mivc_size = ivc.size();
    }
}

