# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Knight-One (Wings of Steel) is a space combat demo built as a C++20 CMake project on Pandora, a
custom engine shared with other projects (e.g. Apsis). It consists of the engine (`pandora/`) and the
game (`game/`).

**Port in progress:** Knight-One was written against an old version of Pandora and has been ported
to the current engine far enough to build on Linux and web, but has not yet been verified at
runtime. Where the game and the engine disagree, use `pandora/src/` as the source of truth, not
the game's call sites.

## Build Commands

### First-time setup

**`pandora/ext/`** (emsdk + FetchContent deps, ~4 GB) is gitignored and not part of the subtree.
Either run `pandora/scripts/setup_emscripten.sh` (and let CMake's FetchContent populate the rest
on configure), or symlink it to an already-populated checkout, e.g.
`ln -s ~/dev/apsis/pandora/ext pandora/ext` — in which case it is *shared*, so a dependency bump
in either tree is seen by both.

### Pandora changes (git subtree)

`pandora/` is a git subtree of `git@github.com:solace-10/pandora.git`, which other projects
share. It is plain files in this repo: edit and commit engine and game changes together, in one
commit if that's natural, on any branch. Sync with upstream via `./scripts/pandora.sh`:
- `status` — engine commits not yet pushed upstream, and upstream commits not yet pulled.
- `sync` — `pull` (squash-merge upstream `main` into `pandora/`), then `push` (split the
  `pandora/` part of our commits out and push it to upstream `main`). Run after `git commit`,
  then `git push` this repo last, since the pull may add a merge commit.
- Only push from this repo's `main`: engine changes on a feature branch go upstream after the
  branch merges, so unlanded experiments don't reach the other projects. The script enforces it.
- Engine changes are seen by every project using Pandora. Changes made only to get Knight-One
  compiling should be done in `game/`, not by reshaping the engine around old game code.
- Don't run `git subtree` by hand with a custom `-m` on `add`, and don't rewrite the
  `Add pandora as a git subtree` commit: its `git-subtree-*` trailers stop `split` from treating
  early history (when `pandora/` was a plain directory) as pandora's.

### Configure (run once or after CMakeLists.txt changes)
```bash
cmake --preset debug-linux     # Linux
cmake --preset debug-windows   # Windows
cmake --preset debug-web       # Web (Emscripten)
```

### Build
```bash
cmake --build build/debug-linux --target setup_compile_commands game   # Linux
cmake --build build/debug-windows --target setup_compile_commands game # Windows
cmake --build build/debug-web --target setup_compile_commands game     # Web
```

Targets: `game` (main executable), `pandora` (engine library).

### Forge

Forge is a Go CLI for the web asset pipeline, in `pandora/tools/forge/`. Prebuilt binaries live in
`pandora/tools/forge/bin/forge-<os>-<arch>` (e.g. `forge-darwin-arm64`).
- `forge manifest` — generate `game/bin/manifest.json` (gitignored; auto-runs during web builds)
- `forge serve` — serve assets locally for development
- `forge upload` — upload assets to Cloudflare R2
- `forge package` — package everything for deployment

`forge manifest` ships everything under `game/bin/data/core/`, so don't put documentation or other
non-runtime files in that tree.

### Dependencies
Linux setup: `sudo apt install clang clangd cmake libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libglfw3-dev libx11-xcb-dev`

## Formatting

Format with clang-format (WebKit-based): `clang-format -i path/to/file.cpp`

## Project Structure

- `pandora/` - Core engine library (rendering, ECS, physics, resources, input, VFS, ImGui)
- `game/` - Main game application:
  - Space exploration with sectors, ships, hyperscape navigation
  - Ship builder with prefab system and hardpoint management
  - Player controller, weapon systems, navigation systems
  - Fleet management and encounter mechanics with a card-based AI system
  - Encounters are loaded at random from `game/bin/data/core/encounters/difficulty<n>/`; encounter
    decks are tier-based cards defining enemy waves and behaviour
  - Entity prefabs (ships, weapons, ammo) in `game/bin/data/core/entity_prefabs/`
  - Assets (models, textures, fonts, UI icons) in `game/assets/`
- `scripts/` - Repo scripts (`pandora.sh`)

## Code Style

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes/Structs | PascalCase | `Sector`, `CameraSystem` |
| Functions/Methods | PascalCase | `Initialize()`, `GetSector()` |
| Member variables | `m_` prefix | `m_ShowDebug` |
| Member pointers | `m_p` prefix | `m_pCamera` |
| Static variables | `s` prefix | `sShaderMap` |
| Global pointers | `g_p` prefix | `g_pGame` |
| Constants | `k` prefix | `kMaxShips` |

### Header Files
- Use `#pragma once`
- Forward declare where possible
- Include order: corresponding header → standard library → third-party → engine → local

### Smart Pointers
```cpp
DECLARE_SMART_PTR(Sector);  // Generates: SectorSharedPtr, SectorWeakPtr, SectorUniquePtr
```

### Namespace
```cpp
namespace WingsOfSteel
{
    class MyClass { };
} // namespace WingsOfSteel
```

### Error Handling
```cpp
Result<DeserializationError, std::string> result = TryDeserializeString(...);
if (result.has_value()) { return result.value(); }

Log::Info() << "Message";
Log::Warning() << "Warning";
Log::Error() << "Error: " << details;
```

### Platform-Specific Code
```cpp
#if defined(TARGET_PLATFORM_WEB)
#elif defined(TARGET_PLATFORM_WINDOWS)
#elif defined(TARGET_PLATFORM_LINUX)
#endif
```

## Architecture

### ECS (EnTT-based)
- Entities: `Scene::CreateEntity()`
- Components: inherit `IComponent`, use `REGISTER_COMPONENT` macro
- Systems: inherit `System`, implement `Initialize()` and `Update(float delta)`

### Resource Loading (async)
```cpp
GetResourceSystem()->RequestResource("/path/resource.json",
    [](ResourceSharedPtr pResource) { /* callback */ });
```

### Global Accessors
```cpp
GetRenderSystem(), GetInputSystem(), GetResourceSystem(),
GetVFS(), GetWindow(), GetActiveScene(), GetDebugRender()
```

### Key Technologies
- **Graphics**: WebGPU — Dawn on native, browser WebGPU via Emscripten on web; ImGui for UI
- **Physics**: Bullet3
- **Math**: GLM
- **Serialization**: nlohmann/json
- **Asset Loading**: tinygltf for models, stb for images
- **Reflection**: magic_enum

## Code Modification Guidelines
- After you modify a code file, build the game to ensure everything compiles (`./build.sh <platform>
  <debug|release>`, platform being `linux`, `windows` or `web`).
