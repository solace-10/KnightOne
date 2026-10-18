#include <numeric>

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include <core/log.hpp>

#include "components/mech_engine_component.hpp"
#include "components/mech_navigation_component.hpp"
#include "systems/mech_navigation_system.hpp"

namespace WingsOfSteel
{

void MechNavigationSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<MechNavigationComponent, const MechEngineComponent, RigidBodyComponent>();

    view.each([this, delta](const auto entity, MechNavigationComponent& mechNavigationComponent, const MechEngineComponent& mechEngineComponent, RigidBodyComponent& rigidBodyComponent) {
        const std::optional<glm::vec3>& thrustDirection = mechNavigationComponent.GetThrust();
        if (thrustDirection.has_value())
        {
            const bool boost = mechNavigationComponent.GetBoost();
            const float linearForce = boost ? mechEngineComponent.boostLinearForce : mechEngineComponent.linearForce;
            const float maxSpeed = boost ? mechEngineComponent.boostMaxSpeed : mechEngineComponent.maxSpeed;

            // Soft cap: stop thrusting once at max speed and let damping bring the speed down, so a dodge
            // or the end of a boost isn't snapped back.
            if (glm::dot(rigidBodyComponent.GetLinearVelocity(), thrustDirection.value()) < maxSpeed)
            {
                rigidBodyComponent.ApplyLinearForce(thrustDirection.value() * linearForce);
            }
        }

        const float dodgeCooldown = glm::max(mechNavigationComponent.GetDodgeCooldown() - delta, 0.0f);
        mechNavigationComponent.SetDodgeCooldown(dodgeCooldown);
        const std::optional<glm::vec3>& dodgeDirection = mechNavigationComponent.GetDodge();
        if (dodgeDirection.has_value())
        {
            if (dodgeCooldown <= 0.0f)
            {
                rigidBodyComponent.SetLinearVelocity(dodgeDirection.value() * mechEngineComponent.dodgeSpeed);
                mechNavigationComponent.SetDodgeCooldown(mechEngineComponent.dodgeCooldown);
            }
            mechNavigationComponent.ClearDodge();
        }

        if (mechNavigationComponent.GetAim().has_value())
        {
            TurnTowards(mechNavigationComponent.GetAim().value(), rigidBodyComponent, mechEngineComponent.torque);
        }
    });
}

} // namespace WingsOfSteel