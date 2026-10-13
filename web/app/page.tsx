'use client';

import React, { useEffect, useState, useCallback } from 'react';
import { LogWatchBridge, IngestionStats, RecordsPage } from '../lib/wasm/bridge_client';
import { DEFAULT_SYNTHETIC_LOG } from '../lib/fixtures/sample_logs';
import { StatCard } from '../components/StatCard';
import { FileUpload } from '../components/FileUpload';
import { RecordTable } from '../components/RecordTable';
import { Pagination } from '../components/Pagination';
import { AlgorithmLab } from '../components/AlgorithmLab';

export default function Home() {
    const [bridge, setBridge] = useState<LogWatchBridge | null>(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState<string | null>(null);
    const [stats, setStats] = useState<IngestionStats | null>(null);
    const [page, setPage] = useState<RecordsPage | null>(null);
    const [currentFilename, setCurrentFilename] = useState<string>('samples/synthetic_access.log (50 records)');
    const [pageIndex, setPageIndex] = useState<number>(0);
    const [pageSize, setPageSize] = useState<number>(10);

    const refreshDataset = useCallback((b: LogWatchBridge, newPageIndex: number, newPageSize: number) => {
        try {
            const currentStats = b.getStats();
            setStats(currentStats);
            const offset = newPageIndex * newPageSize;
            const recordsPage = b.getRecords(offset, newPageSize);
            setPage(recordsPage);
        } catch (err: any) {
            setError(err.message || 'Failed to query WebAssembly bridge.');
        }
    }, []);

    useEffect(() => {
        let isMounted = true;
        LogWatchBridge.create()
            .then((b) => {
                if (!isMounted) return;
                setBridge(b);
                b.loadFromText('/samples/synthetic_access.log', DEFAULT_SYNTHETIC_LOG);
                refreshDataset(b, 0, 10);
                setLoading(false);
            })
            .catch((err) => {
                if (!isMounted) return;
                setError(err.message || 'Failed to initialize WebAssembly engine.');
                setLoading(false);
            });

        return () => {
            isMounted = false;
        };
    }, [refreshDataset]);

    const handleFileLoaded = (filename: string, content: string) => {
        if (!bridge) return;
        setError(null);
        try {
            const ok = bridge.loadFromText(`/${filename}`, content);
            if (!ok) {
                setError(`Parser rejected file "${filename}". Previous dataset preserved.`);
            }
            setCurrentFilename(filename);
            setPageIndex(0);
            refreshDataset(bridge, 0, pageSize);
        } catch (err: any) {
            setError(err.message || 'Error occurred while loading log file into C runtime.');
        }
    };

    const handlePageChange = (newPageIndex: number) => {
        if (!bridge) return;
        setPageIndex(newPageIndex);
        refreshDataset(bridge, newPageIndex, pageSize);
    };

    const handlePageSizeChange = (newPageSize: number) => {
        if (!bridge) return;
        setPageSize(newPageSize);
        setPageIndex(0);
        refreshDataset(bridge, 0, newPageSize);
    };

    return (
        <main style={{ maxWidth: '1100px', margin: '0 auto', padding: '2rem 1.25rem' }}>
            {/* Header with Academic Provenance Badge */}
            <header
                style={{
                    display: 'flex',
                    justifyContent: 'space-between',
                    alignItems: 'center',
                    marginBottom: '2rem',
                    borderBottom: '1px solid #27272a',
                    paddingBottom: '1.25rem',
                    flexWrap: 'wrap',
                    gap: '1rem',
                }}
            >
                <div>
                    <h1 style={{ margin: 0, fontSize: '1.5rem', fontWeight: 700, letterSpacing: '-0.02em' }}>
                        LogWatch Access Log Analyzer
                    </h1>
                    <p style={{ margin: '0.25rem 0 0', color: '#a1a1aa', fontSize: '0.875rem' }}>
                        CSC2101 DSA Pedagogical Laboratory — Authoritative ISO C11 WebAssembly Engine
                    </p>
                </div>
                <div>
                    <span className="badge-synthetic">[SYNTHETIC DATA]</span>
                </div>
            </header>

            {/* Error Banner */}
            {error && (
                <div
                    style={{
                        backgroundColor: 'rgba(239, 68, 68, 0.12)',
                        border: '1px solid #ef4444',
                        borderRadius: '0.5rem',
                        padding: '1rem 1.25rem',
                        color: '#fca5a5',
                        marginBottom: '1.5rem',
                    }}
                >
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                        <h3 style={{ margin: 0, fontSize: '0.95rem', fontWeight: 600, color: '#f87171' }}>
                            WebAssembly Engine Error
                        </h3>
                        <button
                            onClick={() => setError(null)}
                            className="btn"
                            style={{ padding: '0.15rem 0.5rem', fontSize: '0.75rem' }}
                        >
                            ✕ Dismiss
                        </button>
                    </div>
                    <pre
                        style={{
                            margin: '0.5rem 0 0',
                            whiteSpace: 'pre-wrap',
                            fontFamily: 'inherit',
                            fontSize: '0.85rem',
                            color: '#fecaca',
                        }}
                    >
                        {error}
                    </pre>
                </div>
            )}

            {/* Loading State */}
            {loading && !error && (
                <div
                    style={{
                        textAlign: 'center',
                        padding: '4rem 2rem',
                        color: '#a1a1aa',
                        display: 'flex',
                        flexDirection: 'column',
                        alignItems: 'center',
                        gap: '0.75rem',
                    }}
                >
                    <div style={{ fontSize: '1.2rem', color: '#c084fc' }}>⚙</div>
                    <div>Initializing ISO C11 WebAssembly runtime engine...</div>
                    <div style={{ fontSize: '0.8rem', color: '#71717a' }}>
                        Loading /wasm/logwatch_wasm.wasm into browser linear memory
                    </div>
                </div>
            )}

            {!loading && stats && (
                <>
                    {/* Section 1: File Upload & Preset Switcher */}
                    <FileUpload
                        currentFilename={currentFilename}
                        onFileLoaded={handleFileLoaded}
                        onError={(msg) => setError(msg)}
                    />

                    {/* Section 2: Overview / Dataset Ingestion KPIs */}
                    <section style={{ marginBottom: '1.5rem' }}>
                        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '0.75rem' }}>
                            <h2 style={{ fontSize: '1rem', fontWeight: 600, margin: 0, color: '#f4f4f5' }}>
                                Ingestion Accounting Overview
                            </h2>
                            <span style={{ fontSize: '0.75rem', color: '#71717a' }}>
                                Canonical formula: total = valid + malformed + blank
                            </span>
                        </div>

                        <div
                            style={{
                                display: 'grid',
                                gridTemplateColumns: 'repeat(auto-fit, minmax(180px, 1fr))',
                                gap: '0.875rem',
                            }}
                        >
                            <StatCard
                                label="Valid Records"
                                value={stats.valid_records}
                                color="#34d399"
                                sublabel="Loaded into LogArray"
                            />
                            <StatCard
                                label="Malformed Lines"
                                value={stats.malformed_lines}
                                color={stats.malformed_lines > 0 ? '#fbbf24' : '#a1a1aa'}
                                sublabel="Safely skipped"
                            />
                            <StatCard
                                label="Blank Lines"
                                value={stats.blank_lines}
                                color="#a1a1aa"
                                sublabel="Whitespace rows"
                            />
                            <StatCard
                                label="Total Physical Lines"
                                value={stats.total_lines}
                                color="#e4e4e7"
                                sublabel="Lines ingested"
                            />
                            <StatCard
                                label="Accounting Invariant"
                                value={stats.invariant_verified ? 'VERIFIED' : 'FAILED'}
                                color={stats.invariant_verified ? '#34d399' : '#f87171'}
                                sublabel={stats.invariant_verified ? 'Invariant holds' : 'Accounting mismatch'}
                            />
                        </div>
                    </section>

                    {/* Section 3: Minimal Algorithm Lab */}
                    <AlgorithmLab
                        bridge={bridge}
                        totalRecords={stats.valid_records}
                        onDatasetSorted={() => {
                            setPageIndex(0);
                            if (bridge) refreshDataset(bridge, 0, pageSize);
                        }}
                        onError={(msg) => setError(msg)}
                    />

                    {/* Section 4: Log Explorer Table Preview & Pagination */}
                    <section className="card" style={{ padding: 0, overflow: 'hidden' }}>
                        <div
                            style={{
                                display: 'flex',
                                justifyContent: 'space-between',
                                alignItems: 'center',
                                padding: '1rem 1.25rem',
                                borderBottom: '1px solid #27272a',
                                flexWrap: 'wrap',
                                gap: '0.75rem',
                            }}
                        >
                            <div>
                                <h2 style={{ fontSize: '1rem', fontWeight: 600, margin: 0, color: '#f4f4f5' }}>
                                    Log Explorer (Authoritative C11 Dynamic Array)
                                </h2>
                                <p style={{ fontSize: '0.8rem', color: '#a1a1aa', margin: '0.2rem 0 0' }}>
                                    High-density tabular record inspect from WebAssembly memory
                                </p>
                            </div>
                            <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'center' }}>
                                <span style={{ fontSize: '0.75rem', color: '#71717a' }}>
                                    Heap capacity: <strong className="font-mono" style={{ color: '#e4e4e7' }}>{stats.capacity}</strong> slots
                                </span>
                            </div>
                        </div>

                        {/* Record Table */}
                        <RecordTable records={page?.records || []} />

                        {/* Record Pagination */}
                        <Pagination
                            currentPage={pageIndex}
                            pageSize={pageSize}
                            totalRecords={stats.valid_records}
                            onPageChange={handlePageChange}
                            onPageSizeChange={handlePageSizeChange}
                        />
                    </section>
                </>
            )}
        </main>
    );
}
