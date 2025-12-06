#!/usr/bin/env python3
"""Build TactileBrowser for Tactility (ESP-IDF) platform."""

import os
import sys
import subprocess
import shutil
import platform
from pathlib import Path

from tui import TUI


def get_shell():
    """Get appropriate shell for current platform."""
    if platform.system() == "Windows":
        return "cmd.exe"
    return "/bin/bash"


def run_shell_command(cmd, env=None):
    """Run a shell command cross-platform."""
    shell = get_shell()
    if shell == "cmd.exe":
        return subprocess.run([shell, "/c", cmd], env=env, capture_output=True, text=True)
    else:
        return subprocess.run([shell, "-c", cmd], env=env, capture_output=True, text=True)


def setup_esp_idf(workspace_root, tui):
    """Check if ESP-IDF v5.5 is ready. If not, provide instructions."""
    esp_idf_dir = workspace_root / "esp-idf"
    
    # Check if ESP-IDF already exists with export script
    if esp_idf_dir.exists() and (esp_idf_dir / "export.sh").exists():
        tui.success("ESP-IDF found")
        return esp_idf_dir
    
    # Not found - give instructions
    tui.error("ESP-IDF not properly set up")
    tui.info("\nRun this first to install dependencies and set up ESP-IDF:")
    tui.info("  python3 scripts/setup_esp_idf.py")
    tui.info("\nOr manually run:")
    tui.info("  sudo apt-get update")
    tui.info("  sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0")
    sys.exit(1)


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
        # Source the environment and capture all variables
        result = run_shell_command(
            f"source {idf_env_script} 2>/dev/null && env",
            env=env
        )
        if result.returncode == 0:
            for line in result.stdout.splitlines():
                if "=" in line:
                    key, value = line.split("=", 1)
                    env[key] = value
            tui.success("Environment loaded")
        else:
            tui.warning("Could not load ESP-IDF environment - proceeding anyway")
    else:
        tui.warning(f"ESP-IDF export script not found at {idf_env_script}")

    # Parse arguments
    use_custom_sdk = "--local-sdk" in sys.argv or "--custom-sdk" in sys.argv

    if use_custom_sdk:
        tui.subsection("Building with custom SDK")
        env["TACTILITY_SDK_PATH"] = str(tactility_dir / ".tactility")
        cmd = f"source {esp_idf_path / 'export.sh'} 2>/dev/null && python tactility.py build --local-sdk"
        tui.command(cmd)
        cwd_backup = os.getcwd()
        os.chdir(tactility_dir)
        result = run_shell_command(cmd, env=env)
        os.chdir(cwd_backup)
    else:
        tui.subsection("Building with default SDK")
        cmd = f"source {esp_idf_path / 'export.sh'} 2>/dev/null && python tactility.py build --verbose"
        tui.command(cmd)
        cwd_backup = os.getcwd()
        os.chdir(tactility_dir)
        result = run_shell_command(cmd, env=env)
        os.chdir(cwd_backup)

    if result.returncode != 0:
        tui.error("Build failed")
        sys.exit(1)

    tui.section("Build Complete")
    tui.result("Artifact", str(tactility_dir / "build" / "TactileBrowser.app"))


if __name__ == "__main__":
    main()
