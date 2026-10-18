#include "systems/skybox_baker.hpp"

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/log.hpp>
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
    wgpu::Device device = GetRenderSystem()->GetDevice();
    m_UniformsBuffer = device.CreateBuffer(&bufferDescriptor);

    wgpu::BufferDescriptor partialsBufferDescriptor{
        .label = "Skybox SH partials buffer",
        .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc,
        .size = kSHPartialsSize
    };
    m_SHPartialsBuffer = device.CreateBuffer(&partialsBufferDescriptor);

    // MapRead may only be paired with CopyDst, hence the copy out of the partials buffer.
    m_pReadback = std::make_shared<Readback>();
    wgpu::BufferDescriptor readbackBufferDescriptor{
        .label = "Skybox SH readback buffer",
        .usage = wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopyDst,
        .size = kSHPartialsSize
    };
    m_pReadback->buffer = device.CreateBuffer(&readbackBufferDescriptor);

    GetResourceSystem()->RequestResource("/shaders/skybox_bake.wgsl", [this](ResourceSharedPtr pResource) {
        m_pShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreatePipeline();
    });

    GetResourceSystem()->RequestResource("/shaders/skybox_sh.wgsl", [this](ResourceSharedPtr pResource) {
        m_pSHShader = std::dynamic_pointer_cast<ResourceShader>(pResource);
        CreateSHPipeline();
    });

    m_ShaderInjectionSignalId = GetResourceSystem()->GetShaderInjectedSignal().Connect(
        [this](ResourceShader* pResourceShader) {
            if (pResourceShader == m_pShader.get())
            {
                CreatePipeline();
            }
            else if (pResourceShader == m_pSHShader.get())
            {
                CreateSHPipeline();
            }
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

void SkyboxBaker::CreateSHPipeline()
{
    // No layout given, so it is derived from the shader and the bind group is built against it.
    wgpu::ComputePipelineDescriptor descriptor{
        .label = "Skybox SH pipeline",
        .compute = { .module = m_pSHShader->GetShaderModule(), .entryPoint = "projectMain" }
    };
    m_SHPipeline = GetRenderSystem()->GetDevice().CreateComputePipeline(&descriptor);

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

    wgpu::TextureViewDescriptor arrayViewDescriptor{
        .label = "Skybox cubemap array view",
        .dimension = wgpu::TextureViewDimension::e2DArray,
        .arrayLayerCount = 6
    };
    m_ArrayView = m_Cubemap.CreateView(&arrayViewDescriptor);
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

    Project(encoder);

    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    device.GetQueue().Submit(1, &commandBuffer);

    m_SHPendingReadback = true;
    Update();
}

void SkyboxBaker::Project(wgpu::CommandEncoder& encoder)
{
    // Built every bake, as both the cubemap and the pipeline it is laid out against can be replaced.
    std::array<wgpu::BindGroupEntry, 2> entries = {
        wgpu::BindGroupEntry{
            .binding = 0,
            .textureView = m_ArrayView },
        wgpu::BindGroupEntry{
            .binding = 1,
            .buffer = m_SHPartialsBuffer,
            .size = kSHPartialsSize }
    };
    wgpu::BindGroupDescriptor bindGroupDescriptor{
        .label = "Skybox SH bind group",
        .layout = m_SHPipeline.GetBindGroupLayout(0),
        .entryCount = entries.size(),
        .entries = entries.data()
    };
    wgpu::BindGroup bindGroup = GetRenderSystem()->GetDevice().CreateBindGroup(&bindGroupDescriptor);

    wgpu::ComputePassDescriptor computePassDescriptor{
        .label = "Skybox SH compute pass"
    };
    wgpu::ComputePassEncoder computePass = encoder.BeginComputePass(&computePassDescriptor);
    computePass.SetPipeline(m_SHPipeline);
    computePass.SetBindGroup(0, bindGroup);
    computePass.DispatchWorkgroups(kSHTilesPerSide, kSHTilesPerSide, 6);
    computePass.End();
}

void SkyboxBaker::Update()
{
    // The readback buffer can't be copied into while it is mapped. The partials keep the latest
    // bake meanwhile, so whatever was baked during the wait is read once this one lands.
    if (!m_SHPendingReadback || m_pReadback->inFlight)
    {
        return;
    }

    wgpu::Device device = GetRenderSystem()->GetDevice();
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(m_SHPartialsBuffer, 0, m_pReadback->buffer, 0, kSHPartialsSize);
    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    device.GetQueue().Submit(1, &commandBuffer);

    m_SHPendingReadback = false;
    m_pReadback->inFlight = true;

    std::shared_ptr<Readback> pReadback = m_pReadback;
    pReadback->buffer.MapAsync(
        wgpu::MapMode::Read, 0, kSHPartialsSize, wgpu::CallbackMode::AllowSpontaneous,
        [pReadback](wgpu::MapAsyncStatus status, wgpu::StringView message) {
            // A cancelled callback means the device is going away, so the buffer mustn't be touched.
            if (status == wgpu::MapAsyncStatus::CallbackCancelled)
            {
                return;
            }

            pReadback->inFlight = false;
            if (status != wgpu::MapAsyncStatus::Success)
            {
                Log::Error() << "Skybox SH readback failed: " << std::string_view(message.data, message.length);
                return;
            }

            // Summed in double, as there are a few hundred partials of very different magnitudes.
            const glm::vec4* pPartials = static_cast<const glm::vec4*>(pReadback->buffer.GetConstMappedRange(0, kSHPartialsSize));
            std::array<glm::dvec3, 9> sums{};
            double solidAngle = 0.0;
            for (size_t partial = 0; partial < kSHPartialsSize / (9 * sizeof(glm::vec4)); ++partial)
            {
                for (size_t i = 0; i < 9; ++i)
                {
                    sums[i] += glm::dvec3(pPartials[partial * 9 + i]);
                }
                solidAngle += pPartials[partial * 9].w;
            }
            pReadback->buffer.Unmap();

            // The texel solid angles are approximate, so the sum is normalized to the whole sphere.
            // Each band is then convolved with the cosine lobe and divided by pi: 1, 2/3 and 1/4.
            const double normalization = 4.0 * glm::pi<double>() / solidAngle;
            constexpr std::array<double, 9> kBandScale = { 1.0, 2.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0, 0.25, 0.25, 0.25, 0.25, 0.25 };
            AmbientSH ambientSH;
            for (size_t i = 0; i < 9; ++i)
            {
                ambientSH[i] = glm::vec3(sums[i] * normalization * kBandScale[i]);
            }
            pReadback->ambientSH = ambientSH;
        });
}

} // namespace WingsOfSteel
