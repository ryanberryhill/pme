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

#ifndef CARDINALITY_CONSTRAINT_H_INCLUDED
#define CARDINALITY_CONSTRAINT_H_INCLUDED

#include "pme/id.h"
#include "pme/engine/variable_manager.h"

#include <memory>
#include <set>

namespace PME
{
    class TotalizerTree;

    class TotalizerCardinalityConstraint
    {
        public:
            TotalizerCardinalityConstraint(VariableManager & varman);
            ~TotalizerCardinalityConstraint();
            void addInput(ID id);

            unsigned getCardinality() const { return m_cardinality; }
            void increaseCardinality(unsigned n);
            void setCardinality(unsigned n);

            unsigned getInputCardinality() const;
            unsigned getOutputCardinality() const;
            void clearIncrementality();
            ClauseVec CNFize();

            const std::vector<ID> & outputs() const;
            Cube assumeEq(unsigned n) const;
            Cube assumeLEq(unsigned n) const;
            Cube assumeLT(unsigned n) const;
            Cube assumeGEq(unsigned n) const;
            Cube assumeGT(unsigned n) const;

        private:
            VariableManager & m_vars;
            std::unique_ptr<TotalizerTree> m_root;
            unsigned m_cardinality;
            std::vector<ID> m_outputs;
            std::vector<ID> m_inputs;

            ID freshVar();
            void increaseNodeCardinality(TotalizerTree * node,
                                         std::set<TotalizerTree *> & visited);
            ClauseVec CNFize(TotalizerTree * tree);
            void clearIncrementality(TotalizerTree * tree);
            void updateCachedOutputs();
            bool isDirtyClause(const Clause & cls, TotalizerTree * node) const;
    };
}

#endif

