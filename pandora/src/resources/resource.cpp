#include "resources/resource.hpp"

namespace WingsOfSteel::Pandora
{

Resource::Resource()
: m_State(ResourceState::Unloaded)
{

}

Resource::~Resource()
{

}

void Resource::Load(const std::string& path)
{
    m_Path = path;
    SetState(ResourceState::Loading);
}

const std::string& Resource::GetPath() const
{
    return m_Path;
}

ResourceState Resource::GetState() const
{
    return m_State;
}

void Resource::SetState(ResourceState state)
{
    m_State = state;
}

} // namespace WingsOfSteel::Pandora