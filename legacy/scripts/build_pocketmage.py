#!/usr/bin/env python3
"""Build TactileBrowser for PocketMage platform."""

import os
import sys
import subprocess
import shutil
from pathlib import Path

from tui import TUI


def main():
    # Check for CI environment
    ci_mode = "--ci-env" in sys.argv
    tui = TUI(ci_mode=ci_mode)
    
    # Get workspace root (parent of scripts/)
    workspace_root = Path(__file__).parent.parent
    os.chdir(workspace_root)

    pocketmage_dir = workspace_root / "pocketmage-src"
    build_dir = pocketmage_dir / ".pio" / "build"

    tui.banner("PocketMage Build")

    # Setup Python and dependencies
    tui.subsection("Installing PlatformIO")
    tui.command(["python3", "-m", "pip", "install", "--upgrade", "platformio"])
    result = subprocess.run(["python3", "-m", "pip", "install", "--upgrade", "platformio"])
    if result.returncode != 0:
        tui.error("PlatformIO installation failed")
        sys.exit(1)
    tui.success("PlatformIO installed")

    # Build with PlatformIO
    tui.subsection("Building with PlatformIO")
    tui.command(["pio", "run"])
    result = subprocess.run(["pio", "run"], cwd=pocketmage_dir)
    if result.returncode != 0:
        tui.error("PlatformIO build failed")
        sys.exit(1)
    tui.success("Build completed")

    # Prepare browser package
    tui.subsection("Packaging firmware")
    
    browser_icon_src = pocketmage_dir / "browser" / "browser_ICON.bin"
    browser_icon_dst = pocketmage_dir / "browser_ICON.bin"
    shutil.copy2(browser_icon_src, browser_icon_dst)
    tui.success("Copied browser icon")

    # Find firmware.bin in build directory
    firmware_files = list(build_dir.glob("*/firmware.bin"))
    if not firmware_files:
        tui.error("firmware.bin not found in build directory")
        sys.exit(1)

    firmware_src = firmware_files[0]
    firmware_dst = pocketmage_dir / "browser.bin"
    shutil.copy2(firmware_src, firmware_dst)
    tui.success("Copied firmware")

    # Create tar archive
    os.chdir(pocketmage_dir)
    tui.command(["tar", "-cf", "browser.tar", "browser.bin", "browser_ICON.bin"])
    result = subprocess.run(["tar", "-cf", "browser.tar", "browser.bin", "browser_ICON.bin"])
    if result.returncode != 0:
        tui.error("Failed to create archive")
        sys.exit(1)
    tui.success("Archive created")

    # Update latest build
    latest_dir = pocketmage_dir / "latest"
    latest_dir.mkdir(exist_ok=True)
    shutil.copy2("browser.tar", latest_dir / "browser.tar")
    tui.success("Updated latest build")

    # Print results
    tui.section("Build Complete")
    tui.result("Artifact", str(pocketmage_dir / "browser.tar"))


if __name__ == "__main__":
    main()
