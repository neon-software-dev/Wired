/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_GRIDLOGIC_H
#define WIREDEDITOR_GRIDLOGIC_H

#include "Window/ViewportWindow.h"

#include <vector>
#include <algorithm>

namespace Wired
{
    inline float CalculateGridInterval(float scale)
    {
        // Ensure scale is within the viewport limits as it should be
        scale = std::clamp(scale, VIEWPORT_MIN_2D_SCALE, VIEWPORT_MAX_2D_SCALE);

        // 100.0f "base" interval at scale=1.0f, with an inverse relationship to scale
        const float scaledBaseSpacing = 100.0f / scale;

        // Discrete spacing values
        const std::vector<float> spacingSteps {
            0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f
        };

        // Pick the closest step >= baseSpacing
        for (auto step : spacingSteps)
        {
            if (scaledBaseSpacing <= step)
            {
                return step;
            }
        }

        // Cap to max interval size for anything larger
        return spacingSteps.at(spacingSteps.size() - 1);
    }

    inline float CalculateGridLineThickness(float scale)
    {
        // Ensure scale is within the viewport limits as it should be
        scale = std::clamp(scale, VIEWPORT_MIN_2D_SCALE, VIEWPORT_MAX_2D_SCALE);

        // Base thickness is 3.0 at a scale of 1.0
        const float idealThickness = 3.0f * (1.0f / scale);

        // Discrete thickness values
        const std::vector<float> thicknessSteps {
            0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f
        };

        // Find the closest value in thicknessSteps to idealThickness
        float closestThickness = thicknessSteps.at(0);
        float minThicknessDiff = std::abs(idealThickness - closestThickness);

        for (float step : thicknessSteps)
        {
            const float diff = std::abs(idealThickness - step);
            if (diff < minThicknessDiff)
            {
                closestThickness = step;
                minThicknessDiff = diff;
            }
        }

        return closestThickness;
    }
}

#endif //WIREDEDITOR_GRIDLOGIC_H
