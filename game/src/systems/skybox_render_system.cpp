#include "systems/skybox_render_system.hpp"

#include <array>
#include <random>

#include <glm/vec4.hpp>
#include <imgui.h>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/scene.hpp>

#include "components/skybox_component.hpp"

namespace WingsOfSteel
{

namespace
{

// Face size used while a debug UI widget is being dragged, so the sky keeps up with it.
constexpr uint32_t kPreviewResolution = 256;

constexpr std::array<uint32_t, 3> kResolutions = { 512, 1024, 2048 };

} // namespace

SkyboxRenderSystem::SkyboxRenderSystem()
{
    m_pBaker = std::make_unique<SkyboxBaker>();

    wgpu::Device device = GetRenderSystem()->GetDevice();

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Skybox uniforms buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(glm::vec4)
    };
    m_UniformsBuffer = device.CreateBuffer(&bufferDescriptor);

    wgpu::SamplerDescriptor samplerDescriptor{
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear
    };
    m_Sampler = device.CreateSampler(&samplerDescriptor);

    std::array<wgpu::BindGroupLayoutEntry, 3> layoutEntries = {
        wgpu::BindGroupLayoutEntry{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture{
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::Cube } },
        wgpu::BindGroupLayoutEntry{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .sampler{
                .type = wgpu::SamplerBindingType::Filtering } },
        wgpu::BindGroupLayoutEntry{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Fragment,
            .buffer{
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(glm::vec4) } }
    };
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .entryCount = layoutEntries.size(),
        .entries = layoutEntries.data()
    };
    m_BindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    GetResourceSystem()->RequestResource("/shaders/skybox.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateRenderPipeline();

        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                if (pResourceShader == m_pShader.get())
                {
                    CreateRenderPipeline();
                }
            });
    });
}

SkyboxRenderSystem::~SkyboxRenderSystem()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

SkyboxComponent* SkyboxRenderSystem::GetSkyboxComponent() const
{
    if (GetActiveScene() == nullptr)
    {
        return nullptr;
    }

    auto view = GetActiveScene()->GetRegistry().view<SkyboxComponent>();
    for (const auto entity : view)
    {
        return &view.get<SkyboxComponent>(entity);
    }
    return nullptr;
}

void SkyboxRenderSystem::Update(float delta)
{
    DrawDebugUI();

    SkyboxComponent* pSkyboxComponent = GetSkyboxComponent();
    if (!pSkyboxComponent)
    {
        return;
    }

    const glm::vec4 uniforms(pSkyboxComponent->Stars, m_Stage == SkyboxStage::Final ? 1.0f : 0.0f, 0.0f, 0.0f);
    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_UniformsBuffer, 0, &uniforms, sizeof(glm::vec4));

    if (!m_pBaker->IsReady())
    {
        return;
    }

    const bool stale = pSkyboxComponent->Generation != m_BakedGeneration || m_pBaker->GetPipelineGeneration() != m_BakedPipelineGeneration;
    const bool bakePreview = stale && m_IsEditing;
    const bool bakeFull = !bakePreview && (stale || (m_NeedsFullBake && !m_IsEditing));
    if (bakePreview || bakeFull)
    {
        m_pBaker->Bake(*pSkyboxComponent, m_Stage, bakePreview ? kPreviewResolution : pSkyboxComponent->Resolution);
        m_BakedGeneration = pSkyboxComponent->Generation;
        m_BakedPipelineGeneration = m_pBaker->GetPipelineGeneration();
        m_NeedsFullBake = bakePreview;
        CreateBindGroup();
    }
}

void SkyboxRenderSystem::CreateRenderPipeline()
{
    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetSceneFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    std::array<wgpu::BindGroupLayout, 2> bindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_BindGroupLayout
    };
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size()),
        .bindGroupLayouts = bindGroupLayouts.data()
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // Drawn before anything else in the pass, so it needs no depth test and must not write depth.
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = false,
        .depthCompare = wgpu::CompareFunction::Always
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Skybox render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule() },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::None },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };
    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void SkyboxRenderSystem::CreateBindGroup()
{
    std::array<wgpu::BindGroupEntry, 3> entries = {
        wgpu::BindGroupEntry{
            .binding = 0,
            .textureView = m_pBaker->GetCubeView() },
        wgpu::BindGroupEntry{
            .binding = 1,
            .sampler = m_Sampler },
        wgpu::BindGroupEntry{
            .binding = 2,
            .buffer = m_UniformsBuffer,
            .size = sizeof(glm::vec4) }
    };
    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = m_BindGroupLayout,
        .entryCount = entries.size(),
        .entries = entries.data()
    };
    m_BindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);
}

void SkyboxRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (m_RenderPipeline && m_BindGroup)
    {
        renderPass.SetPipeline(m_RenderPipeline);
        renderPass.SetBindGroup(1, m_BindGroup);
        renderPass.Draw(3);
    }
}

void SkyboxRenderSystem::DrawDebugUI()
{
    m_IsEditing = false;

    if (!IsDebugUIVisible())
    {
        return;
    }

    SkyboxComponent* pSkybox = GetSkyboxComponent();
    if (!pSkybox)
    {
        return;
    }

    ImGui::Begin("Skybox", &m_ShowDebugUI, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::PushItemWidth(300);

    const ImGuiColorEditFlags colorFlags = ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float;
    bool changed = false;

    ImGui::SeparatorText("Bake");
    constexpr std::array<const char*, 4> kStageNames = { "Lobes", "Warp", "Dust", "Final" };
    int stage = static_cast<int>(m_Stage);
    if (ImGui::Combo("Stage", &stage, kStageNames.data(), static_cast<int>(kStageNames.size())))
    {
        m_Stage = static_cast<SkyboxStage>(stage);
        changed = true;
    }

    if (ImGui::BeginCombo("Resolution", std::to_string(pSkybox->Resolution).c_str()))
    {
        for (uint32_t resolution : kResolutions)
        {
            if (ImGui::Selectable(std::to_string(resolution).c_str(), resolution == pSkybox->Resolution))
            {
                pSkybox->Resolution = resolution;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("New seed"))
    {
        static std::mt19937 sGenerator{ std::random_device{}() };
        std::uniform_real_distribution<float> distribution(-50.0f, 50.0f);
        pSkybox->Seed = glm::vec3(distribution(sGenerator), distribution(sGenerator), distribution(sGenerator));
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy as JSON"))
    {
        ImGui::SetClipboardText(pSkybox->Serialize().dump(4).c_str());
    }

    const uint32_t faceSize = m_pBaker->GetFaceSize();
    ImGui::Text("Cubemap: 6 x %u², RGBA16F, %.1f MiB", faceSize, 6.0f * faceSize * faceSize * 8.0f / (1024.0f * 1024.0f));

    ImGui::SeparatorText("Composition");
    changed |= ImGui::DragFloat3("Orientation", &pSkybox->Orientation.x, 0.5f, -180.0f, 180.0f, "%.1f");
    changed |= ImGui::DragFloat("Warp strength", &pSkybox->WarpStrength, 0.005f, 0.0f, 0.8f, "%.2f");
    changed |= ImGui::DragFloat("Noise scale", &pSkybox->NoiseScale, 0.01f, 0.5f, 4.0f, "%.2f");

    ImGui::SeparatorText("Dust and light");
    changed |= ImGui::DragFloat("Dust", &pSkybox->Dust, 0.01f, 0.0f, 2.0f, "%.2f");
    changed |= ImGui::ColorEdit3("Absorb", &pSkybox->Absorb.x, colorFlags);
    changed |= ImGui::DragFloat("Floor glow", &pSkybox->FloorGlow, 0.01f, 0.0f, 3.0f, "%.2f");
    changed |= ImGui::ColorEdit3("Floor A", &pSkybox->FloorColorA.x, colorFlags);
    changed |= ImGui::ColorEdit3("Floor B", &pSkybox->FloorColorB.x, colorFlags);

    ImGui::SeparatorText("Stars");
    ImGui::DragFloat("Intensity", &pSkybox->Stars, 0.01f, 0.0f, 2.0f, "%.2f");

    ImGui::SeparatorText("Lobes");
    for (size_t i = 0; i < pSkybox->Lobes.size(); ++i)
    {
        SkyboxLobe& lobe = pSkybox->Lobes[i];
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::TreeNode("Lobe", "Lobe %zu", i + 1))
        {
            changed |= ImGui::DragFloat("Yaw", &lobe.Yaw, 0.5f, -180.0f, 360.0f, "%.1f");
            changed |= ImGui::DragFloat("Pitch", &lobe.Pitch, 0.5f, -90.0f, 90.0f, "%.1f");
            changed |= ImGui::DragFloat("Roll", &lobe.Roll, 0.5f, -180.0f, 180.0f, "%.1f");
            changed |= ImGui::DragFloat("Sharpness X", &lobe.SharpnessX, 0.05f, 0.1f, 50.0f, "%.2f");
            changed |= ImGui::DragFloat("Sharpness Y", &lobe.SharpnessY, 0.05f, 0.1f, 50.0f, "%.2f");
            changed |= ImGui::ColorEdit3("Color", &lobe.Color.x, colorFlags);
            if (ImGui::Button("Remove"))
            {
                pSkybox->Lobes.erase(pSkybox->Lobes.begin() + i);
                changed = true;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (pSkybox->Lobes.size() < SkyboxComponent::kMaxLobes && ImGui::Button("Add lobe"))
    {
        pSkybox->Lobes.emplace_back();
        changed = true;
    }

    ImGui::SeparatorText("Cores");
    for (size_t i = 0; i < pSkybox->Cores.size(); ++i)
    {
        SkyboxCore& core = pSkybox->Cores[i];
        ImGui::PushID(static_cast<int>(SkyboxComponent::kMaxLobes + i));
        if (ImGui::TreeNode("Core", "Core %zu", i + 1))
        {
            changed |= ImGui::DragFloat("Yaw", &core.Yaw, 0.5f, -180.0f, 360.0f, "%.1f");
            changed |= ImGui::DragFloat("Pitch", &core.Pitch, 0.5f, -90.0f, 90.0f, "%.1f");
            changed |= ImGui::DragFloat("Sharpness", &core.Sharpness, 10.0f, 100.0f, 10000.0f, "%.0f");
            changed |= ImGui::ColorEdit3("Color", &core.Color.x, colorFlags);
            if (ImGui::Button("Remove"))
            {
                pSkybox->Cores.erase(pSkybox->Cores.begin() + i);
                changed = true;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (pSkybox->Cores.size() < SkyboxComponent::kMaxCores && ImGui::Button("Add core"))
    {
        pSkybox->Cores.emplace_back();
        changed = true;
    }

    m_IsEditing = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsAnyItemActive();

    ImGui::PopItemWidth();
    ImGui::End();

    if (changed)
    {
        pSkybox->Generation++;
    }
}

} // namespace WingsOfSteel
