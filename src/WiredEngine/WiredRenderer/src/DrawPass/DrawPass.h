/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASS_H
#define WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASS_H

#include "DrawPassCommon.h"

#include "../Util/ViewProjection.h"

#include <Wired/Render/StateUpdate.h>
#include <Wired/GPU/GPUCommon.h>

namespace Wired::Render
{
    struct Global;
    class DataStores;

    enum class DrawPassType
    {
        Object,
        Sprite
    };

    class DrawPass
    {
        public:

            DrawPass(Global* pGlobal, std::string groupName, const DataStores* pDataStores);

            virtual ~DrawPass() = default;

            [[nodiscard]] virtual bool StartUp() = 0;
            virtual void ShutDown() = 0;

            [[nodiscard]] virtual DrawPassType GetDrawPassType() const noexcept = 0;
            [[nodiscard]] virtual std::string GetTag() const noexcept = 0;

            [[nodiscard]] std::optional<ViewProjection> GetViewProjection() const noexcept { return m_viewProjection; }

            /**
             * Do work needed to sync this draw pass with existing data store data
             */
            virtual void ApplyInitialUpdate(GPU::CopyPass copyPass) = 0;

            /**
             * Do work needed to sync this draw pass with the provided state update
             */
            virtual void ApplyStateUpdate(GPU::CopyPass copyPass, const StateUpdate& stateUpdate) = 0;

            bool SetViewProjection(const ViewProjection& viewProjection);

            /**
             * Called when compute draw calls should be calculated, if needed.
             */
            void ComputeDrawCallsIfNeeded(GPU::CommandBufferId commandBufferId);

            [[nodiscard]] bool AreDrawCallsInvalidated() const { return m_drawCallsInvalidated; }
            void MarkDrawCallsInvalidated() { m_drawCallsInvalidated = true; }

            virtual void OnRenderSettingsChanged() {};

        protected:

            virtual void ComputeDrawCalls(GPU::CommandBufferId commandBufferId) = 0;

        protected:

            Global* m_pGlobal;
            std::string m_groupName;
            const DataStores* m_pDataStores;

        private:

            std::optional<ViewProjection> m_viewProjection;
            bool m_drawCallsInvalidated{true};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASS_H
