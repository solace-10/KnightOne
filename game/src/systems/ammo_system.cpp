#include "systems/ammo_system.hpp"

#include <algorithm>

#include <core/random.hpp>
#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <scene/components/entity_reference_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "components/ai_mech_controller_component.hpp"
#include "components/ammo_fired_by_component.hpp"
#include "components/ammo_impact_component.hpp"
#include "components/ammo_movement_component.hpp"
#include "components/ammo_raycast_component.hpp"
#include "components/hull_component.hpp"
#include "components/shield_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "game.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"

namespace WingsOfSteel
{

void AmmoSystem::Initialize(Scene* pScene)
{
    // Initialization stub
}

void AmmoSystem::Update(float delta)
{
    entt::registry& registry = GetActiveScene()->GetRegistry();
    auto view = registry.view<const TransformComponent, AmmoImpactComponent, const AmmoRaycastComponent, EntityReferenceComponent>();

    PhysicsSimulationSystem* pPhysicsSystem = GetActiveScene()->GetSystem<PhysicsSimulationSystem>();

    view.each([delta, this, pPhysicsSystem, &registry](
                  const auto entityHandle,
                  const TransformComponent& transformComponent,
                  AmmoImpactComponent& ammoImpactComponent,
                  const AmmoRaycastComponent& ammoRaycastComponent,
                  EntityReferenceComponent& entityReferenceComponent) {
        glm::vec3 startPos = transformComponent.GetTranslation();
        glm::vec3 direction = transformComponent.GetForward();

        // The raycast must cover at least this frame's movement, or a slow frame lets the projectile
        // skip past a target. It's still capped at the projectile's range.
        float raycastLength = ammoRaycastComponent.GetRaycastLength();
        if (const AmmoMovementComponent* pAmmoMovementComponent = registry.try_get<AmmoMovementComponent>(entityHandle))
        {
            const float step = delta * pAmmoMovementComponent->GetSpeed();
            raycastLength = glm::max(raycastLength, glm::min(step, raycastLength + pAmmoMovementComponent->GetRange()));
        }
        glm::vec3 endPos = startPos + direction * raycastLength;

        // Bullet doesn't order its hits, and the first one that counts stops the projectile.
        std::vector<PhysicsSimulationSystem::RaycastResult> raycastResults = pPhysicsSystem->RaycastAll(startPos, endPos);
        std::sort(raycastResults.begin(), raycastResults.end(), [](const auto& a, const auto& b) { return a.fraction < b.fraction; });
        if (raycastResults.empty())
        {
            if (m_DrawRaycasts)
            {
                GetDebugRender()->Line(startPos, endPos, Color::Yellow);
            }
        }
        else
        {
            for (const auto& result : raycastResults)
            {
                bool hasHitTerminalEntity = false;
                bool hasHitShieldEntity = false;
                bool wasBlockedByShield = false;
                EntitySharedPtr pAmmoEntity = entityReferenceComponent.GetOwner();
                if (pAmmoEntity)
                {
                    EntitySharedPtr pHitEntity = result.pEntity;
                    if (pHitEntity)
                    {
                        hasHitShieldEntity = pHitEntity->HasComponent<ShieldComponent>();
                        if (WasBlockedByShield(pHitEntity, result.position))
                        {
                            hasHitTerminalEntity = true;
                            wasBlockedByShield = true;
                        }

                        if (!hasHitShieldEntity && !wasBlockedByShield)
                        {
                            bool hitEntityStillAlive = true;
                            ApplyHullDamage(pAmmoEntity, pHitEntity, hitEntityStillAlive);
                            hasHitTerminalEntity = true;

                            if (!hitEntityStillAlive)
                            {
                                EntitySharedPtr pKilledBy;
                                if (pAmmoEntity->HasComponent<AmmoFiredByComponent>())
                                {
                                    pKilledBy = pAmmoEntity->GetComponent<AmmoFiredByComponent>().WeaponOwner.lock();
                                }

                                Game::Get()->GetSector()->GetEntityKilledSignal().Emit(pHitEntity, pKilledBy);

                                Game::Get()->GetSector()->RemoveEntity(pHitEntity);
                            }
                        }

                        if (hasHitTerminalEntity)
                        {
                            Game::Get()->GetSector()->RemoveEntity(pAmmoEntity);
                        }
                    }
                }

                if (hasHitTerminalEntity)
                {
                    if (m_DrawRaycasts)
                    {
                        GetDebugRender()->Line(startPos, result.position, Color::Red);
                        GetDebugRender()->Circle(result.position, glm::vec3(0.0f, 1.0f, 0.0f), Color::Red, 0.2f, 8);
                    }
                    break;
                }
                else if (m_DrawRaycasts)
                {
                    GetDebugRender()->Line(startPos, endPos, Color::Yellow);
                }
            }
        }
    });

    // Advance the projectile, stopping at its maximum range. It is killed the frame after it gets
    // there, so the raycast above still covers the end of its range.
    auto movementView = registry.view<TransformComponent, AmmoMovementComponent, EntityReferenceComponent>();
    movementView.each([delta, this, pPhysicsSystem](const auto entity, TransformComponent& transformComponent, AmmoMovementComponent& ammoMovementComponent, EntityReferenceComponent& entityReferenceComponent) {
        const glm::vec3 startPos = transformComponent.GetTranslation();
        const glm::vec3 direction = transformComponent.GetForward();

        const float distance = glm::min(delta * ammoMovementComponent.GetSpeed(), ammoMovementComponent.GetRange());
        const float rangeAfterAdvance = ammoMovementComponent.GetRange() - distance;
        if (ammoMovementComponent.GetRange() <= 0.0f)
        {
            EntitySharedPtr pAmmoEntity = entityReferenceComponent.GetOwner();
            if (pAmmoEntity)
            {
                Game::Get()->GetSector()->RemoveEntity(pAmmoEntity);
            }
        }
        else
        {
            const glm::vec3 offsetPos(startPos + direction * distance);
            transformComponent.transform[3] = glm::vec4(offsetPos.x, offsetPos.y, offsetPos.z, 1.0f);
            ammoMovementComponent.SetRange(rangeAfterAdvance);
        }
    });
}

void AmmoSystem::Instantiate(EntitySharedPtr pWeaponEntity, const WeaponComponent& weaponComponent)
{
    Sector* pSector = Game::Get()->GetSector();
    if (!pSector)
    {
        return;
    }

    glm::mat4 ammoTransform = pWeaponEntity->GetComponent<TransformComponent>().transform;

    // Accuracy changes the initial forward vector of the ammo being fired.
    // Values are in degrees.
    static const std::array<float, static_cast<size_t>(WeaponAccuracy::Count)> sAccuracyModifiers{
        0.0f, // Perfect
        2.5f, // Very high
        5.0f, // High
        10.0f, // Medium
        15.0f, // Low
    };

    if (weaponComponent.m_Accuracy != WeaponAccuracy::Perfect)
    {
        const float halfModifier = sAccuracyModifiers[static_cast<size_t>(weaponComponent.m_Accuracy)] / 2.0f;
        const float deviation = Random::Get(-halfModifier, halfModifier);
        ammoTransform = glm::rotate(ammoTransform, glm::radians(deviation), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    SceneWeakPtr pWeakScene = pSector->GetWeakPtr();
    EntityBuilder::Build(
        pWeakScene,
        weaponComponent.m_Ammo,
        ammoTransform,
        [pWeaponEntity, weaponComponent](EntitySharedPtr pEntity) {
            float range = weaponComponent.m_Range;

            if (pEntity->HasComponent<AmmoRaycastComponent>())
            {
                range -= pEntity->GetComponent<AmmoRaycastComponent>().GetRaycastLength();
            }

            if (pEntity->HasComponent<AmmoMovementComponent>())
            {
                pEntity->GetComponent<AmmoMovementComponent>().SetRange(range);
            }

            AmmoFiredByComponent& ammoFiredByComponent = pEntity->AddComponent<AmmoFiredByComponent>();
            ammoFiredByComponent.Weapon = pWeaponEntity;
            ammoFiredByComponent.WeaponOwner = pWeaponEntity->GetParent();
        });
}

void AmmoSystem::ApplyHullDamage(EntitySharedPtr pAmmoEntity, EntitySharedPtr pHitEntity, bool& hitEntityStillAlive) const
{
    hitEntityStillAlive = true;

    // Don't apply any hull damage if the result of this encounter has already been decided.
    // This avoids edge cases where the player dies after the victory screen shows up, or wins after their own carrier has been destroyed.
    Encounter* pEncounter = Game::Get()->GetSector()->GetEncounter();
    if (pEncounter && pEncounter->GetEncounterResult() != EncounterResult::Undecided)
    {
        return;
    }

    if (pAmmoEntity->HasComponent<AmmoImpactComponent>() && pHitEntity->HasComponent<HullComponent>())
    {
        AmmoImpactComponent& ammoImpactComponent = pAmmoEntity->GetComponent<AmmoImpactComponent>();
        HullComponent& hullComponent = pHitEntity->GetComponent<HullComponent>();

        hullComponent.Health -= ammoImpactComponent.Damage;
        ammoImpactComponent.ArmorPenetration -= hullComponent.Thickness;
        hitEntityStillAlive = (hullComponent.Health > 0.0f);

        // Notify AI that this entity is under fire.
        if (pHitEntity->HasComponent<AIMechControllerComponent>())
        {
            pHitEntity->GetComponent<AIMechControllerComponent>().DefenseContext.underFire = true;
        }
    }
}

bool AmmoSystem::WasBlockedByShield(EntitySharedPtr pHitEntity, const glm::vec3& hitPosition) const
{
    if (!pHitEntity->HasComponent<ShieldComponent>())
    {
        return false;
    }

    const ShieldComponent& shieldComponent = pHitEntity->GetComponent<ShieldComponent>();
    if (shieldComponent.CurrentState != ShieldState::Active)
    {
        return false;
    }

    // Shield only blocks in a 180 degree front arc.
    const TransformComponent& transformComponent = pHitEntity->GetComponent<TransformComponent>();
    const glm::vec3 entityPosition = transformComponent.GetTranslation();
    const glm::vec3 entityForward = transformComponent.GetForward();
    const glm::vec3 toHitPosition = glm::normalize(hitPosition - entityPosition);

    // Dot product > 0 means the hit is in the front hemisphere.
    return glm::dot(entityForward, toHitPosition) > 0.0f;
}

} // namespace WingsOfSteel
