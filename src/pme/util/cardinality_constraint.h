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
    class CardinalityConstraint
    {
        public:
            virtual ~CardinalityConstraint() { }
            virtual void addInput(ID id) = 0;

            template<typename Iterator>
            void addInputs(Iterator begin, Iterator end)
            {
                for (Iterator it = begin; it != end; ++it) { addInput(*it); }
            }

            virtual unsigned getCardinality() const = 0;
            virtual void setCardinality(unsigned n) = 0;

            virtual unsigned getInputCardinality() const = 0;
            virtual unsigned getOutputCardinality() const = 0;

            virtual ClauseVec CNFize() = 0;

            virtual const std::vector<ID> & outputs() const = 0;
            virtual const std::vector<ID> & inputs() const = 0;

            virtual Cube assumeEq(unsigned n) const;
            virtual Cube assumeLEq(unsigned n) const;
            virtual Cube assumeLT(unsigned n) const;
            virtual Cube assumeGEq(unsigned n) const;
            virtual Cube assumeGT(unsigned n) const;
    };

    class TotalizerTree;

    class TotalizerCardinalityConstraint : public CardinalityConstraint
    {
        public:
            TotalizerCardinalityConstraint(VariableManager & varman);
            ~TotalizerCardinalityConstraint();

            virtual void addInput(ID id) override;

            virtual unsigned getCardinality() const override { return m_cardinality; }
            virtual void setCardinality(unsigned n) override;
            void increaseCardinality(unsigned n);

            virtual unsigned getInputCardinality() const override;
            virtual unsigned getOutputCardinality() const override;

            virtual ClauseVec CNFize() override;
            ClauseVec incrementalCNFize();

            virtual const std::vector<ID> & outputs() const override
            { return m_outputs; }
            virtual const std::vector<ID> & inputs() const override
            { return m_inputs; }

        private:
            VariableManager & m_vars;
            std::unique_ptr<TotalizerTree> m_root;
            unsigned m_cardinality;
            std::vector<ID> m_outputs;
            std::vector<ID> m_inputs;

            ID freshVar();
            void increaseNodeCardinality(TotalizerTree * node,
                                         std::set<TotalizerTree *> & visited);
            ClauseVec toCNF(TotalizerTree * tree);
            void clearIncrementality();
            void clearIncrementality(TotalizerTree * tree);
            void updateCachedOutputs();
            bool isDirtyClause(const Clause & cls, TotalizerTree * node) const;
    };

    class SortingConstraint : public CardinalityConstraint
    {
        public:
            SortingConstraint(VariableManager & varman, bool le, bool ge);
            virtual ~SortingConstraint();

            virtual void addInput(ID id) override;

            virtual unsigned getCardinality() const override { return m_cardinality; }
            virtual void setCardinality(unsigned n) override;

            virtual unsigned getInputCardinality() const override;
            virtual unsigned getOutputCardinality() const override;

            virtual ClauseVec CNFize() override;

            virtual const std::vector<ID> & outputs() const override;
            virtual const std::vector<ID> & inputs() const override
            { return m_inputs; }

            virtual Cube assumeEq(unsigned n) const override;
            virtual Cube assumeLEq(unsigned n) const override;
            virtual Cube assumeLT(unsigned n) const override;
            virtual Cube assumeGEq(unsigned n) const override;
            virtual Cube assumeGT(unsigned n) const override;

        private:
            ID freshVar();

            VariableManager & m_vars;
            unsigned m_cardinality;
            std::vector<ID> m_outputs;
            std::vector<ID> m_inputs;
            bool m_cnfized, m_le, m_ge;
    };

    class SortingGEqConstraint : public SortingConstraint
    {
        public:
            SortingGEqConstraint(VariableManager & varman)
                : SortingConstraint(varman, false, true)
            { }
    };

    class SortingLEqConstraint : public SortingConstraint
    {
        public:
            SortingLEqConstraint(VariableManager & varman)
                : SortingConstraint(varman, true, false)
            { }
    };

    class SortingCardinalityConstraint : public SortingConstraint
    {
        public:
            SortingCardinalityConstraint(VariableManager & varman)
                : SortingConstraint(varman, true, true)
            { }
    };
}

#endif

