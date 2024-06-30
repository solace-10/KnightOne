#pragma once

#include <functional>

#include "core/smart_ptr.hpp"

namespace WingsOfSteel::Pandora
{

DECLARE_SMART_PTR(ImGuiSystem);
DECLARE_SMART_PTR(RenderSystem);
DECLARE_SMART_PTR(ResourceSystem);
DECLARE_SMART_PTR(Scene);
DECLARE_SMART_PTR(VFS);
DECLARE_SMART_PTR(Window);

using GameInitializeCallback = std::function<void()>;
using GameUpdateCallback = std::function<void(float)>;
using GameShutdownCallback = std::function<void()>;

void Initialize(GameInitializeCallback gameInitializeCallback, GameUpdateCallback gameUpdateCallback, GameShutdownCallback gameShutdownCallback);
void Update();
void Shutdown();

ImGuiSystem* GetImGuiSystem();
RenderSystem* GetRenderSystem();
ResourceSystem* GetResourceSystem();
Scene* GetActiveScene();
void SetActiveScene(SceneSharedPtr pScene);
VFS* GetVFS();
Window* GetWindow();

} // namespace WingsOfSteel::Pandora