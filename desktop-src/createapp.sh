#!/bin/bash

# Create app bundle structure
APP_NAME="TactileBrowser"
BUNDLE_NAME="${APP_NAME}.app"

mkdir -p "${BUNDLE_NAME}/Contents/MacOS"
mkdir -p "${BUNDLE_NAME}/Contents/Resources"

# Copy executable
cp "./build/main/tactile_browser_test" "${BUNDLE_NAME}/Contents/MacOS/"

# Create Info.plist
cat > "${BUNDLE_NAME}/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>TactileBrowser</string>
    <key>CFBundleIdentifier</key>
    <string>com.nellowtcs.tactilebrowser</string>
    <key>CFBundleName</key>
    <string>Tactile Browser</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

echo "App bundle created: ${BUNDLE_NAME}"