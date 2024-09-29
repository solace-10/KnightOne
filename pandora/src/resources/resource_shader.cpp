#include <sstream>

#include "resources/resource_shader.hpp"

#include "core/log.hpp"
#include "render/rendersystem.hpp"
#include "render/shader_compilation_result.hpp"
#include "render/shader_compiler.hpp"
#include "pandora.hpp"

namespace WingsOfSteel::Pandora
{

ResourceShader::ResourceShader()
{
}

ResourceShader::~ResourceShader()
{
}

void ResourceShader::Load(const std::string& path)
{
    Resource::Load(path);

    GetVFS()->FileRead(path,
        [this](FileReadResult result, FileSharedPtr pFile)
        {
            this->LoadInternal(result, pFile);
        }
    );
}

ResourceType ResourceShader::GetResourceType() const
{
    return ResourceType::Shader;
}

wgpu::ShaderModule ResourceShader::GetShaderModule() const
{
    return m_ShaderModule;
}

const std::string& ResourceShader::GetShaderCode() const
{
    return m_ShaderCode;
}

// Rename to Inject() ?
bool ResourceShader::SetShaderCode(const std::string& code)
{
    return false;
}

void ResourceShader::LoadInternal(FileReadResult result, FileSharedPtr pFile)
{
    if (result == FileReadResult::Ok)
    {
        m_ShaderCode = std::string(pFile->GetData().data(), pFile->GetData().size());

        GetRenderSystem()->GetShaderCompiler()->Compile(m_ShaderCode, [this, pFile](ShaderCompilationResult* pCompilationResult)
        {
            if (pCompilationResult->GetState() == ShaderCompilationResult::State::Success)
            {
                m_ShaderModule = pCompilationResult->GetShaderModule();
                SetState(ResourceState::Loaded);
            }
            else
            {
                std::stringstream ss;
                ss << "Failed to compile shader '" << pFile->GetPath() << "':";
                for (const ShaderCompilationError& error : pCompilationResult->GetErrors())
                {
                    uint32_t lineNumebr = error.GetLineNumber();
                    ss << std::endl << "Error on line " << (error.GetLineNumber()) << ", char " << error.GetLinePosition() << ": " << error.GetMessage();
                }
                Log::Error() << ss.str();
                SetState(ResourceState::Error);
            }
        });
    }
    else
    {
        SetState(ResourceState::Error);   
    }
}

} // namespace WingsOfSteel::Pandora
