#!/usr/bin/env python3
"""Build TactileBrowser for WASM using Emscripten."""

import os
import sys
import subprocess
import shutil
from pathlib import Path

from tui import TUI


def setup_emsdk(workspace_root, tui):
    """Download and setup Emscripten SDK if not already present."""
    emsdk_dir = workspace_root / "emsdk"

    # Check if emcmake is available in PATH or emsdk
    result = subprocess.run(["which", "emcmake"], capture_output=True)
    if result.returncode == 0:
        tui.success("Emscripten available in PATH")
        return

    # Check if emsdk directory exists
    if not emsdk_dir.exists():
        tui.subsection("Downloading Emscripten SDK")
        tui.command(["git", "clone", "--depth", "1", "https://github.com/emscripten-core/emsdk.git", str(emsdk_dir)])
        result = subprocess.run(
            ["git", "clone", "--depth", "1", "https://github.com/emscripten-core/emsdk.git", str(emsdk_dir)]
        )
        if result.returncode != 0:
            tui.error("Failed to clone Emscripten SDK")
            sys.exit(1)
        tui.success("SDK downloaded")
    
    # Install and activate emsdk
    tui.subsection("Installing Emscripten")
    tui.command(["python3", str(emsdk_dir / "emsdk.py"), "install", "latest"])
    result = subprocess.run(
        ["python3", str(emsdk_dir / "emsdk.py"), "install", "latest"]
    )
    if result.returncode != 0:
        tui.error("Installation failed")
        sys.exit(1)
    tui.success("Installation complete")
    
    tui.subsection("Activating Emscripten")
    tui.command(["python3", str(emsdk_dir / "emsdk.py"), "activate", "latest"])
    result = subprocess.run(
        ["python3", str(emsdk_dir / "emsdk.py"), "activate", "latest"]
    )
    if result.returncode != 0:
        tui.error("Activation failed")
        sys.exit(1)
    tui.success("Emscripten setup complete")


def main():
    # Check for CI environment
    ci_mode = "--ci-env" in sys.argv
    tui = TUI(ci_mode=ci_mode)
    
    # Get workspace root
    workspace_root = Path(__file__).parent.parent
    os.chdir(workspace_root)

    wasm_dir = workspace_root / "wasm-src"
    build_dir = wasm_dir / "build"
    dist_dir = wasm_dir / "dist"
    emsdk_dir = workspace_root / "emsdk"

    tui.banner("WASM Build (Emscripten)")

    # Setup Emscripten SDK
    setup_emsdk(workspace_root, tui)
    
    # Source emsdk environment if it exists locally
    env = os.environ.copy()
    emsdk_env_script = emsdk_dir / "emsdk_env.sh"
    if emsdk_env_script.exists():
        tui.info("Loading Emscripten environment")
        result = subprocess.run(
            ["/bin/bash", "-c", f"source {emsdk_env_script} && env"],
            capture_output=True,
            text=True
        )
        if result.returncode == 0:
            for line in result.stdout.splitlines():
                if "=" in line:
                    key, value = line.split("=", 1)
                    env[key] = value
            tui.success("Environment loaded")
    
    # Check for emcmake again
    result = subprocess.run(["which", "emcmake"], capture_output=True, env=env)
    if result.returncode != 0:
        tui.warning("Emscripten toolchain not fully available, build may fail")

    # Build with CMake and Emscripten
    build_dir.mkdir(exist_ok=True)

    tui.subsection("Configuring CMake for WASM")
    tui.command(["emcmake", "cmake", "..", "-DCMAKE_BUILD_TYPE=Release"])
    result = subprocess.run(
        ["emcmake", "cmake", "..", "-DCMAKE_BUILD_TYPE=Release"],
        cwd=build_dir,
        env=env
    )
    if result.returncode != 0:
        tui.error("CMake configuration failed")
        sys.exit(1)
    tui.success("CMake configured")

    tui.subsection("Building WASM")
    tui.command(["emmake", "make", "-j", str(os.cpu_count() or 1)])
    result = subprocess.run(
        ["emmake", "make", "-j", str(os.cpu_count() or 1)],
        cwd=build_dir,
        env=env
    )
    if result.returncode != 0:
        tui.error("Build failed")
        sys.exit(1)
    tui.success("Build complete")

    # Prepare dist directory
    tui.subsection("Preparing distribution")
    dist_dir.mkdir(exist_ok=True)

    # Find and copy WASM files
    wasm_files = list(build_dir.glob("main/TactileBrowserWasm.*"))
    if not wasm_files:
        tui.error("WASM build files not found")
        sys.exit(1)

    for wasm_file in wasm_files:
        shutil.copy2(wasm_file, dist_dir / wasm_file.name)
        tui.success(f"Copied {wasm_file.name}")

    # Copy HTML and glue
    shutil.copy2(
        wasm_dir / "main" / "Source" / "index.html",
        dist_dir / "index.html"
    )
    tui.success("Copied index.html")

    shutil.copy2(
        wasm_dir / "main" / "Source" / "glue.js",
        dist_dir / "glue.js"
    )
    tui.success("Copied glue.js")

    tui.section("Build Complete")
    tui.result("Artifact", str(dist_dir))
    files = [f.name for f in dist_dir.iterdir()]
    tui.result("Files", ", ".join(files))


if __name__ == "__main__":
    main()
