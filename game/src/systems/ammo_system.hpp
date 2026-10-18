#pragma once

#include <glm/vec3.hpp>

#include <scene/systems/system.hpp>
#include <scene/entity.hpp>

#include "components/weapon_component.hpp"

namespace WingsOfSteel
{

class AmmoSystem : public System
{
public:
    AmmoSystem() = default;
    ~AmmoSystem() = default;

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    void Instantiate(EntitySharedPtr pWeaponEntity, const WeaponComponent& weaponComponent);

    bool GetDrawRaycasts() const { return m_DrawRaycasts; }
    void SetDrawRaycasts(bool state) { m_DrawRaycasts = state; }

private:
    void ApplyHullDamage(EntitySharedPtr pAmmoEntity, EntitySharedPtr pHitEntity, bool& hitEntityStillAlive) const;
    bool WasBlockedByShield(EntitySharedPtr pHitEntity, const glm::vec3& hitPosition) const;

    bool m_DrawRaycasts{ false };
};

} // namespace WingsOfSteel
