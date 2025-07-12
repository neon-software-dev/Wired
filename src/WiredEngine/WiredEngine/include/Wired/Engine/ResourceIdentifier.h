/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RESOURCEIDENTIFIER_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RESOURCEIDENTIFIER_H

#include <Wired/Engine/World/WorldCommon.h>

#include <NEON/Common/SharedLib.h>

#include <format>
#include <string>
#include <optional>
#include <format>

namespace Wired::Engine
{
    class NEON_PUBLIC ResourceIdentifier
    {
        public:

            ResourceIdentifier() = default;

            auto operator<=>(const ResourceIdentifier&) const = default;

            void SetContextName(const std::optional<std::string>& contextName) { m_contextName = contextName; }
            [[nodiscard]] std::optional<std::string> GetContextName() const noexcept { return m_contextName; }

            void SetResourceName(const std::string& resourceName) { m_resourceName = resourceName; }
            [[nodiscard]] std::string GetResourceName() const noexcept { return m_resourceName; }

            [[nodiscard]] bool IsValid() const { return !m_resourceName.empty(); }
            [[nodiscard]] bool HasContext() const { return m_contextName.has_value(); }

            [[nodiscard]] std::string GetUniqueName() const
            {
                const std::string contextNameStr = m_contextName.has_value() ? *m_contextName : std::string();
                return std::format("{}-{}", contextNameStr, m_resourceName);
            }

        protected:

            ResourceIdentifier(std::optional<std::string> contextName, std::string resourceName)
                : m_contextName(std::move(contextName))
                , m_resourceName(std::move(resourceName))
            { }

        private:

            std::optional<std::string> m_contextName;
            std::string m_resourceName;
    };

    struct NEON_PUBLIC PackageResourceIdentifier : public ResourceIdentifier
    {
        PackageResourceIdentifier(PackageName packageName, std::string resourceName)
            : ResourceIdentifier(packageName.id, std::move(resourceName))
        { }

        PackageResourceIdentifier(std::string packageName, std::string resourceName)
            : ResourceIdentifier(packageName, std::move(resourceName))
        { }

        explicit PackageResourceIdentifier(const ResourceIdentifier& resource)
            : ResourceIdentifier(resource)
        { }
    };

    using PRI = PackageResourceIdentifier;

    struct NEON_PUBLIC NoContextResourceIdentifier : public ResourceIdentifier
    {
        explicit NoContextResourceIdentifier(std::string _resourceName)
            : ResourceIdentifier(std::nullopt, std::move(_resourceName))
        { }

        explicit NoContextResourceIdentifier(const ResourceIdentifier& resource)
            : ResourceIdentifier(resource)
        { }
    };

    using NCRI = NoContextResourceIdentifier;
}

template<>
struct std::hash<Wired::Engine::ResourceIdentifier>
{
    std::size_t operator()(const Wired::Engine::ResourceIdentifier& o) const noexcept {
        return std::hash<std::string>{}(o.GetUniqueName());
    }
};

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_RESOURCEIDENTIFIER_H
