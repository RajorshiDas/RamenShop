# 3D Ramen Shop

An interactive 3D Japanese ramen shop built from scratch with **OpenGL (fixed-function + a GLSL 1.20 Phong shader), FreeGLUT and GLEW in C++17**. You can walk through the shop and its surroundings, switch every lamp on and off, change between day and night, move objects around, look at the shop from several cameras, and play a ramen cooking mini-game.

Everything is generated in code: all geometry comes from primitives (boxes, cylinders, spheres, tori, lathe shapes) and all textures (wood planks, tiles, plaster, tatami, grass, water, clouds) are procedural. There are no model or image files to load.

## What is in the scene

**The shop (ground floor)**
- Dining counter with six stools, a lacquered counter top and four ramen bowls with steam
- Open kitchen: stove with two large steel stock pots, hood, shelves, fridge, sink, dish rack, stacked bowls, ingredient trays
- Two small customer tables with wooden chairs, each with its own pendant-style table lamp (bell shade on a cord)
- Paper box lanterns, hanging lanterns, a floor lantern tower, pendant glass lamps, a ceiling area-light panel
- Noren curtain, wall clock, menu board, shoji windows and doors (the entrance door and the sliding doors can be clicked open)

**Upstairs tatami room** (reached by the stairs or by clicking the upstairs sliding door)
- Tatami floor with cloth borders, plaster walls with dark timber frame, fusuma sliding doors, wooden board ceiling
- Futon bed with a quilt and pillow, chabudai tea table with a tea set, zabuton cushions, tansu chest, sideboard with a radio, folded futons, a standing electric fan, a ceiling fan, a mirror
- Framed pictures (Mt. Fuji, maneki-neko lucky cat, onigiri, koi, great wave) and a ramen poster, all painted procedurally
- Windows that let the day or night light in

**Outdoors**
- Cherry blossom, maple, pine, broadleaf trees and bamboo (with wind sway), grass, flowers, bushes, a lake with ducks and a bridge, a torii gate, stone lanterns, neighbouring houses
- Sky with sun, moon, stars and clouds, street with cobbles, sidewalks, street lamps, fireflies at night, falling petals

**Things that move**
- A cyclist riding along the street (rolling wheels, pedalling legs), a person walking along the sidewalk, and an orange cat that walks, sits and washes its face
- A nobori banner rippling in the wind, a glass wind chime (furin) swinging as a real pendulum, ceiling and stand fans
- Swaying lanterns, steam, ducks swimming in circles, flickering lamps

## Controls

### Cameras
| Key / input | Action |
|---|---|
| **C** | Cycle camera: Orbit, Walkthrough, Counter View, Upper Room, Pond View |
| **B** | Bowl close-up camera on / back to the previous camera |
| **8** | Light tour: fly to the next lamp and switch it on (**9** toggles the lamp you are looking at) |
| Left mouse drag | Orbit / tilt, or look around (Walkthrough) |
| Mouse wheel | Zoom (or height in Walkthrough) |
| Arrow keys | Orbit left / right, zoom in / out |
| Page Up / Page Down | Camera higher / lower |
| **W A S D**, **Q / E** | Walk, fly down / up (Walkthrough) |
| Left click on a door | Open / close the entrance door, the sliding side door and the upstairs door |

### Lighting
| Key | Action |
|---|---|
| **1** | Dining pendant lamps |
| **2** | Table spotlights (both) |
| **3** | Ceiling area light |
| **4** | Paper box lanterns |
| **5** | Hanging lanterns |
| **6** | Floor lantern tower |
| **7** | Upstairs room light |
| **0** | Sun / moon |
| **-** / **=** / **\\** | Show or hide ambient / diffuse / specular light on every lamp |
| **T** | Day / night |
| **P** | Cycle lighting presets |

### Scene and effects
| Key | Action |
|---|---|
| **Y** | Soft shadows from the pendant lamps (shadow map with PCF) on / off |
| **Z** | Glass refraction on the left table's tumbler on / off |
| **R** | CPU ray tracing on / off (off at start, see below) |
| **G** | Phong / Gouraud shading |
| **H** | Hide / show the roof |
| **O** | Dark edge outlines |
| **F** | Fog |
| **X** | Steam |
| **Space** | Pause / resume animation |
| **Esc** | Quit |

### Moving objects
Press **Tab** to select an object (ramen bowl, kettle, left or right lantern, noren curtain, counter stool), then:
**I / K** move forward / back, **J / L** left / right, **U / N** up / down, **[ / ]** rotate, **; / '** tilt, **, / .** scale. **Tab** again cycles to the next object and finally to none.

### Cooking game
Press **Enter** (or **M**) to open the game screen, then **Enter** or **Space** to start. The game is opt-in, so the program starts in explore mode.
**W A S D** and the mouse to move and look, **E** to use the station in front of you, **1-6** to choose what you take at the prep area (1 raw noodles, 2 empty bowl, 3-6 toppings), **Q** throw away what you hold, **V** turn around, **Esc** pause, **M** back to exploring.
Flow: a customer sits down and orders. Take raw noodles (1) and drop them in the noodle pot, take a bowl (2), scoop the noodles out when they look perfectly cooked, ladle broth from the broth pot, add the toppings at the prep area, then serve the bowl at the counter. Timing, the right toppings and speed decide the score, and you level up for new ingredients and orders.

## How it is built

### Lighting
The scene uses all 8 OpenGL lights, and each lamp has its own ambient, diffuse and specular colour. Lights 2, 3, 5 and 6 are used for the entrance lanterns, street lamps and shop sign outside, and are switched to interior roles (hanging and box lanterns, table spotlight, floor tower) while the inside of the shop is drawn. The global ambient term is zero, so a lamp that is switched off really lights nothing. Glowing surfaces (lantern paper, bulbs, window paper) fade with their lamp.

### Phong shader
`shader.cpp` holds a per-pixel Phong program (GLSL 1.20): ambient, diffuse and specular from every enabled light, spot cones, distance attenuation, fog, a per-light enable mask and the soft-shadow lookup. It is used for all lit geometry; unlit things (steam, text, outlines) go through the same program with lighting off. **G** switches to the plain fixed-function pipeline.

### Shadows
- **Sun / moon shadows outdoors:** the outdoor scene is flattened onto the ground along the sun direction into the stencil buffer, then one soft dark layer is blended, so overlapping shadows never darken twice. Trees near the shop cast their real leaf shapes. At night a few simple static shadows (cherry tree, small posts) are used.
- **Interior:** projected shadows of the furniture from the pendant and table lamps onto the floor.
- **Soft shadows (Y):** a 512 x 512 shadow map from the dining pendant light, filtered with 3 x 3 percentage-closer filtering in the Phong shader.

### Glass refraction (Z)
The tumbler on the left table is drawn last: the screen area behind it is copied to a texture and sampled at an offset computed with `refract()`, plus a Fresnel reflection and lamp highlights. This is a screen-space approximation: only what is already on screen can be seen through the glass.

### Ray tracing (R)
A small, real CPU ray tracer (`rtcore`, `rtscene`, `rtpass`) renders a few featured objects from any camera and composites them over the OpenGL frame:

| Object | What the rays do |
|---|---|
| Two steel stock pots | Mirror-like reflections of counter, bowls, stools and lamps |
| Four ramen bowls | Subtle reflections on glaze and soup |
| Glass ball, drinking glasses, water jug | Refraction through glass and water (Snell's law, total internal reflection), Fresnel reflection |
| Mirror in the upstairs room | A clear reflection of the room |

- **Scene:** `rtscene.cpp` rebuilds the shop for the rays from the same positions and sizes as the OpenGL models (boxes, truncated cones, disks, spheres), with the same textures. Lights come from the shop's own OpenGL lights every frame.
- **Speed:** bounding volume hierarchy over the small shapes; only the featured objects' screen rectangles are traced; all CPU threads share the rows; a ray budget keeps the cost near a fixed time per frame, so close-ups of large areas use one ray per several pixels and are smoothed.
- **Compositing:** the result is drawn only where the depth buffer shows the same surface, so anything in front still hides it.
- **Limits:** only the interior featured objects are traced, the outdoor scene and small props are not part of it, and the console prints the ray count and time every few seconds. Expect a Release build to be much faster than Debug.

### Animation
All motion is driven by one clock (`animTime`), so **Space** pauses everything. The wind chime integrates two coupled pendulums with gravity, damping and a gusty wind force, and the cyclist's wheels roll exactly the distance travelled.

## Project structure

| File | What it does |
|---|---|
| `main.cpp` | Window, GL initialisation, display loop and timer |
| `shapes.h/.cpp` | Vectors, colours, primitives, materials (gloss, PBR-style presets, emission), steam |
| `texture.h/.cpp` | Procedural textures (wood, tile, plaster, tatami, grass, water, clouds, environment map) |
| `lighting.h/.cpp` | Light set-up, interior / exterior light slots, day / night, presets |
| `shader.h/.cpp` | Phong shader, light mask, fog, soft-shadow lookup, interior light scope |
| `camera.h/.cpp` | Camera modes, keyboard and mouse input, light tour, window title |
| `scene.h/.cpp` | Ground, sky, exterior, interior, shadows, upstairs room, moving props (bicycle, cat, person, banner, fans, wind chime) |
| `exterior.h/.cpp` | Shop building, roof, windows and doors, trees, bamboo, lake, street, sidewalk, houses, ducks |
| `furniture.h/.cpp` | Counter, stools, tables, chairs, kitchen equipment, pots |
| `decorations.h/.cpp` | Lanterns, noren, menu board, wall clock, sign |
| `food.h/.cpp` | Ramen bowl, noodles, egg, chashu, nori, onion, chopsticks |
| `objects.h/.cpp` | Selectable objects and their move / rotate / scale offsets |
| `shadowmap.h/.cpp` | Soft shadows (shadow map + PCF) |
| `refraction.h/.cpp` | Screen-space glass refraction |
| `rtcore.h/.cpp` | CPU ray tracer: shapes, BVH, reflection, refraction, Blinn-Phong |
| `rtscene.h/.cpp` | The shop as the ray tracer sees it |
| `rtpass.h/.cpp` | Threaded ray tracing of screen regions and compositing |
| `game.h/.cpp` | Cooking game: state machine, stations, customer, HUD |
| `RamenShop.sln`, `RamenShop.vcxproj`, `packages.config` | Visual Studio solution, project and NuGet packages |

## Building and running

Requirements: Windows, Visual Studio with the C++ desktop workload, a GPU / driver with OpenGL 3.0 or newer (framebuffer objects and float textures are used by the shadow map and the ray-tracing composite).

1. Open `RamenShop.sln` in Visual Studio.
2. Choose **x64** and **Debug** or **Release** (Release is much faster, especially with ray tracing on).
3. Build the solution. The NuGet package `nupengl.core` (FreeGLUT and GLEW) is restored automatically and its DLLs are copied next to the program.
4. Run it. The shop opens in the orbit camera; press **C** to change cameras and **Enter** to start the cooking game.

The program prints `[Phong]`, `[Shadow]`, `[Glass]` and `[RT]` messages in the console on start-up. If an effect cannot start, it is switched off and the scene falls back to the plain look.

## Performance tips
- Use the **Release** configuration.
- If it feels slow, press **R** (ray tracing), **Y** (soft shadows), **F** (fog) or **O** (outlines) to turn those off.
- Trees and bushes use less detail with distance, and off-screen objects are skipped.
