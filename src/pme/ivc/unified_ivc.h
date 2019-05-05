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

#ifndef UNIFIED_IVC_FINDER_H_INCLUDED
#define UNIFIED_IVC_FINDER_H_INCLUDED

#include "pme/ivc/ivc.h"
#include "pme/ivc/correction_set_finder.h"
#include "pme/util/map_solver.h"

#include <list>

namespace PME {
    class UnifiedIVCFinder : public IVCFinder {
        public:
            UnifiedIVCFinder(VariableManager & varman,
                             const TransitionRelation & tr);
            virtual void doFindIVCs() override;
        protected:
            virtual std::ostream & log(int verbosity) const override;

        private:
            MapSolver * createMap();
            CorrectionSetFinder * createFinder();

            void findMCSesUpfront();

            bool getUnexplored();
            bool getUnexploredMin();
            bool getUnexploredMax();
            bool getUnexploredArb();

            void handleMinimalSeed(Seed & seed);
            void handleMaximalSeed(Seed & seed);
            void handleSeed(Seed & seed, bool is_minimal = false, bool is_maximal = false);

            void refineSafe(Seed & seed, const SafetyProof & proof, bool do_shrink);
            void refineUnsafe(Seed & seed, bool do_grow);

            void shrink(Seed & seed, const SafetyProof & proof);
            void grow(Seed & seed);
            void growByBruteForce(Seed & seed);
            void growByMCS(Seed & seed);

            bool isSafe(const Seed & seed, SafetyProof * proof = nullptr);
            SafetyResult checkSafetyCache(const TransitionRelation & partial);
            SafetyResult checkUnsafetyCache(const TransitionRelation & partial);
            SafetyResult isSafeBMC(const TransitionRelation & partial);
            SafetyResult isSafeIC3(const TransitionRelation & partial);
            void cacheCounterExample(const SafetyCounterExample & cex);
            void cacheProof(const SafetyProof & proof, const Seed & seed);

            bool shouldCheckSafety() const;

            void addCOIToMap();

            void recordMIVC(const Seed & mivc);

            Seed negateSeed(const Seed & seed) const;

            DebugTransitionRelation m_debug_tr;
            std::unique_ptr<MapSolver> m_map;
            // Quick hack for checking the map while COI hints are
            // present; they are present in m_map but not m_check_map
            SATArbitraryMapSolver m_check_map;
            std::unique_ptr<CorrectionSetFinder> m_cs_finder;
            Seed m_smallest_ivc;
            unsigned m_mivc_lb;
            unsigned m_seed_count;
            std::list<SafetyProof> m_proofs;
            std::list<SafetyCounterExample> m_cexes;
    };
}

#endif

