#pragma once

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>
#include <resources/resource_texture_2d.hpp>

#include "ui/stackable_element.hpp"
#include "ui/prefab_data.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Image);
class Image : public StackableElement
{
public:
    Image() {}
    ~Image() override {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;

    void Deserialize(const nlohmann::json& data) override;

    void SetSource(const std::string& source);

private:
    Pandora::ResourceTexture2DSharedPtr m_pTexture;
    Property<std::string> m_Source;
};

} // namespace WingsOfSteel::TheBrightestStar::UI