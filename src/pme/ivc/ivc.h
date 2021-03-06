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

#ifndef IVC_H_INCLUDED
#define IVC_H_INCLUDED

#include "pme/engine/global_state.h"
#include "pme/engine/variable_manager.h"
#include "pme/engine/transition_relation.h"
#include "pme/engine/debug_transition_relation.h"
#include "pme/util/map_solver.h"
#include "pme/util/timer.h"

namespace PME {

    // A vector of IDs corresponding to the LHS of AndGates
    typedef std::vector<ID> IVC;
    typedef std::vector<ID> BVC;

    class IVCFinder
    {
        public:
            IVCFinder(VariableManager & varman,
                      const TransitionRelation & tr);

            virtual ~IVCFinder() { }

            void findIVCs();
            virtual void doFindIVCs() = 0;

            size_t numMIVCs() const;
            const IVC & getMIVC(size_t i) const;
            bool minimumIVCKnown() const;
            const IVC & getMinimumIVC() const;

            size_t numBVCBounds() const { return m_bvcs.size(); }
            size_t numBVCsAtBound(size_t i) const { return m_bvcs.at(i).size(); }
            const BVC & getBVC(size_t bound, size_t i) const { return m_bvcs.at(bound).at(i); }

        protected:
            void addMIVC(const IVC & ivc);
            void setMinimumIVC(const IVC & ivc);

            void addBVC(unsigned bound, const BVC & bvc);

            virtual std::ostream & log(int verbosity) const;
            std::ostream & log(LogChannelID channel, int verbosity) const;

            const PMEOptions & opts() const { return GlobalState::options(); }
            PMEStats & stats() const { return GlobalState::stats(); }
            const TransitionRelation & tr() const { return m_tr; }
            VariableManager & vars() { return m_vars; }

        private:
            Timer m_timer;
            VariableManager & m_vars;
            const TransitionRelation & m_tr;

            std::vector<IVC> m_mivcs;
            std::vector<std::vector<BVC>> m_bvcs;
            IVC m_minimum_ivc;
    };
}

#endif

