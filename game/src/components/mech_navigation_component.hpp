#pragma once

#include <optional>

#include <glm/vec3.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class MechNavigationComponent : public IComponent
{
public:
    MechNavigationComponent() = default;
    ~MechNavigationComponent() = default;

    const std::optional<glm::vec3>& GetThrust() const { return m_Thrust; }
    void SetThrust(const glm::vec3& thrust) { m_Thrust = thrust; }
    void ClearThrust() { m_Thrust.reset(); }

    const std::optional<glm::vec3>& GetAim() const { return m_Aim; }
    void SetAim(const glm::vec3& aim) { m_Aim = aim; }
    void ClearAim() { m_Aim.reset(); }

    bool GetBoost() const { return m_Boost; }
    void SetBoost(bool boost) { m_Boost = boost; }

    // A one-shot request, consumed by the MechNavigationSystem.
    const std::optional<glm::vec3>& GetDodge() const { return m_Dodge; }
    void SetDodge(const glm::vec3& direction) { m_Dodge = direction; }
    void ClearDodge() { m_Dodge.reset(); }

    float GetDodgeCooldown() const { return m_DodgeCooldown; }
    void SetDodgeCooldown(float cooldown) { m_DodgeCooldown = cooldown; }

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
    }

private:
    std::optional<glm::vec3> m_Thrust;
    std::optional<glm::vec3> m_Aim;
    bool m_Boost{ false };
    std::optional<glm::vec3> m_Dodge;
    float m_DodgeCooldown{ 0.0f };
};

REGISTER_COMPONENT(MechNavigationComponent, "mech_navigation")

} // namespace WingsOfSteel