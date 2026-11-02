#!/usr/bin/env python3
"""Serve the WASM build artifacts with correct MIME types."""

import argparse
import os
import subprocess
import sys
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


def build_wasm(workspace_root: Path) -> None:
    """Run the existing WASM build script."""
    build_script = workspace_root / "scripts" / "build_wasm.py"
    print("[serve-wasm] Building WASM artifacts...")
    result = subprocess.run([sys.executable, str(build_script)])
    if result.returncode != 0:
        raise SystemExit(result.returncode)


def create_handler() -> type[SimpleHTTPRequestHandler]:
    """Create a handler that knows about .wasm files."""
    class WasmRequestHandler(SimpleHTTPRequestHandler):
        extensions_map = dict(SimpleHTTPRequestHandler.extensions_map)
        extensions_map.update({
            ".wasm": "application/wasm",
            ".js": "application/javascript",
            ".mjs": "application/javascript",
        })

    return WasmRequestHandler


def main() -> None:
    parser = argparse.ArgumentParser(description="Serve the TactileBrowser WASM build output.")
    parser.add_argument("--host", default="127.0.0.1", help="Host interface to bind (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8000, help="Port to listen on (default: 8000)")
    parser.add_argument(
        "--no-build",
        action="store_true",
        help="Skip rebuilding before serving (use existing wasm-src/dist).",
    )
    args = parser.parse_args()

    workspace_root = Path(__file__).resolve().parent.parent
    dist_dir = workspace_root / "wasm-src" / "dist"

    if not args.no_build:
        build_wasm(workspace_root)

    if not dist_dir.exists():
        print("[serve-wasm] Error: wasm-src/dist does not exist. Run build_wasm.py first.", file=sys.stderr)
        raise SystemExit(1)

    os.chdir(dist_dir)
    handler_cls = create_handler()

    server = ThreadingHTTPServer((args.host, args.port), handler_cls)
    url = f"http://{args.host}:{args.port}/"
    print(f"[serve-wasm] Serving {dist_dir} at {url}")
    print("[serve-wasm] Press Ctrl+C to stop.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[serve-wasm] Shutting down...")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
