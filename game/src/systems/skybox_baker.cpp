#include "systems/skybox_baker.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <resources/resource_system.hpp>

#include "components/skybox_component.hpp"

namespace WingsOfSteel
{

namespace
{

// Matches the artifact the skybox was prototyped in: yaw 0 looks down -Z and positive yaw turns
// towards +X.
glm::vec3 YawPitchToDirection(float yaw, float pitch)
{
    const float y = glm::radians(yaw);
    const float p = glm::radians(pitch);
    return glm::vec3(glm::cos(p) * glm::sin(y), glm::sin(p), -glm::cos(p) * glm::cos(y));
}

} // namespace

SkyboxBaker::SkyboxBaker()
{
    static_assert(sizeof(BakeUniforms) % 16 == 0);
    static_assert(SkyboxComponent::kMaxLobes == 8 && SkyboxComponent::kMaxCores == 4, "Array sizes are mirrored in BakeUniforms and skybox_bake.wgsl.");

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "Skybox bake uniforms buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(BakeUniforms)
    };
    m_UniformsBuffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);

    GetResourceSystem()->RequestResource("/shaders/skybox_bake.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreatePipeline();

        m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
            [this](ResourceShader* pResourceShader) {
                if (pResourceShader == m_pShader.get())
                {
                    CreatePipeline();
                }
            });
    });
}

SkyboxBaker::~SkyboxBaker()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

void SkyboxBaker::CreatePipeline()
{
    wgpu::ColorTargetState colorTargetState{
        .format = wgpu::TextureFormat::RGBA16Float
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Skybox bake pipeline",
        .vertex = {
            .module = m_pShader->GetShaderModule() },
        .fragment = &fragmentState
    };
    m_Pipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);

    // The pipeline's layout is derived from the shader, so the bind group has to follow it.
    wgpu::BindGroupEntry bindGroupEntry{
        .binding = 0,
        .buffer = m_UniformsBuffer,
        .size = sizeof(BakeUniforms)
    };
    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .layout = m_Pipeline.GetBindGroupLayout(0),
        .entryCount = 1,
        .entries = &bindGroupEntry
    };
    m_BindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);

    m_PipelineGeneration++;
}

void SkyboxBaker::CreateCubemap(uint32_t faceSize)
{
    wgpu::TextureDescriptor textureDescriptor{
        .label = "Skybox cubemap",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = { faceSize, faceSize, 6 },
        .format = wgpu::TextureFormat::RGBA16Float
    };
    m_Cubemap = GetRenderSystem()->GetDevice().CreateTexture(&textureDescriptor);

    wgpu::TextureViewDescriptor cubeViewDescriptor{
        .label = "Skybox cubemap view",
        .dimension = wgpu::TextureViewDimension::Cube,
        .arrayLayerCount = 6
    };
    m_CubeView = m_Cubemap.CreateView(&cubeViewDescriptor);
    m_FaceSize = faceSize;
}

void SkyboxBaker::Bake(const SkyboxComponent& skyboxComponent, SkyboxStage stage, uint32_t faceSize)
{
    if (!IsReady())
    {
        return;
    }

    if (faceSize != m_FaceSize)
    {
        CreateCubemap(faceSize);
    }

    BakeUniforms uniforms{};

    const glm::vec3& orientation = skyboxComponent.Orientation;
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-orientation.x), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(orientation.y), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(orientation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    uniforms.orientation = glm::transpose(rotation);

    const size_t lobeCount = glm::min(skyboxComponent.Lobes.size(), SkyboxComponent::kMaxLobes);
    for (size_t i = 0; i < lobeCount; ++i)
    {
        const SkyboxLobe& lobe = skyboxComponent.Lobes[i];
        const glm::vec3 z = YawPitchToDirection(lobe.Yaw, lobe.Pitch);
        const glm::vec3 up = glm::abs(z.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 t = glm::normalize(glm::cross(up, z));
        const glm::vec3 b = glm::cross(z, t);
        const float c = glm::cos(glm::radians(lobe.Roll));
        const float s = glm::sin(glm::radians(lobe.Roll));

        uniforms.lobeZ[i] = glm::vec4(z, lobe.SharpnessX);
        uniforms.lobeX[i] = glm::vec4(t * c + b * s, lobe.SharpnessY);
        uniforms.lobeY[i] = glm::vec4(-t * s + b * c, 0.0f);
        uniforms.lobeColor[i] = glm::vec4(lobe.Color, 0.0f);
    }

    const size_t coreCount = glm::min(skyboxComponent.Cores.size(), SkyboxComponent::kMaxCores);
    for (size_t i = 0; i < coreCount; ++i)
    {
        const SkyboxCore& core = skyboxComponent.Cores[i];
        uniforms.coreDirection[i] = glm::vec4(YawPitchToDirection(core.Yaw, core.Pitch), core.Sharpness);
        uniforms.coreColor[i] = glm::vec4(core.Color, 0.0f);
    }

    uniforms.floorColorA = glm::vec4(skyboxComponent.FloorColorA, 0.0f);
    uniforms.floorColorB = glm::vec4(skyboxComponent.FloorColorB, 0.0f);
    uniforms.absorb = glm::vec4(skyboxComponent.Absorb, 0.0f);
    uniforms.seed = glm::vec4(skyboxComponent.Seed, 0.0f);
    uniforms.params = glm::vec4(skyboxComponent.WarpStrength, skyboxComponent.NoiseScale, skyboxComponent.Dust, skyboxComponent.FloorGlow);
    uniforms.counts = glm::uvec4(lobeCount, coreCount, static_cast<uint32_t>(stage), faceSize);

    wgpu::Device device = GetRenderSystem()->GetDevice();
    device.GetQueue().WriteBuffer(m_UniformsBuffer, 0, &uniforms, sizeof(BakeUniforms));

    wgpu::CommandEncoderDescriptor encoderDescriptor{
        .label = "Skybox bake encoder"
    };
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDescriptor);

    for (uint32_t face = 0; face < 6; ++face)
    {
        wgpu::TextureViewDescriptor faceViewDescriptor{
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseArrayLayer = face,
            .arrayLayerCount = 1
        };

        wgpu::RenderPassColorAttachment colorAttachment{
            .view = m_Cubemap.CreateView(&faceViewDescriptor),
            .loadOp = wgpu::LoadOp::Clear,
            .storeOp = wgpu::StoreOp::Store
        };

        wgpu::RenderPassDescriptor renderPassDescriptor{
            .label = "Skybox bake render pass",
            .colorAttachmentCount = 1,
            .colorAttachments = &colorAttachment
        };

        // The face goes in as the instance index, which saves a uniform buffer per face.
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPassDescriptor);
        pass.SetPipeline(m_Pipeline);
        pass.SetBindGroup(0, m_BindGroup);
        pass.Draw(3, 1, 0, face);
        pass.End();
    }

    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    device.GetQueue().Submit(1, &commandBuffer);
}

} // namespace WingsOfSteel
