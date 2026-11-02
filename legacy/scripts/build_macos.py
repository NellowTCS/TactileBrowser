#!/usr/bin/env python3
"""Build TactileBrowser for macOS (Universal binary)."""

import os
import sys
import subprocess
import shutil
from pathlib import Path

from tui import TUI


def setup_sdl2(workspace_root, tui):
    """Download and build SDL2 if not already present."""
    sdl2_source = workspace_root / "sdl2-source"
    sdl2_build = workspace_root / "sdl2-build-universal"
    sdl2_universal = workspace_root / "sdl2-universal"

    # Clone SDL2 if missing
    if not sdl2_source.exists():
        tui.subsection("Cloning SDL2 source")
        tui.command(["git", "clone", "-b", "SDL2", "https://github.com/libsdl-org/SDL.git", str(sdl2_source)])
        result = subprocess.run(
            ["git", "clone", "-b", "SDL2", "https://github.com/libsdl-org/SDL.git", str(sdl2_source)]
        )
        if result.returncode != 0:
            tui.error("Failed to clone SDL2")
            sys.exit(1)
        tui.success("SDL2 cloned")

    # Build SDL2 if missing
    if not (sdl2_build / "install" / "lib" / "libSDL2.dylib").exists():
        sdl2_build.mkdir(parents=True, exist_ok=True)

        tui.subsection("Configuring SDL2 for universal binary")
        tui.command(["cmake", str(sdl2_source), "-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64", "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_INSTALL_PREFIX={sdl2_build / 'install'}"])
        result = subprocess.run(
            [
                "cmake", str(sdl2_source),
                "-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64",
                "-DCMAKE_BUILD_TYPE=Release",
                f"-DCMAKE_INSTALL_PREFIX={sdl2_build / 'install'}"
            ],
            cwd=sdl2_build
        )
        if result.returncode != 0:
            tui.error("CMake configuration failed")
            sys.exit(1)
        tui.success("CMake configuration complete")

        tui.subsection("Building SDL2")
        tui.command(["make", "-j", str(os.cpu_count() or 1)])
        result = subprocess.run(
            ["make", "-j", str(os.cpu_count() or 1)],
            cwd=sdl2_build
        )
        if result.returncode != 0:
            tui.error("SDL2 build failed")
            sys.exit(1)
        tui.success("SDL2 built")

        tui.subsection("Installing SDL2")
        result = subprocess.run(
            ["make", "install"],
            cwd=sdl2_build
        )
        if result.returncode != 0:
            tui.error("SDL2 installation failed")
            sys.exit(1)

        # Verify build
        dylib_path = sdl2_build / "install" / "lib" / "libSDL2-2.0.0.dylib"
        if not dylib_path.exists():
            tui.error("SDL2 build verification failed - dylib not found")
            sys.exit(1)
        tui.success("SDL2 verified")

    # Setup universal SDL2 directory
    sdl2_universal.mkdir(exist_ok=True)
    (sdl2_universal / "lib").mkdir(exist_ok=True)
    (sdl2_universal / "include" / "SDL2").mkdir(parents=True, exist_ok=True)

    tui.subsection("Setting up SDL2 directories")
    
    # Copy SDL2 dylib
    shutil.copy2(
        sdl2_build / "install" / "lib" / "libSDL2-2.0.0.dylib",
        sdl2_universal / "lib" / "libSDL2.dylib"
    )

    # Copy headers
    for header in (sdl2_source / "include").glob("*"):
        if header.is_file():
            shutil.copy2(header, sdl2_universal / "include" / "SDL2" / header.name)
            shutil.copy2(header, sdl2_universal / "include" / header.name)
        elif header.is_dir():
            dest_dir = sdl2_universal / "include" / "SDL2" / header.name
            if dest_dir.exists():
                shutil.rmtree(dest_dir)
            shutil.copytree(header, dest_dir)
            dest_dir2 = sdl2_universal / "include" / header.name
            if dest_dir2.exists():
                shutil.rmtree(dest_dir2)
            shutil.copytree(header, dest_dir2)

    # Create symlink for versioned dylib
    lib_dir = sdl2_universal / "lib"
    versioned_link = lib_dir / "libSDL2-2.0.0.dylib"
    if versioned_link.exists():
        versioned_link.unlink()
    versioned_link.symlink_to("libSDL2.dylib")

    tui.success("SDL2 setup complete")


def main():
    # Check for CI environment
    ci_mode = "--ci-env" in sys.argv
    tui = TUI(ci_mode=ci_mode)
    
    # Get workspace root
    workspace_root = Path(__file__).parent.parent
    os.chdir(workspace_root)

    desktop_dir = workspace_root / "desktop-src"
    sdl2_universal = workspace_root / "sdl2-universal"
    dist_dir = workspace_root / "dist"

    tui.banner("macOS Universal Build")

    # Setup SDL2
    setup_sdl2(workspace_root, tui)

    # Build with CMake
    tui.subsection("Configuring CMake")
    build_dir = desktop_dir / "build"
    build_dir.mkdir(exist_ok=True)

    tui.command(["cmake", "..", "-DCMAKE_BUILD_TYPE=Release", f"-DSDL2_LIBRARY={sdl2_universal / 'lib' / 'libSDL2.dylib'}", f"-DSDL2_INCLUDE_DIR={sdl2_universal / 'include'}"])
    result = subprocess.run(
        [
            "cmake", "..",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DSDL2_LIBRARY={sdl2_universal / 'lib' / 'libSDL2.dylib'}",
            f"-DSDL2_INCLUDE_DIR={sdl2_universal / 'include'}",
            "-DCMAKE_VERBOSE_MAKEFILE=ON"
        ],
        cwd=build_dir
    )
    if result.returncode != 0:
        tui.error("CMake configuration failed")
        sys.exit(1)
    tui.success("CMake configured")

    tui.subsection("Building with Make")
    tui.command(["make", "VERBOSE=1"])
    result = subprocess.run(
        ["make", "VERBOSE=1"],
        cwd=build_dir
    )
    if result.returncode != 0:
        tui.error("Build failed")
        sys.exit(1)
    tui.success("Build complete")

    # Bundle SDL2 dylib with app
    tui.subsection("Bundling SDL2 framework")
    
    app_path = build_dir / "main" / "TactileBrowser.app"
    frameworks_dir = app_path / "Contents" / "Frameworks"
    frameworks_dir.mkdir(parents=True, exist_ok=True)

    shutil.copy2(
        sdl2_universal / "lib" / "libSDL2.dylib",
        frameworks_dir / "libSDL2.dylib"
    )

    # Create versioned symlink
    versioned_link = frameworks_dir / "libSDL2-2.0.0.dylib"
    if versioned_link.exists():
        versioned_link.unlink()
    versioned_link.symlink_to("libSDL2.dylib")

    # Fix dylib paths in executable
    tui.subsection("Fixing dylib paths")
    tui.command(["install_name_tool", "-change", "@rpath/libSDL2-2.0.0.dylib", "@executable_path/../Frameworks/libSDL2-2.0.0.dylib", str(app_path / "Contents" / "MacOS" / "TactileBrowser")])
    result = subprocess.run(
        [
            "install_name_tool",
            "-change", "@rpath/libSDL2-2.0.0.dylib",
            "@executable_path/../Frameworks/libSDL2-2.0.0.dylib",
            str(app_path / "Contents" / "MacOS" / "TactileBrowser")
        ]
    )
    if result.returncode != 0:
        tui.error("Failed to fix dylib paths")
        sys.exit(1)
    tui.success("Dylib paths fixed")

    # Create distribution
    tui.subsection("Creating distribution")
    dist_dir.mkdir(exist_ok=True)
    dist_app = dist_dir / "TactileBrowser.app"
    if dist_app.exists():
        shutil.rmtree(dist_app)
    shutil.copytree(app_path, dist_app)

    tui.section("Build Complete")
    tui.result("Artifact", str(dist_app))


if __name__ == "__main__":
    main()
