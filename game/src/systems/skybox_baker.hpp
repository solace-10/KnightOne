#pragma once

#include <optional>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <webgpu/webgpu_cpp.h>

#include <core/signal.hpp>
#include <core/smart_ptr.hpp>
#include <resources/resource_shader.hpp>

namespace WingsOfSteel
{

class SkyboxComponent;

// How much of the sky to build, for inspecting it a step at a time.
enum class SkyboxStage
{
    Lobes, // The raw lobes over the floor glow; this is the whole composition.
    Warp, // The lobes evaluated along a domain-warped direction, which smears their edges into flow.
    Dust, // Bright cores, then dust lanes that absorb blue more strongly than red.
    Final // As Dust, plus the stars, which are drawn every frame rather than baked.
};

// Renders a SkyboxComponent's nebula into an HDR cubemap.
DECLARE_SMART_PTR(SkyboxBaker);
class SkyboxBaker
{
public:
    SkyboxBaker();
    ~SkyboxBaker();

    bool IsReady() const { return m_Pipeline != nullptr; }

    // Bumped whenever the bake shader is reloaded, as the cubemap is then stale.
    uint32_t GetPipelineGeneration() const { return m_PipelineGeneration; }

    // Recreates the cubemap if the face size changed, so any view taken of it before is stale.
    void Bake(const SkyboxComponent& skyboxComponent, SkyboxStage stage, uint32_t faceSize);

    wgpu::TextureView GetCubeView() const { return m_CubeView; }
    uint32_t GetFaceSize() const { return m_FaceSize; }

private:
    void CreatePipeline();
    void CreateCubemap(uint32_t faceSize);

    // Mirrored in skybox_bake.wgsl; everything is packed into vec4s to sidestep alignment pitfalls.
    struct BakeUniforms
    {
        glm::mat4 orientation; // From world space into the space the composition is authored in.
        glm::vec4 lobeZ[8]; // xyz: lobe axis, w: sharpness across.
        glm::vec4 lobeX[8]; // xyz: lobe tangent, w: sharpness along.
        glm::vec4 lobeY[8]; // xyz: lobe bitangent.
        glm::vec4 lobeColor[8];
        glm::vec4 coreDirection[4]; // xyz: direction, w: sharpness.
        glm::vec4 coreColor[4];
        glm::vec4 floorColorA;
        glm::vec4 floorColorB;
        glm::vec4 absorb;
        glm::vec4 seed;
        glm::vec4 params; // x: warp strength, y: noise scale, z: dust, w: floor glow.
        glm::uvec4 counts; // x: lobe count, y: core count, z: stage, w: face size.
    };

    ResourceShaderSharedPtr m_pShader;
    wgpu::RenderPipeline m_Pipeline;
    wgpu::Buffer m_UniformsBuffer;
    wgpu::BindGroup m_BindGroup;
    wgpu::Texture m_Cubemap;
    wgpu::TextureView m_CubeView;
    uint32_t m_FaceSize{ 0 };
    uint32_t m_PipelineGeneration{ 0 };
    std::optional<SignalId> m_ShaderInjectionSignalId;
};

} // namespace WingsOfSteel
