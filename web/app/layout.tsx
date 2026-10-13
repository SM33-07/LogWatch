import type { Metadata } from 'next';
import './globals.css';

export const metadata: Metadata = {
    title: 'LogWatch — Access Log Analyzer [SYNTHETIC]',
    description: 'Data Structures and Algorithms Web Laboratory powered by pure ISO C11 WebAssembly',
};

export default function RootLayout({
    children,
}: {
    children: React.ReactNode;
}) {
    return (
        <html lang="en" suppressHydrationWarning>
            <body className="min-h-screen antialiased" suppressHydrationWarning>
                {children}
            </body>
        </html>
    );
}
