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

### First-time setup (per worktree — do this automatically before the first build)

We develop in git worktrees under `~/.herdr/worktrees/KnightOne/<branch>`. A fresh worktree is
missing gitignored build prerequisites that are NOT carried across from `main`.
**Before the first build in a worktree, run `./scripts/worktree-setup.sh` without asking.**
It is idempotent (creates only what's absent, borrowing from the canonical checkout at
`~/dev/KnightOne`, overridable via `KNIGHTONE_MAIN_CHECKOUT`) and safe to re-run. It handles:

1. **`pandora/ext/`** (emsdk + FetchContent deps, ~4 GB, gitignored, not part of the subtree) —
   symlinked to the canonical checkout's populated `ext/`. It is therefore *shared*, so a
   dependency bump in either tree is seen by both. Without it, configuring fails with
   `Could not find toolchain file: ".../Emscripten.cmake"`. If the canonical checkout has none,
   run `pandora/scripts/setup_emscripten.sh` (CMake's FetchContent populates the rest).
2. **`.env`** at the repo root (holds `FORGE_AUTH_KEY_SECRET` for `forge upload`, gitignored) —
   copied from the canonical checkout.

After this, `./build.sh <linux|windows|web> <debug|release>` works.

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

### Preferred build entry point
```bash
./build.sh <platform> <build_type>   # platform: linux|windows|web, build_type: debug|release
```

### Configure manually (run once or after CMakeLists.txt changes)
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

Targets: `game` (main executable), `pandora` (engine library). Native builds copy the executable
into `game/bin/`, which is the working directory to run from; web builds copy `game.js` and
`game.wasm` into `webapp/static/` (see below).

### Webapp (SvelteKit)

On web, the page is the SvelteKit app in `webapp/`, not an Emscripten shell. The web build copies
`game.js` and `game.wasm` into `webapp/static/` (gitignored), which the `Canvas` component loads.
```bash
cd webapp
npm install
npm run dev          # Development server on http://localhost:6174
npm run check        # Type-check
npm run build        # Production build into webapp/build/
```

### Forge

Forge is a Go CLI for the asset pipeline, in `pandora/tools/forge/` (see its `README.md`). Prebuilt
binaries live in `pandora/tools/forge/bin/forge-<os>-<arch>` (e.g. `forge-linux-amd64`).
- `forge assets` — compile `game/assets/` and the engine's `pandora/assets/` into
  `game/bin/data/core/`, driven by `*.asset.json` descriptors (PNG/JPG become KTX2). Runs on
  every build; cached. Engine assets land in the reserved, gitignored `<directory>/pandora/`
  paths (e.g. `/shaders/pandora/`), which game assets may not write to.
- `forge manifest` — hash `game/bin/data/core/` into `game/bin/manifest.json` (gitignored)
- `forge upload` — upload the manifest's files to Cloudflare R2, keyed by content hash
- `forge serve` — serve `game/bin/` on `http://127.0.0.1:8080/`
- `forge package` — manifest, upload and inject into `game.html` in one step (Knight-One no longer
  has a `game.html`: the page is the SvelteKit app)

Web builds run `manifest` and `upload` automatically, and the web game fetches its data from R2
rather than from `game/bin/`, so a data change only reaches the web build once it is uploaded
(which needs `.env`).

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
  - Runtime data (models, textures, shaders, UI icons) in `game/bin/data/core/`, loaded through
    the VFS as `/path`. Source assets live in `game/assets/`; Knight-One has no `*.asset.json`
    descriptors yet, so its data is still committed directly rather than compiled by Forge
  - Shaders can `#include "/shaders/..."` other files, resolved at load time. Shared game code
    lives in `/shaders/common/`; engine shaders (e.g. `/shaders/pandora/globals.wgsl`, which every
    scene shader includes for `GlobalUniforms`) come from `pandora/assets/shaders/`
- `webapp/` - SvelteKit + TypeScript frontend for the web build
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

### Rendering
The frame is a list of `Pass`es (`pandora/src/render/pass/`), registered in `Game::Initialize()`
with `GetRenderSystem()->AddPass()` and run in order via `Execute(wgpu::CommandEncoder&)`:
1. `ShadowPass` renders opaque models into the directional light's shadow map, which scene shaders
   sample with `sampleShadow()` from `/shaders/pandora/shadows.wgsl`.
2. `BaseRenderPass` renders the scene in linear HDR into the window's scene texture
   (`Window::GetSceneFormat()`, RGBA16Float). Scene shaders must output linear colour.
3. Game passes that need the scene's depth but must stay out of tonemapping (e.g.
   `ThreatIndicatorRenderPass`) draw into the overlay (`GetOverlayMsaaColorTexture()`, surface
   format, premultiplied) with a read-only depth attachment.
4. `PresentPass` applies bloom, exposure and tonemapping, encodes to the surface and composites
   the overlay.
5. `UIRenderPass` draws ImGui and debug rendering straight onto the surface.

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
- **Asset Loading**: tinygltf for models, KTX2 (KTX-Software) and stb for images
- **Reflection**: magic_enum
- **Assertions**: `PANDORA_ASSERT(expr) << "context"` (compiled out in release) and
  `PANDORA_VERIFY` (always evaluated, always aborts on failure), in `core/assert.hpp`

## Code Modification Guidelines
- After you modify a code file, build the game to ensure everything compiles (`./build.sh <platform>
  <debug|release>`, platform being `linux`, `windows` or `web`).
