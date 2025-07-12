/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_IDS_H
#define WIREDENGINE_WIREDGPUVK_SRC_IDS_H

#include <Wired/GPU/GPUId.h>

#include <NEON/Common/IdSource.h>

namespace Wired::GPU
{
    struct GPUVkIds
    {
        NCommon::IdSource<CommandBufferId> commandBufferIds;
        NCommon::IdSource<ImageId> imageIds;
        NCommon::IdSource<BufferId> bufferIds;
        NCommon::IdSource<SamplerId> samplerIds;
        NCommon::IdSource<PipelineId> pipelineIds;

        void Reset()
        {
            commandBufferIds.Reset();
            imageIds.Reset();
            bufferIds.Reset();
            samplerIds.Reset();
            pipelineIds.Reset();
        }
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_IDS_H
