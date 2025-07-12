/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RenderOutputView.h"

#include <Wired/Engine/EngineCommon.h>
#include <Wired/Engine/SpaceUtil.h>

#include <NEON/Common/Space/SpaceUtil.h>
#include <NEON/Common/Log/ILogger.h>

namespace Wired
{

RenderOutputView::RenderOutputView(Engine::IEngineAccess* pEngine)
    : m_pEngine(pEngine)
{

}

void RenderOutputView::operator()(Render::TextureId textureId)
{
    const auto viewSize = ImGui::GetContentRegionAvail();

    m_windowPos = ImGui::GetWindowPos();
    m_windowContentRegionMin = ImGui::GetWindowContentRegionMin();
    m_windowContentRegionMax = ImGui::GetWindowContentRegionMax();

    //
    // Create a renderer reference to the texture to be displayed
    //
    auto texRefOffscreen = m_pEngine->CreateImGuiTextureReference(
        textureId,
        Render::DefaultSampler::LinearClamp
    );
    if (!texRefOffscreen)
    {
        m_pEngine->GetLogger()->Error("RenderOutputView: Failed to create texture reference for texture: {}", textureId.id);
        return;
    }

    //
    // Create the render output image
    //
    const auto renderRes = m_pEngine->GetRenderSettings().resolution;

    m_blitRects = NCommon::CalculateBlitRects(
        NCommon::BlitType::CenterCrop,
        NCommon::Size2DReal::CastFrom(renderRes),
        NCommon::Size2DReal(viewSize.x, viewSize.y)
    );

    const auto renderSelectRect = m_blitRects->first;

    const ImVec2 topLeft(
        (float)renderSelectRect.x / (float)renderRes.w,
        (float)renderSelectRect.y / (float)renderRes.h
    );
    const ImVec2 bottomRight(
        topLeft.x + ((float)renderSelectRect.w / (float)renderRes.w),
        topLeft.y + ((float)renderSelectRect.h / (float)renderRes.h)
    );

    ImGui::Image(*texRefOffscreen, viewSize, topLeft, bottomRight);
}

std::optional<Engine::VirtualSpacePoint> RenderOutputView::GetMouseVirtualSpacePoint()
{
    // Need this window to have been displayed at least once, generating our blit rects, in
    // order for us to perform the conversion
    if (!m_blitRects) { return std::nullopt; }

    //
    // Get the mouse pos in this-window-relative space (screen space), rather than OS window space
    //
    const auto mouseScreenPos = GetMousePosRelativeToWindowContent();
    if (!mouseScreenPos)
    {
        // Mouse is not over this window
        return std::nullopt;
    }

    //
    // Convert from screen surface point to render surface point
    //
    const auto renderSurface = NCommon::Surface(m_pEngine->GetRenderSettings().resolution);

    const auto renderSurfacePoint = Engine::ScreenSurfacePointToRenderSurfacePoint(
        Engine::ScreenSurfacePoint(mouseScreenPos->x, mouseScreenPos->y),
        m_blitRects->second,
        m_blitRects->first
    );

    if (!renderSurfacePoint)
    {
        // Should never be the case since GetMousePosRelativeToWindowContent should fail if
        // the mouse isn't over the render surface
        return std::nullopt;
    }

    // Convert from render surface point to render space point
    const auto renderSpacePoint = NCommon::MapSurfacePointToPointSpaceCenterOrigin<NCommon::Surface, NCommon::Point2DReal, NCommon::Point3DReal>(
        *renderSurfacePoint, renderSurface);

    // Convert from render space point to virtual space point
    const auto virtualSurface = NCommon::Surface(m_pEngine->GetVirtualResolution());

    const auto virtualSpacePoint = NCommon::Map3DPointBetweenSurfaces<NCommon::Point3DReal, NCommon::Point3DReal>(
        renderSpacePoint, renderSurface, virtualSurface);

    return Engine::VirtualSpacePoint(virtualSpacePoint.x, virtualSpacePoint.y, virtualSpacePoint.z);
}

std::optional<ImVec2> RenderOutputView::GetMousePosRelativeToWindowContent() const
{
    const ImVec2 mousePos = ImGui::GetMousePos();

    const ImVec2 contentOrigin = ImVec2(m_windowPos.x + m_windowContentRegionMin.x, m_windowPos.y + m_windowContentRegionMin.y);
    const ImVec2 contentEnd = ImVec2(m_windowPos.x + m_windowContentRegionMax.x, m_windowPos.y + m_windowContentRegionMax.y);

    if (mousePos.x < contentOrigin.x || mousePos.x > contentEnd.x ||
        mousePos.y < contentOrigin.y || mousePos.y > contentEnd.y)
    {
        return std::nullopt;
    }

    return ImVec2(mousePos.x - contentOrigin.x, mousePos.y - contentOrigin.y);
}

}
