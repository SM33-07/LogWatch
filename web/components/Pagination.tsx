import React from 'react';

interface PaginationProps {
    currentPage: number;
    pageSize: number;
    totalRecords: number;
    onPageChange: (newPage: number) => void;
    onPageSizeChange: (newPageSize: number) => void;
}

export function Pagination({
    currentPage,
    pageSize,
    totalRecords,
    onPageChange,
    onPageSizeChange,
}: PaginationProps) {
    const totalPages = Math.max(1, Math.ceil(totalRecords / pageSize));
    const startRecord = totalRecords === 0 ? 0 : currentPage * pageSize + 1;
    const endRecord = Math.min((currentPage + 1) * pageSize, totalRecords);

    return (
        <div
            style={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center',
                padding: '0.75rem 1rem',
                borderTop: '1px solid #27272a',
                fontSize: '0.875rem',
                color: '#a1a1aa',
                flexWrap: 'wrap',
                gap: '0.75rem',
            }}
        >
            <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
                <span>
                    Showing <strong style={{ color: '#f4f4f5' }}>{startRecord}</strong>–
                    <strong style={{ color: '#f4f4f5' }}>{endRecord}</strong> of{' '}
                    <strong style={{ color: '#f4f4f5' }}>{totalRecords}</strong> records
                </span>

                <label style={{ display: 'flex', alignItems: 'center', gap: '0.4rem', fontSize: '0.8rem' }}>
                    <span>Per page:</span>
                    <select
                        value={pageSize}
                        onChange={(e) => onPageSizeChange(Number(e.target.value))}
                        className="btn-select"
                        style={{
                            background: '#18181b',
                            border: '1px solid #3f3f46',
                            color: '#f4f4f5',
                            padding: '0.2rem 0.5rem',
                            borderRadius: '0.25rem',
                            fontSize: '0.8rem',
                            cursor: 'pointer',
                        }}
                    >
                        <option value={5}>5</option>
                        <option value={10}>10</option>
                        <option value={20}>20</option>
                        <option value={50}>50</option>
                    </select>
                </label>
            </div>

            <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                <button
                    onClick={() => onPageChange(currentPage - 1)}
                    disabled={currentPage <= 0}
                    className="btn"
                    style={{
                        padding: '0.35rem 0.75rem',
                        fontSize: '0.8rem',
                        opacity: currentPage <= 0 ? 0.4 : 1,
                        cursor: currentPage <= 0 ? 'not-allowed' : 'pointer',
                    }}
                >
                    ← Previous
                </button>

                <span style={{ fontSize: '0.8rem', padding: '0 0.5rem' }}>
                    Page <strong style={{ color: '#f4f4f5' }}>{currentPage + 1}</strong> of{' '}
                    <strong style={{ color: '#f4f4f5' }}>{totalPages}</strong>
                </span>

                <button
                    onClick={() => onPageChange(currentPage + 1)}
                    disabled={currentPage >= totalPages - 1}
                    className="btn"
                    style={{
                        padding: '0.35rem 0.75rem',
                        fontSize: '0.8rem',
                        opacity: currentPage >= totalPages - 1 ? 0.4 : 1,
                        cursor: currentPage >= totalPages - 1 ? 'not-allowed' : 'pointer',
                    }}
                >
                    Next →
                </button>
            </div>
        </div>
    );
}
