#!/usr/bin/env python3
"""Setup ESP-IDF v5.5 environment (install dependencies and tools)."""

import os
import sys
import subprocess
import platform
from pathlib import Path

from tui import TUI


def install_system_dependencies(tui):
    """Install system-level dependencies for ESP-IDF."""
    tui.subsection("Installing system dependencies")
    
    system = platform.system()
    
    # Skip on non-Linux systems
    if system != "Linux":
        tui.info(f"Skipping automatic dependency install on {system}")
        tui.info("Please ensure you have: git, cmake, ninja, python3, libffi-dev, libssl-dev, libusb-1.0-0")
        return
    
    deps = [
        "git", "wget", "flex", "bison", "gperf",
        "python3", "python3-pip", "python3-venv",
        "cmake", "ninja-build", "ccache",
        "libffi-dev", "libssl-dev", "dfu-util", "libusb-1.0-0"
    ]
    
    tui.command(["sudo", "apt-get", "update"])
    result = subprocess.run(["sudo", "apt-get", "update"], capture_output=True)
    if result.returncode != 0:
        tui.warning("apt-get update had issues, continuing anyway")
    
    tui.command(["sudo", "apt-get", "install", "-y"] + deps)
    result = subprocess.run(
        ["sudo", "apt-get", "install", "-y"] + deps,
        capture_output=True
    )
    if result.returncode != 0:
        tui.warning("Some dependencies may not have installed, but continuing")
    else:
        tui.success("System dependencies installed")


def setup_esp_idf(workspace_root, tui):
    """Download and setup ESP-IDF v5.5."""
    esp_idf_dir = workspace_root / "esp-idf"
    
    # Check if ESP-IDF already exists with export script
    if esp_idf_dir.exists() and (esp_idf_dir / "export.sh").exists():
        tui.success("ESP-IDF already present")
    else:
        tui.subsection("Downloading ESP-IDF v5.5.1 with submodules")
        tui.command(["git", "clone", "--branch", "v5.5.1", "--depth", "1", "--recursive",
             "https://github.com/espressif/esp-idf.git", str(esp_idf_dir)])
        result = subprocess.run(
            ["git", "clone", "--branch", "v5.5.1", "--depth", "1", "--recursive",
             "https://github.com/espressif/esp-idf.git", str(esp_idf_dir)]
        )
        if result.returncode != 0:
            tui.error("Failed to clone ESP-IDF")
            sys.exit(1)
        tui.success("ESP-IDF downloaded")
    
    # Install ESP-IDF tools for ESP32 and ESP32-S3 targets
    tui.subsection("Installing ESP-IDF build tools for ESP32 and ESP32-S3")
    tui.info("This may take several minutes...")
    
    install_script = esp_idf_dir / "install.sh"
    shell_cmd = f"bash {install_script} esp32,esp32s3"
    tui.command(["bash", str(install_script), "esp32,esp32s3"])
    result = subprocess.run(
        shell_cmd,
        shell=True,
        cwd=esp_idf_dir,
        capture_output=True,
        text=True
    )
    
    if result.returncode != 0:
        # Check if it's just the libusb warning
        if "libusb-1.0" in result.stderr or "openocd" in result.stderr:
            tui.warning("ESP-IDF install had issues with openocd (missing libusb), but core tools should be OK")
        else:
            tui.error("ESP-IDF install failed")
            print(result.stderr[-500:])  # Print last 500 chars of error
            sys.exit(1)
    else:
        tui.success("ESP-IDF tools installed")
    
    tui.success("ESP-IDF setup complete")


def main():
    # Check for CI environment
    ci_mode = "--ci-env" in sys.argv
    tui = TUI(ci_mode=ci_mode)
    
    workspace_root = Path(__file__).parent.parent
    os.chdir(workspace_root)
    
    tui.banner("ESP-IDF Setup")
    
    # Install system deps
    install_system_dependencies(tui)
    
    # Setup ESP-IDF
    setup_esp_idf(workspace_root, tui)
    
    tui.section("Setup Complete")
    tui.info("You can now run: python3 scripts/build_tactility.py")


if __name__ == "__main__":
    main()
