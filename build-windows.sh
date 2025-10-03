#!/bin/bash
# Quick Windows build script using MinGW and RayLib 5.5

MINGW64="x86_64-w64-mingw32-gcc"
RAYLIB_VERSION="5.5"
RAYLIB_DIR="lib/windows/raylib-${RAYLIB_VERSION}_win64_mingw-w64"

# Download RayLib if needed
if [ ! -d "$RAYLIB_DIR" ]; then
    echo "Downloading RayLib 5.5 for Windows..."
    mkdir -p lib/windows && cd lib/windows
    wget -q --show-progress "https://github.com/raysan5/raylib/releases/download/${RAYLIB_VERSION}/raylib-${RAYLIB_VERSION}_win64_mingw-w64.zip"
    unzip -q "raylib-${RAYLIB_VERSION}_win64_mingw-w64.zip"
    rm "raylib-${RAYLIB_VERSION}_win64_mingw-w64.zip"
    cd ../..
fi

# Build
echo "Building Windows executable..."
$MINGW64 main.c -o gltf-viewer.exe \
    -Wall -Wextra -O2 -std=c99 \
    -I${RAYLIB_DIR}/include \
    -L${RAYLIB_DIR}/lib \
    -lraylib -lopengl32 -lgdi32 -lwinmm \
    -static

if [ -f "gltf-viewer.exe" ]; then
    echo "Build successful!"
    ls -lh gltf-viewer.exe
else
    echo "Build failed!"
    exit 1
fi
