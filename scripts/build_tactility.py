#!/usr/bin/env python3
"""Build TactileBrowser for Tactility (ESP-IDF) platform."""

import os
import sys
import subprocess
import shutil
from pathlib import Path

from tui import TUI


def setup_esp_idf(workspace_root, tui):
    """Download and setup ESP-IDF v5.5 if not already present."""
    esp_idf_dir = workspace_root / "esp-idf"
    
    # Check if ESP-IDF already exists
    if esp_idf_dir.exists() and (esp_idf_dir / "tools" / "idf.py").exists():
        tui.success("ESP-IDF already installed")
        return esp_idf_dir

    tui.subsection("Downloading ESP-IDF v5.5")
    tui.command(["git", "clone", "--branch", "v5.5", "--depth", "1", 
         "https://github.com/espressif/esp-idf.git", str(esp_idf_dir)])
    result = subprocess.run(
        ["git", "clone", "--branch", "v5.5", "--depth", "1", 
         "https://github.com/espressif/esp-idf.git", str(esp_idf_dir)]
    )
    if result.returncode != 0:
        tui.error("Failed to clone ESP-IDF")
        sys.exit(1)
    tui.success("ESP-IDF downloaded")
    
    # Install ESP-IDF tools
    tui.subsection("Installing ESP-IDF tools")
    
    install_script = esp_idf_dir / "tools" / "idf_tools.py"
    tui.command(["python3", str(install_script), "install"])
    result = subprocess.run(
        ["python3", str(install_script), "install"],
        cwd=esp_idf_dir
    )
    if result.returncode != 0:
        tui.error("Failed to install ESP-IDF tools")
        sys.exit(1)
    
    tui.success("ESP-IDF setup complete")
    return esp_idf_dir


def main():
    # Check for CI environment
    ci_mode = "--ci-env" in sys.argv
    tui = TUI(ci_mode=ci_mode)
    
    # Get workspace root
    workspace_root = Path(__file__).parent.parent
    os.chdir(workspace_root)

    tactility_dir = workspace_root / "tactility-src"

    tui.banner("Tactility Build (ESP-IDF v5.5)")

    # Setup ESP-IDF
    esp_idf_path = setup_esp_idf(workspace_root, tui)
    
    # Setup ESP-IDF environment
    env = os.environ.copy()
    env["IDF_PATH"] = str(esp_idf_path)
    
    # Source ESP-IDF environment if it exists
    idf_env_script = esp_idf_path / "export.sh"
    if idf_env_script.exists():
        tui.info("Loading ESP-IDF environment")
        result = subprocess.run(
            ["/bin/bash", "-c", f"source {idf_env_script} && env"],
            capture_output=True,
            text=True
        )
        if result.returncode == 0:
            for line in result.stdout.splitlines():
                if "=" in line:
                    key, value = line.split("=", 1)
                    env[key] = value
            tui.success("Environment loaded")

    # Parse arguments
    use_custom_sdk = "--local-sdk" in sys.argv or "--custom-sdk" in sys.argv

    if use_custom_sdk:
        tui.subsection("Building with custom SDK")
        env["TACTILITY_SDK_PATH"] = str(tactility_dir / ".tactility")
        tui.command(["python", "tactility.py", "build", "--local-sdk"])
        result = subprocess.run(
            ["python", "tactility.py", "build", "--local-sdk"],
            cwd=tactility_dir,
            env=env
        )
    else:
        tui.subsection("Building with default SDK")
        tui.command(["python", "tactility.py", "build", "--verbose"])
        result = subprocess.run(
            ["python", "tactility.py", "build", "--verbose"],
            cwd=tactility_dir,
            env=env
        )

    if result.returncode != 0:
        tui.error("Build failed")
        sys.exit(1)

    tui.section("Build Complete")
    tui.result("Artifact", str(tactility_dir / "build" / "TactileBrowser.app"))


if __name__ == "__main__":
    main()
