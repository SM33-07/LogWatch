/**
 * LogWatch — Authoritative WebAssembly Bridge Client
 *
 * Exposes a strongly-typed TypeScript interface to the compiled C11 engine.
 *
 * Strict Non-Negotiable Rules:
 * 1. Zero JavaScript Algorithm Duplication: All DSA logic executes directly in C.
 * 2. Missing WASM Guard: If WebAssembly runtime assets are missing, throws an explicit error.
 *    Never silently fallback to JavaScript sorting or searching.
 */

declare const __non_webpack_require__: any;

export interface IngestionStats {
    provenance: string;
    filepath: string;
    has_file: boolean;
    size: number;
    capacity: number;
    total_lines: number;
    valid_records: number;
    malformed_lines: number;
    blank_lines: number;
    invariant_verified: boolean;
}

export interface LogRecord {
    line: number;
    timestamp: string;
    route: string;
    status: number;
    response_time_ms: number;
    client_id: string;
}

export interface RecordsPage {
    provenance: string;
    total_count: number;
    offset: number;
    limit: number;
    count: number;
    records: LogRecord[];
}

export interface SortResult {
    provenance: string;
    success: boolean;
    algorithm: string;
    sort_key: string;
    ascending: boolean;
    comparisons: number;
    record_swaps: number;
    record_shifts: number;
    elapsed_sec: number;
    error?: string;
}

export interface AlgorithmMetric {
    name: string;
    comparisons: number;
    record_swaps: number;
    record_shifts: number;
    elapsed_sec: number;
}

export interface ComparisonBenchmark {
    provenance: string;
    success: boolean;
    record_count: number;
    sort_key: string;
    ascending: boolean;
    algorithms: AlgorithmMetric[];
    error?: string;
}

export interface SearchResult {
    provenance: string;
    success: boolean;
    query_type: string;
    matches_found: number;
    comparisons: number;
    elapsed_sec: number;
    records: LogRecord[];
}

export class LogWatchBridge {
    private module: any;

    private constructor(wasmModule: any) {
        this.module = wasmModule;
    }

    /**
     * Initializes the LogWatch C WebAssembly runtime module.
     * Guaranteed clone-and-run: Loads from /wasm/logwatch_wasm.wasm.
     * Throws an explicit error if assets are missing.
     */
    public static async create(): Promise<LogWatchBridge> {
        if (typeof window === 'undefined') {
            // Server-side environment: load using filesystem
            const path = await import('path');
            const fs = await import('fs');
            const wasmPath = path.resolve(process.cwd(), 'public/wasm/logwatch_wasm.wasm');
            const jsPath = path.resolve(process.cwd(), 'public/wasm/logwatch_wasm.js');

            if (!fs.existsSync(wasmPath) || !fs.existsSync(jsPath)) {
                throw new Error(
                    '[LogWatch Engine Error] Required WebAssembly runtime assets are missing at web/public/wasm/.\n' +
                    'In accordance with LogWatch architectural rules, C algorithms will NOT be substituted with JavaScript.\n' +
                    'Please verify that web/public/wasm/logwatch_wasm.wasm is present or run "npm run build:wasm" with Emscripten installed.'
                );
            }

            const req = typeof __non_webpack_require__ !== 'undefined' ? __non_webpack_require__ : eval('require');
            const createModule = req(jsPath);
            const mod = await createModule();
            mod._bridge_init();
            return new LogWatchBridge(mod);
        }

        // Browser environment: load script and initialize
        return new Promise<LogWatchBridge>((resolve, reject) => {
            const scriptId = 'logwatch-wasm-loader';
            let script = document.getElementById(scriptId) as HTMLScriptElement | null;

            const onScriptReady = async () => {
                const createModule = (window as any).createLogWatchModule;
                if (typeof createModule !== 'function') {
                    return reject(new Error(
                        '[LogWatch Engine Error] createLogWatchModule was not found on window.\n' +
                        'Ensure web/public/wasm/logwatch_wasm.js is properly served under /wasm/.'
                    ));
                }

                try {
                    const mod = await createModule({
                        locateFile: (filePath: string, prefix: string) => {
                            if (filePath.endsWith('.wasm')) {
                                return '/wasm/' + filePath;
                            }
                            return prefix + filePath;
                        }
                    });
                    mod._bridge_init();
                    resolve(new LogWatchBridge(mod));
                } catch (err: any) {
                    reject(new Error(
                        `[LogWatch Engine Error] Failed to initialize WebAssembly engine: ${err.message}\n` +
                        'C algorithms will NOT be substituted with JavaScript.'
                    ));
                }
            };

            if ((window as any).createLogWatchModule) {
                onScriptReady();
                return;
            }

            if (!script) {
                script = document.createElement('script');
                script.id = scriptId;
                script.src = '/wasm/logwatch_wasm.js';
                document.body.appendChild(script);
            }

            script.addEventListener('load', onScriptReady);
            script.addEventListener('error', () => {
                reject(new Error(
                    '[LogWatch Engine Error] Failed to load WebAssembly loader script from /wasm/logwatch_wasm.js.\n' +
                    'Please verify web/public/wasm/ assets exist.'
                ));
            });
        });
    }

    private callJson(cFuncName: string, ...args: any[]): any {
        const ptr = this.module.ccall(cFuncName, 'number', args.map(() => 'number'), args);
        if (!ptr) {
            throw new Error(`[LogWatch Engine Error] C function ${cFuncName} returned NULL pointer.`);
        }
        const jsonStr = this.module.UTF8ToString(ptr);
        this.module._bridge_free_string(ptr);
        return JSON.parse(jsonStr);
    }

    public loadFromText(virtualPath: string, content: string): boolean {
        const basename = virtualPath.split('/').filter(Boolean).pop() || 'current.log';
        const cleanPath = '/' + basename;
        this.module.FS.writeFile(cleanPath, content);
        return this.module.ccall('bridge_load_from_vfs', 'number', ['string'], [cleanPath]) === 1;
    }

    public getSize(): number {
        return this.module._bridge_get_size();
    }

    public getCapacity(): number {
        return this.module._bridge_get_capacity();
    }

    public getStats(): IngestionStats {
        return this.callJson('bridge_get_stats_json');
    }

    public getRecords(offset: number, limit: number): RecordsPage {
        const ptr = this.module.ccall('bridge_get_records_json', 'number', ['number', 'number'], [offset, limit]);
        const jsonStr = this.module.UTF8ToString(ptr);
        this.module._bridge_free_string(ptr);
        return JSON.parse(jsonStr);
    }

    public sort(algoId: number, sortKey: number, ascending: boolean): SortResult {
        const ptr = this.module.ccall('bridge_sort', 'number', ['number', 'number', 'number'], [algoId, sortKey, ascending ? 1 : 0]);
        const jsonStr = this.module.UTF8ToString(ptr);
        this.module._bridge_free_string(ptr);
        return JSON.parse(jsonStr);
    }

    public compareAllSorts(sortKey: number, ascending: boolean): ComparisonBenchmark {
        const ptr = this.module.ccall('bridge_compare_all_sorts', 'number', ['number', 'number'], [sortKey, ascending ? 1 : 0]);
        const jsonStr = this.module.UTF8ToString(ptr);
        this.module._bridge_free_string(ptr);
        return JSON.parse(jsonStr);
    }

    public searchResponseTime(targetMs: number): SearchResult {
        const ptr = this.module.ccall('bridge_search_response_time', 'number', ['number'], [targetMs]);
        const jsonStr = this.module.UTF8ToString(ptr);
        this.module._bridge_free_string(ptr);
        return JSON.parse(jsonStr);
    }

    public searchThreshold(thresholdMs: number): SearchResult {
        const ptr = this.module.ccall('bridge_search_threshold', 'number', ['number'], [thresholdMs]);
        const jsonStr = this.module.UTF8ToString(ptr);
        this.module._bridge_free_string(ptr);
        return JSON.parse(jsonStr);
    }

    public searchStatus(statusCode: number): SearchResult {
        const ptr = this.module.ccall('bridge_search_status', 'number', ['number'], [statusCode]);
        const jsonStr = this.module.UTF8ToString(ptr);
        this.module._bridge_free_string(ptr);
        return JSON.parse(jsonStr);
    }

    public searchRoute(query: string, prefixOnly: boolean, caseSensitive: boolean): SearchResult {
        const ptr = this.module.ccall(
            'bridge_search_route',
            'number',
            ['string', 'number', 'number'],
            [query, prefixOnly ? 1 : 0, caseSensitive ? 1 : 0]
        );
        const jsonStr = this.module.UTF8ToString(ptr);
        this.module._bridge_free_string(ptr);
        return JSON.parse(jsonStr);
    }

    public clear(): void {
        this.module._bridge_clear();
    }

    public free(): void {
        this.module._bridge_free();
    }
}
