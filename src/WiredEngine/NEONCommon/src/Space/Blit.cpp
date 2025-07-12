/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <NEON/Common/Space/Blit.h>

#include <cassert>
#include <algorithm>

namespace NCommon
{

std::pair<RectReal, RectReal> CalculateBlitRects_CenterCrop(
    const Size2DReal& sourceSize,
    const Size2DReal& targetSize);

std::pair<RectReal, RectReal> CalculateBlitRects_CenterInside(
    const Size2DReal& sourceSize,
    const Size2DReal& targetSize);

std::pair<RectReal, RectReal> CalculateBlitRects(
    const BlitType& blitType,
    const Size2DReal& sourceSize,
    const Size2DReal& targetSize)
{
    switch (blitType)
    {
        case BlitType::CenterCrop: return CalculateBlitRects_CenterCrop(sourceSize, targetSize);
        case BlitType::CenterInside: return CalculateBlitRects_CenterInside(sourceSize, targetSize);
    }

    assert(false);
    return {};
}

std::pair<NCommon::RectReal, NCommon::RectReal> CalculateBlitRects_CenterCrop(
    const NCommon::Size2DReal& sourceSize,
    const NCommon::Size2DReal& targetSize)
{
    const float renderAspectRatio = sourceSize.w / sourceSize.h;
    const float targetAspectRatio = targetSize.w / targetSize.h;

    NCommon::RectReal srcBlit(0, 0, sourceSize.w, sourceSize.h);

    if (renderAspectRatio >= targetAspectRatio)
    {
        // Scale the source's dimensions so that its height matches the target's height, determine
        // from the scaled dimensions what percentage of its scaled width the target's width takes up,
        // crop to that percentage of the original target's size's width, and offset the x offset by
        // half the remaining source width.

        const float scaleFactor = targetSize.h / sourceSize.h;
        const float sourceScaledWidth = scaleFactor * sourceSize.w;
        const float widthRatio = targetSize.w / sourceScaledWidth;

        srcBlit.w = sourceSize.w * widthRatio;
        srcBlit.x = (sourceSize.w - srcBlit.w) / 2.0f;
    }
    else
    {
        const float scaleFactor = targetSize.w / sourceSize.w;
        const float sourceScaledHeight = scaleFactor * sourceSize.h;
        const float heightRatio = targetSize.h / sourceScaledHeight;

        srcBlit.h = sourceSize.h * heightRatio;
        srcBlit.y = (sourceSize.h - srcBlit.h) / 2.0f;
    }

    // Prevent float rounding errors from having a zero width/height when the target
    // has a ridiculously small (1px) dimension
    srcBlit.w = std::max(srcBlit.w, 1.0f);
    srcBlit.h = std::max(srcBlit.h, 1.0f);

    // When center cropping, the entire destination is blitted to
    NCommon::RectReal dstBlit(0, 0, targetSize.w, targetSize.h);

    return {srcBlit, dstBlit};
}

std::pair<NCommon::RectReal, NCommon::RectReal> CalculateBlitRects_CenterInside(
    const NCommon::Size2DReal& sourceSize,
    const NCommon::Size2DReal& targetSize)
{
    const float renderAspectRatio = (float)sourceSize.w / (float)sourceSize.h;
    const float targetAspectRatio = (float)targetSize.w / (float)targetSize.h;

    NCommon::RectReal dstBlit(0, 0, targetSize.w, targetSize.h);

    if (renderAspectRatio >= targetAspectRatio)
    {
        dstBlit.h = sourceSize.h * (targetSize.w / sourceSize.w);
        dstBlit.y = (targetSize.h - dstBlit.h) / 2.0f;
    }
    else
    {
        dstBlit.w = sourceSize.w * (targetSize.h / sourceSize.h);
        dstBlit.x = (targetSize.w - dstBlit.w) / 2.0f;
    }

    // Prevent float rounding errors from having a zero width/height when the target
    // has a ridiculously small (1px) dimension
    dstBlit.w = std::max(dstBlit.w, 1.0f);
    dstBlit.h = std::max(dstBlit.h, 1.0f);

    // When centering inside the entire source is blitted from
    NCommon::RectReal srcBlit(0, 0, sourceSize.w, sourceSize.h);

    return {srcBlit, dstBlit};
}

}
