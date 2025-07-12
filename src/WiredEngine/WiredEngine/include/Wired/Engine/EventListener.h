/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_EVENTLISTENER_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_EVENTLISTENER_H

#include "IEngineAccess.h"

#include <Wired/Platform/Event/Events.h>

#include <Wired/Render/RenderSettings.h>

namespace Wired::Engine
{
    class EventListener
    {
        public:

            virtual ~EventListener() = default;

            virtual void OnSimulationStep(unsigned int timeStepMs) { (void)timeStepMs; };
            virtual void OnKeyEvent(const Platform::KeyEvent& event) { (void)event; };
            virtual void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) { (void)event; };
            virtual void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) { (void)event; };
            virtual void OnRenderSettingsChanged(const Render::RenderSettings& renderSettings) { (void)renderSettings; };
    };
}


#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_EVENTLISTENER_H
