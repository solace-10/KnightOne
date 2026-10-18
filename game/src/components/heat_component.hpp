#pragma once

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

class HeatComponent : public IComponent
{
public:
    float currentHeat{ 0.0f };
    float overheatThreshold{ 0.0f };
    float dissipationRate{ 0.0f };
    float dissipationDelay{ 0.0f };

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        overheatThreshold = Json::DeserializeFloat(pContext, json, "overheat_threshold", 0.0f);
        dissipationRate = Json::DeserializeFloat(pContext, json, "dissipation_rate", 0.0f);
        dissipationDelay = Json::DeserializeFloat(pContext, json, "dissipation_delay", 0.0f);
    }
};

REGISTER_COMPONENT(HeatComponent, "heat")

} // namespace WingsOfSteel
