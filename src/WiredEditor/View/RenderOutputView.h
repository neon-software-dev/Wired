/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_VIEW_RENDEROUTPUTVIEW_H
#define WIREDEDITOR_VIEW_RENDEROUTPUTVIEW_H

#include <Wired/Engine/IEngineAccess.h>

#include <NEON/Common/Build.h>
#include <NEON/Common/Space/Rect.h>

#include <imgui.h>

#include <string>

namespace Wired
{
    class RenderOutputView
    {
        public:

            explicit RenderOutputView(Engine::IEngineAccess* pEngine);

            void operator()(Render::TextureId textureId);

            // Note that this does not correct for any camera view, e.g. if the mouse is in the center
            // of the view, it will always return 0,0
            [[nodiscard]] std::optional<Engine::VirtualSpacePoint> GetMouseVirtualSpacePoint();

        private:

            [[nodiscard]] std::optional<ImVec2> GetMousePosRelativeToWindowContent() const;

        private:

            Engine::IEngineAccess* m_pEngine;

            std::optional<std::pair<NCommon::RectReal, NCommon::RectReal>> m_blitRects;

            ImVec2 m_windowPos{};
            ImVec2 m_windowContentRegionMin{};
            ImVec2 m_windowContentRegionMax{};
    };
}

#endif //WIREDEDITOR_VIEW_RENDEROUTPUTVIEW_H
