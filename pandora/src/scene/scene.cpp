#include "scene/scene.hpp"

#include "core/log.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/entity_reference_component.hpp"
#include "scene/components/transform_component.hpp"
#include "scene/entity.hpp"
#include "scene/systems/system.hpp"

namespace WingsOfSteel
{

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::Initialize()
{
}

void Scene::Update(float delta)
{
    ProcessEntitiesToAdd();

    for (auto& pSystem : m_Systems)
    {
        pSystem->Update(delta);
    }

    ProcessEntitiesToRemove();
}

EntitySharedPtr Scene::CreateEntity()
{
    EntitySharedPtr pEntity = std::make_shared<Entity>(this);
    m_EntitiesPendingAdd.push_back(pEntity);

    pEntity->m_EntityHandle = m_Registry.create();

    pEntity->AddComponent<EntityReferenceComponent>(pEntity);

    return pEntity;
}

void Scene::RemoveEntity(EntitySharedPtr pEntity)
{
    pEntity->m_MarkedForRemoval = true;
}

void Scene::SetCamera(EntitySharedPtr pCamera)
{
    if (!pCamera->HasComponent<CameraComponent>())
    {
        Log::Error() << "Trying to add a camera without a CameraComponent.";
        m_pCamera.reset();
    }
    else
    {
        m_pCamera = pCamera;
    }
}

EntitySharedPtr Scene::GetCamera() const
{
    return m_pCamera.lock();
}

void Scene::ProcessEntitiesToAdd()
{
    for (auto& pEntity : m_EntitiesPendingAdd)
    {
        m_Entities.push_back(pEntity);
        pEntity->OnAddedToScene();
    }
    m_EntitiesPendingAdd.clear();
}

void Scene::ProcessEntitiesToRemove()
{
    for (auto& pEntity : m_Entities)
    {
        if (pEntity->m_MarkedForRemoval)
        {
            pEntity->OnRemovedFromScene();
            m_Registry.destroy(pEntity->m_EntityHandle);
        }
    }

    std::erase_if(m_Entities, [](EntitySharedPtr pEntity) { return pEntity->m_MarkedForRemoval; });
}

entt::registry& Scene::GetRegistry()
{
    return m_Registry;
}

} // namespace WingsOfSteel