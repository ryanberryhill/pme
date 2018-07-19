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

#include "pme/util/hybrid_debugger.h"

namespace PME {

    HybridDebugger::HybridDebugger(VariableManager & varman,
                                   const DebugTransitionRelation & tr,
                                   GlobalState & gs)
        : m_bmc(varman, tr, gs),
          m_ic3(varman, tr, gs),
          m_kmax(0)
    {
        setKMax(gs.opts.hybrid_debug_bmc_frames);
    }

    void HybridDebugger::setCardinality(unsigned n)
    {
        m_bmc.setCardinality(n);
        m_ic3.setCardinality(n);
    }

    void HybridDebugger::clearCardinality()
    {
        m_bmc.clearCardinality();
        m_ic3.clearCardinality();
    }

    Debugger::Result HybridDebugger::debugOverGates(const std::vector<ID> & gates)
    {
        Result result;
        result.first = false;

        // Do BMC if it's enabled
        if (m_kmax > 0) { result = m_bmc.debugOverGates(gates); }

        // If BMC gave an unknown result, do IC3
        if (result.first == false)
        {
            result = m_ic3.debugOverGates(gates);
        }

        return result;
    }

    Debugger::Result HybridDebugger::debug()
    {
        Result result;
        result.first = false;

        // Do BMC if it's enabled
        if (m_kmax > 0) { result = m_bmc.debug(); }

        // If BMC gave an unknown result, do IC3
        if (result.first == false)
        {
            result = m_ic3.debug();
        }

        return result;
    }

    void HybridDebugger::blockSolution(const std::vector<ID> & soln)
    {
        m_bmc.blockSolution(soln);
        m_ic3.blockSolution(soln);
    }

    void HybridDebugger::setKMax(unsigned k)
    {
        m_bmc.setKMax(k);
    }

    IC3::LemmaID HybridDebugger::addLemma(const Cube & c, unsigned level)
    {
        return m_ic3.addLemma(c, level);
    }

    std::vector<Cube> HybridDebugger::getFrameCubes(unsigned n) const
    {
        return m_ic3.getFrameCubes(n);
    }

    unsigned HybridDebugger::numFrames() const
    {
        return m_ic3.numFrames();
    }
}

