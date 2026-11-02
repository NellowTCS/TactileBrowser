#!/bin/bash
set -e

# Config
GITHUB_TOKEN="${GITHUB_TOKEN:?Need to set GITHUB_TOKEN env var}"
REPO="NellowTCS/Tactility"
WORKFLOW_FILE="build-sdk.yml"
DEST_BASE=".tactility"
SDK_VERSION="0.6.0-SNAPSHOT4"
PLATFORMS=("esp32" "esp32s3")

# Step 1: Get latest successful workflow run
echo "Fetching latest successful workflow run for $WORKFLOW_FILE..."
RUN_ID=$(curl -s -H "Authorization: token $GITHUB_TOKEN" \
    "https://api.github.com/repos/$REPO/actions/workflows/$WORKFLOW_FILE/runs?per_page=1&status=success" \
    | jq -r '.workflow_runs[0].id')

if [ -z "$RUN_ID" ] || [ "$RUN_ID" == "null" ]; then
    echo "Failed to get latest workflow run ID."
    exit 1
fi
echo "Latest successful run ID: $RUN_ID"

# Step 2: Get artifacts for that run
echo "Fetching artifacts for run $RUN_ID..."
ARTIFACTS_JSON=$(curl -s -H "Authorization: token $GITHUB_TOKEN" \
    "https://api.github.com/repos/$REPO/actions/runs/$RUN_ID/artifacts")

# Step 3: Download and extract each platform SDK
for PLATFORM in "${PLATFORMS[@]}"; do
    DOWNLOAD_URL=$(echo "$ARTIFACTS_JSON" | jq -r ".artifacts[] | select(.name == \"TactilitySDK-$PLATFORM\") | .archive_download_url")

    if [ -z "$DOWNLOAD_URL" ] || [ "$DOWNLOAD_URL" == "null" ]; then
        echo "TactilitySDK-$PLATFORM not found."
        exit 1
    fi

    echo "Downloading TactilitySDK-$PLATFORM..."
    TEMP_ZIP=$(mktemp)
    curl -L -H "Authorization: token $GITHUB_TOKEN" -o "$TEMP_ZIP" "$DOWNLOAD_URL"

    TARGET_DIR="$DEST_BASE/$SDK_VERSION-$PLATFORM/TactilitySDK"
    mkdir -p "$TARGET_DIR"

    # Extract into temp dir first
    TEMP_DIR=$(mktemp -d)
    unzip -q "$TEMP_ZIP" -d "$TEMP_DIR"

    # Move contents to target directory
    cp -a "$TEMP_DIR/." "$TARGET_DIR/"

    # Clean up temp files
    rm -rf "$TEMP_DIR" "$TEMP_ZIP"

    echo "Downloaded and extracted TactilitySDK-$PLATFORM to $TARGET_DIR"
done

echo "All SDKs downloaded successfully."
