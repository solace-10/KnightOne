#pragma once

#include <optional>
#include <vector>

#include <glm/vec3.hpp>
#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <resources/resource_shader.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

// Draws every projectile with an AmmoTracerComponent as a camera-facing quad stretched along its
// flight path, in a single instanced draw from BaseRenderPass's post-scene callback. Tracers are
// additive, so they need no sorting, and are drawn in HDR so they bloom.
class ProjectileRenderSystem : public System
{
public:
    ProjectileRenderSystem();
    ~ProjectileRenderSystem();

    void Initialize(Scene* pScene) override {}
    void Update(float delta) override {}

    void Render(wgpu::RenderPassEncoder& renderPass);

private:
    void CreateRenderPipeline();

    // Matches InstanceInput in projectile.wgsl.
    struct Instance
    {
        glm::vec3 start;
        float length;
        glm::vec3 direction;
        float width;
        glm::vec3 color;
        float _padding;
    };

    ResourceShaderSharedPtr m_pShader;
    wgpu::RenderPipeline m_RenderPipeline;
    wgpu::Buffer m_InstanceBuffer;
    size_t m_InstanceCapacity{ 0 };
    std::vector<Instance> m_Instances;
    std::optional<SignalId> m_ShaderInjectionSignalId;
};

} // namespace WingsOfSteel
