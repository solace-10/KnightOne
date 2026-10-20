#pragma once

#include <optional>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <imgui/idebugui.hpp>
#include <render/pass/pass.hpp>
#include <render/pass/shadow_pass.hpp>
#include <resources/resource_shader.hpp>

namespace WingsOfSteel
{

// Light shafts through a sphere of dust around the camera's target, cut by the directional light's
// shadow map. Ray marched at a fraction of the window's resolution, then upsampled and blended into
// the scene's HDR colour, so it is bloomed and tonemapped with everything else. Must run after
// BaseRenderPass, whose depth it reads.
//
// The debug UI can override the light the shafts come from without touching the scene's lighting.
// The shafts' shadows must then come from that direction too, so the override renders its own
// shadow map.
DECLARE_SMART_PTR(VolumetricRenderPass);
class VolumetricRenderPass : public Pass, public IDebugUI
{
public:
    VolumetricRenderPass();
    ~VolumetricRenderPass() override;

    void Execute(wgpu::CommandEncoder& encoder) override;
    void DrawDebugUI() override;

private:
    enum class View
    {
        Composite,
        Scatter,
        Transmittance,
        Off
    };

    // The light the shafts come from: the directional light's, or the override's.
    struct Light
    {
        glm::vec3 towardsLight;
        glm::vec3 color;
        glm::mat4 shadowViewProjection;
        wgpu::TextureView shadowMap;
    };

    void CreateNoiseTexture();
    void CreateRenderPipelines();
    void UpdateMarchTexture();
    void UpdateOverrideShadowMap();
    glm::vec3 GetOverrideDirection() const;
    Light GetLight(const glm::vec3& volumeCenter, float volumeRadius) const;
    ShadowPass* GetShadowPass() const;
    void DrawShadowDirections(const glm::vec3& towardsLight, float length) const;
    void StartOverride();
    void ApplyOverrideToLight() const;

    ResourceShaderSharedPtr m_pMarchShader;
    ResourceShaderSharedPtr m_pCompositeShader;
    std::optional<SignalId> m_ShaderInjectionSignalId;
    wgpu::RenderPipeline m_MarchPipeline;
    wgpu::RenderPipeline m_CompositePipeline;
    wgpu::BindGroupLayout m_MarchBindGroupLayout;
    wgpu::Buffer m_MarchSettingsBuffer;
    wgpu::Buffer m_CompositeSettingsBuffer;
    wgpu::Texture m_NoiseTexture;
    wgpu::TextureView m_NoiseTextureView;
    wgpu::Sampler m_NoiseSampler;
    wgpu::Sampler m_ShadowSampler;
    wgpu::Texture m_MarchTexture;
    wgpu::TextureView m_MarchTextureView;
    wgpu::Texture m_OverrideShadowMap;
    wgpu::TextureView m_OverrideShadowMapView;
    ShadowPassUniquePtr m_pOverrideShadowPass;
    uint32_t m_Frame{ 0 };

    View m_View{ View::Composite };
    int m_StepCount{ 32 };
    int m_ResolutionDivisor{ 2 };
    bool m_Jitter{ true };
    bool m_AnimateJitter{ false }; // Only settles into a smooth result once temporal accumulation averages it.
    float m_Extinction{ 0.012f };
    float m_Clumpiness{ 0.55f };
    float m_LargeNoiseScale{ 0.012f };
    float m_SmallNoiseScale{ 0.045f };
    float m_Wind[3]{ 0.004f, 0.0012f, -0.003f };
    float m_Anisotropy{ 0.7f };
    float m_ScatterIntensity{ 8.0f };
    float m_AmbientIntensity{ 0.05f };
    float m_RadiusFraction{ 1.0f }; // Of the shadow map's radius, beyond which there are no shadows to cut shafts.
    float m_EdgeFalloff{ 14.0f };
    int m_CelBands{ 0 };
    bool m_ShowVolume{ false };
    bool m_ShowShadowDirections{ false };
    bool m_ShowShadowMap{ false };

    bool m_OverrideLight{ false };
    float m_OverrideAngle{ 0.0f }; // As DirectionalLightComponent's, in degrees.
    float m_OverridePitch{ 0.0f };
    glm::vec3 m_OverrideColor{ 1.0f };
    int m_OverrideShadowMapSize{ 1024 };
};

} // namespace WingsOfSteel
