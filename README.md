# GLTF Viewer

A lightweight and minimal GLTF/GLB file viewer written in C using RayLib. This program provides a simple way to view 3D models exported from Blender or other 3D modeling software with an intuitive camera control system.

## Features

- **GLTF/GLB Support**: Load and display GLTF 2.0 and GLB (binary GLTF) files
- **Dual Camera Modes**: 
  - **Orbit Camera**: Traditional 3D model viewer with rotate, pan, and zoom
  - **Isometric Strategy Camera**: RTS-style camera with fixed angle, WASD panning, and edge scrolling
- **Interactive Controls**: Intuitive mouse and keyboard controls for both camera modes
- **Unit System**: Spawn and control small units with RTS-style commanding
  - Right-click to command selected units to move to target position
  - Control groups (1-9) for quick unit selection and camera focus
  - Advanced collision detection with actual mesh geometry (not just bounding boxes)
  - Units avoid collisions with terrain, buildings, and each other using raycasting
  - Terrain-aware movement - units follow ground height on uneven surfaces
  - RTS-style selection box for selecting multiple units
  - Units form formations when commanded to the same location
  - Units represented as colored cubes (white/lime/skyblue based on state)
  - Visual command marker shows target location
  - Group numbers displayed above units for easy identification
- **Minimal UI**: Clean interface with compact information display
- **Grid Display**: Optional grid for spatial reference
- **Auto-scaling**: Automatically adjusts camera distance based on model size
- **Coordinate Axes**: Visual reference for X (red), Y (green), and Z (blue) axes

## Requirements

- C compiler (gcc, clang, or MSVC)
- RayLib library (version 5.5 or later)
- OpenGL support

## Building

### Linux

```bash
# Build the program
make

# Run with default model
make run

# Run with custom model
make run-file FILE=your-model.glb
```

### Windows Cross-Compilation (from Linux)

```bash
# Complete build process (installs dependencies if needed)
./build-windows.sh --all

# Or step by step:
./build-windows.sh --install   # Install MinGW
./build-windows.sh --download  # Download RayLib 5.5
./build-windows.sh --build     # Build Windows executable
./build-windows.sh --package   # Create Windows package
```

### Windows (Native)

Use the pre-built `gltf-viewer.exe` or compile with MinGW using the provided Makefile.

### macOS

```bash
# Install RayLib using Homebrew
brew install raylib

# Build the program
make

# Run
./gltf-viewer your-model.glb
```

## Usage

### Command Line

```bash
# Run with default model (ibm-pc.glb)
./gltf-viewer

# Run with a specific model
./gltf-viewer path/to/your-model.glb
```

### Controls

#### Camera Mode Selection
- **TAB**: Switch between Orbit and Isometric camera modes

#### Orbit Camera Mode
- **Left Mouse Button + Drag**: Rotate camera around model
- **Middle Mouse Button + Drag**: Pan camera
- **Right Mouse Button**: Command selected units to target position
- **Mouse Wheel**: Zoom in/out
- **R**: Reset camera to default position

#### Isometric Strategy Camera Mode
- **WASD/Arrow Keys**: Pan camera across the scene
- **Mouse Edge Scrolling**: Move mouse to screen edges to pan
- **Middle Mouse Button + Drag**: Pan camera
- **Left Mouse Button + Drag**: Select units with selection box
- **Right Mouse Button**: Command selected units to target position
- **Mouse Wheel**: Zoom in/out (adjusts camera height)
- **R**: Reset camera to default position

#### Unit Controls
- **SPACE**: Spawn 5 new units at random positions
- **C**: Clear all units from the scene
- **DELETE**: Delete selected units
- **U**: Toggle unit visibility
- **Left Click + Drag**: Select units within box (Isometric mode)
- **Right Click**: Command selected units to move to clicked position
  - Units will form a grid formation at the target
  - Green marker shows command location
  - Units turn skyblue when following commands
  - Units return to wandering (white/lime) when they reach their target

#### Control Groups
- **Ctrl+1 to Ctrl+9**: Assign selected units to control groups 1-9
- **1 to 9**: Select control group and center camera on units
  - Instantly selects all units in the group
  - Camera automatically moves to show the group
  - Group number displayed above each unit
  - Multiple groups can be created for different unit formations

#### Display Options
- **G**: Toggle grid display
- **X**: Toggle coordinate axes display
- **I**: Toggle information overlay
- **U**: Toggle unit display
- **ESC**: Exit the program

## Window Specifications

- Resolution: 800x600 pixels
- Frame rate: 60 FPS
- Background: Dark gray

## Supported Formats

The viewer supports:
- GLTF 2.0 (.gltf) with separate texture files
- GLB (Binary GLTF) with embedded textures
- Models with multiple meshes and materials
- PBR materials (rendered with RayLib's default shading)
- Terrain and building models for strategy game scenarios
- Low-poly models recommended for best collision detection performance

## Example Workflow

1. Create your 3D scene in Blender
2. Export as GLB file (File → Export → glTF 2.0)
   - Choose "glTF Binary (.glb)" format for embedded textures
   - Enable "Apply Modifiers" if using modifiers
3. Run the viewer with your exported file:
   ```bash
   ./gltf-viewer my-scene.glb
   ```

## Information Display

The compact info panel shows:
- Current camera mode (ORBIT or ISOMETRIC)
- **MODEL**: Shows current camera mode (ORBIT or ISOMETRIC)
- Number of meshes
- Total triangle count  
- Total vertex count
- Active units count and selected units
- Active control groups (shows which groups have units)
- Camera distance from target (Orbit mode)
- Camera height (Isometric mode)
- Edge scroll indicator (Isometric mode, when mouse is near screen edges)

## Tips for Best Results

1. **Model Centering**: Models are automatically centered based on their bounding box
2. **Scale**: The viewer auto-adjusts the camera distance based on model size
3. **Textures**: Embed textures in GLB format for easier distribution
4. **Performance**: The viewer is optimized for smooth performance with typical models
5. **Terrain Models**: Load low-poly terrain models - units will walk on the surface and follow height variations
6. **Building Models**: Units detect and avoid buildings/obstacles using precise mesh collision detection
7. **Units**: Units automatically avoid collisions with actual model geometry (not just bounding boxes)
8. **Strategy View**: Use isometric mode for the best RTS-style experience with units
9. **Unit Commands**: Selected units will navigate around obstacles to reach commanded positions
10. **Formation Movement**: Multiple selected units automatically form grid formations when commanded
11. **Clicking on Terrain**: Right-click commands work on terrain surfaces - units will move to the exact clicked point
12. **Control Groups**: Use number keys for RTS-style unit management - quickly switch between different squads

## Troubleshooting

### Model doesn't load
- Check that the file path is correct
- Ensure the file is a valid GLTF 2.0 or GLB file
- Check console output for error messages

### Textures not showing
- Make sure textures are embedded (GLB) or in the correct relative path (GLTF)
- Check that texture formats are supported by RayLib

### Performance issues
- Try reducing polygon count in your 3D modeling software
- Close other graphics-intensive applications

## Building from Source

The source code consists of a single `main.c` file with minimal dependencies. The program is designed to be simple, portable, and easy to modify.

### Project Structure
```
gltf-viewer/
├── main.c              # Main program source
├── Makefile            # Build configuration
├── build-windows.sh    # Windows cross-compilation script
├── README.md           # This file
└── ibm-pc.glb         # Example model
```

## Windows Distribution

The Windows package includes:
- `gltf-viewer.exe` - Standalone executable (statically linked)
- `run.bat` - Batch file for easy launching
- `drag-model-here.bat` - Drag-and-drop launcher
- `README.txt` - Windows-specific instructions

## License

This viewer is provided as-is for educational and personal use. Feel free to modify and distribute as needed.

## Credits

- Built with [RayLib](https://www.raylib.com/) - A simple and easy-to-use library for videogames programming
- Example IBM PC model for testing