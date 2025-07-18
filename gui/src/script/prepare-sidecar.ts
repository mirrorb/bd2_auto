import fs from 'fs-extra';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const projectRoot = path.resolve(__dirname, '..', '..', '..');

const cppBuildDir = path.join(projectRoot, 'core', 'build');

const sidecarDestDir = path.join(projectRoot, 'gui', 'src-tauri', 'core');

const mode = process.argv[2];
if (!mode) {
    console.error('[Sidecar Prep] FATAL ERROR: Build mode argument is missing.');
    console.error('[Sidecar Prep] Expected command format: `tsx prepare-sidecar.ts [debug|release]`');
    process.exit(1);
}
if (mode !== 'debug' && mode !== 'release') {
    console.error(`[Sidecar Prep] FATAL ERROR: Invalid build mode argument "${mode}".`);
    console.error('[Sidecar Prep] Must be either "debug" or "release".');
    process.exit(1);
}


const sourceDir = path.join(cppBuildDir, mode, 'bin', 'core');

console.log(`[Sidecar Prep] Starting preparation for ${mode} mode.`);
console.log(`[Sidecar Prep] Project Root: ${projectRoot}`);
console.log(`[Sidecar Prep] Source Path: ${sourceDir}`);
console.log(`[Sidecar Prep] Destination Path: ${sidecarDestDir}`);

try {
    fs.emptyDirSync(sidecarDestDir);
    console.log('[Sidecar Prep] Destination directory cleaned.');

    if (!fs.existsSync(sourceDir)) {
        console.error(`\n[Sidecar Prep] FATAL ERROR: Source directory not found!`);
        console.error(`[Sidecar Prep] > Directory "${sourceDir}" does not exist.`);
        console.error(`[Sidecar Prep] > Please make sure you have successfully compiled the C++ project in the 'core' directory first.\n`);
        process.exit(1);
    }

    fs.copySync(sourceDir, sidecarDestDir);
    console.log(`[Sidecar Prep] Successfully copied artifacts from ${path.basename(sourceDir)}.`);

} catch (error) {
    console.error('[Sidecar Prep] An unexpected error occurred during file operations:', error);
    process.exit(1);
}

console.log('[Sidecar Prep] Preparation complete. Tauri may now proceed.');
