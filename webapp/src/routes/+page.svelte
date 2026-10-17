<script lang="ts">
	import Canvas from '$lib/components/Canvas.svelte';
</script>

<svelte:head>
	<title>Knight-One</title>
</svelte:head>

<main class="app">
	<Canvas />
</main>

<style>
	:global(*) {
		margin: 0;
		padding: 0;
		box-sizing: border-box;
	}

	:global(html, body) {
		width: 100%;
		height: 100%;
		overflow: hidden;
		background: #000;
	}

	.app {
		width: 100vw;
		height: 100vh;
		position: relative;
	}
</style>
