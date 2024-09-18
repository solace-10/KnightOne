#pragma once

#include <string>

#include "resources/resource.fwd.hpp"

#include "vfs/vfs.hpp"

namespace WingsOfSteel::Pandora
{

class Resource
{
public:
    Resource();
    virtual ~Resource();

    virtual void Load(const std::string& path);
    virtual ResourceType GetResourceType() const = 0;

    const std::string& GetName() const;
    const std::string& GetPath() const;
    ResourceState GetState() const;

protected:
    void SetState(ResourceState state);

private:
    std::string m_Name;
    std::string m_Path;
    ResourceState m_State;
};

} // namespace WingsOfSteel::Pandora