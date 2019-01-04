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

#include "pme/ic3/inductive_trace.h"

#include <algorithm>
#include <cassert>

namespace PME { namespace IC3 {

    Cube sortCube(const Cube & cube)
    {
        Cube copy = cube;
        std::sort(copy.begin(), copy.end());
        return copy;
    }

    LemmaID InductiveTrace::addLemma(const Cube & cube, unsigned level)
    {
        Cube cube_sorted = sortCube(cube);

        if (lemmaExists(cube_sorted))
        {
            LemmaData & lemma = getMutableLemma(cube_sorted);

            if (lemma.deleted)
            {
                lemma.deleted = false;
            }
            else
            {
                assert(lemma.level <= level);
                m_frames.removeLemmaFromFrame(lemma.id, lemma.level);
            }

            m_frames.addLemmaToFrame(lemma.id, level);
            lemma.level = level;

            return lemma.id;
        }
        else
        {
            LemmaID id = m_lemmas.size();
            m_lemmas.push_back(LemmaData(id, cube_sorted, level));

            const LemmaData & lemma = getLemma(id);
            m_frames.addLemmaToFrame(lemma.id, level);
            m_cube_to_lemma_id[cube_sorted] = lemma.id;

            return lemma.id;
        }
    }

    unsigned InductiveTrace::levelOf(LemmaID id) const
    {
        const LemmaData & lemma = getLemma(id);
        return lemma.level;
    }

    unsigned InductiveTrace::levelOf(const Cube & cube) const
    {
        assert(lemmaExists(cube));
        return levelOf(IDOf(cube));
    }

    bool InductiveTrace::lemmaExists(const Cube & cube) const
    {
        Cube cube_sorted = sortCube(cube);

        return m_cube_to_lemma_id.count(cube) > 0;
    }

    bool InductiveTrace::lemmaIsActive(const Cube & cube) const
    {
        Cube cube_sorted = sortCube(cube);

        if (!lemmaExists(cube)) { return false; }

        const LemmaData & lemma = getLemma(cube);

        return !lemma.deleted;
    }

    LemmaID InductiveTrace::IDOf(const Cube & cube) const
    {
        Cube cube_sorted = sortCube(cube);
        LemmaID id = m_cube_to_lemma_id.at(cube_sorted);
        return id;
    }

    const LemmaData & InductiveTrace::getLemma(const Cube & cube) const
    {
        return getLemma(IDOf(cube));
    }

    const LemmaData & InductiveTrace::getLemma(LemmaID id) const
    {
        return m_lemmas.at(id);
    }

    LemmaData & InductiveTrace::getMutableLemma(const Cube & cube)
    {
        return getMutableLemma(IDOf(cube));
    }

    LemmaData & InductiveTrace::getMutableLemma(LemmaID id)
    {
        return m_lemmas.at(id);
    }

    Frame InductiveTrace::getFullFrame(unsigned level) const
    {
        Frame full = getFrame(LEVEL_INF);
        for (size_t i = level; i < numFrames(); ++i)
        {
            const Frame & fi = getFrame(i);
            full.insert(fi.begin(), fi.end());
        }

        return full;
    }

    const Frame & InductiveTrace::getFrame(unsigned level) const
    {
        return m_frames.getFrame(level);
    }

    size_t InductiveTrace::numFrames() const
    {
        return m_frames.numFrames();
    }

    void InductiveTrace::removeLemma(LemmaID id)
    {
        LemmaData & lemma = getMutableLemma(id);
        lemma.deleted = true;
        m_frames.removeLemmaFromFrame(lemma.id, lemma.level);
    }

    const Cube & InductiveTrace::cubeOf(LemmaID id) const
    {
        return getLemma(id).cube;
    }

    void InductiveTrace::pushLemma(LemmaID id, unsigned level)
    {
        LemmaData & lemma = getMutableLemma(id);
        assert(lemma.level <= level);
        m_frames.removeLemmaFromFrame(lemma.id, lemma.level);
        m_frames.addLemmaToFrame(lemma.id, level);
        lemma.level = level;
    }

    void InductiveTrace::clearUnusedFrames()
    {
        unsigned k = numFrames();
        for (unsigned i = numFrames() - 1; i > 0; --i)
        {
            const Frame & frame = getFrame(i);
            if (frame.empty())
            {
                k = i;
            }
            else
            {
                break;
            }
        }
        m_frames.shrink(k);
    }

    void InductiveTrace::clear()
    {
        m_frames.clear();
        m_lemmas.clear();
        m_cube_to_lemma_id.clear();
    }


    void Frames::addLemmaToFrame(const LemmaID id, unsigned level)
    {
        Frame & fi = getMutableFrame(level);
        fi.insert(id);
    }

    void Frames::removeLemmaFromFrame(const LemmaID id, unsigned level)
    {
        size_t erased = getMutableFrame(level).erase(id);
        ((void)(erased));
        assert(erased == 1);
    }

    bool Frames::frameExists(unsigned level) const
    {
        return level == LEVEL_INF || level < m_frames.size();
    }

    Frame & Frames::getMutableFrame(unsigned level)
    {
        if (level == LEVEL_INF) { return m_frame_inf; }
        else if (!frameExists(level))
        {
            createFrame(level);
        }

        return m_frames.at(level);
    }

    const Frame & Frames::getFrame(unsigned level) const
    {
        if (level == LEVEL_INF) { return m_frame_inf; }
        assert(frameExists(level));
        return m_frames.at(level);
    }

    size_t Frames::numFrames() const
    {
        return m_frames.size();
    }

    void Frames::createFrame(unsigned level)
    {
        assert(!frameExists(level));
        assert(level != LEVEL_INF);
        assert(m_frames.size() <= level);
        m_frames.resize(level + 1);
    }

    void Frames::shrink(unsigned frames)
    {
        assert(frames <= m_frames.size());
        m_frames.resize(frames);
    }

    void Frames::clear()
    {
        m_frames.clear();
        m_frame_inf.clear();
    }
} }

