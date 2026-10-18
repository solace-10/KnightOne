#include <imgui.h>

#include <core/log.hpp>
#include <pandora.hpp>
#include <physics/collision_shape.hpp>
#include <render/debug_render.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <scene/components/ambient_light_component.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/debug_render_component.hpp>
#include <scene/components/directional_light_component.hpp>
#include <scene/components/model_component.hpp>
#include <scene/components/rigid_body_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/systems/model_render_system.hpp>
#include <scene/systems/physics_simulation_system.hpp>

#include "components/ai_mech_controller_component.hpp"
#include "components/mech_modules_component.hpp"
#include "components/player_controller_component.hpp"
#include "components/sector_camera_component.hpp"
#include "components/shield_component.hpp"
#include "components/weapon_component.hpp"
#include "entity_builder/entity_builder.hpp"
#include "sector/encounter.hpp"
#include "sector/sector.hpp"
#include "systems/ai_mech_controller_system.hpp"
#include "systems/ai_strategic_system.hpp"
#include "systems/ai_strikecraft_controller_system.hpp"
#include "systems/ammo_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/carrier_system.hpp"
#include "systems/debug_render_system.hpp"
#include "systems/heat_management_system.hpp"
#include "systems/mech_navigation_system.hpp"
#include "systems/player_controller_system.hpp"
#include "systems/projectile_render_system.hpp"
#include "systems/shield_system.hpp"
#include "systems/skybox_render_system.hpp"
#include "systems/ship_navigation_system.hpp"
#include "systems/target_overlay_system.hpp"
#include "systems/threat_indicator_system.hpp"
#include "systems/weapon_system.hpp"

namespace WingsOfSteel
{

Sector::Sector()
{
}

Sector::~Sector()
{
}

void Sector::Initialize()
{
    Scene::Initialize();

    AddSystem<ModelRenderSystem>();
    AddSystem<PhysicsSimulationSystem>();
    AddSystem<AIStrategicSystem>();
    AddSystem<AIMechControllerSystem>();
    AddSystem<AIStrikecraftControllerSystem>();
    AddSystem<CarrierSystem>();
    AddSystem<PlayerControllerSystem>();
    AddSystem<MechNavigationSystem>();
    AddSystem<ShipNavigationSystem>();
    AddSystem<WeaponSystem>();
    AddSystem<AmmoSystem>();
    AddSystem<TargetOverlaySystem>();
    AddSystem<ThreatIndicatorSystem>();
    AddSystem<ShieldSystem>();
    AddSystem<HeatManagementSystem>();
    AddSystem<SkyboxRenderSystem>();
    AddSystem<ProjectileRenderSystem>();

    // Make sure these systems are added after everything else that might modify transforms,
    // otherwise the camera and debug rendering will be offset by a frame.
    AddSystem<CameraSystem>();
    AddSystem<DebugRenderSystem>();

    m_pCamera = CreateEntity();
    m_pCamera->AddComponent<CameraComponent>(70.0f, 1.0f, 5000.0f);

    SectorCameraComponent& sectorCameraComponent = m_pCamera->AddComponent<SectorCameraComponent>();
    sectorCameraComponent.position = glm::vec3(0.0f, 45.0f, 30.0f);
    sectorCameraComponent.target = glm::vec3(0.0f, 0.0f, 0.0f);
    sectorCameraComponent.maximumDrift = glm::vec3(0.0f, 0.0f, 0.0f);
    SetCamera(m_pCamera);

    SpawnSkybox();
    SpawnLight();
    SpawnPlayerFleet();

    m_pEncounter = std::make_unique<Encounter>();
    m_pEncounter->Initialize(std::static_pointer_cast<Sector>(shared_from_this()));
}

void Sector::Update(float delta)
{
    Scene::Update(delta);

    m_pEncounter->Update(delta);

    if (m_ShowGrid)
    {
        GetDebugRender()->XZSquareGrid(-1000.0f, 1000.0f, -1.0f, 100.0f, Color::White);
    }

    DrawCameraDebugUI();
}

void Sector::ShowCameraDebugUI(bool state)
{
    m_ShowCameraDebugUI = state;
}

void Sector::ShowGrid(bool state)
{
    m_ShowGrid = state;
}

void Sector::DrawCameraDebugUI()
{
    if (!m_ShowCameraDebugUI)
    {
        return;
    }

    ImGui::Begin("Camera", &m_ShowCameraDebugUI);

    SectorCameraComponent& sectorCameraComponent = m_pCamera->GetComponent<SectorCameraComponent>();

    ImGui::Checkbox("Debug Draw", &sectorCameraComponent.debugDraw);

    CameraSystem* pCameraSystem = GetSystem<CameraSystem>();
    bool freeCamera = pCameraSystem->IsFreeCameraEnabled();
    if (ImGui::Checkbox("Free camera", &freeCamera))
    {
        pCameraSystem->SetFreeCameraEnabled(freeCamera);
    }
    float freeCameraSpeed = pCameraSystem->GetFreeCameraSpeed();
    if (ImGui::SliderFloat("Free camera speed", &freeCameraSpeed, 1.0f, 500.0f))
    {
        pCameraSystem->SetFreeCameraSpeed(freeCameraSpeed);
    }

    ImGui::End();
}

void Sector::SpawnSkybox()
{
    SceneWeakPtr pWeakScene = GetWeakPtr();
    EntityBuilder::Build(pWeakScene, "/entity_prefabs/skyboxes/ember_reach.json", glm::mat4(1.0f), [](EntitySharedPtr pEntity) {});
}

void Sector::SpawnLight()
{
    m_pLight = CreateEntity();

    DirectionalLightComponent& directionalLightComponent = m_pLight->AddComponent<DirectionalLightComponent>();
    directionalLightComponent.SetAngle(226.0f);
    directionalLightComponent.SetPitch(40.0f);
    directionalLightComponent.SetColor(1.0f, 0.96f, 0.68f);

    AmbientLightComponent& ambientLightComponent = m_pLight->AddComponent<AmbientLightComponent>();
    ambientLightComponent.SetColor(0.10f, 0.14f, 0.17f);    
}

void Sector::SpawnPlayerFleet()
{
    SceneWeakPtr pWeakScene = GetWeakPtr();

    SpawnMech(glm::vec3(-200.0f, 0.0f, 0.0f), 90.0f, true, WingRole::Offense);
    SpawnMech(glm::vec3(-200.0f, 0.0f, 30.0f), 90.0f, false, WingRole::Defense);
    SpawnMech(glm::vec3(-200.0f, 0.0f, -30.0f), 90.0f, false, WingRole::Interception);

    EntityBuilder::Build(pWeakScene, "/entity_prefabs/player/carrier.json", glm::translate(glm::mat4(1.0f), glm::vec3(-250.0f, 0.0f, 0.0f)), [pWeakScene](EntitySharedPtr pEntity) {
        SectorSharedPtr pScene = std::dynamic_pointer_cast<Sector>(pWeakScene.lock());
        if (pScene)
        {
            pScene->m_pCarrier = pEntity;
        }
    });
}

void Sector::SpawnMech(const glm::vec3& position, float angle, bool isPlayerMech, WingRole role)
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::rotate(transform, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

    SceneWeakPtr pWeakScene = GetWeakPtr();
    EntityBuilder::Build(pWeakScene, "/entity_prefabs/player/mech.json", transform, [pWeakScene, isPlayerMech, role](EntitySharedPtr pMechEntity) {
        SectorSharedPtr pScene = std::dynamic_pointer_cast<Sector>(pWeakScene.lock());
        if (pScene)
        {
            WingComponent& wingComponent = pMechEntity->AddComponent<WingComponent>();
            wingComponent.Role = role;
            
            if (isPlayerMech)
            {
                pScene->m_pPlayerMech = pMechEntity;
                pMechEntity->AddComponent<PlayerControllerComponent>();
                pScene->m_pCamera->GetComponent<SectorCameraComponent>().anchorEntity = pMechEntity;
            }
            else
            {
                AIMechControllerComponent& mechControllerComponent = pMechEntity->AddComponent<AIMechControllerComponent>();
                mechControllerComponent.OffenseContext.pOwner = pMechEntity;
                mechControllerComponent.NavigationContext.pOwner = pMechEntity;
                mechControllerComponent.DefenseContext.pOwner = pMechEntity;
            }

            const WeaponFriendOrFoe friendOrFoe = isPlayerMech ? WeaponFriendOrFoe::Disabled : WeaponFriendOrFoe::Enabled;
            pScene->GetSystem<WeaponSystem>()->AttachWeapon(
                "/entity_prefabs/weapons/mech/shield_r.json",
                pMechEntity,
                "RightArm",
                false,
                friendOrFoe);

            pScene->GetSystem<WeaponSystem>()->AttachWeapon(
                "/entity_prefabs/weapons/mech/rotary_cannon_l.json",
                pMechEntity,
                "LeftArm",
                false,
                friendOrFoe);

            SceneWeakPtr pLocalWeakScene = pScene->GetWeakPtr();
            EntityBuilder::Build(pLocalWeakScene, "/entity_prefabs/player/mech_energy_shield.json", glm::mat4(1.0f), [pLocalWeakScene, pMechEntity](EntitySharedPtr pShieldEntity) {
                SectorSharedPtr pScene = std::dynamic_pointer_cast<Sector>(pLocalWeakScene.lock());
                if (!pScene)
                {
                    return;
                }

                if (!pShieldEntity->HasComponent<ShieldComponent>())
                {
                    Log::Error() << "Mech's shield has no ShieldComponent.";
                    return;
                }

                pShieldEntity->SetParent(pMechEntity);
                pShieldEntity->GetComponent<ShieldComponent>().SetOwner(pShieldEntity);
                
                if (!pMechEntity->HasComponent<MechModulesComponent>())
                {
                    Log::Error() << "Mech has no MechModulesComponent.";
                    return;
                }

                pMechEntity->GetComponent<MechModulesComponent>().EnergyShield = pShieldEntity;
            });
        }
    });
}

} // namespace WingsOfSteel
