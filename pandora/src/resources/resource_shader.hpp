#pragma once

#include "resources/resource.hpp"

#include <webgpu/webgpu_cpp.h>

namespace WingsOfSteel::Pandora
{

class ResourceShader : public Resource
{
public:
    ResourceShader();
    ~ResourceShader() override;

    void Load(const std::string& path) override;
    ResourceType GetResourceType() const override;

    wgpu::ShaderModule GetShaderModule() const;

    const std::string& GetShaderCode() const;
    bool SetShaderCode(const std::string& code);

private:
    void LoadInternal(FileReadResult result, FileSharedPtr pFile);

    wgpu::ShaderModule m_ShaderModule;

    std::string m_ShaderCode;
};

} // namespace WingsOfSteel::Pandora