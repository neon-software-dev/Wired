/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_STATE_COMMANDBUFFERS_H
#define WIREDENGINE_WIREDGPUVK_SRC_STATE_COMMANDBUFFERS_H

#include <Wired/GPU/GPUId.h>

#include "CommandBuffer.h"

#include <unordered_map>
#include <memory>
#include <thread>
#include <string>
#include <expected>
#include <optional>
#include <mutex>

namespace Wired::GPU
{
    struct Global;
    class VulkanCommandPool;

    class CommandBuffers
    {
        public:

            explicit CommandBuffers(Global* pGlobal);
            ~CommandBuffers();

            void Destroy();

            [[nodiscard]] std::expected<CommandBuffer*, bool> AcquireCommandBuffer(VulkanCommandPool* pCommandPool,
                                                                                   CommandBufferType type,
                                                                                   const std::string& tag);
            [[nodiscard]] std::optional<CommandBuffer*> GetCommandBuffer(CommandBufferId commandBufferId) const;
            void DestroyCommandBuffer(CommandBufferId commandBufferId);

            void RunCleanUp();

        private:

            Global* m_pGlobal;

            std::unordered_map<CommandBufferId, std::unique_ptr<CommandBuffer>> m_commandBuffers;
            mutable std::mutex m_commandBuffersMutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_STATE_COMMANDBUFFERS_H
