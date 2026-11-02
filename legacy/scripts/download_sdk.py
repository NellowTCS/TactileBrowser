#!/usr/bin/env python3
"""Download latest Tactility SDK artifacts from GitHub Actions."""

import os
import sys
import subprocess
import json
import urllib.request
import urllib.error
import zipfile
import shutil
from pathlib import Path

from tui import TUI


def github_request(url, token, tui):
    """Make authenticated GitHub API request."""
    headers = {
        "Authorization": f"token {token}",
        "Accept": "application/vnd.github.v3+json"
    }
    req = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(req) as response:
            return json.loads(response.read())
    except urllib.error.URLError as e:
        tui.error(f"GitHub API request failed: {e}")
        return None


def main():
    # Check for CI environment
    ci_mode = "--ci-env" in sys.argv
    tui = TUI(ci_mode=ci_mode)

    # Get GitHub token from environment
    github_token = os.environ.get("GITHUB_TOKEN")
    if not github_token:
        tui.error("GITHUB_TOKEN environment variable not set")
        tui.info("Usage: GITHUB_TOKEN=your_token python scripts/download_sdk.py")
        sys.exit(1)

    workspace_root = Path(__file__).parent.parent
    os.chdir(workspace_root)

    tui.banner("Download Tactility SDK")

    repo = "NellowTCS/Tactility"
    workflow_file = "build-sdk.yml"

    tui.info(f"Fetching latest successful {workflow_file} run")

    # Get latest successful workflow run
    runs_url = f"https://api.github.com/repos/{repo}/actions/workflows/{workflow_file}/runs?per_page=1&status=success"
    runs_data = github_request(runs_url, github_token, tui)

    if not runs_data or "workflow_runs" not in runs_data or not runs_data["workflow_runs"]:
        tui.error("No successful workflow runs found")
        sys.exit(1)

    run_id = runs_data["workflow_runs"][0]["id"]
    tui.success(f"Found run ID: {run_id}")

    # Get artifacts for this run
    artifacts_url = f"https://api.github.com/repos/{repo}/actions/runs/{run_id}/artifacts"
    artifacts_data = github_request(artifacts_url, github_token, tui)

    if not artifacts_data or "artifacts" not in artifacts_data:
        tui.error("No artifacts found")
        sys.exit(1)

    # Download each platform SDK
    for platform in ["esp32", "esp32s3"]:
        artifact_name = f"TactilitySDK-{platform}"
        tui.subsection(f"Downloading {artifact_name}")

        # Find artifact
        artifact = None
        for art in artifacts_data["artifacts"]:
            if art["name"] == artifact_name:
                artifact = art
                break

        if not artifact:
            tui.error(f"{artifact_name} not found")
            sys.exit(1)

        tui.success(f"Found artifact: {artifact_name}")

        # Download artifact
        download_url = artifact["archive_download_url"]
        temp_zip = Path(f"/tmp/{artifact_name}.zip")

        tui.info("Downloading from GitHub Actions")
        req = urllib.request.Request(download_url, headers={"Authorization": f"token {github_token}"})
        try:
            with urllib.request.urlopen(req) as response:
                with open(temp_zip, "wb") as f:
                    f.write(response.read())
            tui.success(f"Downloaded to {temp_zip}")
        except urllib.error.URLError as e:
            tui.error(f"Download failed: {e}")
            sys.exit(1)

        # Extract SDK
        target_dir = workspace_root / "tactility-src" / ".tactility" / f"0.6.0-SNAPSHOT12-{platform}" / "TactilitySDK"
        target_dir.mkdir(parents=True, exist_ok=True)

        tui.info(f"Extracting to {target_dir}")
        temp_extract = Path(f"/tmp/{artifact_name}_extract")
        if temp_extract.exists():
            shutil.rmtree(temp_extract)
        temp_extract.mkdir()

        try:
            with zipfile.ZipFile(temp_zip, "r") as zip_ref:
                zip_ref.extractall(temp_extract)
        except zipfile.BadZipFile as e:
            tui.error(f"Failed to extract: {e}")
            sys.exit(1)

        # Move contents to target
        for item in temp_extract.iterdir():
            dest = target_dir / item.name
            if dest.exists():
                shutil.rmtree(dest)
            shutil.move(str(item), str(dest))

        # Cleanup
        shutil.rmtree(temp_extract)
        temp_zip.unlink()

        tui.success(f"Extracted {artifact_name}")

    tui.section("Download Complete")


if __name__ == "__main__":
    main()
