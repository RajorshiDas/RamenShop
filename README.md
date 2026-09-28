# 3D Ramen Shop: setup and how the code works

Everything is in one file, `main.cpp`. It uses classic OpenGL plus **freeglut** (which also gives you GLU for cylinders, disks and spheres). No textures, shaders, lighting, animation or GUI.

The only extras are a few camera keys so you can look around, and thin dark edge outlines so shapes are readable without lighting:

| Key | Action |
|---|---|
| ← / → | orbit around the shop |
| ↑ / ↓ | zoom in / out |
| Page Up / Page Down | camera higher / lower |
| H | hide / show the roof (look inside) |
| O | outlines on / off |
| Esc | quit |

---

## Quick start with the ready-made project (RamenShop.zip)

1. Unzip `RamenShop.zip` somewhere simple, like `C:\Projects\RamenShop` (not inside the zip viewer).
2. Double-click `RamenShop.sln`. Visual Studio 2022 opens it (VS 2019 asks to retarget; click OK).
3. In Solution Explorer, right-click **Solution 'RamenShop'** > **Restore NuGet Packages** (needs internet the first time).
4. Press **F5**. The build copies `freeglut.dll` next to the program automatically.

If it still says a file is missing, open **Project > Manage NuGet Packages > Installed**, uninstall and reinstall `nupengl.core`, and build again.

---

## Visual Studio setup by hand (freeglut via NuGet)

1. **File > New > Project > Empty Project** (C++). Name it e.g. `RamenShop`.
2. In Solution Explorer, right-click **Source Files > Add > Existing Item...** and add `main.cpp`.
3. **Project > Manage NuGet Packages... > Browse**, search **`nupengl.core`**, click **Install**. This gives you freeglut (and GLEW, which this project doesn't use). The headers, `.lib` files and `freeglut.dll` are set up for you.
4. Press **F5**.

**Alternative (vcpkg):** `vcpkg install freeglut:x64-windows`, then `vcpkg integrate install`. Build as x64.

**Common problems**
- *Cannot open include file `GL/freeglut.h`*: the NuGet package isn't installed for *this* project. Repeat step 3.
- *`freeglut.dll` was not found* when running: copy `freeglut.dll` from the NuGet package folder (under `packages\` in your solution folder) next to your `.exe`.
- *Linker errors*: make sure the platform (x64 / Win32) matches the one the library was installed for.

You don't need to add `opengl32.lib`, `glu32.lib` or `freeglut.lib` by hand. `freeglut.h` links them automatically in Visual Studio.

---

## 1. What each shape function does

Every basic shape takes `(position, rotation, scale, colour)`. At scale `{1,1,1}` each one is about 1 unit (≈ 1 metre) big.

| Function | What it draws | Where `pos` is |
|---|---|---|
| `drawCube` | 1×1×1 box | centre |
| `drawCuboid` | box of size `{w,h,d}` | centre of the **bottom** face (it stands on `pos.y`) |
| `drawCylinder` | cylinder, diameter 1, height 1, pointing up | centre of the bottom |
| `drawCylinderCustom` | cylinder where you choose bottom radius, top radius and height (tapered) | centre of the bottom |
| `drawCone` | cone, base diameter 1, height 1 | centre of the base |
| `drawSphere` | sphere, diameter 1 | centre |
| `drawTorus` | ring/donut lying flat; optional tube and ring radius | centre |
| `drawPlane` | flat 1×1 square on the ground | centre |
| `drawBoard` | thin board 1 × 0.05 × 1 (lying flat) | centre |
| `drawWedge` | triangular prism (roof gable, mountain picture) | centre of the bottom |
| `drawBowl` | foot ring + sloped open wall + bottom + rim | centre of the bottom |
| `drawPlate` | very flat bowl | centre of the bottom |
| `drawCup` | open cylinder with a ring handle | centre of the bottom |
| `drawBottle` | body + shoulder + neck + cap (made of cylinders) | centre of the bottom |

`drawBottle`, `drawCup`, `drawPlate` and `drawPlant` are listed in several of your sections. Each exists **once** and is reused everywhere.

Bigger objects (`drawTable`, `drawRamenBowl`, `drawShopBuilding` and so on) take `(position, rotation = none, scale = normal)`, so you can call them with only a position: `drawTable({ 2, FLOOR_Y, 1 });`

## 2. Changing an object's position

Change the first `{x, y, z}`:

```cpp
drawTable({ 2.9f, FLOOR_Y, 1.9f });   // original
drawTable({ 1.0f, FLOOR_Y, 0.5f });   // moved left and backward
```

`x` is left/right, `y` is up/down, `z` is forward/backward (+Z is toward the street). Things inside the shop sit on `FLOOR_Y` (0.1, the top of the wooden floor). Things outside sit on `y = 0` (grass) or `0.12` (sidewalk).

## 3. Changing its size

Change the scale `{sx, sy, sz}`. `{1,1,1}` is normal size, `{2,2,2}` is double, and `{1, 0.5f, 1}` is half as tall:

```cpp
drawPlant({ 4.3f, 0.12f, 4.5f }, NO_ROT, { 1.3f, 1.3f, 1.3f });   // 30% bigger plant
drawTable({ 0, FLOOR_Y, 0 }, NO_ROT, { 2, 1, 1 });                  // twice as long
```

For basic shapes, the scale **is** the size. `drawCube({0,1,0}, NO_ROT, {10, 2, 0.2f}, WALL)` is a box 10 wide, 2 tall and 0.2 thick.

## 4. Rotating it

Change the rotation `{rx, ry, rz}` (degrees):

- `ry` turns it left/right (most common): `drawChair({x, FLOOR_Y, z}, { 0, 180, 0 });` makes the chair face the other way.
- `rx` tilts it forward/back. The roof boards use `{ 20.6f, 0, 0 }`.
- `rz` rolls it sideways. `{ 0, 0, -90 }` lays an upright cylinder along the X axis (the roof ridge, chopsticks, the noren rod).

Rotation happens around the object's own `pos`, so for floor objects it turns in place.

## 5. How shapes are combined into larger objects

Every draw function follows the same pattern:

```cpp
void drawTable(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();                    // 1. remember the current coordinate system
    applyTransform(pos, rot, scale);   // 2. move / rotate / scale to where the table goes

    // 3. draw parts in the table's OWN local coordinates (0,0,0 = the table's feet)
    drawCuboid({ 0, 0.72f, 0 }, NO_ROT, { 1.2f, 0.06f, 0.8f }, LIGHT_WOOD);   // top
    drawCuboid({ 0.52f, 0, 0.32f }, NO_ROT, { 0.07f, 0.72f, 0.07f }, DARK_WOOD); // a leg
    ...

    glPopMatrix();                     // 4. go back, so nothing else is affected
}
```

`applyTransform()` does `glTranslatef`, then `glRotatef` for X, Y and Z, then `glScalef`. Because of step 2, moving, turning or scaling the table moves every part together. Because of steps 1 and 4, it never affects anything drawn afterwards.

Objects can contain other objects, several levels deep:

```
display()
 ├── drawGround()    grass plane, drawSidewalk(), drawStreet()
 ├── drawExterior()  drawShopBuilding(), drawRoof(), drawDoor(), drawWindow() x2,
 │                   drawSignBoard(), lanterns, plants, bench, lamps
 └── drawInterior()  drawCounter() + stools + ramen sets, 2 tables with 4 chairs each,
                     drawKitchen() (cabinets, stove, drawCookingPot(), drawSink(), plates),
                     drawShelf() x2 with bowls/cups/bottles, menu boards, wall pictures
```

Example: `drawRamenBowl()` = `drawBowl()` + a flat `drawCylinder()` (soup) + `drawRamenNoodles()` (tori) + `drawMeat()` + `drawEgg()` (spheres) + `drawSeaweed()` (board) + `drawGreenOnion()` (tiny tori). The food is designed with the bowl 1 unit wide. The shop then draws the whole thing at scale `0.22`, so everything shrinks together.

Reuse example: the sliding door panel is just `drawWindow()` scaled tall and thin, with a paper board behind it.

## 6. Where to modify the code for your own objects

- **Move, add or remove things in the scene:** section 9, in `drawExterior()` and `drawInterior()`. This is where every object is placed. For example, copy a `drawTable(...)` line and change its position.
- **Change what an object looks like:** edit its function in sections 4–8. For example, add a 5th leg in `drawTable()` or make the counter longer in `drawCounter()`.
- **Make a new object:** copy an existing function such as `drawBench()`, rename it (`drawFridge`), keep the `glPushMatrix / applyTransform / glPopMatrix` lines, and replace the parts in the middle with basic shapes. Then call it from `drawInterior()`.
- **Colours:** the list of `const Color` values near the top of the file.
- **Smoothness of round shapes:** `SLICES` and `STACKS` near the top.

Tip: build a new object around `(0,0,0)` with its bottom at `y = 0`. Then a single `pos` places it on any floor.

---

## Splitting into files later (optional)

When you are comfortable, cut the sections into files:

- `shapes.h/.cpp`: section 1–3 (Vec3, Color, colours, applyTransform, basic shapes)
- `shop.h/.cpp`: sections 4–6 and 8
- `food.h/.cpp`: section 7
- `main.cpp`: sections 9–10

In each `.h`, put the function declarations **with** their default arguments, for example `void drawTable(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);`. In the `.cpp`, write the definitions **without** the `= ...` defaults. Colour constants and `quad`/`showOutlines` need `extern` in the header, with the single definition in `shapes.cpp`.
