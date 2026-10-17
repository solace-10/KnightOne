#pragma once

#if defined(TARGET_PLATFORM_WEB)

namespace WingsOfSteel
{

enum class EncounterResult;
struct EncounterStats;

// Sends game events to the webapp, which handles them via the global `knightOneCallbacks` object
// (see webapp/src/lib/stores/game.ts).
namespace WebInterop
{
    void NotifyEncounterResolved(EncounterResult result, const EncounterStats& stats);
} // namespace WebInterop

} // namespace WingsOfSteel

#endif // TARGET_PLATFORM_WEB
