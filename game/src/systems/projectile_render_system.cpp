#include "systems/projectile_render_system.hpp"

#include <array>

#include <pandora.hpp>
#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/scene.hpp>

#include "components/ammo_raycast_component.hpp"
#include "components/ammo_tracer_component.hpp"

namespace WingsOfSteel
{

ProjectileRenderSystem::ProjectileRenderSystem()
{
    GetResourceSystem()->RequestResource("/shaders/projectile.wgsl", [this](ResourceSharedPtr pResource) {
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

ProjectileRenderSystem::~ProjectileRenderSystem()
{
    if (GetResourceSystem() && m_ShaderInjectionSignalId.has_value())
    {
        GetResourceSystem()->GetShaderInjectedSignal().Disconnect(m_ShaderInjectionSignalId.value());
    }
}

void ProjectileRenderSystem::CreateRenderPipeline()
{
    // Additive, so tracers can be drawn in any order.
    wgpu::BlendState blendState{
        .color{
            .srcFactor = wgpu::BlendFactor::One,
            .dstFactor = wgpu::BlendFactor::One },
        .alpha{
            .srcFactor = wgpu::BlendFactor::Zero,
            .dstFactor = wgpu::BlendFactor::One }
    };
    wgpu::ColorTargetState colorTargetState{
        .format = GetWindow()->GetSceneFormat(),
        .blend = &blendState
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    wgpu::BindGroupLayout globalUniformsLayout = GetRenderSystem()->GetGlobalUniformsLayout();
    wgpu::PipelineLayoutDescriptor pipelineLayoutDescriptor{
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &globalUniformsLayout
    };
    wgpu::PipelineLayout pipelineLayout = GetRenderSystem()->GetDevice().CreatePipelineLayout(&pipelineLayoutDescriptor);

    // Hidden by the scene, but translucent, so tracers don't hide each other.
    wgpu::DepthStencilState depthState{
        .format = wgpu::TextureFormat::Depth32Float,
        .depthWriteEnabled = false,
        .depthCompare = wgpu::CompareFunction::Less
    };

    std::array<wgpu::VertexAttribute, 3> attributes = {
        wgpu::VertexAttribute{ .format = wgpu::VertexFormat::Float32x4, .offset = offsetof(Instance, start), .shaderLocation = 0 },
        wgpu::VertexAttribute{ .format = wgpu::VertexFormat::Float32x4, .offset = offsetof(Instance, direction), .shaderLocation = 1 },
        wgpu::VertexAttribute{ .format = wgpu::VertexFormat::Float32x4, .offset = offsetof(Instance, color), .shaderLocation = 2 }
    };
    wgpu::VertexBufferLayout instanceLayout{
        .stepMode = wgpu::VertexStepMode::Instance,
        .arrayStride = sizeof(Instance),
        .attributeCount = attributes.size(),
        .attributes = attributes.data()
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .label = "Projectile render pipeline",
        .layout = pipelineLayout,
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = &instanceLayout },
        .primitive = { .topology = wgpu::PrimitiveTopology::TriangleList, .cullMode = wgpu::CullMode::None },
        .depthStencil = &depthState,
        .multisample = { .count = RenderSystem::MsaaSampleCount },
        .fragment = &fragmentState
    };
    m_RenderPipeline = GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

void ProjectileRenderSystem::Render(wgpu::RenderPassEncoder& renderPass)
{
    if (!m_RenderPipeline || !GetActiveScene())
    {
        return;
    }

    m_Instances.clear();
    auto view = GetActiveScene()->GetRegistry().view<const TransformComponent, const AmmoRaycastComponent, const AmmoTracerComponent>();
    view.each([this](const TransformComponent& transformComponent, const AmmoRaycastComponent& ammoRaycastComponent, const AmmoTracerComponent& ammoTracerComponent) {
        m_Instances.push_back({ .start = transformComponent.GetTranslation(),
            .length = ammoRaycastComponent.GetRaycastLength(),
            .direction = transformComponent.GetForward(),
            .width = ammoTracerComponent.Width,
            .color = ammoTracerComponent.Color });
    });

    if (m_Instances.empty())
    {
        return;
    }

    if (m_Instances.size() > m_InstanceCapacity)
    {
        m_InstanceCapacity = std::max<size_t>(m_Instances.size() * 2, 256);
        wgpu::BufferDescriptor bufferDescriptor{
            .label = "Projectile instance buffer",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
            .size = m_InstanceCapacity * sizeof(Instance)
        };
        m_InstanceBuffer = GetRenderSystem()->GetDevice().CreateBuffer(&bufferDescriptor);
    }

    GetRenderSystem()->GetDevice().GetQueue().WriteBuffer(m_InstanceBuffer, 0, m_Instances.data(), m_Instances.size() * sizeof(Instance));

    renderPass.SetPipeline(m_RenderPipeline);
    renderPass.SetVertexBuffer(0, m_InstanceBuffer, 0, m_Instances.size() * sizeof(Instance));
    renderPass.Draw(6, static_cast<uint32_t>(m_Instances.size()));
}

} // namespace WingsOfSteel
