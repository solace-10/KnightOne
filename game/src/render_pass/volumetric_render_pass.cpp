#include "render_pass/volumetric_render_pass.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include <FastNoiseLite.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <imgui/imgui.hpp>

#include <pandora.hpp>
#include <render/debug_render.hpp>
#include <render/lighting/lighting_system.hpp>
#include <render/pass/shadow_pass.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

namespace WingsOfSteel
{

namespace
{

    constexpr const char* kMarchShaderPath = "/shaders/volumetric_march.wgsl";
    constexpr const char* kCompositeShaderPath = "/shaders/volumetric_composite.wgsl";
    constexpr uint32_t kNoiseSize = 64;
    constexpr wgpu::TextureFormat kMarchFormat = wgpu::TextureFormat::RGBA16Float;

    // Mirrors Settings in volumetric_march.wgsl.
    struct MarchSettings
    {
        glm::mat4 inverseViewProjection;
        glm::vec3 lightColor;
        float volumeRadius;
        glm::vec3 volumeCenter;
        float extinction;
        glm::vec3 wind;
        float edgeFalloff;
        glm::vec2 noiseScales;
        float clumpiness;
        float anisotropy;
        float scatterIntensity;
        float ambientIntensity;
        float stepCount;
        float jitter;
        float frame;
        float padding[3];
    };
    static_assert(sizeof(MarchSettings) == 160);

    // Mirrors Settings in volumetric_composite.wgsl.
    struct CompositeSettings
    {
        float nearPlane;
        float farPlane;
        uint32_t mode;
    };

} // namespace

VolumetricRenderPass::VolumetricRenderPass()
    : Pass("Volumetric render pass")
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    wgpu::BufferDescriptor marchBufferDescriptor{
        .label = "Volumetric march settings",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(MarchSettings)
    };
    m_MarchSettingsBuffer = device.CreateBuffer(&marchBufferDescriptor);

    wgpu::BufferDescriptor compositeBufferDescriptor{
        .label = "Volumetric composite settings",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(CompositeSettings)
    };
    m_CompositeSettingsBuffer = device.CreateBuffer(&compositeBufferDescriptor);

    std::array<wgpu::BindGroupLayoutEntry, 4> layoutEntries = {
        wgpu::BindGroupLayoutEntry{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Fragment,
            .buffer{
                .type = wgpu::BufferBindingType::Uniform,
                .minBindingSize = sizeof(MarchSettings) } },
        wgpu::BindGroupLayoutEntry{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture{
                .sampleType = wgpu::TextureSampleType::Depth,
                .multisampled = true } },
        wgpu::BindGroupLayoutEntry{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture{
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = wgpu::TextureViewDimension::e3D } },
        wgpu::BindGroupLayoutEntry{
            .binding = 3,
            .visibility = wgpu::ShaderStage::Fragment,
            .sampler{
                .type = wgpu::SamplerBindingType::Filtering } }
    };

    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor{
        .label = "Volumetric march",
        .entryCount = layoutEntries.size(),
        .entries = layoutEntries.data()
    };
    m_MarchBindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDescriptor);

    CreateNoiseTexture();

    GetResourceSystem()->RequestResources({ kMarchShaderPath, kCompositeShaderPath }, [this](const std::unordered_map<std::string, ResourceSharedPtr>& resources) {
        m_pMarchShader = std::dynamic_pointer_cast<ResourceShader>(resources.at(kMarchShaderPath));
        m_pCompositeShader = std::dynamic_pointer_cast<ResourceShader>(resources.at(kCompositeShaderPath));
        CreateRenderPipelines();

        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                if (pResourceShader == m_pMarchShader.get() || pResourceShader == m_pCompositeShader.get())
                {
                    CreateRenderPipelines();
                }
            });
    });
}

VolumetricRenderPass::~VolumetricRenderPass()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

// FastNoiseLite's noise doesn't tile, so the sampler mirrors it instead: the seams are continuous,
// and with two octaves at unrelated scales the mirroring is hard to spot.
void VolumetricRenderPass::CreateNoiseTexture()
{
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    noise.SetFrequency(4.0f / static_cast<float>(kNoiseSize));

    std::vector<float> values;
    values.reserve(kNoiseSize * kNoiseSize * kNoiseSize);
    for (uint32_t z = 0; z < kNoiseSize; z++)
    {
        for (uint32_t y = 0; y < kNoiseSize; y++)
        {
            for (uint32_t x = 0; x < kNoiseSize; x++)
            {
                values.push_back(noise.GetNoise(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
            }
        }
    }

    // Stretched to the full range, which density()'s thresholds expect.
    const auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
    const float minValue = *minIt;
    const float range = std::max(*maxIt - minValue, 1e-6f);
    std::vector<uint8_t> texels(values.size());
    std::transform(values.begin(), values.end(), texels.begin(), [minValue, range](float value) {
        return static_cast<uint8_t>((value - minValue) / range * 255.0f + 0.5f);
    });

    wgpu::Device device = GetRenderSystem()->GetDevice();
    wgpu::TextureDescriptor textureDescriptor{
        .label = "Volumetric noise",
        .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
        .dimension = wgpu::TextureDimension::e3D,
        .size{ .width = kNoiseSize, .height = kNoiseSize, .depthOrArrayLayers = kNoiseSize },
        .format = wgpu::TextureFormat::R8Unorm
    };
    m_NoiseTexture = device.CreateTexture(&textureDescriptor);
    m_NoiseTextureView = m_NoiseTexture.CreateView();

    wgpu::TexelCopyTextureInfo destination{
        .texture = m_NoiseTexture,
        .aspect = wgpu::TextureAspect::All
    };
    wgpu::TexelCopyBufferLayout sourceLayout{
        .offset = 0,
        .bytesPerRow = kNoiseSize,
        .rowsPerImage = kNoiseSize
    };
    wgpu::Extent3D size{ kNoiseSize, kNoiseSize, kNoiseSize };
    device.GetQueue().WriteTexture(&destination, texels.data(), texels.size(), &sourceLayout, &size);

    wgpu::SamplerDescriptor samplerDescriptor{
        .label = "Volumetric noise",
        .addressModeU = wgpu::AddressMode::MirrorRepeat,
        .addressModeV = wgpu::AddressMode::MirrorRepeat,
        .addressModeW = wgpu::AddressMode::MirrorRepeat,
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear
    };
    m_NoiseSampler = device.CreateSampler(&samplerDescriptor);
}

void VolumetricRenderPass::CreateRenderPipelines()
{
    wgpu::Device device = GetRenderSystem()->GetDevice();

    std::array<wgpu::BindGroupLayout, 2> marchBindGroupLayouts = {
        GetRenderSystem()->GetGlobalUniformsLayout(),
        m_MarchBindGroupLayout
    };
    wgpu::PipelineLayoutDescriptor marchPipelineLayoutDescriptor{
        .bindGroupLayoutCount = marchBindGroupLayouts.size(),
        .bindGroupLayouts = marchBindGroupLayouts.data()
    };
    wgpu::PipelineLayout marchPipelineLayout = device.CreatePipelineLayout(&marchPipelineLayoutDescriptor);

    wgpu::ColorTargetState marchTarget{ .format = kMarchFormat };
    wgpu::FragmentState marchFragment{
        .module = m_pMarchShader->GetShaderModule(),
        .entryPoint = "marchMain",
        .targetCount = 1,
        .targets = &marchTarget
    };
    wgpu::RenderPipelineDescriptor marchDescriptor{
        .label = "Volumetric march pipeline",
        .layout = marchPipelineLayout,
        .vertex = {
            .module = m_pMarchShader->GetShaderModule(),
            .entryPoint = "vertexMain" },
        .fragment = &marchFragment
    };
    m_MarchPipeline = device.CreateRenderPipeline(&marchDescriptor);

    // scene * transmittance + scattered, with the march's rgb as the scattered light and its alpha as
    // the transmittance. The scene's alpha is left alone.
    const wgpu::BlendState blendState{
        .color = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::One, .dstFactor = wgpu::BlendFactor::SrcAlpha },
        .alpha = { .operation = wgpu::BlendOperation::Add, .srcFactor = wgpu::BlendFactor::Zero, .dstFactor = wgpu::BlendFactor::One }
    };
    wgpu::ColorTargetState compositeTarget{
        .format = GetWindow()->GetSceneFormat(),
        .blend = &blendState
    };
    wgpu::FragmentState compositeFragment{
        .module = m_pCompositeShader->GetShaderModule(),
        .entryPoint = "compositeMain",
        .targetCount = 1,
        .targets = &compositeTarget
    };
    wgpu::RenderPipelineDescriptor compositeDescriptor{
        .label = "Volumetric composite pipeline",
        .vertex = {
            .module = m_pCompositeShader->GetShaderModule(),
            .entryPoint = "vertexMain" },
        .fragment = &compositeFragment
    };
    m_CompositePipeline = device.CreateRenderPipeline(&compositeDescriptor);
}

void VolumetricRenderPass::UpdateMarchTexture()
{
    const uint32_t divisor = static_cast<uint32_t>(m_ResolutionDivisor);
    const uint32_t width = std::max(1u, (GetWindow()->GetWidth() + divisor - 1) / divisor);
    const uint32_t height = std::max(1u, (GetWindow()->GetHeight() + divisor - 1) / divisor);
    if (m_MarchTexture && m_MarchTexture.GetWidth() == width && m_MarchTexture.GetHeight() == height)
    {
        return;
    }

    wgpu::TextureDescriptor textureDescriptor{
        .label = "Volumetric march",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .size{ .width = width, .height = height },
        .format = kMarchFormat
    };
    m_MarchTexture = GetRenderSystem()->GetDevice().CreateTexture(&textureDescriptor);
    m_MarchTextureView = m_MarchTexture.CreateView();
}

// The directional light's, unless the override is on.
glm::vec3 VolumetricRenderPass::GetLightColor() const
{
    if (m_OverrideColor)
    {
        return m_Color;
    }
    const DirectionalLight& directionalLight = GetRenderSystem()->GetLightingSystem()->GetDirectionalLight();
    return directionalLight.Color * directionalLight.Intensity;
}

// A line from every model, away from the light: the direction of its shadow. Shadows are parallel,
// so in perspective these converge on a vanishing point, as the shafts should.
void VolumetricRenderPass::DrawShadowDirections(float length) const
{
    const glm::vec3& towardsLight = GetRenderSystem()->GetLightingSystem()->GetDirectionalLight().Direction;
    GetActiveScene()->GetRegistry().view<ModelComponent, TransformComponent>().each([&towardsLight, length](const auto entity, ModelComponent& modelComponent, TransformComponent& transformComponent) {
        const glm::vec3 position(transformComponent.transform[3]);
        GetDebugRender()->Line(position, position - towardsLight * length, Color::Yellow);
    });
}

void VolumetricRenderPass::Execute(wgpu::CommandEncoder& encoder)
{
    if (IsDebugUIVisible())
    {
        DrawDebugUI();
    }

    Scene* pScene = GetActiveScene();
    EntitySharedPtr pCamera = pScene ? pScene->GetCamera() : nullptr;
    if (!m_MarchPipeline || !m_CompositePipeline || !pCamera || m_View == View::Off)
    {
        return;
    }

    UpdateMarchTexture();

    Camera& camera = pCamera->GetComponent<CameraComponent>().camera;
    const glm::vec3 volumeCenter = camera.GetTarget();
    const float volumeRadius = m_RadiusFraction * GetRenderSystem()->GetLightingSystem()->GetShadowRadius();

    if (m_ShowVolume)
    {
        GetDebugRender()->Sphere(volumeCenter, Color::Cyan, volumeRadius);
    }
    if (m_ShowShadowDirections)
    {
        DrawShadowDirections(2.0f * volumeRadius);
    }

    const MarchSettings marchSettings{
        .inverseViewProjection = glm::inverse(camera.GetProjectionMatrix() * camera.GetViewMatrix()),
        .lightColor = GetLightColor(),
        .volumeRadius = volumeRadius,
        .volumeCenter = volumeCenter,
        .extinction = m_Extinction,
        .wind = glm::vec3(m_Wind[0], m_Wind[1], m_Wind[2]),
        .edgeFalloff = m_EdgeFalloff,
        .noiseScales = glm::vec2(m_LargeNoiseScale, m_SmallNoiseScale),
        .clumpiness = m_Clumpiness,
        .anisotropy = m_Anisotropy,
        .scatterIntensity = m_ScatterIntensity,
        .ambientIntensity = m_AmbientIntensity,
        .stepCount = static_cast<float>(m_StepCount),
        .jitter = m_Jitter ? 1.0f : 0.0f,
        .frame = m_AnimateJitter ? static_cast<float>(m_Frame++ % 64) : 0.0f
    };
    const CompositeSettings compositeSettings{
        .nearPlane = camera.GetNearPlane(),
        .farPlane = camera.GetFarPlane(),
        .mode = static_cast<uint32_t>(m_View)
    };
    wgpu::Device device = GetRenderSystem()->GetDevice();
    device.GetQueue().WriteBuffer(m_MarchSettingsBuffer, 0, &marchSettings, sizeof(marchSettings));
    device.GetQueue().WriteBuffer(m_CompositeSettingsBuffer, 0, &compositeSettings, sizeof(compositeSettings));

    // Rebuilt every frame, as resizing the window replaces its textures.
    wgpu::TextureView& depthView = GetWindow()->GetDepthTexture().GetTextureView();
    std::array<wgpu::BindGroupEntry, 4> marchEntries = {
        wgpu::BindGroupEntry{ .binding = 0, .buffer = m_MarchSettingsBuffer, .size = sizeof(MarchSettings) },
        wgpu::BindGroupEntry{ .binding = 1, .textureView = depthView },
        wgpu::BindGroupEntry{ .binding = 2, .textureView = m_NoiseTextureView },
        wgpu::BindGroupEntry{ .binding = 3, .sampler = m_NoiseSampler }
    };
    wgpu::BindGroupDescriptor marchBindGroupDescriptor{
        .layout = m_MarchBindGroupLayout,
        .entryCount = marchEntries.size(),
        .entries = marchEntries.data()
    };
    wgpu::BindGroup marchBindGroup = device.CreateBindGroup(&marchBindGroupDescriptor);

    std::array<wgpu::BindGroupEntry, 3> compositeEntries = {
        wgpu::BindGroupEntry{ .binding = 0, .buffer = m_CompositeSettingsBuffer, .size = sizeof(CompositeSettings) },
        wgpu::BindGroupEntry{ .binding = 1, .textureView = m_MarchTextureView },
        wgpu::BindGroupEntry{ .binding = 2, .textureView = depthView }
    };
    wgpu::BindGroupDescriptor compositeBindGroupDescriptor{
        .layout = m_CompositePipeline.GetBindGroupLayout(0),
        .entryCount = compositeEntries.size(),
        .entries = compositeEntries.data()
    };
    wgpu::BindGroup compositeBindGroup = device.CreateBindGroup(&compositeBindGroupDescriptor);

    wgpu::RenderPassColorAttachment marchAttachment{
        .view = m_MarchTextureView,
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 }
    };
    wgpu::RenderPassDescriptor marchPassDescriptor{
        .label = "Volumetric march",
        .colorAttachmentCount = 1,
        .colorAttachments = &marchAttachment
    };
    wgpu::RenderPassEncoder marchPass = encoder.BeginRenderPass(&marchPassDescriptor);
    marchPass.SetPipeline(m_MarchPipeline);
    GetRenderSystem()->BindGlobalUniforms(marchPass);
    marchPass.SetBindGroup(1, marchBindGroup);
    marchPass.Draw(3);
    marchPass.End();

    wgpu::RenderPassColorAttachment compositeAttachment{
        .view = GetWindow()->GetSceneColorTexture().GetTextureView(),
        .loadOp = wgpu::LoadOp::Load,
        .storeOp = wgpu::StoreOp::Store
    };
    wgpu::RenderPassDescriptor compositePassDescriptor{
        .label = "Volumetric composite",
        .colorAttachmentCount = 1,
        .colorAttachments = &compositeAttachment
    };
    wgpu::RenderPassEncoder compositePass = encoder.BeginRenderPass(&compositePassDescriptor);
    compositePass.SetPipeline(m_CompositePipeline);
    compositePass.SetBindGroup(0, compositeBindGroup);
    compositePass.Draw(3);
    compositePass.End();
}

void VolumetricRenderPass::DrawDebugUI()
{
    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_Once);
    if (ImGui::Begin("Volumetrics", &m_ShowDebugUI))
    {
        int view = static_cast<int>(m_View);
        if (ImGui::Combo("View", &view, "Composite\0Scattered light\0Transmittance\0Off\0"))
        {
            m_View = static_cast<View>(view);
        }
        ImGui::Checkbox("Show volume", &m_ShowVolume);
        ImGui::Checkbox("Show shadow directions", &m_ShowShadowDirections);
        ImGui::SetItemTooltip("A line from every model, away from the light. Parallel, so in perspective they converge.");

        ImGui::SeparatorText("March");
        ImGui::SliderInt("Steps per ray", &m_StepCount, 4, 128);
        int resolution = m_ResolutionDivisor == 1 ? 0 : (m_ResolutionDivisor == 2 ? 1 : 2);
        if (ImGui::Combo("Resolution", &resolution, "1/1\0"
                                                    "1/2\0"
                                                    "1/4\0"))
        {
            m_ResolutionDivisor = 1 << resolution;
        }
        ImGui::Checkbox("Jittered start offset", &m_Jitter);
        ImGui::BeginDisabled(!m_Jitter);
        ImGui::Checkbox("Animate jitter", &m_AnimateJitter);
        ImGui::SetItemTooltip("Changes the offsets every frame. Crawls until temporal accumulation averages it.");
        ImGui::EndDisabled();

        ImGui::SeparatorText("Medium");
        ImGui::SliderFloat("Extinction (per m)", &m_Extinction, 0.0f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Clumpiness", &m_Clumpiness, 0.0f, 1.0f);
        ImGui::SliderFloat("Large noise scale", &m_LargeNoiseScale, 0.001f, 0.1f, "%.4f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Small noise scale", &m_SmallNoiseScale, 0.001f, 0.2f, "%.4f", ImGuiSliderFlags_Logarithmic);
        ImGui::DragFloat3("Wind (tiles/s)", m_Wind, 0.0001f, -0.05f, 0.05f, "%.4f");
        ImGui::SliderFloat("Anisotropy (g)", &m_Anisotropy, 0.0f, 0.95f);

        ImGui::SeparatorText("Light");
        ImGui::SliderFloat("Scatter intensity", &m_ScatterIntensity, 0.0f, 64.0f);
        ImGui::SliderFloat("Ambient intensity", &m_AmbientIntensity, 0.0f, 8.0f);

        if (ImGui::Checkbox("Override color", &m_OverrideColor) && m_OverrideColor)
        {
            const DirectionalLight& directionalLight = GetRenderSystem()->GetLightingSystem()->GetDirectionalLight();
            m_Color = directionalLight.Color * directionalLight.Intensity;
        }

        if (m_OverrideColor)
        {
            ImGui::ColorEdit3("Color", &m_Color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
        }

        ImGui::SeparatorText("Volume");
        ImGui::SliderFloat("Radius (of shadow radius)", &m_RadiusFraction, 0.1f, 1.0f);
        ImGui::SliderFloat("Edge falloff (m)", &m_EdgeFalloff, 0.1f, 100.0f);
        ImGui::TextDisabled("Centred on the camera's target. Shadow radius: %.0f m", GetRenderSystem()->GetLightingSystem()->GetShadowRadius());
    }
    ImGui::End();
}

} // namespace WingsOfSteel
