# Wired

<!-- Version and License Badges -->
![Version](https://img.shields.io/badge/version-0.0.1-green.svg?style=flat-square) 
![License](https://img.shields.io/badge/license-GNU%20GPLv3-green?style=flat-square) 
![Language](https://img.shields.io/badge/language-C++23-green.svg?style=flat-square) 

Wired is a cross-platform, C++23, game and render engine.

The project includes a Vulkan 1.3 implementation of the Wired GPU abstraction.

![Alt text](screenshots/collage.webp "Collage")
*A collage of test scenes rendered with the Wired engine*

## High-Level Features

- Cross Platform
- 2D & 3D Rendering
- Entity Component System
- Asset Loading System
- Point, Spot, and Directional Lights
- Single, Cubic, and Stablized Cascaded Shadow Mapping
- Material, Mesh, and Texture Systems
- PBR lighting
- Heightmap Mesh Generation
- Compute-Based GPU Culling, LOD Selection, Post Processing
- Node and Skeleton-Based Model Animations
- 3D Physics System
- Global and 3D Spatialized Audio System
- Player Controller
- Input Handling
  
## Building The Engine

Have available on the command line:
- CMake
- Python3 (If using any of the helper scripts)
- A C++ compiler

### Option 1 - Using the build script
The ```build.[sh/bat]``` script will configure dependencies, invoke CMake, and build (a distro build of) the engine for you.

### Option 2 - Building manually
The engine is defined by a typical CMake project, located in src.

The ```external/prepare_dependencies.[sh/bat]``` helper script uses vcpkg to fetch the engine's dependencies. It will create a local vcpkg repo and install the engine's dependencies into it.

Either use the prepare_dependencies script or provide dependencies manually to CMake later via your own means.

The ```CMakePresets.json``` file defines configurations for typical build presets. Preset names follow the pattern: ```desktop-[debug/release/distro]-[windows/linux]```.

Set ```BUILD_SHARED_LIBS ON/OFF``` for whether to build Wired as shared or static libraries.

**Sample CMake invocation:**
``` cmake src --preset desktop-debug-linux -DBUILD_SHARED_LIBS=OFF ```

See also: The ```build.sh``` script, which does the above for you.
