#pragma once

#include <optional>

#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <imgui/idebugui.hpp>
#include <resources/resource_shader.hpp>
#include <scene/systems/system.hpp>

#include "systems/skybox_baker.hpp"

namespace WingsOfSteel
{

class SkyboxComponent;

// Bakes the scene's SkyboxComponent into a cubemap whenever it changes, and draws it behind
// everything else from BaseRenderPass's pre-scene callback. The cubemap's SH become the ambient light.
class SkyboxRenderSystem : public System, public IDebugUI
{
public:
    SkyboxRenderSystem();
    ~SkyboxRenderSystem();

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override;

    void Render(wgpu::RenderPassEncoder& renderPass);

    void DrawDebugUI() override;

private:
    void CreateRenderPipeline();
    void CreateBindGroup();
    SkyboxComponent* GetSkyboxComponent() const;

    SkyboxBakerUniquePtr m_pBaker;
    ResourceShaderSharedPtr m_pShader;
    wgpu::RenderPipeline m_RenderPipeline;
    wgpu::BindGroupLayout m_BindGroupLayout;
    wgpu::BindGroup m_BindGroup;
    wgpu::Buffer m_UniformsBuffer;
    wgpu::Sampler m_Sampler;
    std::optional<SignalId> m_ShaderInjectionSignalId;

    SkyboxStage m_Stage{ SkyboxStage::Final };
    uint32_t m_BakedGeneration{ 0 };
    uint32_t m_BakedPipelineGeneration{ 0 };
    bool m_IsEditing{ false }; // A debug UI widget is being dragged, so bakes are kept small.
    bool m_NeedsFullBake{ false };
};

} // namespace WingsOfSteel
