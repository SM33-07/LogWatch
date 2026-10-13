import React, { useState } from 'react';
import {
    LogWatchBridge,
    SortResult,
    ComparisonBenchmark,
    SearchResult,
} from '../lib/wasm/bridge_client';
import { RecordTable } from './RecordTable';

interface AlgorithmLabProps {
    bridge: LogWatchBridge | null;
    totalRecords: number;
    onDatasetSorted: () => void;
    onError: (msg: string) => void;
}

type LabTab = 'sort' | 'compare' | 'search';

export function AlgorithmLab({
    bridge,
    totalRecords,
    onDatasetSorted,
    onError,
}: AlgorithmLabProps) {
    const [activeTab, setActiveTab] = useState<LabTab>('sort');

    // --- SORT STATE ---
    const [algoId, setAlgoId] = useState<number>(0); // 0=Bubble, 1=Selection, 2=Insertion
    const [sortKey, setSortKey] = useState<number>(3); // 3=Response Time
    const [ascending, setAscending] = useState<boolean>(true);
    const [sortResult, setSortResult] = useState<SortResult | null>(null);
    const [isSorting, setIsSorting] = useState<boolean>(false);

    // --- COMPARISON BENCHMARK STATE ---
    const [compareKey, setCompareKey] = useState<number>(3);
    const [compareAscending, setCompareAscending] = useState<boolean>(true);
    const [benchmark, setBenchmark] = useState<ComparisonBenchmark | null>(null);
    const [isComparing, setIsComparing] = useState<boolean>(false);

    // --- SEARCH STATE ---
    const [searchType, setSearchType] = useState<
        'response_time' | 'threshold' | 'status' | 'route_sub' | 'route_prefix'
    >('threshold');
    const [searchParamNumber, setSearchParamNumber] = useState<string>('100');
    const [searchParamRoute, setSearchParamRoute] = useState<string>('/api/snippets');
    const [searchCaseSensitive, setSearchCaseSensitive] = useState<boolean>(false);
    const [searchResult, setSearchResult] = useState<SearchResult | null>(null);
    const [isSearching, setIsSearching] = useState<boolean>(false);
    const [searchError, setSearchError] = useState<string | null>(null);

    // --- SORT HANDLER ---
    const handleExecuteSort = () => {
        if (!bridge) return;
        setIsSorting(true);
        try {
            const res = bridge.sort(algoId, sortKey, ascending);
            if (!res.success) {
                onError(res.error || 'Sort failed in C WebAssembly engine.');
            } else {
                setSortResult(res);
                onDatasetSorted();
            }
        } catch (err: any) {
            onError(err.message || 'Error occurred during C WebAssembly sort.');
        } finally {
            setIsSorting(false);
        }
    };

    // --- COMPARISON HANDLER ---
    const handleExecuteCompare = () => {
        if (!bridge) return;
        setIsComparing(true);
        try {
            const res = bridge.compareAllSorts(compareKey, compareAscending);
            if (!res.success) {
                onError(res.error || 'Comparison benchmark failed in C WebAssembly engine.');
            } else {
                setBenchmark(res);
            }
        } catch (err: any) {
            onError(err.message || 'Error occurred during comparison benchmark.');
        } finally {
            setIsComparing(false);
        }
    };

    // --- SEARCH HANDLER ---
    const handleExecuteSearch = () => {
        if (!bridge) return;
        setSearchError(null);
        setIsSearching(true);

        try {
            let res: SearchResult | null = null;
            if (searchType === 'response_time') {
                const val = parseInt(searchParamNumber, 10);
                if (isNaN(val) || val < 0) {
                    setSearchError('Response time must be a non-negative integer (ms).');
                    setIsSearching(false);
                    return;
                }
                res = bridge.searchResponseTime(val);
            } else if (searchType === 'threshold') {
                const val = parseInt(searchParamNumber, 10);
                if (isNaN(val) || val < 0) {
                    setSearchError('Threshold must be a non-negative integer (ms).');
                    setIsSearching(false);
                    return;
                }
                res = bridge.searchThreshold(val);
            } else if (searchType === 'status') {
                const val = parseInt(searchParamNumber, 10);
                if (isNaN(val) || val < 100 || val > 599) {
                    setSearchError('HTTP status code must be an integer between 100 and 599.');
                    setIsSearching(false);
                    return;
                }
                res = bridge.searchStatus(val);
            } else if (searchType === 'route_sub') {
                if (!searchParamRoute.trim()) {
                    setSearchError('Route search string cannot be empty.');
                    setIsSearching(false);
                    return;
                }
                res = bridge.searchRoute(searchParamRoute.trim(), false, searchCaseSensitive);
            } else if (searchType === 'route_prefix') {
                if (!searchParamRoute.trim()) {
                    setSearchError('Route prefix cannot be empty.');
                    setIsSearching(false);
                    return;
                }
                res = bridge.searchRoute(searchParamRoute.trim(), true, searchCaseSensitive);
            }

            if (res) {
                setSearchResult(res);
            }
        } catch (err: any) {
            setSearchError(err.message || 'Error during linear search query.');
        } finally {
            setIsSearching(false);
        }
    };

    const hasRecords = totalRecords > 0;

    // Helper for relative comparison bar widths
    const getMaxComparisons = () => {
        if (!benchmark || !benchmark.algorithms.length) return 1;
        return Math.max(...benchmark.algorithms.map((a) => a.comparisons), 1);
    };

    const getMaxMoves = () => {
        if (!benchmark || !benchmark.algorithms.length) return 1;
        return Math.max(
            ...benchmark.algorithms.map((a) => Math.max(a.record_swaps, a.record_shifts)),
            1
        );
    };

    return (
        <section className="card" style={{ marginBottom: '1.5rem', padding: '1.25rem' }}>
            {/* Tab Navigation */}
            <div
                style={{
                    display: 'flex',
                    justifyContent: 'space-between',
                    alignItems: 'center',
                    borderBottom: '1px solid #27272a',
                    paddingBottom: '0.75rem',
                    marginBottom: '1.25rem',
                    flexWrap: 'wrap',
                    gap: '0.75rem',
                }}
            >
                <div style={{ display: 'flex', gap: '0.5rem' }}>
                    <button
                        onClick={() => setActiveTab('sort')}
                        className={`btn ${activeTab === 'sort' ? 'btn-primary' : ''}`}
                    >
                        ⚡ Sorting Engine
                    </button>
                    <button
                        onClick={() => setActiveTab('compare')}
                        className={`btn ${activeTab === 'compare' ? 'btn-primary' : ''}`}
                    >
                        ⚖ Algorithm Comparison
                    </button>
                    <button
                        onClick={() => setActiveTab('search')}
                        className={`btn ${activeTab === 'search' ? 'btn-primary' : ''}`}
                    >
                        🔍 Linear Search Engine
                    </button>
                </div>
                <div style={{ fontSize: '0.75rem', color: '#a1a1aa' }}>
                    Authoritative C11 WebAssembly Execution
                </div>
            </div>

            {/* TAB 1: SORTING CONTROLS */}
            {activeTab === 'sort' && (
                <div>
                    <div
                        style={{
                            display: 'grid',
                            gridTemplateColumns: 'repeat(auto-fit, minmax(180px, 1fr))',
                            gap: '1rem',
                            marginBottom: '1.25rem',
                        }}
                    >
                        {/* Algorithm Selector */}
                        <div>
                            <label style={{ display: 'block', fontSize: '0.75rem', color: '#a1a1aa', marginBottom: '0.3rem', textTransform: 'uppercase' }}>
                                Algorithm
                            </label>
                            <select
                                value={algoId}
                                onChange={(e) => setAlgoId(Number(e.target.value))}
                                className="btn"
                                style={{ width: '100%', padding: '0.45rem 0.6rem', background: '#18181b', color: '#f4f4f5' }}
                            >
                                <option value={0}>Bubble Sort (Early Exit)</option>
                                <option value={1}>Selection Sort (Min Swaps)</option>
                                <option value={2}>Insertion Sort (Adaptive Shifts)</option>
                            </select>
                        </div>

                        {/* Sort Key Selector */}
                        <div>
                            <label style={{ display: 'block', fontSize: '0.75rem', color: '#a1a1aa', marginBottom: '0.3rem', textTransform: 'uppercase' }}>
                                Sort Key
                            </label>
                            <select
                                value={sortKey}
                                onChange={(e) => setSortKey(Number(e.target.value))}
                                className="btn"
                                style={{ width: '100%', padding: '0.45rem 0.6rem', background: '#18181b', color: '#f4f4f5' }}
                            >
                                <option value={0}>Timestamp (ISO-8601)</option>
                                <option value={1}>Route</option>
                                <option value={2}>Status Code</option>
                                <option value={3}>Response Time (ms)</option>
                                <option value={4}>Client ID</option>
                            </select>
                        </div>

                        {/* Direction Toggle */}
                        <div>
                            <label style={{ display: 'block', fontSize: '0.75rem', color: '#a1a1aa', marginBottom: '0.3rem', textTransform: 'uppercase' }}>
                                Direction
                            </label>
                            <select
                                value={ascending ? 'asc' : 'desc'}
                                onChange={(e) => setAscending(e.target.value === 'asc')}
                                className="btn"
                                style={{ width: '100%', padding: '0.45rem 0.6rem', background: '#18181b', color: '#f4f4f5' }}
                            >
                                <option value="asc">Ascending ↑</option>
                                <option value="desc">Descending ↓</option>
                            </select>
                        </div>

                        {/* Sort Button */}
                        <div style={{ display: 'flex', alignItems: 'flex-end' }}>
                            <button
                                onClick={handleExecuteSort}
                                disabled={!hasRecords || isSorting}
                                className="btn btn-primary"
                                style={{ width: '100%', padding: '0.45rem 1rem' }}
                            >
                                {isSorting ? 'Sorting in C...' : 'Execute Sort'}
                            </button>
                        </div>
                    </div>

                    {/* Sort Result Metrics Card */}
                    {sortResult && (
                        <div
                            style={{
                                background: '#121215',
                                border: '1px solid #3f3f46',
                                borderRadius: '0.5rem',
                                padding: '1rem',
                                marginTop: '1rem',
                            }}
                        >
                            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '0.75rem' }}>
                                <span style={{ fontSize: '0.875rem', fontWeight: 600, color: '#c084fc' }}>
                                    ✓ {sortResult.algorithm} Finished on Active LogArray
                                </span>
                                <span style={{ fontSize: '0.75rem', color: '#71717a' }}>
                                    Key: <strong style={{ color: '#e4e4e7' }}>{sortResult.sort_key}</strong> ({sortResult.ascending ? 'Ascending' : 'Descending'})
                                </span>
                            </div>

                            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(140px, 1fr))', gap: '0.75rem' }}>
                                <div style={{ background: '#18181b', padding: '0.6rem 0.8rem', borderRadius: '0.375rem', border: '1px solid #27272a' }}>
                                    <div style={{ fontSize: '0.7rem', color: '#a1a1aa', textTransform: 'uppercase' }}>Comparisons</div>
                                    <div className="font-mono" style={{ fontSize: '1.2rem', fontWeight: 700, color: '#38bdf8' }}>
                                        {sortResult.comparisons.toLocaleString()}
                                    </div>
                                </div>
                                <div style={{ background: '#18181b', padding: '0.6rem 0.8rem', borderRadius: '0.375rem', border: '1px solid #27272a' }}>
                                    <div style={{ fontSize: '0.7rem', color: '#a1a1aa', textTransform: 'uppercase' }}>Record Swaps</div>
                                    <div className="font-mono" style={{ fontSize: '1.2rem', fontWeight: 700, color: '#f59e0b' }}>
                                        {sortResult.record_swaps.toLocaleString()}
                                    </div>
                                </div>
                                <div style={{ background: '#18181b', padding: '0.6rem 0.8rem', borderRadius: '0.375rem', border: '1px solid #27272a' }}>
                                    <div style={{ fontSize: '0.7rem', color: '#a1a1aa', textTransform: 'uppercase' }}>Record Shifts</div>
                                    <div className="font-mono" style={{ fontSize: '1.2rem', fontWeight: 700, color: '#a855f7' }}>
                                        {sortResult.record_shifts.toLocaleString()}
                                    </div>
                                </div>
                                <div style={{ background: '#18181b', padding: '0.6rem 0.8rem', borderRadius: '0.375rem', border: '1px solid #27272a' }}>
                                    <div style={{ fontSize: '0.7rem', color: '#a1a1aa', textTransform: 'uppercase' }}>Elapsed Time</div>
                                    <div className="font-mono" style={{ fontSize: '1.2rem', fontWeight: 700, color: '#34d399' }}>
                                        {(sortResult.elapsed_sec * 1000).toFixed(3)} ms
                                    </div>
                                    <div style={{ fontSize: '0.65rem', color: '#71717a' }}>Informational timer</div>
                                </div>
                            </div>

                            <div style={{ fontSize: '0.75rem', color: '#71717a', marginTop: '0.75rem' }}>
                                * Record Integrity guaranteed: Complete ~364-byte records moved. Ties broken deterministically by source line number.
                            </div>
                        </div>
                    )}
                </div>
            )}

            {/* TAB 2: ALGORITHM COMPARISON BENCHMARK */}
            {activeTab === 'compare' && (
                <div>
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '1rem', flexWrap: 'wrap', gap: '0.75rem' }}>
                        <div>
                            <div style={{ fontSize: '0.875rem', fontWeight: 600, color: '#f4f4f5' }}>
                                Non-Destructive Benchmark on Cloned Inputs
                            </div>
                            <div style={{ fontSize: '0.8rem', color: '#a1a1aa', marginTop: '0.2rem' }}>
                                Executes Bubble, Selection, and Insertion Sort on three independent copies via <code>log_array_clone</code>. Active array is preserved.
                            </div>
                        </div>

                        <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'center' }}>
                            <select
                                value={compareKey}
                                onChange={(e) => setCompareKey(Number(e.target.value))}
                                className="btn"
                                style={{ background: '#18181b', color: '#f4f4f5', fontSize: '0.8rem' }}
                            >
                                <option value={3}>Key: Response Time</option>
                                <option value={2}>Key: Status Code</option>
                                <option value={1}>Key: Route</option>
                                <option value={0}>Key: Timestamp</option>
                                <option value={4}>Key: Client ID</option>
                            </select>

                            <select
                                value={compareAscending ? 'asc' : 'desc'}
                                onChange={(e) => setCompareAscending(e.target.value === 'asc')}
                                className="btn"
                                style={{ background: '#18181b', color: '#f4f4f5', fontSize: '0.8rem' }}
                            >
                                <option value="asc">Ascending ↑</option>
                                <option value="desc">Descending ↓</option>
                            </select>

                            <button
                                onClick={handleExecuteCompare}
                                disabled={!hasRecords || isComparing}
                                className="btn btn-primary"
                                style={{ fontSize: '0.8rem' }}
                            >
                                {isComparing ? 'Benchmarking...' : 'Run 3-Way Comparison'}
                            </button>
                        </div>
                    </div>

                    {benchmark && benchmark.algorithms && (
                        <div>
                            {/* Algorithm Cards Grid */}
                            <div
                                style={{
                                    display: 'grid',
                                    gridTemplateColumns: 'repeat(auto-fit, minmax(240px, 1fr))',
                                    gap: '1rem',
                                    marginBottom: '1.25rem',
                                }}
                            >
                                {benchmark.algorithms.map((algo) => (
                                    <div
                                        key={algo.name}
                                        style={{
                                            background: '#121215',
                                            border: '1px solid #27272a',
                                            borderRadius: '0.5rem',
                                            padding: '1rem',
                                        }}
                                    >
                                        <div style={{ fontSize: '1rem', fontWeight: 700, color: '#f4f4f5', marginBottom: '0.5rem' }}>
                                            {algo.name}
                                        </div>

                                        <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem', fontSize: '0.85rem' }}>
                                            <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                                                <span style={{ color: '#a1a1aa' }}>Comparisons:</span>
                                                <strong className="font-mono" style={{ color: '#38bdf8' }}>
                                                    {algo.comparisons.toLocaleString()}
                                                </strong>
                                            </div>
                                            <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                                                <span style={{ color: '#a1a1aa' }}>Record Swaps:</span>
                                                <strong className="font-mono" style={{ color: '#f59e0b' }}>
                                                    {algo.record_swaps.toLocaleString()}
                                                </strong>
                                            </div>
                                            <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                                                <span style={{ color: '#a1a1aa' }}>Record Shifts:</span>
                                                <strong className="font-mono" style={{ color: '#a855f7' }}>
                                                    {algo.record_shifts.toLocaleString()}
                                                </strong>
                                            </div>
                                            <div style={{ display: 'flex', justifyContent: 'space-between', borderTop: '1px solid #27272a', paddingTop: '0.4rem' }}>
                                                <span style={{ color: '#a1a1aa' }}>Elapsed:</span>
                                                <span className="font-mono" style={{ color: '#34d399' }}>
                                                    {(algo.elapsed_sec * 1000).toFixed(3)} ms
                                                </span>
                                            </div>
                                        </div>
                                    </div>
                                ))}
                            </div>

                            {/* Horizontal Comparison Bars */}
                            <div style={{ background: '#121215', border: '1px solid #27272a', borderRadius: '0.5rem', padding: '1rem' }}>
                                <div style={{ fontSize: '0.8rem', fontWeight: 600, color: '#a1a1aa', textTransform: 'uppercase', marginBottom: '0.75rem' }}>
                                    Pairwise Comparisons Comparison Bar
                                </div>
                                <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
                                    {benchmark.algorithms.map((algo) => {
                                        const pct = Math.max(4, Math.round((algo.comparisons / getMaxComparisons()) * 100));
                                        return (
                                            <div key={algo.name} style={{ display: 'flex', alignItems: 'center', gap: '0.75rem', fontSize: '0.8rem' }}>
                                                <span style={{ width: '110px', color: '#e4e4e7' }}>{algo.name}:</span>
                                                <div style={{ flex: 1, background: '#27272a', height: '12px', borderRadius: '6px', overflow: 'hidden' }}>
                                                    <div style={{ width: `${pct}%`, background: '#38bdf8', height: '100%', borderRadius: '6px' }} />
                                                </div>
                                                <span className="font-mono" style={{ width: '60px', textAlign: 'right', color: '#a1a1aa' }}>
                                                    {algo.comparisons}
                                                </span>
                                            </div>
                                        );
                                    })}
                                </div>

                                <div style={{ fontSize: '0.8rem', fontWeight: 600, color: '#a1a1aa', textTransform: 'uppercase', marginTop: '1rem', marginBottom: '0.75rem' }}>
                                    Record Moves (Swaps vs Shifts) Comparison Bar
                                </div>
                                <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
                                    {benchmark.algorithms.map((algo) => {
                                        const moves = Math.max(algo.record_swaps, algo.record_shifts);
                                        const pct = Math.max(4, Math.round((moves / getMaxMoves()) * 100));
                                        const color = algo.record_shifts > 0 ? '#a855f7' : '#f59e0b';
                                        return (
                                            <div key={algo.name} style={{ display: 'flex', alignItems: 'center', gap: '0.75rem', fontSize: '0.8rem' }}>
                                                <span style={{ width: '110px', color: '#e4e4e7' }}>{algo.name}:</span>
                                                <div style={{ flex: 1, background: '#27272a', height: '12px', borderRadius: '6px', overflow: 'hidden' }}>
                                                    <div style={{ width: `${pct}%`, background: color, height: '100%', borderRadius: '6px' }} />
                                                </div>
                                                <span className="font-mono" style={{ width: '60px', textAlign: 'right', color: '#a1a1aa' }}>
                                                    {moves} {algo.record_shifts > 0 ? '(shifts)' : '(swaps)'}
                                                </span>
                                            </div>
                                        );
                                    })}
                                </div>
                            </div>
                        </div>
                    )}
                </div>
            )}

            {/* TAB 3: LINEAR SEARCH ENGINE */}
            {activeTab === 'search' && (
                <div>
                    <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem', marginBottom: '1.25rem' }}>
                        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(200px, 1fr))', gap: '0.75rem' }}>
                            {/* Search Type Selector */}
                            <div>
                                <label style={{ display: 'block', fontSize: '0.75rem', color: '#a1a1aa', marginBottom: '0.3rem', textTransform: 'uppercase' }}>
                                    Query Capability
                                </label>
                                <select
                                    value={searchType}
                                    onChange={(e) => {
                                        setSearchType(e.target.value as any);
                                        setSearchError(null);
                                    }}
                                    className="btn"
                                    style={{ width: '100%', padding: '0.45rem 0.6rem', background: '#18181b', color: '#f4f4f5' }}
                                >
                                    <option value="threshold">Response Time Threshold (&gt;= ms)</option>
                                    <option value="response_time">Exact Response Time (== ms)</option>
                                    <option value="status">Exact HTTP Status Code (== code)</option>
                                    <option value="route_sub">Route Substring Match</option>
                                    <option value="route_prefix">Route Prefix Match</option>
                                </select>
                            </div>

                            {/* Query Parameter Input */}
                            <div>
                                <label style={{ display: 'block', fontSize: '0.75rem', color: '#a1a1aa', marginBottom: '0.3rem', textTransform: 'uppercase' }}>
                                    Search Query Target
                                </label>
                                {searchType === 'response_time' || searchType === 'threshold' || searchType === 'status' ? (
                                    <input
                                        type="number"
                                        value={searchParamNumber}
                                        onChange={(e) => setSearchParamNumber(e.target.value)}
                                        placeholder={searchType === 'status' ? 'e.g. 200' : 'e.g. 100'}
                                        className="btn"
                                        style={{ width: '100%', padding: '0.45rem 0.6rem', background: '#18181b', color: '#f4f4f5', textAlign: 'left' }}
                                    />
                                ) : (
                                    <input
                                        type="text"
                                        value={searchParamRoute}
                                        onChange={(e) => setSearchParamRoute(e.target.value)}
                                        placeholder="e.g. /api/snippets"
                                        className="btn"
                                        style={{ width: '100%', padding: '0.45rem 0.6rem', background: '#18181b', color: '#f4f4f5', textAlign: 'left' }}
                                    />
                                )}
                            </div>

                            {/* Case Sensitive Option (for routes) & Search Button */}
                            <div style={{ display: 'flex', alignItems: 'flex-end', gap: '0.5rem' }}>
                                {(searchType === 'route_sub' || searchType === 'route_prefix') && (
                                    <label style={{ display: 'flex', alignItems: 'center', gap: '0.3rem', fontSize: '0.8rem', color: '#a1a1aa', marginBottom: '0.4rem' }}>
                                        <input
                                            type="checkbox"
                                            checked={searchCaseSensitive}
                                            onChange={(e) => setSearchCaseSensitive(e.target.checked)}
                                        />
                                        Case-sensitive
                                    </label>
                                )}

                                <button
                                    onClick={handleExecuteSearch}
                                    disabled={!hasRecords || isSearching}
                                    className="btn btn-primary"
                                    style={{ flex: 1, padding: '0.45rem 1rem' }}
                                >
                                    {isSearching ? 'Scanning...' : 'Execute Search'}
                                </button>
                            </div>
                        </div>

                        {searchError && (
                            <div style={{ color: '#f87171', fontSize: '0.85rem' }}>
                                ⚠ {searchError}
                            </div>
                        )}
                    </div>

                    {/* Search Results Display */}
                    {searchResult && (
                        <div style={{ background: '#121215', border: '1px solid #27272a', borderRadius: '0.5rem', padding: '1rem', marginTop: '1rem' }}>
                            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '0.75rem', flexWrap: 'wrap', gap: '0.5rem' }}>
                                <div>
                                    <span style={{ fontSize: '0.9rem', fontWeight: 600, color: '#34d399' }}>
                                        Linear Search Complete: Found {searchResult.matches_found} matching record(s)
                                    </span>
                                    <span style={{ fontSize: '0.75rem', color: '#a1a1aa', marginLeft: '0.5rem' }}>
                                        ({searchResult.comparisons} comparisons evaluated in C)
                                    </span>
                                </div>
                                <button
                                    onClick={() => setSearchResult(null)}
                                    className="btn"
                                    style={{ padding: '0.2rem 0.5rem', fontSize: '0.75rem' }}
                                >
                                    ✕ Clear Results
                                </button>
                            </div>

                            {/* Search Matches Table */}
                            <div style={{ border: '1px solid #27272a', borderRadius: '0.375rem', overflow: 'hidden' }}>
                                <RecordTable records={searchResult.records} />
                            </div>

                            <div style={{ fontSize: '0.75rem', color: '#71717a', marginTop: '0.5rem' }}>
                                * Multi-match linear search preserves source order without modifying dataset memory.
                            </div>
                        </div>
                    )}
                </div>
            )}
        </section>
    );
}
