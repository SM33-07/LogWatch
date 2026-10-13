import React from 'react';
import { LogRecord } from '../lib/wasm/bridge_client';

interface RecordTableProps {
    records: LogRecord[];
}

export function RecordTable({ records }: RecordTableProps) {
    if (records.length === 0) {
        return (
            <div style={{ textAlign: 'center', padding: '3rem 1rem', color: '#71717a', fontSize: '0.9rem' }}>
                No records loaded. Select a synthetic preset or upload a log file.
            </div>
        );
    }

    const getStatusColor = (status: number) => {
        if (status >= 500) return '#f87171'; // 5xx Server Error
        if (status >= 400) return '#fbbf24'; // 4xx Client Error
        if (status >= 300) return '#38bdf8'; // 3xx Redirect
        return '#34d399'; // 2xx Success
    };

    return (
        <div style={{ overflowX: 'auto' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: '0.85rem' }}>
                <thead>
                    <tr style={{ borderBottom: '1px solid #27272a', textAlign: 'left', color: '#a1a1aa' }}>
                        <th style={{ padding: '0.6rem 0.75rem', width: '50px' }}>Line</th>
                        <th style={{ padding: '0.6rem 0.75rem', width: '180px' }}>Timestamp</th>
                        <th style={{ padding: '0.6rem 0.75rem' }}>Route</th>
                        <th style={{ padding: '0.6rem 0.75rem', width: '70px' }}>Status</th>
                        <th style={{ padding: '0.6rem 0.75rem', width: '110px' }}>Duration (ms)</th>
                        <th style={{ padding: '0.6rem 0.75rem', width: '110px' }}>Client ID</th>
                    </tr>
                </thead>
                <tbody className="font-mono">
                    {records.map((r, i) => (
                        <tr
                            key={`${r.line}-${i}`}
                            className="table-row-hover"
                            style={{ borderBottom: '1px solid #1f1f23' }}
                        >
                            <td style={{ padding: '0.5rem 0.75rem', color: '#71717a' }}>{r.line}</td>
                            <td style={{ padding: '0.5rem 0.75rem', color: '#e4e4e7' }}>{r.timestamp}</td>
                            <td style={{ padding: '0.5rem 0.75rem', color: '#c084fc' }}>{r.route}</td>
                            <td style={{ padding: '0.5rem 0.75rem', fontWeight: 600, color: getStatusColor(r.status) }}>
                                {r.status}
                            </td>
                            <td style={{ padding: '0.5rem 0.75rem', color: '#e4e4e7' }}>{r.response_time_ms}</td>
                            <td style={{ padding: '0.5rem 0.75rem', color: '#a1a1aa' }}>{r.client_id}</td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
}
