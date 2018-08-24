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

#include "pme/util/cardinality_constraint.h"

#include <cassert>
#include <algorithm>

namespace PME
{
    class TotalizerTree
    {
        private:
            std::unique_ptr<TotalizerTree> m_left, m_right;
            std::vector<ID> m_outputs;
            std::set<ID> m_dirty;
            ID m_input; // for leaf nodes
        public:
            TotalizerTree() : m_input(ID_NULL) { }

            // Leaf constructor
            TotalizerTree(ID id) : m_input(id)
            {
                m_outputs.push_back(id);
            }

            std::unique_ptr<TotalizerTree> & left() { return m_left; }
            std::unique_ptr<TotalizerTree> & right() { return m_right; }
            size_t outputSize() const { return m_outputs.size(); }
            size_t inputSize() const
            {
                if (m_input != ID_NULL) { assert(!m_left); assert(!m_right); return 1; }
                size_t s = 0;
                if (m_left)  { s += m_left->outputSize(); }
                if (m_right) { s += m_right->outputSize(); }
                return s;
            }
            void markClean() { m_dirty.clear(); }
            void markDirty()
            {
                m_dirty.clear();
                m_dirty.insert(m_outputs.begin(), m_outputs.end());
            }
            bool isDirty() const { return !m_dirty.empty(); }
            bool isDirty(ID id) const { return m_dirty.count(id) > 0; }
            bool isClean() const { return !isDirty(); }

            bool isLeaf() const { return m_input != ID_NULL; }

            void addOutput(ID id) { m_outputs.push_back(id); m_dirty.insert(id); }

            template <typename Iterator>
            void addOutputs(Iterator begin, Iterator end)
            {
                for (auto it = begin; it != end; ++it)
                {
                    addOutput(*it);
                }
            }

            auto begin_outputs() const -> decltype(m_outputs.cbegin())
            {
                return m_outputs.cbegin();
            }

            auto end_outputs() const -> decltype(m_outputs.cbegin())
            {
                return m_outputs.cend();
            }
    };

    TotalizerCardinalityConstraint::TotalizerCardinalityConstraint(VariableManager & varman)
        : m_vars(varman),
          m_cardinality(0)
    { }

    TotalizerCardinalityConstraint::~TotalizerCardinalityConstraint()
    { }

    unsigned TotalizerCardinalityConstraint::getInputCardinality() const
    {
        return m_inputs.size();
    }

    unsigned TotalizerCardinalityConstraint::getOutputCardinality() const
    {
        return m_root ? m_root->outputSize() : 0;
    }

    void TotalizerCardinalityConstraint::increaseNodeCardinality(TotalizerTree * node,
                                                        std::set<TotalizerTree *> & visited)
    {
        assert(node != nullptr);
        if (node->left() && !visited.count(node->left().get()))
        {
            increaseNodeCardinality(node->left().get(), visited);
        }

        if (node->right() && !visited.count(node->right().get()))
        {
            increaseNodeCardinality(node->right().get(), visited);
        }

        visited.insert(node);

        // While the node's output cardinality needs to increase
        unsigned target = std::min((size_t) m_cardinality, node->inputSize());
        while (node->outputSize() < target)
        {
            // increase it
            node->addOutput(freshVar());
        }
    }

    void TotalizerCardinalityConstraint::setCardinality(unsigned n)
    {
        if (n <= m_cardinality) { return; }
        else
        {
            m_cardinality = n;
            if (!m_root) { return; }

            // Walk the tree post-order, increasing cardinality if needed at
            // each node we visit
            std::set<TotalizerTree *> visited;
            increaseNodeCardinality(m_root.get(), visited);
            updateCachedOutputs();
        }
    }

    void TotalizerCardinalityConstraint::increaseCardinality(unsigned n)
    {
        if (n < m_cardinality)
        {
            throw std::invalid_argument("n must be at least the current cardinality");
        }
        else
        {
            setCardinality(n);
        }
    }

    void TotalizerCardinalityConstraint::updateCachedOutputs()
    {
        m_outputs.clear();
        if (m_root)
        {
            m_outputs.insert(m_outputs.end(), m_root->begin_outputs(),
                                              m_root->end_outputs());
        }
    }

    void TotalizerCardinalityConstraint::addInput(ID id)
    {
        m_inputs.push_back(id);
        if (m_root)
        {
            TotalizerTree * new_root = new TotalizerTree();
            new_root->left().swap(m_root);
            assert(!m_root);
            new_root->right().reset(new TotalizerTree(id));

            unsigned target = std::min((size_t) m_cardinality, new_root->inputSize());
            while (new_root->outputSize() < target)
            {
                new_root->addOutput(freshVar());
            }

            m_root.reset(new_root);
        }
        else
        {
            m_root.reset(new TotalizerTree(id));
        }
        updateCachedOutputs();
    }

    ID TotalizerCardinalityConstraint::freshVar()
    {
        return m_vars.getNewID("cardinality_internal");
    }

    ClauseVec TotalizerCardinalityConstraint::CNFize()
    {
        return CNFize(m_root.get());
    }

    void TotalizerCardinalityConstraint::clearIncrementality()
    {
        clearIncrementality(m_root.get());
    }

    void TotalizerCardinalityConstraint::clearIncrementality(TotalizerTree * tree)
    {
        if (tree == nullptr) { return; }
        tree->markDirty();
        if (tree->left()) { clearIncrementality(tree->left().get()); }
        if (tree->right()) { clearIncrementality(tree->right().get()); }
    }

    ClauseVec TotalizerCardinalityConstraint::CNFize(TotalizerTree * tree)
    {
        ClauseVec cnf;
        assert(tree != nullptr);

        if (tree->isClean())
        {
            assert(!tree->left() || tree->left()->isClean());
            assert(!tree->right() || tree->right()->isClean());
            return cnf;
        }

        // Handle the case where this is a leaf
        if (tree->isLeaf()) { return cnf; }

        // Recursively handle the subtrees
        ClauseVec lcnf = CNFize(tree->left().get());
        ClauseVec rcnf = CNFize(tree->right().get());
        cnf.insert(cnf.end(), lcnf.begin(), lcnf.end());
        cnf.insert(cnf.end(), rcnf.begin(), rcnf.end());

        // Notation and algorithm are based on "Efficient CNF Encoding of
        // Boolean Cardinality Constraints"
        // a_vec relates to the left subtree and b_vec to the right subtree,
        // while r_vec relates to this subtree
        std::vector<ID> a_vec, b_vec, r_vec;
        a_vec.push_back(ID_TRUE);
        b_vec.push_back(ID_TRUE);
        r_vec.push_back(ID_TRUE);

        r_vec.insert(r_vec.end(), tree->begin_outputs(), tree->end_outputs());

        if (tree->left())
        {
            a_vec.insert(a_vec.end(), tree->left()->begin_outputs(),
                                      tree->left()->end_outputs());
        }

        if (tree->right())
        {
            b_vec.insert(b_vec.end(), tree->right()->begin_outputs(),
                                      tree->right()->end_outputs());
        }

        a_vec.push_back(ID_FALSE);
        b_vec.push_back(ID_FALSE);
        r_vec.push_back(ID_FALSE);

        // Note: this implementation is almost exactly as the paper describes.
        // It could very easily be optimized somewhat but it's probably not
        // important to do so
        for (size_t alpha = 0; alpha < a_vec.size() - 1; ++alpha)
        {
            ID a0 = a_vec.at(alpha);
            ID a1 = a_vec.at(alpha + 1);
            for (size_t beta = 0; beta < b_vec.size() - 1; ++beta)
            {
                ID b0 = b_vec.at(beta);
                ID b1 = b_vec.at(beta + 1);

                size_t theta = alpha + beta;
                if (theta + 1 >= r_vec.size()) { continue; }

                ID r0 = r_vec.at(theta);
                ID r1 = r_vec.at(theta + 1);

                // c1 encodes the >= constraint
                if (a0 != ID_FALSE && b0 != ID_FALSE && r0 != ID_TRUE)
                {
                    Clause c1 = { negate(a0), negate(b0), r0 };
                    if (isDirtyClause(c1, tree)) { cnf.push_back(c1); }
                }

                // c2 encodes the <=
                if (a1 != ID_TRUE && b1 != ID_TRUE && r1 != ID_FALSE)
                {
                    Clause c2 = { a1, b1, negate(r1) };
                    if (isDirtyClause(c2, tree)) { cnf.push_back(c2); }
                }
            }
        }

        tree->markClean();
        return cnf;
    }

    bool TotalizerCardinalityConstraint::isDirtyClause(const Clause & cls, TotalizerTree * node) const
    {
        for (ID id : cls)
        {
            id = strip(id);
            if (node->isDirty(id)) { return true; }
        }
        return false;
    }

    const std::vector<ID> & TotalizerCardinalityConstraint::outputs() const
    {
        return m_outputs;
    }

    Cube TotalizerCardinalityConstraint::assumeEq(unsigned n) const
    {
        if (n == getInputCardinality() && n == getOutputCardinality())
        {
            return m_inputs;
        }
        else if (n >= getOutputCardinality())
        {
            throw std::invalid_argument("Tried to assumeEq cardinality >= "
                                        "current output cardinality");
        }

        Cube assumps;
        assumps.reserve(getOutputCardinality());
        for (unsigned i = 0; i < outputs().size(); ++i)
        {
            unsigned lit = outputs().at(i);
            if (n > 0 && i <= n - 1) { assumps.push_back(lit); }
            else { assumps.push_back(negate(lit)); }
        }

        return assumps;
    }

    Cube TotalizerCardinalityConstraint::assumeLEq(unsigned n) const
    {
        if (n == getInputCardinality() && n == getOutputCardinality())
        {
            Cube empty;
            return empty;
        }
        else if (n >= getOutputCardinality())
        {
            throw std::invalid_argument("Tried to assumeLEq cardinality >= "
                                        "current output cardinality");
        }

        Cube assumps;
        assumps.reserve(getOutputCardinality());
        for (unsigned i = 0; i < outputs().size(); ++i)
        {
            unsigned lit = outputs().at(i);
            if (i >= n) { assumps.push_back(negate(lit)); }
        }

        return assumps;
    }

    Cube TotalizerCardinalityConstraint::assumeLT(unsigned n) const
    {
        if (n == 0) { throw std::invalid_argument("Tried to assume cardinality < 0"); }
        if (n > getOutputCardinality())
        {
            throw std::invalid_argument("Tried to assumeLT cardinality >= "
                                        "current output cardinality");
        }

        Cube assumps;
        assumps.reserve(getOutputCardinality());
        for (unsigned i = 0; i < outputs().size(); ++i)
        {
            unsigned lit = outputs().at(i);
            if (n > 0 && i >= n - 1) { assumps.push_back(negate(lit)); }
        }

        return assumps;
    }

    Cube TotalizerCardinalityConstraint::assumeGEq(unsigned n) const
    {
        if (n == getInputCardinality() && n == getOutputCardinality())
        {
            return m_inputs;
        }
        else if (n >= getOutputCardinality())
        {
            throw std::invalid_argument("Tried to assumeGEq cardinality >= "
                                        "current output cardinality");
        }

        Cube assumps;
        assumps.reserve(getOutputCardinality());
        for (unsigned i = 0; i < outputs().size(); ++i)
        {
            unsigned lit = outputs().at(i);
            if (n > 0 && i <= n - 1) { assumps.push_back(lit); }
        }

        return assumps;
    }

    Cube TotalizerCardinalityConstraint::assumeGT(unsigned n) const
    {
        if (n >= getOutputCardinality())
        {
            throw std::invalid_argument("Tried to assumeGT cardinality >= "
                                        "current output cardinality");
        }

        Cube assumps;
        assumps.reserve(getOutputCardinality());
        for (unsigned i = 0; i < outputs().size(); ++i)
        {
            unsigned lit = outputs().at(i);
            if (i <= n) { assumps.push_back(lit); }
        }

        return assumps;
    }

}

