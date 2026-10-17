/**
 * Svelte stores for Knight-One game state, fed by C++ via WebInterop
 * (game/src/emscripten/web_interop.cpp).
 */

import { derived, writable } from 'svelte/store';

/**
 * Field names match those set in WebInterop::NotifyEncounterResolved.
 */
export interface EncounterResult {
	result: 'victory' | 'defeat';
	durationSeconds: number;
	kills: number;
}

export const encounterResult = writable<EncounterResult | null>(null);

export const debugUIVisible = writable(false);

/**
 * Whether the game wants the pointer locked to the canvas: the cursor must be free for any UI.
 */
export const pointerLockWanted = derived(
	[encounterResult, debugUIVisible],
	([$encounterResult, $debugUIVisible]) => !$encounterResult && !$debugUIVisible
);

/**
 * Initialize the global knightOneCallbacks object that C++ calls into.
 * Must be called before the WASM module loads.
 */
export function initializeCallbacks(): void {
	const callbacks = {
		onEncounterResolved: (result: EncounterResult) => encounterResult.set(result),
		onDebugUIVisibilityChanged: (visible: boolean) => debugUIVisible.set(visible)
	};
	(window as unknown as { knightOneCallbacks: typeof callbacks }).knightOneCallbacks = callbacks;
}
