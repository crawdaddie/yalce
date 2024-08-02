import { defineConfig } from 'vite'
import { nodePolyfills } from 'vite-plugin-node-polyfills'
import { viteStaticCopy } from 'vite-plugin-static-copy'

export default defineConfig({
  plugins: [nodePolyfills(),viteStaticCopy({
      targets: [
        {
          src: 'assets/jit.wasm',
          dest: 'assets'
        }
      ]
    })],
  define: {
    global: 'globalThis',
  },
  build: {
    target: 'esnext', // This enables support for top-level await
    outDir: '../docs',
    assetsDir: 'assets',
    rollupOptions: {
      input: {
        main: 'index.html',
      },
    },
  },
})
