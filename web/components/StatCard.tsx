import React from 'react';

interface StatCardProps {
    label: string;
    value: string | number;
    color?: string;
    sublabel?: string;
}

export function StatCard({ label, value, color, sublabel }: StatCardProps) {
    return (
        <div className="card">
            <div style={{ fontSize: '0.75rem', color: '#a1a1aa', textTransform: 'uppercase', letterSpacing: '0.05em' }}>
                {label}
            </div>
            <div
                className="font-mono"
                style={{
                    fontSize: '1.5rem',
                    fontWeight: 700,
                    marginTop: '0.25rem',
                    color: color || '#f4f4f5',
                }}
            >
                {value}
            </div>
            {sublabel && (
                <div style={{ fontSize: '0.75rem', color: '#71717a', marginTop: '0.25rem' }}>
                    {sublabel}
                </div>
            )}
        </div>
    );
}
