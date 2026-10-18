#pragma once

#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class HeatManagementSystem : public System
{
public:
    HeatManagementSystem() = default;
    ~HeatManagementSystem() = default;

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;
};

} // namespace WingsOfSteel
