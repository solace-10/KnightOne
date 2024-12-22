#include <sstream>

#include <magic_enum.hpp>

#include <core/log.hpp>
#include <imgui/imgui_system.hpp>
#include <pandora.hpp>

#include "buffered_texture_2d.hpp"
#include "dome.hpp"
#include "texture_processor.hpp"

namespace WingsOfSteel::Dome
{

Dome::Dome()
{

}

Dome::~Dome()
{

}

void Dome::Initialize()
{
    using namespace Pandora;

    Log::Info() << "Initializing Dome...";
    GetImGuiSystem()->SetGameMenuBarCallback([this](){ DrawImGuiMenuBar(); });

    m_pTextureProcessor = std::make_unique<TextureProcessor>();

    m_pSourceTexture = std::make_unique<BufferedTexture2D>("Dome source texture");
    if (!m_pSourceTexture->Load("data/core/source/nebula_1.png"))
    {
        Log::Error() << "Failed to load texture.";
    }
    else
    {
        m_pGreyscaleTexture = m_pTextureProcessor->GetGreyscale(m_pSourceTexture.get());
        //m_EdgePoints = GetEdgePoints(m_Threshold);
    }
}

void Dome::Update(float delta)
{
    using namespace ImGui;
    const float menubarSize = 24.0f;
    SetNextWindowPos(ImVec2(0, menubarSize));

    ImVec2 size = GetIO().DisplaySize;
    size.y -= menubarSize;

    SetNextWindowSize(size, ImGuiCond_Always);
    int flags = 0;
    flags |= ImGuiWindowFlags_AlwaysAutoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoTitleBar;
    Begin("Dome", nullptr, flags);



    // Left
    static int selectedTextureIndex = 0; // Here we store our selection data as an index.
    {
        ImGui::BeginChild("left pane", ImVec2(300, 0), ImGuiChildFlags_Border);

        const char* items[] = { "Original", "Greyscale" };

        // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
        const char* combo_preview_value = items[selectedTextureIndex];

        if (ImGui::BeginCombo("Texture", combo_preview_value, 0))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (selectedTextureIndex == n);
                if (ImGui::Selectable(items[n], is_selected))
                {
                    selectedTextureIndex = n;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::EndChild();
    }
    ImGui::SameLine();

    // Right
    {
        ImGui::BeginChild("item view", ImVec2(1024, 1024), 0, ImGuiWindowFlags_NoScrollbar); // Leave room for 1 line below us

        BufferedTexture2D* pSelectedTexture = nullptr;
        switch (selectedTextureIndex)
        {
            case 0: pSelectedTexture = m_pSourceTexture.get(); break;
            case 1: pSelectedTexture = m_pGreyscaleTexture.get(); break;
        };

        if (pSelectedTexture)
        {
            ImTextureID textureId = pSelectedTexture->GetTextureView().Get();
            const float textureWidth = 1024.0f;
            const float textureHeight = 1024.0f;
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 uvMin = ImVec2(0.0f, 0.0f);
            ImVec2 uvMax = ImVec2(1.0f, 1.0f);
            ImVec2 c = ImGui::GetCursorScreenPos();
            ImGui::Image(textureId, ImVec2(textureWidth, textureHeight), uvMin, uvMax);

            // ImDrawList* pDrawList = ImGui::GetWindowDrawList();
            // ImU32 pointColor = ImGui::GetColorU32(IM_COL32(0, 255, 0, 255));
            // int pointRadius = 1;

            // for (auto& point : m_EdgePoints)
            // {
            //     ImVec2 p1 = ImVec2(c.x + point.x - pointRadius, c.y + point.y - pointRadius);
            //     ImVec2 p2 = ImVec2(c.x + point.x + pointRadius, c.y + point.y - pointRadius);
            //     ImVec2 p3 = ImVec2(c.x + point.x + pointRadius, c.y + point.y + pointRadius);
            //     ImVec2 p4 = ImVec2(c.x + point.x - pointRadius, c.y + point.y + pointRadius);
            //     pDrawList->AddQuadFilled(p1, p2, p3, p4, pointColor);
            // }
        }

        ImGui::EndChild();
    }

    End();
}

void Dome::Shutdown()
{

}

// Called from ImGuiSystem::Update() to draw any menus in the menu bar.
void Dome::DrawImGuiMenuBar()
{

}

} // namespace WingsOfSteel::Dome