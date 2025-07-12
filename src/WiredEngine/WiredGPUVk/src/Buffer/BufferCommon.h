/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERCOMMON_H
#define WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERCOMMON_H

namespace Wired::GPU
{
    enum class BufferUsageMode
    {
        TransferSrc,
        TransferDst,
        VertexRead,
        IndexRead,
        Indirect,
        GraphicsUniformRead,
        GraphicsStorageRead,
        ComputeUniformRead,
        ComputeStorageRead,
        ComputeStorageReadWrite
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERCOMMON_H
