#pragma once

#include <scene/components/icomponent.hpp>
#include <scene/components/component_factory.hpp>

namespace WingsOfSteel
{

class MechEngineComponent : public IComponent
{
public:
    float linearForce{ 0.0f };
    float torque{ 0.0f };
    float maxSpeed{ 0.0f };
    float boostLinearForce{ 0.0f };
    float boostMaxSpeed{ 0.0f };
    float dodgeSpeed{ 0.0f };
    float dodgeCooldown{ 0.0f };

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        linearForce = Json::DeserializeFloat(pContext, json, "linear_force", 0.0f);
        torque = Json::DeserializeFloat(pContext, json, "torque", 0.0f);
        maxSpeed = Json::DeserializeFloat(pContext, json, "max_speed", 0.0f);
        boostLinearForce = Json::DeserializeFloat(pContext, json, "boost_linear_force", 0.0f);
        boostMaxSpeed = Json::DeserializeFloat(pContext, json, "boost_max_speed", 0.0f);
        dodgeSpeed = Json::DeserializeFloat(pContext, json, "dodge_speed", 0.0f);
        dodgeCooldown = Json::DeserializeFloat(pContext, json, "dodge_cooldown", 0.0f);
    }
};

REGISTER_COMPONENT(MechEngineComponent, "mech_engine")

} // namespace WingsOfSteel