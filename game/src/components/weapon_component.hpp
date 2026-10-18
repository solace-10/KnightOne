#pragma once

#include <optional>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <core/log.hpp>
#include <core/smart_ptr.hpp>
#include <scene/entity.hpp>
#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>
#include <pandora.hpp>

namespace WingsOfSteel
{

enum class WeaponAccuracy
{
    Perfect,
    VeryHigh,
    High,
    Medium,
    Low,
    Unused,

    Count
};

enum class WeaponFriendOrFoe
{
    Enabled,
    Disabled
};

class WeaponComponent : public IComponent
{
public:
    WeaponComponent() = default;

    std::string m_AttachmentPointName;
    glm::mat4 m_AttachmentPointTransform{ 1.0f };
    float m_ArcMinDegrees{ 0.0f };
    float m_ArcMaxDegrees{ 0.0f };
    float m_AngleDegrees{ 0.0f };
    float m_Range{ 100.0f };
    float m_RateOfFire{ 1.0f }; // Ammo fired per second.
    float m_FireTimer{ 0.0f }; // Number in seconds until the weapon can be fired again.
    int m_ClipSize{ 0 }; // Rounds per clip. 0 means infinite ammo.
    int m_Clips{ 0 }; // Clips remaining, including the loaded one. 0 means unlimited.
    float m_ReloadTime{ 0.0f }; // Seconds to load a new clip.
    int m_AmmoInClip{ 0 };
    std::optional<float> m_ReloadTimer; // Seconds until the reload completes; set while reloading.
    bool m_WantsToReload{ false }; // A controller has requested this weapon to reload.
    bool m_WantsToFire{ false }; // A controller has requested this weapon to fire.
    bool m_AutomatedTargeting{ true }; // AI-controlled target acquisition.
    WeaponAccuracy m_Accuracy{ WeaponAccuracy::Perfect };
    bool m_Dummy{ false }; // This weapon is a dummy and doesn't do anything; attached for visual interest but controlled by a different system.
    WeaponFriendOrFoe m_FriendOrFoe{ WeaponFriendOrFoe::Enabled }; // If a weapon has FoF enabled, it will not open fire if its trajectory would intersect an ally.

    std::string m_Ammo;

    std::optional<glm::vec3> m_TargetPosition;
    std::optional<glm::vec3> m_TargetVelocity;

    // Owner: the Entity that has this component.
    void SetOwner(EntityWeakPtr pOwner) { m_pOwner = pOwner; }
    EntityWeakPtr GetOwner() { return m_pOwner; }

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        m_Range = Json::DeserializeFloat(pContext, json, "range", 0.0f);
        m_RateOfFire = Json::DeserializeFloat(pContext, json, "rate_of_fire", 0.0f);
        m_Ammo = Json::DeserializeString(pContext, json, "ammo", "");
        m_Dummy = Json::DeserializeBool(pContext, json, "dummy", false);
        m_ClipSize = Json::DeserializeInteger(pContext, json, "clip_size", 0);
        m_Clips = Json::DeserializeInteger(pContext, json, "clips", 0);
        m_ReloadTime = Json::DeserializeFloat(pContext, json, "reload_time", 0.0f);
        m_AmmoInClip = m_ClipSize;

        const std::string accuracy = Json::DeserializeString(pContext, json, "accuracy", "unused");
        if (accuracy == "perfect")
        {
            m_Accuracy = WeaponAccuracy::Perfect;
        }
        else if (accuracy == "very_high")
        {
            m_Accuracy = WeaponAccuracy::VeryHigh;
        }
        else if (accuracy == "high")
        {
            m_Accuracy = WeaponAccuracy::High;
        }
        else if (accuracy == "medium")
        {
            m_Accuracy = WeaponAccuracy::Medium;
        }
        else if (accuracy == "low")
        {
            m_Accuracy = WeaponAccuracy::Low;
        }
        else if (accuracy == "unused")
        {
            m_Accuracy = WeaponAccuracy::Unused;
        }
        else
        {
            Log::Error() << "Unable to deserialize WeaponComponent, invalid 'accuracy' value: '" << accuracy << "'. "
                << "Value must be 'perfect', 'very_high', 'high', 'medium', 'low' or 'unused'.";
        }
    }

private:
    EntityWeakPtr m_pOwner;
};

REGISTER_COMPONENT(WeaponComponent, "weapon")

} // namespace WingsOfSteel
