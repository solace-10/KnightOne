<script lang="ts">
	// ponytail: placeholder state, replace with stores fed by WebInterop when hooked up to the game.
	const health = 72;
	const heat = 58;
	const activeSlot = 1;
	const weapons = [
		{ code: 'AC-30', name: 'Autocannon', clip: 32, cap: 40, clips: 6 },
		{ code: 'RG-2', name: 'Railgun', clip: 3, cap: 4, clips: 5 },
		{ code: 'MS-12', name: 'Swarm pod', clip: 12, cap: 12, clips: 2 },
		{ code: 'PL-7', name: 'Pulse laser', clip: 9, cap: 60, clips: 4 }
	];

	const hot = $derived(heat >= 80);
</script>

<svelte:head>
	<link
		rel="stylesheet"
		href="https://fonts.googleapis.com/css2?family=Rajdhani:wght@500;600;700&display=swap"
	/>
</svelte:head>

<div class="hud">
	<div class="status">
		<div class="gauge integrity">
			<div class="readout">
				<span class="label">INTEGRITY</span>
				<span class="value">{health}</span>
			</div>
			<div class="track"><div class="fill" style:width="{health}%"></div></div>
		</div>
		<svg width="28" height="28" viewBox="0 0 28 28" aria-hidden="true">
			<path d="M14 2 L26 14 L14 26 L2 14 Z" fill="none" stroke="#7FE7FF" stroke-width="1.5" />
			<path d="M14 9 L19 14 L14 19 L9 14 Z" fill="#7FE7FF" />
		</svg>
		<div class="gauge heat" class:hot>
			<div class="readout">
				<span class="value">{heat}°</span>
				<span class="label">HEAT</span>
				{#if hot}<span class="vent">VENT</span>{/if}
			</div>
			<div class="track">
				<div class="fill" style:width="{heat}%"></div>
				<div class="threshold"></div>
			</div>
		</div>
	</div>

	<div class="weapons">
		{#each weapons as w, i}
			{@const slot = i + 1}
			{@const low = w.clip / w.cap <= 0.25}
			{#if slot === activeSlot}
				<div class="card active" class:low>
					<div class="header">
						<span class="slot">{slot}</span>
						<span class="code">{w.code}</span>
					</div>
					<div class="name">{w.name}</div>
					<div class="ammo">
						<span class="clip">{w.clip}</span>
						<span class="cap">/ {w.cap}</span>
						<span class="clips">×{w.clips}</span>
					</div>
				</div>
			{:else}
				<div class="card idle" class:low>
					<div class="header">
						<span class="slot">{slot}</span>
						<span class="name">{w.name}</span>
					</div>
					<div class="ammo">
						<span class="clip">{w.clip}/{w.cap}</span>
						<span class="clips">×{w.clips}</span>
					</div>
				</div>
			{/if}
		{/each}
	</div>
</div>

<style>
	.hud {
		--cyan: #7fe7ff;
		--text: #eaf6ff;
		--warn: #ff8a3d;

		position: fixed;
		left: 50%;
		bottom: 32px;
		width: 880px;
		transform: translateX(-50%);
		display: flex;
		flex-direction: column;
		gap: 18px;
		color: var(--text);
		font-family: 'Rajdhani', system-ui, sans-serif;
		pointer-events: none;
		user-select: none;
	}

	.status {
		display: flex;
		align-items: center;
		gap: 20px;
	}

	.gauge {
		flex: 1 1 0;
		display: flex;
		flex-direction: column;
		gap: 6px;
	}

	.readout {
		display: flex;
		align-items: baseline;
		gap: 10px;
	}

	.label {
		font-size: 14px;
		font-weight: 600;
		letter-spacing: 0.3em;
	}

	.value {
		font-size: 30px;
		font-weight: 700;
		line-height: 1;
	}

	.track {
		position: relative;
		display: flex;
		height: 6px;
	}

	.integrity .readout,
	.integrity .track {
		justify-content: flex-end;
	}

	.integrity .label {
		color: var(--cyan);
	}

	.integrity .track {
		background: rgba(127, 231, 255, 0.16);
	}

	.integrity .fill {
		background: var(--cyan);
	}

	.heat {
		--heat: #ffc58a;
	}

	.heat.hot {
		--heat: var(--warn);
	}

	.heat .value,
	.heat .label {
		color: var(--heat);
	}

	.heat .track {
		background: rgba(255, 197, 138, 0.16);
	}

	.heat .fill {
		background: var(--heat);
	}

	.threshold {
		position: absolute;
		left: 80%;
		top: -5px;
		width: 2px;
		height: 16px;
		background: var(--warn);
	}

	.vent {
		margin-left: auto;
		padding: 1px 10px;
		border: 1px solid var(--warn);
		color: var(--warn);
		font-size: 13px;
		font-weight: 700;
		letter-spacing: 0.3em;
	}

	.weapons {
		display: flex;
		justify-content: center;
		align-items: flex-end;
		gap: 10px;
	}

	.card {
		display: flex;
		flex-direction: column;
		gap: 4px;
	}

	.header {
		display: flex;
		align-items: center;
		gap: 8px;
	}

	.slot {
		display: inline-flex;
		align-items: center;
		justify-content: center;
		font-weight: 700;
	}

	.clips {
		margin-left: auto;
	}

	.active {
		width: 290px;
		padding: 12px 16px 14px;
		background: rgba(8, 20, 30, 0.82);
		border: 1px solid rgba(127, 231, 255, 0.55);
		border-top: 3px solid var(--cyan);
	}

	.active .slot {
		width: 22px;
		height: 22px;
		border: 1px solid var(--cyan);
		color: var(--cyan);
		font-size: 13px;
	}

	.active .code {
		font-size: 13px;
		font-weight: 600;
		letter-spacing: 0.24em;
		color: var(--cyan);
	}

	.active .name {
		font-size: 26px;
		font-weight: 700;
		line-height: 1.05;
		text-transform: uppercase;
		letter-spacing: 0.06em;
	}

	.active .ammo {
		display: flex;
		align-items: baseline;
		gap: 6px;
	}

	.active .clip {
		font-size: 52px;
		font-weight: 700;
		line-height: 1;
	}

	.active.low .clip {
		color: var(--warn);
	}

	.active .cap {
		font-size: 22px;
		font-weight: 600;
		color: rgba(234, 246, 255, 0.65);
	}

	.active .clips {
		font-size: 24px;
		font-weight: 700;
	}

	.idle {
		width: 150px;
		padding: 10px 12px;
		background: rgba(8, 20, 30, 0.6);
		border: 1px solid rgba(127, 231, 255, 0.22);
		color: rgba(234, 246, 255, 0.72);
	}

	.idle .slot {
		width: 18px;
		height: 18px;
		border: 1px solid rgba(127, 231, 255, 0.5);
		font-size: 11px;
	}

	.idle .name {
		font-size: 15px;
		font-weight: 600;
		text-transform: uppercase;
		letter-spacing: 0.08em;
	}

	.idle .ammo {
		display: flex;
		align-items: baseline;
		gap: 6px;
		font-size: 19px;
		font-weight: 600;
	}

	.idle.low .clip {
		color: #ffa36b;
	}
</style>
