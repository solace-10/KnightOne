#pragma once

#include <vector>

#include <glm/vec3.hpp>

#include <scene/components/component_factory.hpp>
#include <scene/components/icomponent.hpp>

namespace WingsOfSteel
{

// An anisotropic spherical Gaussian: a soft, stretchable blob of light on the sky.
struct SkyboxLobe
{
    float Yaw{ 0.0f }; // In degrees; 0 looks down -Z.
    float Pitch{ 0.0f }; // In degrees; positive looks up.
    float Roll{ 0.0f }; // In degrees, around the lobe's own axis.
    float SharpnessX{ 4.0f }; // Lambda: how quickly the lobe falls off across its length.
    float SharpnessY{ 1.0f }; // Mu: how quickly the lobe falls off along its length.
    glm::vec3 Color{ 1.0f }; // Linear HDR.
};

// A tight, bright highlight, with a faint halo that follows the warped nebula around it.
struct SkyboxCore
{
    float Yaw{ 0.0f };
    float Pitch{ 0.0f };
    float Sharpness{ 2000.0f };
    glm::vec3 Color{ 1.0f };
};

class SkyboxComponent : public IComponent
{
public:
    static constexpr size_t kMaxLobes = 8;
    static constexpr size_t kMaxCores = 4;

    SkyboxComponent() = default;

    void Deserialize(const ResourceDataStore* pContext, const Json::Data& json) override;

    // The inverse of Deserialize(), so values tweaked in the debug UI can be pasted into a prefab.
    Json::Data Serialize() const;

    std::vector<SkyboxLobe> Lobes;
    std::vector<SkyboxCore> Cores;

    // Rotates the whole composition, which is authored around yaw 0, pitch 0. The sector camera
    // looks down at roughly -56 degrees, so that is where the sky needs to be pitched to be seen.
    glm::vec3 Orientation{ 0.0f, -56.0f, 0.0f }; // Yaw, pitch and roll, in degrees.

    glm::vec3 FloorColorA{ 0.05f, 0.022f, 0.06f }; // Background glow towards the composition's up.
    glm::vec3 FloorColorB{ 0.016f, 0.03f, 0.06f }; // Background glow towards the composition's down.
    float FloorGlow{ 1.0f };

    glm::vec3 Absorb{ 0.9f, 1.5f, 2.3f }; // Per channel absorption of the dust lanes; blue is absorbed the most.
    float Dust{ 0.6f };

    float WarpStrength{ 0.35f };
    float NoiseScale{ 1.6f };
    glm::vec3 Seed{ 0.0f };

    float Stars{ 1.0f }; // Drawn every frame rather than baked, so changing it needs no rebake.

    uint32_t Resolution{ 1024 }; // Of each cubemap face.
    uint32_t Generation{ 1 }; // Bumped on any change that needs the cubemap rebaked.
};

REGISTER_COMPONENT(SkyboxComponent, "skybox")

} // namespace WingsOfSteel
