# Makefile for GLTF Viewer
# Supports native compilation and Windows cross-compilation from Linux

# Program name
TARGET = gltf-viewer

# Source files
SOURCES = main.c

# Default compiler
CC = gcc

# Windows cross-compiler
MINGW_CC = x86_64-w64-mingw32-gcc

# Common compiler flags
CFLAGS = -Wall -Wextra -O2 -std=c99

# Platform-specific libraries
LIBS_LINUX = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
LIBS_WINDOWS = -lraylib -lopengl32 -lgdi32 -lwinmm
LIBS_MACOS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

# Detect OS for native compilation
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS = $(LIBS_LINUX)
endif
ifeq ($(UNAME_S),Darwin)
    LIBS = $(LIBS_MACOS)
endif
ifeq ($(OS),Windows_NT)
    LIBS = $(LIBS_WINDOWS)
    TARGET := $(TARGET).exe
endif

# Default target
all: $(TARGET)

# Native compilation
$(TARGET): $(SOURCES)
	$(CC) $(SOURCES) -o $(TARGET) $(CFLAGS) $(LIBS)

# Windows cross-compilation from Linux
windows: check-mingw
	$(MINGW_CC) $(SOURCES) -o $(TARGET).exe $(CFLAGS) $(LIBS_WINDOWS) -static

# Windows cross-compilation with dynamic linking
windows-dynamic: check-mingw
	$(MINGW_CC) $(SOURCES) -o $(TARGET).exe $(CFLAGS) $(LIBS_WINDOWS)

# Windows 32-bit cross-compilation
windows32: check-mingw32
	i686-w64-mingw32-gcc $(SOURCES) -o $(TARGET)32.exe $(CFLAGS) $(LIBS_WINDOWS) -static

# Check if MinGW cross-compiler is installed
check-mingw:
	@command -v $(MINGW_CC) >/dev/null 2>&1 || { \
		echo "==============================================="; \
		echo "ERROR: MinGW cross-compiler not found!"; \
		echo ""; \
		echo "To install on Ubuntu/Debian:"; \
		echo "  sudo apt-get install mingw-w64"; \
		echo ""; \
		echo "To install on Fedora:"; \
		echo "  sudo dnf install mingw64-gcc"; \
		echo ""; \
		echo "To install on Arch:"; \
		echo "  sudo pacman -S mingw-w64-gcc"; \
		echo "==============================================="; \
		exit 1; \
	}
	@echo "MinGW cross-compiler found: $(MINGW_CC)"

check-mingw32:
	@command -v i686-w64-mingw32-gcc >/dev/null 2>&1 || { \
		echo "ERROR: MinGW 32-bit cross-compiler not found!"; \
		echo "Install with: sudo apt-get install mingw-w64"; \
		exit 1; \
	}

# Download pre-compiled RayLib for Windows (MinGW)
download-raylib-windows:
	@echo "Downloading RayLib for Windows MinGW..."
	@mkdir -p lib/windows
	@cd lib/windows && \
		wget -q --show-progress https://github.com/raysan5/raylib/releases/download/4.5.0/raylib-4.5.0_win64_mingw-w64.zip && \
		unzip -q raylib-4.5.0_win64_mingw-w64.zip && \
		echo "RayLib for Windows downloaded successfully!"

# Windows compilation with downloaded RayLib
windows-with-raylib: check-mingw download-raylib-windows
	$(MINGW_CC) $(SOURCES) -o $(TARGET).exe $(CFLAGS) \
		-I./lib/windows/raylib-4.5.0_win64_mingw-w64/include \
		-L./lib/windows/raylib-4.5.0_win64_mingw-w64/lib \
		-lraylib -lopengl32 -lgdi32 -lwinmm -static

# Build all targets
all-platforms: all windows

# Run the program with default model
run: $(TARGET)
	./$(TARGET)

# Run with a specific model file
run-file: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make run-file FILE=yourmodel.glb"; \
	else \
		./$(TARGET) $(FILE); \
	fi

# Test Windows executable with Wine
test-windows: windows
	@command -v wine >/dev/null 2>&1 || { \
		echo "Wine is not installed. Install it to test Windows executables on Linux."; \
		echo "  Ubuntu/Debian: sudo apt-get install wine"; \
		echo "  Fedora: sudo dnf install wine"; \
		echo "  Arch: sudo pacman -S wine"; \
		exit 1; \
	}
	wine $(TARGET).exe

# Clean build files
clean:
	rm -f $(TARGET) $(TARGET).exe $(TARGET)32.exe
	rm -rf lib/

# Clean and rebuild
rebuild: clean all

# Install dependencies for native Linux compilation
install-deps-ubuntu:
	sudo apt-get update
	sudo apt-get install -y build-essential libraylib-dev libgl1-mesa-dev

install-deps-fedora:
	sudo dnf install -y gcc raylib-devel mesa-libGL-devel

install-deps-arch:
	sudo pacman -S --needed gcc raylib mesa

# Install MinGW for cross-compilation
install-mingw-ubuntu:
	sudo apt-get update
	sudo apt-get install -y mingw-w64 mingw-w64-tools

install-mingw-fedora:
	sudo dnf install -y mingw64-gcc mingw64-winpthreads-static

install-mingw-arch:
	sudo pacman -S --needed mingw-w64-gcc

# Help
help:
	@echo "GLTF Viewer Makefile - Native and Cross-Compilation"
	@echo "===================================================="
	@echo ""
	@echo "Native Compilation:"
	@echo "  make              - Build for current system"
	@echo "  make run          - Build and run with default model"
	@echo "  make run-file FILE=x - Build and run with specific model"
	@echo ""
	@echo "Windows Cross-Compilation (from Linux):"
	@echo "  make windows      - Build Windows exe (static linking)"
	@echo "  make windows-dynamic - Build Windows exe (dynamic linking)"
	@echo "  make windows32    - Build 32-bit Windows exe"
	@echo "  make windows-with-raylib - Build with downloaded RayLib"
	@echo "  make test-windows - Test Windows exe with Wine"
	@echo ""
	@echo "Build All:"
	@echo "  make all-platforms - Build for both Linux and Windows"
	@echo ""
	@echo "Utilities:"
	@echo "  make clean        - Remove all built files"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Dependency Installation:"
	@echo "  make install-deps-ubuntu  - Install RayLib on Ubuntu/Debian"
	@echo "  make install-deps-fedora  - Install RayLib on Fedora"
	@echo "  make install-deps-arch    - Install RayLib on Arch"
	@echo ""
	@echo "Cross-Compilation Setup:"
	@echo "  make install-mingw-ubuntu - Install MinGW on Ubuntu/Debian"
	@echo "  make install-mingw-fedora - Install MinGW on Fedora"
	@echo "  make install-mingw-arch   - Install MinGW on Arch"
	@echo "  make download-raylib-windows - Download RayLib for Windows"
	@echo ""
	@echo "Example Workflow for Windows Build:"
	@echo "  1. make install-mingw-ubuntu  # Install cross-compiler"
	@echo "  2. make windows-with-raylib   # Build Windows exe"
	@echo "  3. make test-windows          # Test with Wine"

.PHONY: all windows windows-dynamic windows32 windows-with-raylib \
        check-mingw check-mingw32 download-raylib-windows \
        all-platforms run run-file test-windows clean rebuild \
        install-deps-ubuntu install-deps-fedora install-deps-arch \
        install-mingw-ubuntu install-mingw-fedora install-mingw-arch help
