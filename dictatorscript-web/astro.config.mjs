// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

import tailwindcss from '@tailwindcss/vite';

// https://astro.build/config
export default defineConfig({
    integrations: [
        starlight({
            title: 'DictatorScript',
            customCss: ['./src/styles/global.css'],
            favicon: '/favicon.ico',
            head: [
                { tag: 'link', attrs: { rel: 'icon', type: 'image/png', href: '/icon.png' } },
                { tag: 'link', attrs: { rel: 'apple-touch-icon', href: '/icon.png' } },
            ],
            social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/TaherMustansir1929/DictatorScript' }],
            sidebar: [
                {
                    label: 'Getting Started',
                    items: [
                        { label: 'Installation', slug: 'getting-started/installation' },
                        { label: 'First Program', slug: 'getting-started/first-program' },
                    ],
                },
                {
                    label: 'Language Reference',
                    items: [
                        { label: 'Basics', slug: 'language/basics' },
                        { label: 'Control Flow', slug: 'language/control-flow' },
                        { label: 'Functions', slug: 'language/functions' },
                        { label: 'Memory Management', slug: 'language/memory' },
                        { label: 'Advanced Features', slug: 'language/advanced' },
                    ],
                },
                {
                    label: 'Tooling',
                    items: [
                        { label: 'VS Code Extension', slug: 'tooling/vscode' },
                    ],
                },
            ],
        }),
    ],

    vite: {
        plugins: [tailwindcss()],
    },
});