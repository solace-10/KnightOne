#include <pandora.hpp>
#include <scene/scene.hpp>

#include "components/heat_component.hpp"
#include "systems/heat_management_system.hpp"

namespace WingsOfSteel
{

void HeatManagementSystem::Initialize(Scene* pScene)
{
}

void HeatManagementSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<HeatComponent>();
    view.each([delta](const auto entity, HeatComponent& heatComponent) {
    });
}

} // namespace WingsOfSteel
