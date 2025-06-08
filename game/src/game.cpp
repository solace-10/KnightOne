#include <imgui/imgui_system.hpp>
#include <pandora.hpp>
#include <scene/camera.hpp>
#include <scene/entity.hpp>
#include <scene/scene.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "game.hpp"
#include "hyperscape/hyperscape.hpp"
#include "items/item_info.hpp"
#include "items/item_manager.hpp"
#include "sector/sector.hpp"
#include "sector/sector_signal.hpp"
#include "ui/prefab_editor.hpp"

namespace WingsOfSteel::TheBrightestStar
{

Game* g_pGame = nullptr;

Game::Game()
{
}

Game::~Game()
{
}

Game* Game::Get()
{
    return g_pGame;
}

void Game::Initialize()
{
    g_pGame = this;

    Pandora::GetImGuiSystem()->SetGameMenuBarCallback([this]() { DrawImGuiMenuBar(); });

    m_pPrefabEditor = std::make_unique<UI::PrefabEditor>();
    m_pPrefabEditor->Initialize();

    m_pItemManager = std::make_unique<ItemManager>();
    // m_pSectorGenerator = std::make_unique<SectorGenerator>();

    m_pHyperscape = std::make_unique<Hyperscape>();
    m_pHyperscape->Initialize();

    auto sectorSignals = m_pHyperscape->GetSignals();
    assert(sectorSignals.size() == 1);
    SectorSignal* pSectorSignal = sectorSignals.back();

    m_pSector = pSectorSignal->Spawn();
    Pandora::SetActiveScene(m_pSector);

    // m_pCamera = std::make_shared<OrbitCamera>();
    // m_pCamera->LookAt(
    //     glm::vec3(0.0f, 0.0f, 10.0f),
    //     glm::vec3(0.0f, 0.0f, 0.0f),
    //     glm::vec3(0.0f, 1.0f, 0.0f)
    // );

    // m_pMenuScene = std::make_shared<Pandora::Scene>();
    //  m_pMenuScene->AddEntity(m_pCamera);
    //  m_pMenuScene->SetCamera(m_pCamera);
    // Pandora::SetActiveScene(m_pMenuScene);
}

void Game::Update(float delta)
{
    m_pPrefabEditor->DrawPrefabEditor();
}

void Game::Shutdown()
{
}

// Called from ImGuiSystem::Update() to draw any menus in the menu bar.
void Game::DrawImGuiMenuBar()
{
    using namespace Pandora;

    if (m_pSector)
    {
        if (ImGui::BeginMenu("Sector"))
        {
            static bool sShowCameraWindow = false;
            if (ImGui::MenuItem("Camera", nullptr, &sShowCameraWindow))
            {
                m_pSector->ShowCameraDebugUI(sShowCameraWindow);
            }
            if (ImGui::BeginMenu("Physics"))
            {
                PhysicsSimulationSystem* pPhysicsSystem = m_pSector->GetSystem<PhysicsSimulationSystem>();
                if (pPhysicsSystem)
                {
                    PhysicsVisualization* pVizualisation = pPhysicsSystem->GetVizualisation();
                    if (pVizualisation)
                    {
                        ImGui::SeparatorText("Debug rendering");
                        bool wireframe = pVizualisation->IsEnabled(PhysicsVisualization::Mode::Wireframe);
                        if (ImGui::MenuItem("Wireframe", nullptr, &wireframe))
                        {
                            pVizualisation->SetEnabled(PhysicsVisualization::Mode::Wireframe, wireframe);
                        }

                        bool aabb = pVizualisation->IsEnabled(PhysicsVisualization::Mode::AABB);
                        if (ImGui::MenuItem("AABB", nullptr, &aabb))
                        {
                            pVizualisation->SetEnabled(PhysicsVisualization::Mode::AABB, aabb);
                        }

                        bool transforms = pVizualisation->IsEnabled(PhysicsVisualization::Mode::Transforms);
                        if (ImGui::MenuItem("Transforms", nullptr, &transforms))
                        {
                            pVizualisation->SetEnabled(PhysicsVisualization::Mode::Transforms, transforms);
                        }

                        bool rayTests = pVizualisation->IsEnabled(PhysicsVisualization::Mode::RayTests);
                        if (ImGui::MenuItem("Ray tests", nullptr, &rayTests))
                        {
                            pVizualisation->SetEnabled(PhysicsVisualization::Mode::RayTests, rayTests);
                        }

                        bool contactPoints = pVizualisation->IsEnabled(PhysicsVisualization::Mode::ContactPoints);
                        if (ImGui::MenuItem("Contact points", nullptr, &contactPoints))
                        {
                            pVizualisation->SetEnabled(PhysicsVisualization::Mode::ContactPoints, contactPoints);
                        }
                    }
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
    }

    if (ImGui::BeginMenu("UI"))
    {
        static bool sShowPrefabEditor = false;
        if (ImGui::MenuItem("Prefab Editor", nullptr, &sShowPrefabEditor))
        {
            m_pPrefabEditor->ShowPrefabEditor(sShowPrefabEditor);
        }
        ImGui::EndMenu();
    }
}

} // namespace WingsOfSteel::TheBrightestStar