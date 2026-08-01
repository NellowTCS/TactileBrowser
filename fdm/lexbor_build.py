#!/usr/bin/env python3
import os
import subprocess
import shutil

PROJECT_DIR = os.getcwd()
LEXBOR_DIR = os.path.join(PROJECT_DIR, "lexbor")
BUILD_DIR = os.path.join(PROJECT_DIR, ".pio", "lexbor_build")

# Create build directory
os.makedirs(BUILD_DIR, exist_ok=True)
os.chdir(BUILD_DIR)

# Get ESP32 toolchain from environment (set by PlatformIO)
print(f"Environment CC: {os.environ.get('CC')}")
print(f"Environment PATH: {os.environ.get('PATH', '')[:200]}...")

toolchain_prefix = os.environ.get('CC', '').replace('-gcc', '')
print(f"Initial toolchain_prefix from CC: '{toolchain_prefix}'")

# Check if the toolchain path is wrong (codespace vs runner)
if toolchain_prefix and 'codespace' in toolchain_prefix:
    print("Detected codespace path, correcting for GitHub Actions runner")
    toolchain_prefix = toolchain_prefix.replace('codespace', 'runner')
    print(f"Corrected toolchain_prefix: '{toolchain_prefix}'")

if not toolchain_prefix or not os.path.exists(toolchain_prefix + 'gcc'):
    print("Toolchain not found or invalid, searching for correct path...")
    # Try to find the toolchain in common PlatformIO locations
    possible_paths = [
        '/home/runner/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-',
        '/home/codespace/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-',
        '/opt/esp32-toolchain/bin/xtensa-esp32s3-elf-'
    ]

    for path in possible_paths:
        if os.path.exists(path + 'gcc'):
            toolchain_prefix = path
            print(f"Found toolchain at: {path}")
            break
    else:
        # Last resort - assume it's in PATH
        toolchain_prefix = 'xtensa-esp32s3-elf-'
        print(f"No toolchain found, using PATH: {toolchain_prefix}")

print(f"Final toolchain_prefix: '{toolchain_prefix}'")
print(f"Checking if gcc exists: {os.path.exists(toolchain_prefix + 'gcc')}")

# Set environment for cross-compilation
env = os.environ.copy()
env['CC'] = toolchain_prefix + 'gcc'
env['CXX'] = toolchain_prefix + 'g++'
env['AR'] = toolchain_prefix + 'ar'
env['RANLIB'] = toolchain_prefix + 'ranlib'

print(f"Using compilers:")
print(f"  CC: {env['CC']}")
print(f"  CXX: {env['CXX']}")
print(f"  AR: {env['AR']}")
print(f"  RANLIB: {env['RANLIB']}")

# Temporarily move fs.c out of the way since ESP32 doesn't support dirent.h
fs_c_path = os.path.join(LEXBOR_DIR, "source", "lexbor", "ports", "posix", "lexbor", "core", "fs.c")
fs_c_backup = fs_c_path + ".backup"
if os.path.exists(fs_c_path):
    shutil.move(fs_c_path, fs_c_backup)

# Run CMake with flags for static library only and cross-compilation
subprocess.run([
    "cmake", LEXBOR_DIR,
    "-DLEXBOR_BUILD_STATIC=ON",
    "-DLEXBOR_BUILD_SHARED=OFF",
    "-DLEXBOR_BUILD_TESTS=OFF",
    "-DLEXBOR_BUILD_EXAMPLES=OFF",
    "-DLEXBOR_INSTALL_HEADERS=OFF",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_SYSTEM_NAME=Generic",
    "-DCMAKE_C_COMPILER=" + env['CC'],
    "-DCMAKE_CXX_COMPILER=" + env['CXX'],
    "-DCMAKE_AR=" + env['AR'],
    "-DCMAKE_RANLIB=" + env['RANLIB'],
    "-DCMAKE_C_FLAGS=-mlongcalls",
    "-DCMAKE_CXX_FLAGS=-mlongcalls"
], env=env, check=True)

# Build
subprocess.run(["make"], env=env, check=True)

# Copy static library to lexbor dir
shutil.copy("liblexbor_static.a", LEXBOR_DIR)

# Clean up build directory
os.chdir(PROJECT_DIR)
shutil.rmtree(BUILD_DIR)

# Restore fs.c
if os.path.exists(fs_c_backup):
    shutil.move(fs_c_backup, fs_c_path)

print("Lexbor static library built using ESP32 toolchain and build folder cleaned.")
