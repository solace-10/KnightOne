#include <vector>

#include <render/rendersystem.hpp>
#include <render/window.hpp>
#include <resources/resource_shader.hpp>
#include <resources/resource_system.hpp>
#include <pandora.hpp>

#include "test_entity.hpp"

namespace WingsOfSteel::TheBrightestStar
{

TestEntity::TestEntity()
{

}

TestEntity::~TestEntity()
{

}

void TestEntity::OnAddedToScene(Pandora::Scene* pScene)
{
    Pandora::Entity::OnAddedToScene(pScene);

    static std::vector<float> sVertexData =
    {
        -0.5, -0.5, 0.0,
        +0.5, -0.5, 0.0,
        +0.0, +0.5, 0.0
    };

    wgpu::BufferDescriptor bufferDescriptor{
        .label = "TestEntity vertex buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
        .size = sVertexData.size() * sizeof(float)
    };
    wgpu::Device pDevice = Pandora::GetRenderSystem()->GetDevice();
    m_VertexBuffer = pDevice.CreateBuffer(&bufferDescriptor);
    pDevice.GetQueue().WriteBuffer(m_VertexBuffer, 0, sVertexData.data(), bufferDescriptor.size);

    Pandora::GetResourceSystem()->RequestResource("/shaders/diffuse.wgsl", [this](Pandora::ResourceSharedPtr pResource) {
        this->m_pShader = std::dynamic_pointer_cast<Pandora::ResourceShader>(pResource);
        this->CreateRenderPipeline();
    });
}

void TestEntity::Update(float delta) 
{
    Pandora::Entity::Update(delta);
}

void TestEntity::Render(wgpu::RenderPassEncoder renderPass)
{
    Pandora::Entity::Render(renderPass);

    if (m_RenderPipeline)
    {
        renderPass.SetPipeline(m_RenderPipeline);
        renderPass.SetVertexBuffer(0, m_VertexBuffer, 0, m_VertexBuffer.GetSize());
        renderPass.Draw(3);
    }
}

void TestEntity::CreateRenderPipeline()
{
    wgpu::ColorTargetState colorTargetState{
        .format = Pandora::GetWindow()->GetTextureFormat()
    };

    wgpu::FragmentState fragmentState{
        .module = m_pShader->GetShaderModule(),
        .targetCount = 1,
        .targets = &colorTargetState
    };

    wgpu::VertexAttribute posititionAttribute{
        .format = wgpu::VertexFormat::Float32x3,
        .offset = 0,
        .shaderLocation = 0
    };

    wgpu::VertexBufferLayout vertexBufferLayout{
        .arrayStride = 3 * sizeof(float),
        .stepMode = wgpu::VertexStepMode::Vertex,
        .attributeCount = 1,
        .attributes = &posititionAttribute
    };

    wgpu::RenderPipelineDescriptor descriptor{
        .vertex = {
            .module = m_pShader->GetShaderModule(),
            .bufferCount = 1,
            .buffers = &vertexBufferLayout
        },
        .fragment = &fragmentState};
    m_RenderPipeline = Pandora::GetRenderSystem()->GetDevice().CreateRenderPipeline(&descriptor);
}

} // namespace WingsOfSteel::TheBrightestStar