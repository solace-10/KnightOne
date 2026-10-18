#include "components/skybox_component.hpp"

#include <core/log.hpp>

namespace WingsOfSteel
{

namespace
{

Json::Data ToJson(const glm::vec3& value)
{
    return Json::Data::array({ value.x, value.y, value.z });
}

} // namespace

void SkyboxComponent::Deserialize(const ResourceDataStore* pContext, const Json::Data& json)
{
    Orientation = Json::DeserializeVec3(pContext, json, "orientation", Orientation);
    FloorColorA = Json::DeserializeVec3(pContext, json, "floor_color_a", FloorColorA);
    FloorColorB = Json::DeserializeVec3(pContext, json, "floor_color_b", FloorColorB);
    FloorGlow = Json::DeserializeFloat(pContext, json, "floor_glow", FloorGlow);
    Absorb = Json::DeserializeVec3(pContext, json, "absorb", Absorb);
    Dust = Json::DeserializeFloat(pContext, json, "dust", Dust);
    WarpStrength = Json::DeserializeFloat(pContext, json, "warp_strength", WarpStrength);
    NoiseScale = Json::DeserializeFloat(pContext, json, "noise_scale", NoiseScale);
    Seed = Json::DeserializeVec3(pContext, json, "seed", Seed);
    Stars = Json::DeserializeFloat(pContext, json, "stars", Stars);
    Resolution = Json::DeserializeUnsignedInteger(pContext, json, "resolution", Resolution);

    Lobes.clear();
    auto lobesIt = json.find("lobes");
    if (lobesIt != json.cend() && lobesIt->is_array())
    {
        for (const Json::Data& lobeData : *lobesIt)
        {
            if (Lobes.size() == kMaxLobes)
            {
                Log::Warning() << "Skybox has more than " << kMaxLobes << " lobes; the rest are ignored.";
                break;
            }

            SkyboxLobe lobe;
            lobe.Yaw = Json::DeserializeFloat(pContext, lobeData, "yaw", lobe.Yaw);
            lobe.Pitch = Json::DeserializeFloat(pContext, lobeData, "pitch", lobe.Pitch);
            lobe.Roll = Json::DeserializeFloat(pContext, lobeData, "roll", lobe.Roll);
            lobe.SharpnessX = Json::DeserializeFloat(pContext, lobeData, "sharpness_x", lobe.SharpnessX);
            lobe.SharpnessY = Json::DeserializeFloat(pContext, lobeData, "sharpness_y", lobe.SharpnessY);
            lobe.Color = Json::DeserializeVec3(pContext, lobeData, "color", lobe.Color);
            Lobes.push_back(lobe);
        }
    }

    Cores.clear();
    auto coresIt = json.find("cores");
    if (coresIt != json.cend() && coresIt->is_array())
    {
        for (const Json::Data& coreData : *coresIt)
        {
            if (Cores.size() == kMaxCores)
            {
                Log::Warning() << "Skybox has more than " << kMaxCores << " cores; the rest are ignored.";
                break;
            }

            SkyboxCore core;
            core.Yaw = Json::DeserializeFloat(pContext, coreData, "yaw", core.Yaw);
            core.Pitch = Json::DeserializeFloat(pContext, coreData, "pitch", core.Pitch);
            core.Sharpness = Json::DeserializeFloat(pContext, coreData, "sharpness", core.Sharpness);
            core.Color = Json::DeserializeVec3(pContext, coreData, "color", core.Color);
            Cores.push_back(core);
        }
    }

    Generation++;
}

Json::Data SkyboxComponent::Serialize() const
{
    Json::Data lobes = Json::Data::array();
    for (const SkyboxLobe& lobe : Lobes)
    {
        lobes.push_back({
            { "yaw", lobe.Yaw },
            { "pitch", lobe.Pitch },
            { "roll", lobe.Roll },
            { "sharpness_x", lobe.SharpnessX },
            { "sharpness_y", lobe.SharpnessY },
            { "color", ToJson(lobe.Color) } });
    }

    Json::Data cores = Json::Data::array();
    for (const SkyboxCore& core : Cores)
    {
        cores.push_back({
            { "yaw", core.Yaw },
            { "pitch", core.Pitch },
            { "sharpness", core.Sharpness },
            { "color", ToJson(core.Color) } });
    }

    return {
        { "type", "skybox" },
        { "orientation", ToJson(Orientation) },
        { "floor_color_a", ToJson(FloorColorA) },
        { "floor_color_b", ToJson(FloorColorB) },
        { "floor_glow", FloorGlow },
        { "absorb", ToJson(Absorb) },
        { "dust", Dust },
        { "warp_strength", WarpStrength },
        { "noise_scale", NoiseScale },
        { "seed", ToJson(Seed) },
        { "stars", Stars },
        { "resolution", Resolution },
        { "lobes", lobes },
        { "cores", cores }
    };
}

} // namespace WingsOfSteel
