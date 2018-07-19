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

#ifndef HYBRID_DEBUGGER_H_INCLUDED
#define HYBRID_DEBUGGER_H_INCLUDED

#include "pme/util/debugger.h"
#include "pme/util/bmc_debugger.h"
#include "pme/util/ic3_debugger.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/debug_transition_relation.h"
#include "pme/engine/global_state.h"

namespace PME {

    class HybridDebugger : public Debugger
    {
        public:
            HybridDebugger(VariableManager & varman,
                           const DebugTransitionRelation & tr,
                           GlobalState & gs);

            // Debugging functions
            virtual void setCardinality(unsigned n) override;
            virtual void clearCardinality() override;

            virtual Result debug() override;
            virtual void blockSolution(const std::vector<ID> & soln) override;

            // BMC functions
            void setKMax(unsigned k);

            // IC3 functions
            IC3::LemmaID addLemma(const Cube & c, unsigned level);
            std::vector<Cube> getFrameCubes(unsigned n) const;
            unsigned numFrames() const;

        private:
            BMCDebugger m_bmc;
            IC3Debugger m_ic3;
    };

}

#endif

