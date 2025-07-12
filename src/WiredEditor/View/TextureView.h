/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_VIEW_TEXTUREVIEW_H
#define WIREDEDITOR_VIEW_TEXTUREVIEW_H

#include <Wired/Engine/IEngineAccess.h>
#include <Wired/Engine/IResources.h>

#include <NEON/Common/Build.h>
#include <NEON/Common/Space/Blit.h>
#include <NEON/Common/Log/ILogger.h>

#include <imgui.h>

#include <string>

namespace Wired
{
    /**
     * Creates an ImGui Image for a given render texture.
     *
     * If blitType is CenterInside, the Image's size will dynamic depending on
     * how large the texture can be to fit properly within the available space,
     * and the source texture will fully display without cropping.
     *
     * If blitType is CenterCrop, the Image's size will fill all available space,
     * and will the source texture will be cropped down as needed.
     */
    SUPPRESS_IS_NOT_USED
    static void TextureView(Engine::IEngineAccess* pEngine,
                            NCommon::BlitType blitType,
                            Render::TextureId textureId)
    {
        const auto viewSize = ImGui::GetContentRegionAvail();

        const auto textureSize = pEngine->GetResources()->GetTextureSize(textureId);
        if (!textureSize)
        {
            pEngine->GetLogger()->Error("TextureView: Unable to retrieve texture size: {}", textureId.id);
            return;
        }

        //
        // Create an ImGui reference to the texture to be displayed
        //
        auto texRefOffscreen = pEngine->CreateImGuiTextureReference(textureId, Render::DefaultSampler::LinearClamp);

        //
        // Calculate blit rects to determine what to select from the source texture and the ImGui Image size
        //
        auto rect = NCommon::CalculateBlitRects(
            blitType,
            NCommon::Size2DReal((float)textureSize->w, (float)textureSize->h),
            NCommon::Size2DReal(viewSize.x, viewSize.y)
        );

        const auto textureSelectRect = rect.first;

        const ImVec2 textureSelectTopLeft(
            (float)textureSelectRect.x / (float)textureSize->w,
            (float)textureSelectRect.y / (float)textureSize->h
        );
        const ImVec2 textureSelectBottomRight(
            textureSelectTopLeft.x + ((float)textureSelectRect.w / (float)textureSize->w),
            textureSelectTopLeft.y + ((float)textureSelectRect.h / (float)textureSize->h)
        );

        //
        // Create the image
        //
        ImGui::Image(
            *texRefOffscreen,
            ImVec2(rect.second.w, rect.second.h),
            textureSelectTopLeft,
            textureSelectBottomRight
        );
    }
}

#endif //WIREDEDITOR_VIEW_TEXTUREVIEW_H
