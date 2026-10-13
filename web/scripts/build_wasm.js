#!/usr/bin/env node
/**
 * LogWatch — WebAssembly Build Script for Contributors
 *
 * Compiles the authoritative C11 engine into WebAssembly assets
 * under web/public/wasm/ and syncs with web/lib/wasm/.
 *
 * Prerequisites: Emscripten SDK (emcc)
 */

const { execSync, spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const projectRoot = path.resolve(__dirname, '../..');
const publicWasmDir = path.resolve(__dirname, '../public/wasm');
const libWasmDir = path.resolve(__dirname, '../lib/wasm');

function checkEmcc() {
    try {
        const res = spawnSync('where.exe', ['emcc'], { encoding: 'utf8' });
        if (res.status === 0 && res.stdout.trim().length > 0) {
            return true;
        }
    } catch (e) {
        // Fallback check
    }

    try {
        const res = spawnSync('emcc', ['--version'], { encoding: 'utf8' });
        if (res.status === 0) {
            return true;
        }
    } catch (e) {
        // emcc not directly in PATH
    }

    // Check if C:\emsdk exists
    if (fs.existsSync('C:\\emsdk\\emsdk_env.ps1') || fs.existsSync('C:\\emsdk\\emsdk_env.bat')) {
        return 'emsdk_available';
    }

    return false;
}

const emccStatus = checkEmcc();
if (!emccStatus) {
    console.error('======================================================================');
    console.error('[ERROR] Emscripten compiler (emcc) was not found in PATH or at C:\\emsdk.');
    console.error('The WebAssembly build requires the Emscripten SDK.');
    console.error('Setup steps for contributors:');
    console.error('  1. git clone https://github.com/emscripten-core/emsdk.git C:\\emsdk');
    console.error('  2. cd C:\\emsdk && emsdk.bat install latest && emsdk.bat activate latest');
    console.error('  3. In your active shell, run: C:\\emsdk\\emsdk_env.bat (or .ps1)');
    console.error('======================================================================');
    process.exit(1);
}

console.log('[BUILD] Setting up output directories...');
if (!fs.existsSync(publicWasmDir)) {
    fs.mkdirSync(publicWasmDir, { recursive: true });
}
if (!fs.existsSync(libWasmDir)) {
    fs.mkdirSync(libWasmDir, { recursive: true });
}

console.log('[BUILD] Compiling C source files to WebAssembly via emcc...');
const srcFiles = [
    'src/wasm_bridge.c',
    'src/log_array.c',
    'src/log_entry.c',
    'src/parser.c',
    'src/sort.c',
    'src/search.c'
].map(f => path.resolve(projectRoot, f));

const exportedFuncs = "['_bridge_init','_bridge_load_from_vfs','_bridge_load_file','_bridge_clear','_bridge_free','_bridge_get_size','_bridge_get_capacity','_bridge_get_stats_json','_bridge_get_records_json','_bridge_sort','_bridge_compare_all_sorts','_bridge_search_response_time','_bridge_search_threshold','_bridge_search_status','_bridge_search_route','_bridge_free_string','_malloc','_free']";
const exportedMethods = "['ccall','cwrap','UTF8ToString','stringToUTF8','FS','lengthBytesUTF8']";

const targetJs = path.resolve(publicWasmDir, 'logwatch_wasm.js');

let emccCmd = 'emcc';
let envPrefix = '';
if (emccStatus === 'emsdk_available') {
    // If not in current PATH, invoke via emsdk_env in powershell
    envPrefix = '& C:\\emsdk\\emsdk_env.ps1; ';
}

const compileCmd = `${envPrefix}emcc -std=c11 -Wall -Wextra -pedantic -I"${path.resolve(projectRoot, 'include')}" -O2 ${srcFiles.map(f => `"${f}"`).join(' ')} -s MODULARIZE=1 -s EXPORT_NAME="createLogWatchModule" -s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 -s NO_EXIT_RUNTIME=1 -s "EXPORTED_FUNCTIONS=${exportedFuncs}" -s "EXPORTED_RUNTIME_METHODS=${exportedMethods}" -o "${targetJs}"`;

try {
    execSync(compileCmd, {
        cwd: projectRoot,
        stdio: 'inherit',
        shell: 'powershell.exe'
    });
} catch (err) {
    console.error('[ERROR] Compilation failed:', err.message);
    process.exit(1);
}

// Copy to web/lib/wasm
fs.copyFileSync(targetJs, path.resolve(libWasmDir, 'logwatch_wasm.js'));
const targetWasm = path.resolve(publicWasmDir, 'logwatch_wasm.wasm');
if (fs.existsSync(targetWasm)) {
    fs.copyFileSync(targetWasm, path.resolve(libWasmDir, 'logwatch_wasm.wasm'));
}

console.log('======================================================================');
console.log('[SUCCESS] WebAssembly assets successfully generated:');
console.log(`  - ${path.relative(projectRoot, targetJs)}`);
console.log(`  - ${path.relative(projectRoot, targetWasm)}`);
console.log('[NOTICE] Contributor Reminder: You must commit these pre-compiled assets');
console.log('         under web/public/wasm/ whenever C algorithms or bridge code change');
console.log('         so standard end-users can clone and run without Emscripten.');
console.log('======================================================================');
