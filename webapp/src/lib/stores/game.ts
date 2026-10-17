/**
 * Svelte stores for Knight-One game state, fed by C++ via WebInterop
 * (game/src/emscripten/web_interop.cpp).
 */

import { writable } from 'svelte/store';

/**
 * Field names match those set in WebInterop::NotifyEncounterResolved.
 */
export interface EncounterResult {
	result: 'victory' | 'defeat';
	durationSeconds: number;
	kills: number;
}

export const encounterResult = writable<EncounterResult | null>(null);

/**
 * Initialize the global knightOneCallbacks object that C++ calls into.
 * Must be called before the WASM module loads.
 */
export function initializeCallbacks(): void {
	const callbacks = {
		onEncounterResolved: (result: EncounterResult) => encounterResult.set(result)
	};
	(window as unknown as { knightOneCallbacks: typeof callbacks }).knightOneCallbacks = callbacks;
}
