<script lang="ts">
	import { onMount } from 'svelte';

	interface EmscriptenModule {
		print(...args: unknown[]): void;
		canvas: HTMLCanvasElement;
		setStatus(text: string): void;
		totalDependencies: number;
		monitorRunDependencies(left: number): void;
		requestFullscreen?(lockPointer: boolean, resizeCanvas: boolean): void;
	}

	let canvas: HTMLCanvasElement;
	let statusText = $state('Downloading...');
	let showOverlay = $state(true);
	let failed = $state(false);

	onMount(() => {
		let lastStatus = { time: Date.now(), text: '' };

		// The global Module object that the Emscripten-generated game.js picks up.
		const Module: EmscriptenModule = {
			print(...args: unknown[]) {
				console.log(...args);
			},
			canvas,
			setStatus(text: string) {
				if (failed || text === lastStatus.text) return;
				const m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
				const now = Date.now();
				if (m && now - lastStatus.time < 30) return;
				lastStatus = { time: now, text };
				statusText = m ? m[1] : text;
				if (!text) {
					showOverlay = false;
				}
			},
			totalDependencies: 0,
			monitorRunDependencies(left: number) {
				this.totalDependencies = Math.max(this.totalDependencies, left);
				Module.setStatus(
					left
						? `Preparing... (${this.totalDependencies - left}/${this.totalDependencies})`
						: 'All downloads complete.'
				);
			}
		};
		(window as unknown as { Module: EmscriptenModule }).Module = Module;

		const fail = (text: string) => {
			statusText = text;
			failed = true;
		};

		window.onerror = () => fail('Exception thrown, see JavaScript console');

		const script = document.createElement('script');
		script.src = '/game.js';
		script.async = true;
		script.onerror = () => fail('Failed to load game.js');
		document.body.appendChild(script);
	});

	function onMouseDown(e: MouseEvent) {
		e.preventDefault();
		if (document.pointerLockElement !== canvas) {
			canvas.requestPointerLock();
		}
	}

	function requestFullscreen() {
		(window as unknown as { Module: EmscriptenModule }).Module.requestFullscreen?.(true, true);
	}
</script>

<canvas
	bind:this={canvas}
	id="canvas"
	class="game-canvas"
	tabindex="-1"
	oncontextmenu={(e) => e.preventDefault()}
	onauxclick={(e) => e.preventDefault()}
	onmousedown={onMouseDown}
></canvas>

{#if showOverlay}
	<div class="overlay">
		{#if !failed}
			<div class="spinner"></div>
		{/if}
		<div class="status">{statusText}</div>
	</div>
{/if}

<button class="fullscreen-btn" onclick={requestFullscreen}>Fullscreen</button>

<style>
	.game-canvas {
		position: absolute;
		top: 0;
		left: 0;
		width: 100%;
		height: 100%;
		display: block;
		border: none;
		background: #000;
	}

	.overlay {
		position: fixed;
		inset: 0;
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		background: rgba(0, 0, 0, 0.8);
		z-index: 10;
		pointer-events: none;
	}

	.spinner {
		height: 50px;
		width: 50px;
		animation: rotation 0.8s linear infinite;
		border: 10px solid rgb(0, 150, 240);
		border-top-color: rgb(100, 0, 200);
		border-radius: 100%;
		background-color: rgb(200, 100, 250);
	}

	@keyframes rotation {
		from {
			transform: rotate(0deg);
		}
		to {
			transform: rotate(360deg);
		}
	}

	.status {
		color: white;
		margin-top: 1em;
		font-family: sans-serif;
	}

	.fullscreen-btn {
		position: fixed;
		bottom: 20px;
		right: 20px;
		z-index: 20;
		padding: 10px 20px;
		font-size: 14px;
		cursor: pointer;
	}
</style>
