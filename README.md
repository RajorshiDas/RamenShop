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
| `shadowmap.h / shadowmap.cpp` | Soft shadows: 512x512 shadow map with PCF for the dining pendant light |
| `refraction.h / refraction.cpp` | Screen-space glass refraction for the tumbler on the left customer table |
| `rtcore.h / rtcore.cpp` | CPU ray tracer (no OpenGL): boxes, cones, disks, spheres, BVH, reflections, Snell refraction, Blinn-Phong |
| `rtscene.h / rtscene.cpp` | The shop for the ray tracer (pots, bowls, glass ball, mirror and their surroundings) and the new ball / mirror models |
| `rtpass.h / rtpass.cpp` | Traces those objects' screen regions on all CPU threads and composites the result over the frame |
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
- **Y** - Toggle soft shadows (shadow mapping with PCF)
- **Z** - Toggle glass refraction on the table tumbler
- **R** - Toggle ray tracing: pots, ramen bowls, glass ball, upper-room mirror (real CPU ray tracing, off at start)
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

## Soft shadows and glass refraction

Both effects are rasterization techniques that run in the existing OpenGL context and GLSL 1.20 Phong shader.
They are **not** ray tracing. Each one can be switched off, and each falls back to the original look if it
cannot start. The console prints `[Shadow] ... ready` and `[Glass] ... ready` at start-up when they work.

**Soft shadows (Y)** - one shadow-casting light, the dining pendants above the counter (`GL_LIGHT1`). Every frame
the counter, stools, tables, chairs and ramen bowls are drawn from the light into a 512x512 depth texture. The Phong
shader compares each pixel with it using 3x3 percentage-closer filtering, so the pendant light fades out softly
behind those objects. While it is on, the old projected pendant shadow on the floor is not drawn. The table
spotlight and outdoor sun shadows stay as before. Needs Phong shading: in Gouraud mode (G) the projected shadows
are used. For sharper shadows, set `SHADOW_MAP_SIZE` to 1024 in `shadowmap.cpp` if the frame rate allows.

**Glass refraction (Z)** - an empty tumbler on the left customer table. After the scene is drawn, the screen area
behind the tumbler is copied into a texture, and the tumbler is drawn with a shader that reads that texture at an
offset computed with `refract()` from the glass normal, plus a Fresnel reflection and the lamp highlights. This is
a **screen-space approximation**: only what is already on screen can be seen through the glass, and nearby objects
in front of it can bleed into the distortion at its edges. With Z off, the ordinary blended glass tumbler is drawn
in the same place.

## Ray tracing (R)

Real ray tracing, done on the CPU, for a few objects. It works from every camera; press **R** (off at start).

| Object | What the rays do |
|---|---|
| The two steel stock pots on the stove | Mirror-like reflections of the counter, bowls, stools, lamps and each other |
| The four ramen bowls on the counter | Reflections on the red glaze and on the soup surface (subtle, like real glaze) |
| Glass ball on the counter (new) | Refraction with Snell's law (the room behind it appears upside down), Fresnel reflection |
| Mirror in the upper tatami room (new, back wall, left of the sliding doors) | A clear reflection of the room that moves with the camera |

- **Scene**: `rtscene.cpp` describes these objects and everything around them that rays can hit (counter, kitchen,
  hood, stools, tables, noren, lamps, walls, floors; upstairs the tatami, walls, tea table and tea set, chest, futons,
  bed and lamp) with boxes, truncated cones, disks and spheres at the same positions and sizes as the OpenGL models,
  using the same textures and colours. Lamps are light-emitting shapes, so they appear in reflections. Lighting
  comes from the shop's own OpenGL lights every frame, so day / night and the light keys apply.
- **Algorithm** (`rtcore.cpp`): for every pixel of the objects' screen rectangles an eye ray is tested against the
  ray-traced objects. On metal, glaze, soup and the mirror a reflection ray is traced (Fresnel / Schlick weights, up to
  two bounces); in the glass ball rays are refracted with Snell's law (n = 1.5) at every surface, with total
  internal reflection. Surfaces the rays reach are shaded with the same Blinn-Phong model as the OpenGL scene.
- **Acceleration**: a bounding volume hierarchy (BVH) over the small shapes, the large ones (floors, walls, ceilings)
  tested directly; only the objects' screen rectangles are traced; all CPU threads share the rows; a ray budget
  adapts so the ray tracer stays near 18 ms per frame (larger areas are traced at one ray per 2x2 pixels).
- **Compositing** (`rtpass.cpp`): the result is drawn over the OpenGL frame only where its depth buffer shows the
  same surface, so anything in front of a pot, bowl, ball or mirror still hides it. Pots and bowls keep their OpenGL
  lighting and get the reflection added on top; the glass ball and the mirror are fully ray traced.
- **Limitations**: the outdoor scene, small details (handles, toppings, steam, cups on the counter) and the glass
  ware are not in the ray-traced scene, so they do not appear in reflections. An object moved off its upright axis
  with the object keys keeps its OpenGL look. Expect roughly 5-20 ms of CPU time per frame while these objects are
  on screen (printed in the console every 3 s); nothing runs when R is off or none of them is in view.
- **Build**: no new libraries or project settings; plain C++17 and `std::thread`.
