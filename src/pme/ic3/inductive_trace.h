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

#ifndef INDUCTIVE_TRACE_H_INCLUDED
#define INDUCTIVE_TRACE_H_INCLUDED

#include "src/pme/pme.h"
#include "src/pme/ic3/ic3.h"

#include <vector>
#include <unordered_map>

namespace PME { namespace IC3 {

    class Frames
    {
        public:
            void addLemmaToFrame(LemmaID id, unsigned level);
            void removeLemmaFromFrame(LemmaID id, unsigned level);
            const Frame & getFrame(unsigned level) const;
            size_t numFrames() const;
            void shrink(unsigned frames);
            void clear();
        private:
            bool frameExists(unsigned level) const;
            Frame & getMutableFrame(unsigned level);
            void createFrame(unsigned level);
            std::vector<Frame> m_frames;
            Frame m_frame_inf;
    };

    struct LemmaData
    {
        LemmaData(LemmaID id, const Cube & cube, unsigned level) :
            id(id), cube(cube), level(level), deleted(false)
        { }

        LemmaID id;
        Cube cube;
        unsigned level;
        bool deleted;
    };

    class InductiveTrace
    {
        public:
            typedef std::vector<LemmaData>::const_iterator lemma_iterator;
            const LemmaData & getLemma(const Cube & cube) const;
            const LemmaData & getLemma(LemmaID id) const;
            Frame getFullFrame(unsigned level) const;
            const Frame & getFrame(unsigned level) const;
            size_t numFrames() const;

            const Cube & cubeOf(LemmaID id) const;
            LemmaID IDOf(const Cube & cube) const;

            unsigned levelOf(LemmaID id) const;
            unsigned levelOf(const Cube & cube) const;
            bool lemmaExists(const Cube & cube) const;
            bool lemmaIsActive(const Cube & cube) const;

            LemmaID addLemma(const Cube & cube, unsigned level);
            void removeLemma(LemmaID id);
            void pushLemma(LemmaID id, unsigned level);

            void clearUnusedFrames();

            lemma_iterator begin_lemmas() const { return m_lemmas.cbegin(); }
            lemma_iterator end_lemmas() const { return m_lemmas.cend(); }

            void clear();

        private:
            LemmaData & getMutableLemma(const Cube & cube);
            LemmaData & getMutableLemma(LemmaID id);

            Frames m_frames;
            std::vector<LemmaData> m_lemmas;
            std::unordered_map<Cube, LemmaID> m_cube_to_lemma_id;

    };
} }

#endif

