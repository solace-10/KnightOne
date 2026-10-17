<script lang="ts">
	import { encounterResult } from '$lib/stores/game';
	import logo from '$lib/assets/logo.png';
	import jumpIcon from '../../../../game/bin/data/core/ui/icons/encounter_button_jump.png';

	const content = {
		victory: { heading: 'VICTORY', button: '"Mission accomplished, returning to the Everflame."' },
		defeat: { heading: 'DEFEAT', button: '"Mission failed. All units, withdraw."' }
	};

	function formatDuration(totalSeconds: number): string {
		return `${Math.floor(totalSeconds / 60)}m${totalSeconds % 60}s`;
	}

	// Release the mouse so the window can be interacted with.
	$effect(() => {
		if ($encounterResult) {
			document.exitPointerLock();
		}
	});
</script>

{#if $encounterResult}
	{@const text = content[$encounterResult.result]}
	<div class="window">
		<div class="heading1">
			<svg width="372" height="42" aria-hidden="true">
				<polygon points="0,0 192,0 160,42 0,42" fill-opacity={30 / 255} />
				{#each [0, 1, 2, 3] as i}
					{@const x = 192 + i * 44}
					<polygon
						points="{x + 16},0 {x + 48},0 {x + 16},42 {x - 16},42"
						fill-opacity={(30 - i * 8) / 255}
					/>
				{/each}
			</svg>
			<h1>{text.heading}</h1>
		</div>

		<h2>ENGAGEMENT STATS</h2>

		<div class="stats">
			<dl>
				<div><dt>TYPE</dt><dd>PATROL</dd></div>
				<div>
					<dt>DEPLOYMENT DURATION</dt>
					<dd>{formatDuration($encounterResult.durationSeconds)}</dd>
				</div>
				<div><dt>KILLS</dt><dd>{$encounterResult.kills}</dd></div>
			</dl>
			<div class="logo" style:mask-image="url({logo})"></div>
		</div>

		<button class="important">
			<span class="icon"><img src={jumpIcon} alt="" /></span>
			{text.button}
		</button>
	</div>
{/if}

<style>
	/* Ported from the ImGui UI (game/src/ui/theme.hpp, window.cpp, heading.cpp, button.cpp). */
	.window {
		--accent: rgb(240, 54, 84);
		--text: rgb(240, 240, 240);
		--grid: rgba(255, 255, 255, 0.0235);

		position: fixed;
		top: 50%;
		left: 50%;
		transform: translate(-50%, -50%);
		width: 700px;
		height: 450px;
		display: flex;
		flex-direction: column;
		padding: 8px 8px 16px;
		border-top: 6px solid var(--accent);
		background:
			linear-gradient(to right, var(--grid) 1.5px, transparent 1.5px) 32px 32px / 48px 48px border-box,
			linear-gradient(to bottom, var(--grid) 1.5px, transparent 1.5px) 32px 32px / 48px 48px border-box,
			rgba(46, 46, 46, 0.94);
		clip-path: polygon(0 0, 100% 0, 100% calc(100% - 16px), calc(100% - 16px) 100%, 0 100%);
		color: var(--text);
		font-family: 'Supply Mono', monospace;
		font-size: 16px;
		font-weight: 400;
	}

	.heading1 {
		position: relative;
		flex: none;
		height: 44px;
	}

	.heading1 svg {
		display: block;
		fill: white;
	}

	h1 {
		position: absolute;
		top: 8px;
		left: 8px;
		font-size: 32px;
		font-weight: 400;
		line-height: 1;
	}

	h2 {
		flex: none;
		height: 25px;
		margin: 4px 0 7px;
		border-bottom: 1px solid var(--accent);
		color: var(--accent);
		font-size: 24px;
		font-weight: 600;
		line-height: 24px;
	}

	.stats {
		flex: 1;
		display: flex;
		gap: 8px;
		min-height: 0;
	}

	dl {
		flex: 1;
		padding-top: 4px;
	}

	dl div {
		display: flex;
		height: 24px;
	}

	dt {
		width: 300px;
	}

	dd {
		flex: 1;
		text-align: right;
	}

	.logo {
		flex: none;
		width: 320px;
		height: 288px;
		background: rgb(240, 56, 83);
		mask-repeat: no-repeat;
	}

	.important {
		--hover: transparent;

		appearance: none;
		position: relative;
		flex: none;
		height: 38px;
		margin-top: 8px;
		padding: 12px 0 0 48px;
		border: none;
		background:
			url("data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' width='64' height='38'><g fill='rgb(240,54,84)'><path fill-opacity='.2' d='M0 0h32L0 38h-32zM64 0h32L64 38H32z'/><path fill-opacity='.1' d='M32 0h32L32 38H0zM96 0h32L96 38H64z'/></g></svg>")
				0 0 / 64px 38px repeat-x,
			linear-gradient(var(--hover), var(--hover)) 16px 0 / calc(100% - 16px) 100% no-repeat;
		animation: stripes 2s linear infinite;
		color: rgb(255, 115, 140);
		font: inherit;
		line-height: 16px;
		text-align: left;
		white-space: nowrap;
		cursor: pointer;
	}

	.important:hover {
		--hover: rgba(240, 54, 84, 0.3);
		color: rgb(255, 166, 184);
	}

	.important::after {
		content: '';
		position: absolute;
		inset: 0 0 0 16px;
		border: 1px solid var(--accent);
		pointer-events: none;
	}

	@keyframes stripes {
		from {
			background-position: 0 0, 16px 0;
		}
		to {
			background-position: 64px 0, 16px 0;
		}
	}

	/* The icon is opaque white-on-black, tinted by multiplying with the accent like ImGui does. */
	.icon {
		position: absolute;
		top: 0;
		left: 0;
		width: 38px;
		height: 38px;
		overflow: hidden;
		border-radius: 4px;
		background: var(--accent);
		isolation: isolate;
	}

	.icon img {
		display: block;
		mix-blend-mode: multiply;
	}
</style>
