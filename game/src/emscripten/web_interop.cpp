#if defined(TARGET_PLATFORM_WEB)

#include "emscripten/web_interop.hpp"

#include <chrono>
#include <string>

#include <emscripten/val.h>

#include "sector/encounter.hpp"
#include "sector/encounter_stats.hpp"

namespace WingsOfSteel::WebInterop
{

void NotifyEncounterResolved(EncounterResult result, const EncounterStats& stats)
{
    emscripten::val callbacks = emscripten::val::global("knightOneCallbacks");
    if (callbacks.isUndefined() || callbacks.isNull())
    {
        return;
    }

    emscripten::val onEncounterResolved = callbacks["onEncounterResolved"];
    if (onEncounterResolved.isUndefined())
    {
        return;
    }

    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(stats.finishedAt - stats.startedAt);

    emscripten::val interop = emscripten::val::object();
    interop.set("result", std::string(result == EncounterResult::Victory ? "victory" : "defeat"));
    interop.set("durationSeconds", static_cast<int>(duration.count()));
    interop.set("kills", stats.kills);
    onEncounterResolved(interop);
}

} // namespace WingsOfSteel::WebInterop

#endif // TARGET_PLATFORM_WEB
