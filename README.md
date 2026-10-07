# 3D Ramen Shop

A 3D interactive ramen shop built with OpenGL and FreeGLUT in C++17. Explore a detailed Japanese ramen restaurant, toggle lighting and visual effects, manipulate objects, and play a ramen cooking mini-game.

## Project Structure

### Source Files

| File | Description |
|------|-------------|
| `main.cpp` | Application entry point, window setup, GL initialization, FreeGLUT loop |
| `shapes.h / shapes.cpp` | Vectors, colors, quadric helpers, transforms, geometric primitives, steam particles |
| `texture.h / texture.cpp` | Procedural texture generation (wood, tile, wall, asphalt, concrete) |
| `objects.h / objects.cpp` | Scene-object selection and delta-transform system |
| `furniture.h / furniture.cpp` | Stools, counter, chairs, tables, shelves, kitchen equipment |
| `food.h / food.cpp` | Ramen bowls, chashu, eggs, nori, green onions, chopsticks |
| `decorations.h / decorations.cpp` | Paper lanterns, noren curtains, menu boards, wall art |
| `exterior.h / exterior.cpp` | Shop structure, roof, windows, street, sidewalk, trees |
| `scene.h / scene.cpp` | High-level draw functions (ground, exterior, interior), feature toggles |
| `lighting.h / lighting.cpp` | Lighting setup, toggles, and presets |
| `camera.h / camera.cpp` | Orbit, FPS walkthrough, and focused counter camera modes |
| `shader.h / shader.cpp` | Shading modes (Phong / Gouraud), edge outlines, fog |
| `raytracer.h / raytracer.cpp` | Real-time ray tracing with configurable bounces and soft shadows |
| `game.h / game.cpp` | Ramen cooking mini-game (order taking, cooking, serving, scoring) |

### Build Files

| File | Description |
|------|-------------|
| `RamenShop.sln` | Visual Studio solution file |
| `RamenShop.vcxproj` | Visual Studio project file |
| `packages.config` | NuGet package dependencies |

## Controls

### Camera
- **C** - Cycle camera mode (Orbit / Walkthrough / Focused Counter)
- **Left Mouse Drag** - Orbit/tilt or look around
- **Scroll Wheel** - Zoom or height adjust
- **Arrow Keys** - Orbit horizontally / zoom (Orbit mode)
- **Page Up / Page Down** - Camera higher / lower (Orbit mode)
- **W / A / S / D** - Move (Walkthrough mode)
- **Q / E** - Fly down / up (Walkthrough mode)

### Scene
- **R** - Toggle real-time ray tracing
- **B** - Cycle ray tracing bounce depth (1-5)
- **Y** - Toggle soft shadows
- **G** - Toggle Phong / Gouraud shading
- **T** - Toggle Day / Night
- **H** - Hide / show roof
- **O** - Dark edge outlines on / off
- **F** - Atmospheric fog on / off
- **X** - Rising steam particles on / off
- **Space** - Pause / resume animation
- **Esc** - Quit

### Lighting
- **1** - Toggle Ambient Light
- **2** - Toggle Diffuse Reflection
- **3** - Toggle Specular Highlights
- **4** - Toggle Sky Light (Sun / Moon)
- **5** - Toggle Lamp Lights
- **6** - Toggle Kitchen Light
- **7** - Toggle Decor Lights
- **P** - Cycle Lighting Presets

### Object Manipulation
- **Tab** - Cycle selected object
- **I / K** - Move +Z / -Z
- **J / L** - Move -X / +X
- **U / N** - Move +Y / -Y
- **[ / ]** - Rotate yaw
- **; / '** - Rotate pitch
- **, / .** - Scale down / up

### Game Mode
- **Enter** - Start the cooking game
- **E** - Interact with stations
- **1-6** - Choose ingredient at prep area
- **Q** - Discard held item
- **M** - Return to explore mode

## Building

Open `RamenShop.sln` in Visual Studio and build the solution. Requires OpenGL and FreeGLUT.
