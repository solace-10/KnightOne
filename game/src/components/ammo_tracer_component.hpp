#pragma once

#include <glm/vec3.hpp>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

// How ProjectileRenderSystem draws a projectile. The tracer spans the projectile's raycast, so what
// is drawn is what can hit.
class AmmoTracerComponent : public IComponent
{
public:
    AmmoTracerComponent() = default;
    ~AmmoTracerComponent() = default;

    glm::vec3 Color{ 4.0f, 1.6f, 0.4f }; // Linear HDR: values above 1 bloom.
    float Width{ 0.4f };

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override
    {
        Color = Json::DeserializeVec3(pContext, json, "color", Color);
        Width = Json::DeserializeFloat(pContext, json, "width", Width);
    }
};

REGISTER_COMPONENT(AmmoTracerComponent, "ammo_tracer")

} // namespace WingsOfSteel
