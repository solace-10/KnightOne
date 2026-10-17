import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig } from 'vite';

export default defineConfig({
	plugins: [sveltekit()],
	server: {
		port: 6174,
		// UI assets shared with the game.
		fs: { allow: ['../game/assets/fonts', '../game/bin/data/core/ui/icons'] }
	}
});
