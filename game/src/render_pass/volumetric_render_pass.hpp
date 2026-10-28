#pragma once

#include <optional>

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <imgui/idebugui.hpp>
#include <render/pass/pass.hpp>
#include <resources/resource_shader.hpp>

namespace WingsOfSteel
{

// Light shafts through a sphere of dust around the camera's target, cut by the directional light's
// shadow map. Ray marched at a fraction of the window's resolution, then upsampled and blended into
// the scene's HDR colour, so it is bloomed and tonemapped with everything else. Must run after
// BaseRenderPass, whose depth it reads.
//
// The debug UI can override the shafts' colour without touching the scene's lighting.
DECLARE_SMART_PTR(VolumetricRenderPass);
class VolumetricRenderPass : public Pass, public IDebugUI
{
public:
    VolumetricRenderPass();
    ~VolumetricRenderPass() override;

    void Execute(wgpu::CommandEncoder& encoder) override;
    void DrawDebugUI() override;

    float GetExtinction() const;
    void SetExtinction(float value);
    float GetClumpiness() const;
    void SetClumpiness(float value);
    float GetAnisotropy() const;
    void SetAnisotropy(float value);
    float GetScatterIntensity() const;
    void SetScatterIntensity(float value);
    float GetAmbientIntensity() const;
    void SetAmbientIntensity(float value);
    bool IsColorOverridden() const;
    void SetColorOverridden(bool state);
    const glm::vec3& GetColor() const;
    void SetColor(const glm::vec3& color);

private:
    enum class View
    {
        Composite,
        Scatter,
        Transmittance,
        Off
    };

    void CreateNoiseTexture();
    void CreateRenderPipelines();
    void UpdateMarchTexture();
    glm::vec3 GetLightColor() const;
    void DrawShadowDirections(float length) const;

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
    wgpu::Texture m_MarchTexture;
    wgpu::TextureView m_MarchTextureView;
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
    bool m_ShowVolume{ false };
    bool m_ShowShadowDirections{ false };
    bool m_OverrideColor{ false };
    glm::vec3 m_Color{ 1.0f };
};

inline float VolumetricRenderPass::GetExtinction() const
{
    return m_Extinction;
}

inline void VolumetricRenderPass::SetExtinction(float value)
{
    m_Extinction = value;
}

inline float VolumetricRenderPass::GetClumpiness() const
{
    return m_Clumpiness;
}

inline void VolumetricRenderPass::SetClumpiness(float value)
{
    m_Clumpiness = value;
}

inline float VolumetricRenderPass::GetAnisotropy() const
{
    return m_Anisotropy;
}

inline void VolumetricRenderPass::SetAnisotropy(float value)
{
    m_Anisotropy = value;
}

inline float VolumetricRenderPass::GetScatterIntensity() const
{
    return m_ScatterIntensity;
}

inline void VolumetricRenderPass::SetScatterIntensity(float value)
{
    m_ScatterIntensity = value;
}

inline float VolumetricRenderPass::GetAmbientIntensity() const
{
    return m_AmbientIntensity;
}

inline void VolumetricRenderPass::SetAmbientIntensity(float value)
{
    m_AmbientIntensity = value;
}

inline bool VolumetricRenderPass::IsColorOverridden() const
{
    return m_OverrideColor;
}

inline void VolumetricRenderPass::SetColorOverridden(bool state)
{
    m_OverrideColor = state;
}

inline const glm::vec3& VolumetricRenderPass::GetColor() const
{
    return m_Color;
}

inline void VolumetricRenderPass::SetColor(const glm::vec3& color)
{
    m_Color = color;
}

} // namespace WingsOfSteel
