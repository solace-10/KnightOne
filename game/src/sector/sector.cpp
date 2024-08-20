#include <imgui.h>

#include <scene/components/camera_component.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/orbit_camera_component.hpp>
#include <scene/components/transform_component.hpp>

#include "sector/sector_info.hpp"
#include "sector/sector.hpp"
#include "sector/signal/signal.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Sector::Sector(SectorInfoSharedPtr& pSectorInfo)
: m_pSectorInfo(pSectorInfo)
, m_ShowSignalsDebugUI(false)
{

}

Sector::~Sector()
{

}

void Sector::Initialize()
{
    using namespace Pandora;

    Scene::Initialize();

    AddSystem(std::make_unique<CameraSystem>());
    AddSystem(std::make_unique<DebugRenderSystem>());

    m_pCamera = CreateEntity();
    m_pCamera->AddComponent<CameraComponent>(70.0f, 1.0f, 5000.0f);
    TransformComponent& transformComponent = m_pCamera->AddComponent<TransformComponent>();
    transformComponent.transform = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    OrbitCameraComponent& orbitCameraComponent = m_pCamera->AddComponent<OrbitCameraComponent>();
    orbitCameraComponent.distance = 1000.0f;
    orbitCameraComponent.orbitAngle = glm::radians(-90.0f);
    orbitCameraComponent.pitch = 0.0f;
    orbitCameraComponent.minimumPitch = glm::radians(0.0f);
    orbitCameraComponent.maximumPitch = glm::radians(80.0f);
    SetCamera(m_pCamera); 

    for (auto& pSignal : m_pSectorInfo->GetSignals())
    {
        pSignal->Spawn(this);
    }

    SpawnPlayerShip();

    DrawSignalsDebugUI();
}

void Sector::Update(float delta)
{
    Pandora::Scene::Update(delta);

    DrawSignalsDebugUI();
}

void Sector::ShowSignalsDebugUI(bool state)
{
    m_ShowSignalsDebugUI = state;
}

void Sector::DrawSignalsDebugUI()
{
    if (!m_ShowSignalsDebugUI)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(512, 512));
    ImGui::Begin("Signals", &m_ShowSignalsDebugUI);

    if (ImGui::BeginTable("table", 4, ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Difficulty");
        ImGui::TableSetupColumn("X");
        ImGui::TableSetupColumn("Z");
        ImGui::TableHeadersRow();

        for (auto& pSignal : m_pSectorInfo->GetSignals())
        {
            ImGui::TableNextRow();
            const glm::vec3& pos = pSignal->GetPosition();
            ImGui::TableNextColumn(); ImGui::Text("%s", pSignal->GetName().c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.2f", pSignal->GetSignalDifficulty());
            ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(pos.x));
            ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(pos.z));
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void Sector::SpawnPlayerShip()
{
    using namespace Pandora;

    EntitySharedPtr pPlayerShip = CreateEntity();

    DebugRenderComponent& debugRenderComponent = pPlayerShip->AddComponent<DebugRenderComponent>();
    debugRenderComponent.color = Color::Cyan;
    debugRenderComponent.shape = DebugRenderShape::Box;
    debugRenderComponent.length = 50.0f;
    debugRenderComponent.width = 25.0f;
    debugRenderComponent.height = 10.0f;

    TransformComponent& transformComponent = pPlayerShip->AddComponent<TransformComponent>();
    transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
}

} // namespace WingsOfSteel::TheBrightestStar