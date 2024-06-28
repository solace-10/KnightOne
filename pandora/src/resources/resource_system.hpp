#pragma once

#include <functional>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>

#include "resources/resource.fwd.hpp"

namespace WingsOfSteel::Pandora
{

class ResourceSystem
{
public:
    ResourceSystem();
    ~ResourceSystem();
    void Update();

    void RequestResource(const std::string& path, OnResourceAvailableCallback onResourceLoaded);

private:
    template<typename T> void RegisterResource(const std::string& extension)
    {
        m_ResourceCreationFunctions[extension] = []()
        {
            return std::make_shared<T>();
        };
    }

    std::optional<std::string> GetExtension(const std::string& path) const;

    std::unordered_map<std::string, ResourceSharedPtr> m_Resources;

    using ResourceCreationFunction = std::function<ResourceSharedPtr()>;
    std::unordered_map<std::string, ResourceCreationFunction> m_ResourceCreationFunctions;

    struct PendingResource
    {
        ResourceSharedPtr pResource;
        OnResourceAvailableCallback onResourceAvailable;
    };
    std::list<PendingResource> m_PendingResources;
};

} // namespace WingsOfSteel::Pandora