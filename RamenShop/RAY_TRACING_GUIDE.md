# 🍜 Real-Time Ray Tracing Engine - 3D Ramen Shop

## 🌟 Overview
Your Ramen Shop now features a **Real-Time GPU Ray Tracing Engine** integrated directly into the application! 

With a single key press (**`R`**), you can toggle between traditional rasterization (Gouraud/Phong) and **true physical Ray Tracing** with multiple specular reflection bounces, dielectric glass refraction, ray-traced soft shadows, and contact ambient occlusion.

---

## 🎮 Controls Quick Reference

| Key | Function | Description |
|:---:|:---|:---|
| **`R`** | **Toggle Ray Tracing** | Switch between Real-Time Ray Tracing and Rasterized mode |
| **`B`** | **Cycle Ray Bounces** | Adjust reflection/refraction bounce depth (1 to 5 bounces) |
| **`Y`** | **Toggle Soft Shadows** | Switch between smooth penumbra soft shadows and sharp hard shadows |
| **`C`** | **Cycle Camera Mode** | Orbit Camera ↔ Walkthrough (FPS) ↔ Focused Counter View |
| **`T`** | **Day / Night Cycle** | Sunlight streaming through open facade ↔ Cozy nighttime lanterns |
| **`H`** | **Toggle Roof** | Remove roof to inspect the ray-traced interior from orbit view |
| **`1` – `7`** | **Toggle Lights** | Dynamically enable/disable Ambient, Moon/Sun, Pendant, Lanterns, Spotlight, Area Lights |
| **`W A S D`** | **Walk Around** | Freely explore the shop in Walkthrough mode while ray tracing in real time |
| **Mouse Drag** | **Look / Orbit** | Smooth camera orbit or first-person look with instant ray updates |

---

## 🔬 Ray Tracing Capabilities & Physics

### 1. 🪞 Recursive Specular Reflections
- **Polished Lacquer Counter**: The wooden service counter has an ultra-low roughness lacquer coat that casts sharp, crisp mirror reflections of the ramen bowls, chopsticks, glass carafe, lanterns, and customer stools.
- **Stainless Steel Stockpots & Stool Columns**: Conductive metallic Fresnel reflection with 95% specularity, reflecting the interior environment and cooking area.
- **Ceramic Glazed Floor**: Subtly reflects the stools, timber framing, and warm overhead pendant light with realistic Fresnel falloff.
- **Tonkotsu Broth Liquid**: Mirror-smooth liquid surface inside each bowl reflecting the overhead ceiling lanterns and room interior.
- **Up to 5 Bounces**: Press **`B`** to see reflections inside reflections (e.g., chrome stools reflecting the counter reflecting the lanterns).

### 2. 🧊 Dielectric Glass Refraction (Snell's Law)
- **Clear Glass Water Pitcher & Carafe**:
  - Uses Snell's law refraction ($n = 1.52$ for optical glass, $n = 1.33$ for internal water volume).
  - Light rays bend as they penetrate the cylinder, magnifying and distorting the countertop and wooden textures behind it.
  - Floating lemon slice inside refracts through the glass cylinder wall.
  - Total Internal Reflection (TIR) and Fresnel transmission blend at grazing angles.
- **Clear Glass Spice / Condiment Jars**:
  - Three spice jars containing Shichimi togarashi (chili), pickled garlic, and roasted sesame seeds, with translucent refractive glass walls.

### 3. 🌑 True Ray-Traced Shadow Rays
- **Directional Sunlight / Moonlight**: Casts long, crisp shadows through the front opening and timber lattice windows onto the sidewalk and shop floor.
- **Warm Interior Pendant Light**: Casts radial shadows from customer stools, counter edges, and tableware.
- **Exterior Street Lanterns**: Cast cozy, flickering warm light and shadows across the sidewalk and awning.
- **Chef Prep Spotlight**: Downward conical beam with sharp cutoff and penumbra falloff.
- **Soft Shadows (Key `Y`)**: Uses stratified multi-jittered shadow rays to produce realistic soft penumbras.

### 4. 🌓 Dynamic Day & Night Real-Time Lighting
- **Night Mode (`T`)**: Deep evening sky, glowing moon, warm 2700K interior tungsten illumination, glowing paper lanterns, and cozy ambient atmosphere.
- **Day Mode (`T`)**: Bright clear blue sky, warm sunlight streaming into the shop, higher ambient clarity, and natural daytime illumination.

### 5. 🍜 Detailed Scene Modeling in Ray Tracer
- **3 Steaming Ceramic Ramen Bowls** with rich golden broth, boiled egg halves (white + golden yolk), tender braised chashu pork, nori seaweed, and chopsticks.
- **Customer Stools** with polished chrome pedestal bases, stainless steel columns, and crimson fabric cushions.
- **Kitchen Cooking Station** with prep table, wall shelves, and dual large stainless stockpots.
- **Front Awning & Timber Framing** with traditional indigo awning, timber posts, and shoji window glow.

---

## 🏗️ Architecture & Implementation Files

- **`raytracer.h`**: Public API and state management (`useRayTracing`, `rayTraceBounces`, `rayTraceSoftShadows`, `renderRayTracedFrame()`).
- **`raytracer.cpp`**: GLSL Ray Tracing engine, camera basis math, ray-AABB/cylinder/sphere/disc intersection mathematics, multi-bounce recursive optics, soft shadow sampling, and on-screen HUD badge.
- **`camera.cpp` / `camera.h`**: Integrated `R`, `B`, `Y` hotkeys, camera orientation calculations, and live window title status display.
- **`main.cpp`**: Integration into the display loop and startup initialization (`initRayTracer()`).
