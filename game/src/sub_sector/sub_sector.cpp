#include <imgui.h>

#include <render/debug_render.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/transform_component.hpp>
#include <pandora.hpp>

#include "components/player_controller_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "components/sub_sector_camera_component.hpp"
#include "sector/sector_info.hpp"
#include "sub_sector/encounters/encounter_window.hpp"
#include "sub_sector/sub_sector.hpp"
#include "systems/camera_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/player_controller_system.hpp"
#include "systems/ship_navigation_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

SubSector::SubSector()
{

}

SubSector::~SubSector()
{

}

void SubSector::Initialize()
{
    using namespace Pandora;

    Scene::Initialize();

    AddSystem(std::make_unique<PlayerControllerSystem>());
    AddSystem(std::make_unique<ShipNavigationSystem>());

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem(std::make_unique<CameraSystem>());
    AddSystem(std::make_unique<DebugRenderSystem>());

    m_pCamera = CreateEntity();
    m_pCamera->AddComponent<CameraComponent>(70.0f, 1.0f, 5000.0f);

    SubSectorCameraComponent& subSectorCameraComponent = m_pCamera->AddComponent<SubSectorCameraComponent>();
    subSectorCameraComponent.position = glm::vec3(0.0f, 200.0f, -75.0f);
    subSectorCameraComponent.target = glm::vec3(0.0f, 0.0f, 0.0f);
    subSectorCameraComponent.maximumDrift = glm::vec3(0.0f, 0.0f, 0.0f);
    SetCamera(m_pCamera); 

    SpawnDome();
    SpawnPlayerShip();
    subSectorCameraComponent.anchorEntity = m_pPlayerShip;

    m_pEncounterWindow = std::make_unique<EncounterWindow>();
    m_pEncounterWindow->Initialize();
    m_pEncounterWindow->LoadPrefab("/ui/prefabs/encounter.json");
}

void SubSector::Update(float delta)
{
    Pandora::Scene::Update(delta);

    Pandora::GetDebugRender()->Line(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1000.0f),
        Pandora::Color::White
    );

    Pandora::GetDebugRender()->Line(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1000.0f, 0.0f),
        Pandora::Color::Red
    );

    Pandora::GetDebugRender()->XZSquareGrid(-1000.0f, 1000.0f, 0.0f, 100.0f, Pandora::Color::White);

    if (m_pEncounterWindow)
    {
        m_pEncounterWindow->Render();
    }

    DrawCameraDebugUI();
}

void SubSector::ShowCameraDebugUI(bool state)
{
    m_ShowCameraDebugUI = state;
}

void SubSector::DrawCameraDebugUI()
{
    if (!m_ShowCameraDebugUI)
    {
        return;
    }

    ImGui::Begin("Camera", &m_ShowCameraDebugUI);

    SubSectorCameraComponent& subSectorCameraComponent = m_pCamera->GetComponent<SubSectorCameraComponent>();

    ImGui::Checkbox("Debug Draw", &subSectorCameraComponent.debugDraw);

    const glm::vec3& position = subSectorCameraComponent.position;
    float fposition[3] = { position.x, position.y, position.z };
    if (ImGui::InputFloat3("Eye", fposition))
    {
        subSectorCameraComponent.position = glm::vec3(fposition[0], fposition[1], fposition[2]);
    }

    const glm::vec3& drift = subSectorCameraComponent.maximumDrift;
    float fdrift[3] = { drift.x, drift.y, drift.z };
    if (ImGui::InputFloat3("Drift", fdrift))
    {
        subSectorCameraComponent.maximumDrift = glm::vec3(fdrift[0], fdrift[1], fdrift[2]);
    }

    ImGui::End();
}

void SubSector::SpawnDome()
{
    using namespace Pandora;

    m_pDome = CreateEntity();

    SubSectorCameraComponent& subSectorCameraComponent = m_pCamera->GetComponent<SubSectorCameraComponent>();

    TransformComponent& transformComponent = m_pDome->AddComponent<TransformComponent>();
    transformComponent.transform = glm::scale(glm::translate(glm::mat4(1.0f), subSectorCameraComponent.position + glm::vec3(0.0f, 0.0f, 100.0f)), glm::vec3(20.0f));

    m_pDome->AddComponent<ModelComponent>("/models/dome/dome.glb");
}

void SubSector::SpawnPlayerShip()
{
    using namespace Pandora;

    m_pPlayerShip = CreateEntity();

    // DebugRenderComponent& debugRenderComponent = m_pPlayerShip->AddComponent<DebugRenderComponent>();
    // debugRenderComponent.color = Color::Cyan;
    // debugRenderComponent.shape = DebugRenderShape::Box;
    // debugRenderComponent.length = 50.0f;
    // debugRenderComponent.width = 25.0f;
    // debugRenderComponent.height = 10.0f;

    TransformComponent& transformComponent = m_pPlayerShip->AddComponent<TransformComponent>();
    transformComponent.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    m_pPlayerShip->AddComponent<ModelComponent>("/models/flagship/light_carrier/light_carrier.glb");
    m_pPlayerShip->AddComponent<ShipNavigationComponent>();
    m_pPlayerShip->AddComponent<PlayerControllerComponent>();
}

} // namespace WingsOfSteel::TheBrightestStar