#pragma once

#include <vector>

#include <entt/entt.hpp>
#include <webgpu/webgpu_cpp.h>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(Entity);
DECLARE_SMART_PTR(Scene);
DECLARE_SMART_PTR(System);

class Scene
{
public:
    Scene();
    ~Scene();

    virtual void Initialize();
    virtual void Update(float delta);
    virtual void Render(wgpu::RenderPassEncoder renderPass);

    EntitySharedPtr CreateEntity();
    void RemoveEntity(EntitySharedPtr pEntity);

    void AddSystem(SystemUniquePtr pSystem);

    void SetCamera(EntitySharedPtr pCamera);
    EntitySharedPtr GetCamera() const;

    friend Entity;

private:
    void ProcessEntitiesToAdd();
    void ProcessEntitiesToRemove();

    std::vector<EntitySharedPtr> m_Entities;
    std::vector<EntitySharedPtr> m_EntitiesPendingAdd;
    EntityWeakPtr m_pCamera;
    entt::registry m_Registry;
    std::vector<SystemUniquePtr> m_Systems;
};

} // namespace WingsOfSteel::Pandora