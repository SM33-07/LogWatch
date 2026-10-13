import React, { useRef } from 'react';
import { DEFAULT_SYNTHETIC_LOG, MALFORMED_SYNTHETIC_LOG } from '../lib/fixtures/sample_logs';

interface FileUploadProps {
    currentFilename: string;
    onFileLoaded: (name: string, content: string) => void;
    onError: (msg: string) => void;
}

export function FileUpload({ currentFilename, onFileLoaded, onError }: FileUploadProps) {
    const fileInputRef = useRef<HTMLInputElement>(null);

    const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (event) => {
            const text = event.target?.result;
            if (typeof text === 'string') {
                onFileLoaded(file.name, text);
            }
        };
        reader.onerror = () => {
            onError(`Failed to read file: ${file.name}`);
        };
        reader.readAsText(file);

        // Reset input so same file can be reloaded if needed
        if (fileInputRef.current) {
            fileInputRef.current.value = '';
        }
    };

    return (
        <div className="card" style={{ marginBottom: '1.5rem' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: '1rem' }}>
                <div>
                    <div style={{ fontSize: '0.875rem', fontWeight: 600, color: '#f4f4f5' }}>
                        Dataset Ingestion Source
                    </div>
                    <div style={{ fontSize: '0.8rem', color: '#a1a1aa', marginTop: '0.2rem' }}>
                        Active file:{' '}
                        <code className="font-mono" style={{ color: '#c084fc', background: '#27272a', padding: '0.1rem 0.4rem', borderRadius: '0.25rem' }}>
                            {currentFilename || 'None'}
                        </code>
                    </div>
                </div>

                <div style={{ display: 'flex', gap: '0.5rem', flexWrap: 'wrap', alignItems: 'center' }}>
                    <input
                        ref={fileInputRef}
                        type="file"
                        accept=".log,.txt"
                        onChange={handleFileChange}
                        style={{ display: 'none' }}
                        id="log-file-input"
                    />

                    <label
                        htmlFor="log-file-input"
                        className="btn btn-primary"
                        style={{ cursor: 'pointer', display: 'inline-flex', alignItems: 'center', gap: '0.35rem' }}
                    >
                        <span>📁 Upload Log File</span>
                    </label>

                    <button
                        onClick={() => onFileLoaded('samples/synthetic_access.log (50 records)', DEFAULT_SYNTHETIC_LOG)}
                        className="btn"
                        title="Load canonical 50-row synthetic access log"
                    >
                        ↺ Default 50-Row Sample
                    </button>

                    <button
                        onClick={() => onFileLoaded('fixtures/synthetic_malformed.log', MALFORMED_SYNTHETIC_LOG)}
                        className="btn"
                        title="Load sample with malformed and blank lines to demonstrate invariant"
                    >
                        ⚠ Malformed Fixture Demo
                    </button>
                </div>
            </div>
        </div>
    );
}
