# TactileBrowser Build Scripts

## Quick Start

### macOS (Desktop)
```bash
python3 scripts/build_macos.py
```

### PocketMage (PlatformIO)
```bash
python3 scripts/build_pocketmage.py
```

### WASM (Emscripten)
```bash
python3 scripts/build_wasm.py
```

#### Serve the WASM build
```bash
python3 scripts/serve_wasm.py           # rebuilds and serves wasm-src/dist
python3 scripts/serve_wasm.py --no-build --port 8080  # reuse artifacts, custom port
```

### Tactility (ESP-IDF v5.5)

First, set up ESP-IDF and install dependencies:
```bash
python3 scripts/setup_esp_idf.py
```

Then build:
```bash
python3 scripts/build_tactility.py
```

### Download Tactility SDK 
(Don't use unless you specifically need a custom SDK)

```bash
GITHUB_TOKEN=your_token python3 scripts/download_sdk.py
```

## Build Artifacts

All scripts output artifacts to platform-specific directories:

- **macOS**: `dist/TactileBrowser.app/`
- **PocketMage**: `pocketmage-src/browser.tar`
- **WASM**: `wasm-src/dist/`
- **Tactility**: `tactility-src/build/TactileBrowser.app`