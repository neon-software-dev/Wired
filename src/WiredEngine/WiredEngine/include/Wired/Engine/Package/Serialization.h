/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SERIALIZATION_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SERIALIZATION_H

#include "PackageManifest.h"
#include "Scene.h"

#include <NEON/Common/SharedLib.h>

#include <expected>
#include <vector>
#include <cstddef>
#include <memory>

namespace Wired::Engine
{
    template <typename T>
    [[nodiscard]] std::expected<T, bool> ObjectFromBytes(const std::vector<std::byte>& bytes);

    template <typename T>
    [[nodiscard]] std::expected<std::vector<std::byte>, bool> ObjectToBytes(const T& obj);

    template <>
    [[nodiscard]] NEON_PUBLIC std::expected<std::vector<std::byte>, bool> ObjectToBytes(const PackageManifest& obj);

    template <>
    [[nodiscard]] NEON_PUBLIC std::expected<std::vector<std::byte>, bool> ObjectToBytes(const std::shared_ptr<Scene>& obj);
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SERIALIZATION_H
