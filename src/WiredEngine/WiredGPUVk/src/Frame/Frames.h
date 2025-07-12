/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_FRAME_FRAMES_H
#define WIREDENGINE_WIREDGPUVK_SRC_FRAME_FRAMES_H

#include "Frame.h"

#include <vector>

namespace Wired::GPU
{
    struct Global;

    class Frames
    {
        public:

            explicit Frames(Global* pGlobal);
            ~Frames();

            [[nodiscard]] bool Create();
            void Destroy();

            void StartFrame();
            void EndFrame();

            [[nodiscard]] const Frame& GetCurrentFrame() const noexcept { return m_frames.at(m_currentFrameIndex); }
            [[nodiscard]] Frame& GetCurrentFrame() noexcept { return m_frames.at(m_currentFrameIndex); }
            [[nodiscard]] Frame& GetNextFrame() noexcept { return m_frames.at((m_currentFrameIndex + 1U) % (uint32_t)m_frames.size()); }

            void OnRenderSettingsChanged();

        private:

            [[nodiscard]] bool RecreateFrames();

        private:

            Global* m_pGlobal;

            std::vector<Frame> m_frames;
            uint32_t m_currentFrameIndex{0};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_FRAME_FRAMES_H
