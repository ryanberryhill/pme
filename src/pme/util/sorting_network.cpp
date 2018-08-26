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

#include "pme/util/sorting_network.h"

#include <cassert>

namespace PME {

    std::vector<ID> freshVars(VariableManager & varman, size_t n)
    {
        std::vector<ID> f;
        f.reserve(n);

        for (size_t i = 0; i < n; ++i)
        {
            f.push_back(varman.getNewID());
        }

        return f;
    }

    std::vector<ID> takeOdd(const std::vector<ID> & vec)
    {
        std::vector<ID> odd;
        odd.reserve((vec.size() + 1) / 2);
        for (size_t i = 1; i < vec.size(); i += 2)
        {
            odd.push_back(vec.at(i));
        }
        return odd;
    }

    std::vector<ID> takeEven(const std::vector<ID> & vec)
    {
        std::vector<ID> even;
        even.reserve(vec.size() / 2);
        for (size_t i = 0; i < vec.size(); i += 2)
        {
            even.push_back(vec.at(i));
        }
        return even;
    }

    // y = OR(x_1, x_2)
    ClauseVec partialComp(ID x_1, ID x_2, ID y, bool le, bool ge)
    {
        assert(le || ge);
        ClauseVec cnf;
        if (le)
        {
            // x_1 => y, x_2 => y
            cnf.push_back({negate(x_1), y});
            cnf.push_back({negate(x_2), y});
        }
        if (ge)
        {
            // ~x_1 & ~x_2 => ~y
            cnf.push_back({x_1, x_2, negate(y)});
        }
        return cnf;
    }

    std::vector<ID> zipperMerge(const std::vector<ID> & even,
                                const std::vector<ID> & odd,
                                size_t a, size_t b)
    {
        // Three cases: 1. same length (both even)
        //              2. even - odd == 1 (one even one odd)
        //              3. even - odd == 2 (both odd)
        // This is what we'll eventually do in each case:
        // Case 1: take even.front, odd.back and compare 2-3, 4-5, etc.
        // Case 2: take even.front, compare 2-3, 4-5, etc.
        // Case 3: take even.front, odd.back, compare 2-3, 4-5, etc.
        assert(even.size() >= odd.size());

        size_t diff = even.size() - odd.size();

        // Sanity check
        assert(diff <= 2);
        if (diff == 0) { assert(a % 2 == 0); assert(b % 2 == 0); }
        if (diff == 1) { assert((a % 2) != (b % 2)); }
        if (diff == 2) { assert(a % 2 == 1); assert(a % 2 == 1); }

        std::vector<ID> merged;
        merged.reserve(odd.size() + even.size());

        if (diff <= 1)
        {
            for (size_t i = 0; i < odd.size(); ++i)
            {
                merged.push_back(even.at(i));
                merged.push_back(odd.at(i));
            }

            if (diff == 1)
            {
                merged.push_back(even.back());
            }
        }
        else
        {
            // This is the tricky case.
            // Here's the trick. indices 0, 2, ..., a-1, a, a+2, ... a+b
            //                                          ^-- HERE
            // need to come from the even side. Other indices from odd.
            assert(diff == 2);
            size_t i_even = 0;
            size_t i_odd = 0;

            while (i_odd < odd.size())
            {
                merged.push_back(even.at(i_even));
                merged.push_back(odd.at(i_odd));

                i_even++;
                i_odd++;

                if (merged.size() == a + 1)
                {
                    merged.push_back(even.at(i_even));
                    i_even++;
                }
            }

            assert(i_even == even.size() - 1);
            merged.push_back(even.back());
        }

        return merged;
    }

    ClauseVec comp(ID x_1, ID x_2, ID y_1, ID y_2, bool le, bool ge)
    {
        assert(le || ge);

        ClauseVec cnf;

        if (le)
        {
            cnf.push_back({negate(x_1), y_1});
            cnf.push_back({negate(x_2), y_1});
            cnf.push_back({negate(x_1), negate(x_2), y_2});
        }

        if (ge)
        {
            cnf.push_back({x_1, negate(y_2)});
            cnf.push_back({x_2, negate(y_2)});
            cnf.push_back({x_1, x_2, negate(y_1)});
        }

        return cnf;
    }

    ClauseVec compHalf(ID x_1, ID x_2, ID y_1, ID y_2)
    {
        return comp(x_1, x_2, y_1, y_2, true, false);
    }

    ClauseVec compFull(ID x_1, ID x_2, ID y_1, ID y_2)
    {
        return comp(x_1, x_2, y_1, y_2, true, true);
    }

    CNFNetwork mergeNetwork(VariableManager & varman,
                            const std::vector<ID> & inputs_a,
                            const std::vector<ID> & inputs_b,
                            bool le, bool ge)
    {
        const std::vector<ID> & a_vec = inputs_a.size() <= inputs_b.size() ? inputs_a : inputs_b;
        const std::vector<ID> & b_vec = inputs_a.size() <= inputs_b.size() ? inputs_b : inputs_a;
        size_t a = a_vec.size();
        size_t b = b_vec.size();
        std::vector<ID> outputs;

        assert(a <= b);
        assert(le || ge);

        ClauseVec cnf;

        if (a == 1 && b == 1)
        {
            // Base case: both size 1, return a 2-comparator
            outputs = freshVars(varman, 2);
            cnf = comp(a_vec.at(0), b_vec.at(0), outputs.at(0), outputs.at(1), le, ge);
        }
        else if (a_vec.empty())
        {
            // Base case: a is empty, return b
            outputs = b_vec;
        }
        else
        {
            // Recursive case: build an odd and even network and merge them.
            std::vector<ID> a_odd = takeOdd(a_vec);
            std::vector<ID> a_even = takeEven(a_vec);
            std::vector<ID> b_odd = takeOdd(b_vec);
            std::vector<ID> b_even = takeEven(b_vec);

            CNFNetwork odd_network = mergeNetwork(varman, a_odd, b_odd, le, ge);
            CNFNetwork even_network = mergeNetwork(varman, a_even, b_even, le, ge);

            const ClauseVec & odd_cnf = odd_network.second;
            const ClauseVec & even_cnf = even_network.second;

            const std::vector<ID> & z_odd = odd_network.first;
            const std::vector<ID> & z_even = even_network.first;

            std::vector<ID> z = zipperMerge(z_even, z_odd, a, b);

            cnf.reserve(cnf.size() + odd_cnf.size() + even_cnf.size());
            cnf.insert(cnf.end(), odd_cnf.begin(), odd_cnf.end());
            cnf.insert(cnf.end(), even_cnf.begin(), even_cnf.end());

            assert(outputs.empty());
            outputs.reserve(a + b);
            outputs.push_back(z.at(0));

            for (size_t i = 1; i < z.size() - 1; i += 2)
            {
                ID z_i1 = z.at(i);
                ID z_i2 = z.at(i + 1);
                ID y_i1 = varman.getNewID();
                ID y_i2 = varman.getNewID();

                ClauseVec comp_cnf = comp(z_i1, z_i2, y_i1, y_i2, le, ge);

                cnf.insert(cnf.end(), comp_cnf.begin(), comp_cnf.end());

                outputs.push_back(y_i1);
                outputs.push_back(y_i2);
            }

            if ((a + b) % 2 == 0)
            {
                assert(outputs.size() == a + b - 1);
                outputs.push_back(z.back());
            }
        }

        return std::make_pair(outputs, cnf);
    }

    CNFNetwork sortingNetwork(VariableManager & varman,
                              const std::vector<ID> & inputs,
                              bool le, bool ge)
    {
        std::vector<ID> outputs;
        ClauseVec cnf;

        size_t n = inputs.size();
        assert(n > 0);

        if (n == 1)
        {
            // Base case: single element is already sorted
            outputs = inputs;
        }
        else if (n == 2)
        {
            // Base case: for two elements, a single merge network
            ID x_1 = inputs.at(0);
            ID x_2 = inputs.at(1);
            std::tie(outputs, cnf) = mergeNetwork(varman, {x_1}, {x_2}, le, ge);
        }
        else
        {
            // Recursive case. Split at n/2 and merge
            std::vector<ID> inputs_left, inputs_right;
            size_t l = n / 2;
            inputs_left.reserve(l);
            inputs_right.reserve(n - l);

            inputs_left.insert(inputs_left.end(), inputs.begin(), inputs.begin() + l);
            inputs_right.insert(inputs_right.end(), inputs.begin() + l, inputs.end());

            assert(inputs_left.size() + inputs_right.size() == inputs.size());

            std::vector<ID> outputs_left, outputs_right;
            ClauseVec cnf_left, cnf_right;

            std::tie(outputs_left, cnf_left) = sortingNetwork(varman, inputs_left, le, ge);
            std::tie(outputs_right, cnf_right) = sortingNetwork(varman, inputs_right, le, ge);


            ClauseVec cnf_merge;
            std::tie(outputs, cnf_merge) =
                mergeNetwork(varman, outputs_left, outputs_right, le, ge);

            cnf.reserve(cnf_left.size() + cnf_right.size() + cnf_merge.size());
            cnf.insert(cnf.end(), cnf_left.begin(), cnf_left.end());
            cnf.insert(cnf.end(), cnf_right.begin(), cnf_right.end());
            cnf.insert(cnf.end(), cnf_merge.begin(), cnf_merge.end());
        }

        return std::make_pair(outputs, cnf);
    }

    CNFNetwork simpMergeNetwork(VariableManager & varman,
                                const std::vector<ID> & inputs_a,
                                const std::vector<ID> & inputs_b,
                                unsigned c,
                                bool le, bool ge)
    {
        std::vector<ID> outputs;
        ClauseVec cnf;

        std::vector<ID> a_vec = inputs_a.size() <= inputs_b.size() ? inputs_a : inputs_b;
        std::vector<ID> b_vec = inputs_a.size() <= inputs_b.size() ? inputs_b : inputs_a;
        size_t a = a_vec.size();
        size_t b = b_vec.size();

        if (a > c)
        {
            a_vec.erase(a_vec.begin() + c, a_vec.end());
            a = a_vec.size();
            assert(a == c);
        }

        if (b > c)
        {
            b_vec.erase(b_vec.begin() + c, b_vec.end());
            b = b_vec.size();
            assert(b == c);
        }

        assert(a <= b);
        assert(b <= c);
        assert(le || ge);

        if (a == 0)
        {
            outputs = b_vec;
        }
        else if (a == 1 && b == 1 && c == 1)
        {
            // Base case: output = 1 if any input is 1
            ID y = varman.getNewID();
            ID x_a = a_vec.at(0);
            ID x_b = b_vec.at(0);

            ClauseVec or_cnf = partialComp(x_a, x_b, y, le, ge);

            cnf.insert(cnf.end(), or_cnf.begin(), or_cnf.end());
            outputs.push_back(y);
        }
        else if (a + b <= c)
        {
            // Simple case, just merge
            std::tie(outputs, cnf) = mergeNetwork(varman, a_vec, b_vec, le, ge);
        }
        else
        {
            // a + b > c, but each of a,b <= c
            outputs.reserve(c);
            bool is_even = (c % 2) == 0;

            std::vector<ID> a_odd = takeOdd(a_vec);
            std::vector<ID> a_even = takeEven(a_vec);
            std::vector<ID> b_odd = takeOdd(b_vec);
            std::vector<ID> b_even = takeEven(b_vec);

            size_t odd_size = is_even ? c / 2 : (c - 1) / 2;
            size_t even_size = is_even ? c / 2 + 1 : (c + 1) / 2;

            CNFNetwork odd_network = simpMergeNetwork(varman, a_odd, b_odd, odd_size, le, ge);
            CNFNetwork even_network = simpMergeNetwork(varman, a_even, b_even, even_size, le, ge);

            const ClauseVec & odd_cnf = odd_network.second;
            const ClauseVec & even_cnf = even_network.second;

            const std::vector<ID> & z_odd = odd_network.first;
            const std::vector<ID> & z_even = even_network.first;

            cnf.reserve(odd_cnf.size() + even_cnf.size());
            cnf.insert(cnf.end(), odd_cnf.begin(), odd_cnf.end());
            cnf.insert(cnf.end(), even_cnf.begin(), even_cnf.end());

            // Take z_0
            outputs.push_back(z_even.at(0));

            if (is_even)
            {
                // c is even
                assert(z_even.size() == c / 2 + 1);
                assert(z_odd.size() == c / 2);

                // Compare even-odd pairs.
                for (size_t i = 0; i < c / 2 - 1; ++i)
                {
                    ID x_1 = z_even.at(i + 1);
                    ID x_2 = z_odd.at(i);
                    ID y_1 = varman.getNewID();
                    ID y_2 = varman.getNewID();

                    ClauseVec comp_cnf = comp(x_1, x_2, y_1, y_2, le, ge);
                    cnf.insert(cnf.end(), comp_cnf.begin(), comp_cnf.end());

                    outputs.push_back(y_1);
                    outputs.push_back(y_2);
                }

                // Last output = OR(z_c, z_c+1) i.e., (OR(odd.back(), even.back())
                ID y_c = varman.getNewID();
                ClauseVec or_cnf = partialComp(z_even.back(), z_odd.back(), y_c, le, ge);
                cnf.insert(cnf.end(), or_cnf.begin(), or_cnf.end());
                outputs.push_back(y_c);
            }
            else
            {
                // c is odd
                assert(z_odd.size() == (c - 1) / 2);
                assert(z_even.size() == (c + 1) / 2);

                // Compare even-odd pairs.
                for (size_t i = 0; i < (c - 1) / 2; ++i)
                {
                    ID x_1 = z_even.at(i + 1);
                    ID x_2 = z_odd.at(i);
                    ID y_1 = varman.getNewID();
                    ID y_2 = varman.getNewID();

                    ClauseVec comp_cnf = comp(x_1, x_2, y_1, y_2, le, ge);
                    cnf.insert(cnf.end(), comp_cnf.begin(), comp_cnf.end());

                    outputs.push_back(y_1);
                    outputs.push_back(y_2);
                }
            }
        }

        return std::make_pair(outputs, cnf);
    }

    CNFNetwork cardinalityNetwork(VariableManager & varman,
                                  const std::vector<ID> & inputs,
                                  unsigned m, bool le, bool ge)
    {
        std::vector<ID> outputs;
        ClauseVec cnf;

        size_t n = inputs.size();
        assert(n > 0);
        assert(m > 0);

        if (n <= m)
        {
            // Simple case: just a sorting network
            std::tie(outputs, cnf) = sortingNetwork(varman, inputs, le, ge);
        }
        else
        {
            // Recursive case. Split on l = n / 2, take cardinality networks
            // for each side of the split and merge with an m-simlified merge
            // network
            assert(n >= 2);
            size_t l = n / 2;

            size_t size_left = std::min(l, (size_t) m);
            size_t size_right = std::min(n - l, (size_t) m);

            std::vector<ID> outputs_left, outputs_right;
            ClauseVec cnf_left, cnf_right;

            std::vector<ID> left(inputs.begin(), inputs.begin() + l);
            std::vector<ID> right(inputs.begin() + l, inputs.end());

            std::tie(outputs_left, cnf_left) = cardinalityNetwork(varman, left, m, le, ge);
            std::tie(outputs_right, cnf_right) = cardinalityNetwork(varman, right, m, le, ge);

            assert(outputs_left.size() == size_left);
            assert(outputs_right.size() == size_right);

            ClauseVec cnf_merge;
            std::tie(outputs, cnf_merge) =
                simpMergeNetwork(varman, outputs_left, outputs_right, m, le, ge);

            cnf.reserve(cnf_left.size() + cnf_right.size() + cnf_merge.size());
            cnf.insert(cnf.end(), cnf_left.begin(), cnf_left.end());
            cnf.insert(cnf.end(), cnf_right.begin(), cnf_right.end());
            cnf.insert(cnf.end(), cnf_merge.begin(), cnf_merge.end());
        }

        return std::make_pair(outputs, cnf);
    }
}

