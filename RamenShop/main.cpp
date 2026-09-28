/*
 * ============================================================================
 *  3D RAMEN SHOP  -  geometry only
 *  C++ + classic (fixed-function) OpenGL + freeglut
 *
 *  No textures, no shaders, no lighting, no animation, no GUI.
 *  Every object is built from a few reusable basic shapes.
 * ============================================================================
 *
 *  Coordinate system:
 *      X = left / right
 *      Y = up / down        (the ground is at Y = 0)
 *      Z = forward / backward
 *  The shop is centred around X = 0, Z = 0.
 *  The FRONT of the shop (door, sign) faces +Z, towards the street.
 *  1 unit is roughly 1 metre.
 *
 *  File layout (top to bottom):
 *      1. Setup: includes, Vec3 / Color, colours, global settings
 *      2. applyTransform()  - position / rotation / scale in one place
 *      3. BASIC SHAPES      - drawCube, drawCuboid, drawCylinder, drawSphere,
 *                             drawCone, drawTorus, drawPlane, drawBoard,
 *                             drawCylinderCustom, drawWedge, drawBowl,
 *                             drawPlate, drawCup, drawBottle
 *      4. EXTERIOR          - drawShopBuilding, drawRoof, drawDoor, drawWindow,
 *                             drawSignBoard, drawStreet, drawSidewalk,
 *                             drawLamp, drawPlant
 *      5. FURNITURE         - drawTable, drawChair, drawStool, drawBench
 *      6. INTERIOR          - drawCounter, drawShelf, drawCabinet, drawSink,
 *                             drawCookingPot, drawKitchen
 *      7. FOOD              - drawRamenNoodles, drawEgg, drawMeat, drawSeaweed,
 *                             drawGreenOnion, drawRamenBowl, drawChopsticks,
 *                             drawSpoon
 *      8. DECORATIONS       - drawLantern, drawMenuBoard, drawWallDecoration
 *      9. SCENE             - drawGround, drawExterior, drawInterior, display
 *     10. Window, camera keys and main()
 *
 *  Camera keys (only so you can look around the model):
 *      LEFT / RIGHT arrows : orbit around the shop
 *      UP / DOWN arrows    : zoom in / out
 *      PAGE UP / PAGE DOWN : camera higher / lower
 *      H                   : hide / show the roof (to look inside)
 *      O                   : dark edge outlines on / off
 *      ESC                 : quit
 */

#include <GL/freeglut.h>   // also includes gl.h and glu.h
#include <cmath>
#include <cstdlib>


// ============================================================================
// 1. SETUP
// ============================================================================

const float PI = 3.14159265f;
const int SLICES = 24;     // how round circles look (more = smoother)
const int STACKS = 16;     // same, for spheres (top to bottom)

// A position, rotation (in degrees) or scale: three numbers x, y, z.
struct Vec3 { float x, y, z; };

// A colour: red, green, blue, each from 0.0 to 1.0.
struct Color { float r, g, b; };

const Vec3 NO_ROT = { 0, 0, 0 };   // "no rotation"
const Vec3 ONE    = { 1, 1, 1 };   // "normal size"

// ---- Colours (change these to recolour the whole scene) ----
const Color SKY          = { 0.10f, 0.10f, 0.16f };   // dark evening sky
const Color GRASS        = { 0.40f, 0.62f, 0.33f };
const Color ASPHALT      = { 0.25f, 0.25f, 0.27f };
const Color SIDEWALK     = { 0.72f, 0.72f, 0.70f };
const Color GRAY         = { 0.50f, 0.50f, 0.50f };
const Color DARK_GRAY    = { 0.22f, 0.22f, 0.24f };
const Color WHITE        = { 1.00f, 1.00f, 1.00f };
const Color BLACK        = { 0.08f, 0.08f, 0.08f };
const Color WALL         = { 0.93f, 0.88f, 0.76f };   // cream plaster
const Color DARK_WOOD    = { 0.35f, 0.20f, 0.10f };
const Color WOOD         = { 0.60f, 0.40f, 0.22f };
const Color LIGHT_WOOD   = { 0.82f, 0.64f, 0.42f };
const Color FLOOR_WOOD   = { 0.55f, 0.42f, 0.30f };
const Color ROOF_TILE    = { 0.27f, 0.31f, 0.38f };
const Color RED          = { 0.80f, 0.10f, 0.10f };
const Color DARK_RED     = { 0.50f, 0.05f, 0.05f };
const Color NAVY         = { 0.10f, 0.15f, 0.35f };
const Color GOLD         = { 0.95f, 0.80f, 0.25f };
const Color PAPER        = { 0.97f, 0.95f, 0.88f };
const Color METAL        = { 0.45f, 0.45f, 0.50f };
const Color STEEL        = { 0.78f, 0.78f, 0.80f };
const Color CUSHION      = { 0.70f, 0.12f, 0.12f };
const Color CLAY_POT     = { 0.72f, 0.40f, 0.25f };
const Color SOIL         = { 0.30f, 0.20f, 0.10f };
const Color LEAF         = { 0.20f, 0.55f, 0.22f };
const Color LEAF_LIGHT   = { 0.35f, 0.68f, 0.30f };
const Color BOWL_RED     = { 0.78f, 0.18f, 0.14f };
const Color PLATE_WHITE  = { 0.92f, 0.94f, 0.97f };
const Color CUP_GREEN    = { 0.55f, 0.70f, 0.55f };
const Color SOY          = { 0.28f, 0.12f, 0.06f };
const Color BROTH        = { 0.87f, 0.62f, 0.32f };
const Color NOODLE       = { 0.98f, 0.88f, 0.50f };
const Color EGG_WHITE    = { 1.00f, 0.99f, 0.94f };
const Color YOLK         = { 1.00f, 0.60f, 0.10f };
const Color MEAT         = { 0.80f, 0.52f, 0.42f };
const Color MEAT_EDGE    = { 0.55f, 0.30f, 0.20f };
const Color NORI         = { 0.08f, 0.18f, 0.10f };
const Color ONION_GREEN  = { 0.40f, 0.80f, 0.30f };
const Color MOUNTAIN     = { 0.35f, 0.45f, 0.65f };
const Color TILE_FLOOR   = { 0.55f, 0.53f, 0.50f };   // gray tile floor (Image 1)
const Color UPPER_WALL   = { 0.15f, 0.18f, 0.28f };   // dark navy upper facade (Image 2)
const Color LANTERN_CREAM = { 0.95f, 0.90f, 0.75f };  // cream paper lanterns (exterior)
const Color LANTERN_ORANGE = { 0.95f, 0.55f, 0.15f }; // orange cylinder lanterns (interior)
const Color FENCE_WOOD   = { 0.28f, 0.16f, 0.08f };   // dark fence pickets
const Color TRUNK        = { 0.30f, 0.22f, 0.12f };   // tree trunk
const Color AWNING_BLUE  = { 0.18f, 0.22f, 0.35f };   // zigzag awning

// ---- Global settings ----
GLUquadric* quad = nullptr;     // GLU helper object used for cylinders, disks, spheres
bool  showRoof     = true;
bool  showOutlines = true;      // thin dark edges so shapes are readable without lighting
float camAngle     = 30.0f;     // degrees around the shop
float camDistance  = 20.0f;
float camHeight    = 8.0f;

const float FLOOR_Y = 0.1f;     // top of the shop's wooden floor (floor is 0.1 thick)


// ============================================================================
// 2. TRANSFORM HELPER
//    Every draw function calls this right after glPushMatrix().
//    Order: move to position -> rotate -> scale.
// ============================================================================

void applyTransform(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(rot.x, 1, 0, 0);    // rotate around X axis (tilt forward/back)
    glRotatef(rot.y, 0, 1, 0);    // rotate around Y axis (turn left/right)
    glRotatef(rot.z, 0, 0, 1);    // rotate around Z axis (roll sideways)
    glScalef(scale.x, scale.y, scale.z);
}

void setColor(Color c)
{
    glColor3f(c.r, c.g, c.b);
}

// Darker version of a colour, used for outlines.
Color darker(Color c)
{
    return { c.r * 0.55f, c.g * 0.55f, c.b * 0.55f };
}

// Set emission (self-glow) on a surface: makes lanterns/paper glow even in shadow.
// Call setEmission(0,0,0) afterwards to turn it off for the next object.
void setEmission(float r, float g, float b)
{
    GLfloat em[] = { r, g, b, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
}
void clearEmission() { setEmission(0, 0, 0); }

// ---- Small internal helpers (used INSIDE shape functions, no push/pop of their own) ----

// Open tube (no caps) standing on y0, pointing up +Y.
void rawTube(float bottomRadius, float topRadius, float height, float y0 = 0)
{
    glPushMatrix();
    glTranslatef(0, y0, 0);
    glRotatef(-90, 1, 0, 0);      // GLU builds cylinders along +Z; turn them to +Y
    gluCylinder(quad, bottomRadius, topRadius, height, SLICES, 1);
    glPopMatrix();
}

// Flat filled circle lying at height y.
void rawDisk(float radius, float y)
{
    glPushMatrix();
    glTranslatef(0, y, 0);
    glRotatef(-90, 1, 0, 0);
    gluDisk(quad, 0, radius, SLICES, 1);
    glPopMatrix();
}

// Dark circle line at height y (outline for round shapes).
void rawCircleOutline(float radius, float y, Color c)
{
    if (!showOutlines || radius <= 0) return;
    glDisable(GL_LIGHTING);
    setColor(darker(c));
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < SLICES; i++) {
        float a = 2 * PI * i / SLICES;
        glVertex3f(radius * cos(a), y, radius * sin(a));
    }
    glEnd();
    glEnable(GL_LIGHTING);
    setColor(c);
}


// ============================================================================
// 3. BASIC SHAPES
//    All take:  position, rotation (degrees), scale, colour.
//    At scale {1,1,1} every basic shape is about 1 unit big.
// ============================================================================

// CUBE: 1 x 1 x 1 box, CENTRED on pos.
void drawCube(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    glutSolidCube(1.0);
    if (showOutlines) {
        glDisable(GL_LIGHTING);
        setColor(darker(c));
        glutWireCube(1.0);
        glEnable(GL_LIGHTING);
    }
    glPopMatrix();
}

// RECTANGULAR CUBOID: a box whose BOTTOM sits on pos.y.
// size = width (x), height (y), depth (z). Handy for anything standing on a floor.
void drawCuboid(Vec3 pos, Vec3 rot, Vec3 size, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, ONE);
    drawCube({ 0, size.y / 2, 0 }, NO_ROT, size, c);   // lift by half its height
    glPopMatrix();
}

// CYLINDER WITH DIFFERENT HEIGHT / RADIUS: bottom on pos.y, points up +Y.
// bottomRadius and topRadius can differ (tapered), topRadius = 0 makes a cone.
void drawCylinderCustom(Vec3 pos, Vec3 rot, Vec3 scale, Color c,
                        float bottomRadius, float topRadius, float height)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    rawTube(bottomRadius, topRadius, height);   // side
    rawDisk(bottomRadius, 0);                   // bottom cap
    rawDisk(topRadius, height);                 // top cap
    rawCircleOutline(bottomRadius, 0, c);
    rawCircleOutline(topRadius, height, c);
    glPopMatrix();
}

// CYLINDER: diameter 1, height 1, bottom on pos.y.
void drawCylinder(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    drawCylinderCustom(pos, rot, scale, c, 0.5f, 0.5f, 1.0f);
}

// CONE: base diameter 1, height 1, base on pos.y, tip pointing up.
void drawCone(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    drawCylinderCustom(pos, rot, scale, c, 0.5f, 0.0f, 1.0f);
}

// SPHERE: diameter 1, CENTRED on pos.
void drawSphere(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    gluSphere(quad, 0.5, SLICES, STACKS);
    glPopMatrix();
}

// TORUS (ring / donut): lies flat like a ring on a table, CENTRED on pos.
// tubeRadius = thickness of the ring, ringRadius = size of the ring.
void drawTorus(Vec3 pos, Vec3 rot, Vec3 scale, Color c,
               float tubeRadius = 0.1f, float ringRadius = 0.4f)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    glRotatef(90, 1, 0, 0);    // GLUT's torus stands up; lay it flat
    glutSolidTorus(tubeRadius, ringRadius, 12, SLICES);
    glPopMatrix();
}

// PLANE: flat 1 x 1 square lying on the ground (XZ), CENTRED on pos.
void drawPlane(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-0.5f, 0, -0.5f);
    glVertex3f( 0.5f, 0, -0.5f);
    glVertex3f( 0.5f, 0,  0.5f);
    glVertex3f(-0.5f, 0,  0.5f);
    glEnd();
    glPopMatrix();
}

// THIN RECTANGULAR BOARD: 1 x 0.05 x 1, CENTRED on pos (lying flat).
// scale.y makes it thicker/thinner. Rotate {90,0,0} to stand it up.
void drawBoard(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    drawCube(pos, rot, { scale.x, scale.y * 0.05f, scale.z }, c);
}

// WEDGE (triangular prism): used for the roof gable and the mountain picture.
// Bottom is 1 x 1 on pos.y, the ridge (top edge) runs along X at height 1.
void drawWedge(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    glBegin(GL_TRIANGLES);                         // the two triangle ends
    glNormal3f(-1, 0, 0);
    glVertex3f(-0.5f, 0, -0.5f); glVertex3f(-0.5f, 0, 0.5f); glVertex3f(-0.5f, 1, 0);
    glNormal3f( 1, 0, 0);
    glVertex3f( 0.5f, 0, -0.5f); glVertex3f( 0.5f, 0, 0.5f); glVertex3f( 0.5f, 1, 0);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(0, -1, 0);
    glVertex3f(-0.5f, 0, -0.5f); glVertex3f(0.5f, 0, -0.5f); glVertex3f(0.5f, 0, 0.5f); glVertex3f(-0.5f, 0, 0.5f); // bottom
    glNormal3f(0, 1, 2);                          // front slope (GL_NORMALIZE handles length)
    glVertex3f(-0.5f, 0,  0.5f); glVertex3f(0.5f, 0,  0.5f); glVertex3f(0.5f, 1, 0);    glVertex3f(-0.5f, 1, 0);    // front slope
    glNormal3f(0, 1, -2);                          // back slope
    glVertex3f(-0.5f, 0, -0.5f); glVertex3f(-0.5f, 1, 0);    glVertex3f(0.5f, 1, 0);    glVertex3f(0.5f, 0, -0.5f); // back slope
    glEnd();
    glPopMatrix();
}

// SIMPLE BOWL: foot ring + sloped open wall + bottom + round rim.
// Diameter 1 at the top, 0.4 tall, bottom on pos.y.
void drawBowl(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    rawTube(0.22f, 0.22f, 0.06f);          // foot ring
    rawTube(0.25f, 0.50f, 0.34f, 0.06f);   // sloped side wall (open at top)
    rawDisk(0.25f, 0.06f);                 // inside bottom
    glPushMatrix();                        // rounded rim
    glTranslatef(0, 0.4f, 0);
    glRotatef(90, 1, 0, 0);
    glutSolidTorus(0.015, 0.5, 8, SLICES);
    glPopMatrix();
    rawCircleOutline(0.5f, 0.4f, c);
    rawCircleOutline(0.25f, 0.06f, c);
    glPopMatrix();
}

// PLATE: very flat bowl. Diameter 1, 0.08 tall, bottom on pos.y.
void drawPlate(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    rawTube(0.30f, 0.50f, 0.08f);          // sloped edge
    rawDisk(0.30f, 0.005f);                // flat middle
    rawCircleOutline(0.5f, 0.08f, c);
    rawCircleOutline(0.3f, 0.005f, c);
    glPopMatrix();
}

// CUP: open cylinder with a bottom and a ring handle. About 0.7 wide, 1 tall.
void drawCup(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    rawTube(0.30f, 0.35f, 1.0f);           // wall
    rawDisk(0.30f, 0.0f);                  // bottom
    glPushMatrix();                        // handle: a standing ring on the +X side
    glTranslatef(0.42f, 0.5f, 0);
    glutSolidTorus(0.05, 0.18, 8, 16);
    glPopMatrix();
    rawCircleOutline(0.35f, 1.0f, c);
    glPopMatrix();
}

// BOTTLE: built from other basic shapes. 1 unit tall, bottom on pos.y.
void drawBottle(Vec3 pos, Vec3 rot, Vec3 scale, Color c, Color capColor = RED)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.4f, 0.55f, 0.4f }, c);              // body
    drawCylinderCustom({ 0, 0.55f, 0 }, NO_ROT, ONE, c, 0.2f, 0.07f, 0.15f);  // shoulder
    drawCylinder({ 0, 0.70f, 0 }, NO_ROT, { 0.14f, 0.2f, 0.14f }, c);         // neck
    drawCylinder({ 0, 0.90f, 0 }, NO_ROT, { 0.17f, 0.1f, 0.17f }, capColor);  // cap
    glPopMatrix();
}


// ============================================================================
// 4. EXTERIOR
//    Bigger objects: each one = push, applyTransform, several basic shapes
//    placed in its OWN local coordinates, pop.
// ============================================================================

// Building: open front on ground floor, dark navy upper facade (like reference photos).
// 10 wide (x -5..5), 8 deep (z -4..4). Ground floor 3.3 tall, upper facade +2.0.
void drawShopBuilding(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float GH = 3.3f;   // ground floor ceiling height
    float UH = 2.0f;   // upper facade height
    float TH = GH + UH; // 5.3 total

    // == FLOOR: gray tiles (Image 1 shows tiled floor) ==
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 10, 0.1f, 8 }, TILE_FLOOR);

    // == GROUND FLOOR: cream/wood walls, OPEN FRONT (Image 1 & 2) ==
    drawCuboid({  0,    0, -3.9f }, NO_ROT, { 10,   GH, 0.2f }, WALL);  // back wall
    // Side walls: wood lower half, cream upper (like Image 2 exterior)
    for (int sx = -1; sx <= 1; sx += 2) {
        drawCuboid({ sx * 4.9f, 0, 0 },    NO_ROT, { 0.2f, 1.2f, 8 }, WOOD);
        drawCuboid({ sx * 4.9f, 1.2f, 0 }, NO_ROT, { 0.2f, GH - 1.2f, 8 }, WALL);
    }

    // FRONT: open - just structural dark wood posts (no wall at ground level)
    drawCuboid({ -4.9f, 0, 3.9f }, NO_ROT, { 0.25f, GH, 0.25f }, DARK_WOOD);
    drawCuboid({  4.9f, 0, 3.9f }, NO_ROT, { 0.25f, GH, 0.25f }, DARK_WOOD);
    drawCuboid({ -1.8f, 0, 3.9f }, NO_ROT, { 0.15f, GH, 0.15f }, DARK_WOOD);
    drawCuboid({  1.8f, 0, 3.9f }, NO_ROT, { 0.15f, GH, 0.15f }, DARK_WOOD);
    // Horizontal beam across front at ceiling height
    drawCuboid({ 0, GH - 0.15f, 3.95f }, NO_ROT, { 10.2f, 0.18f, 0.14f }, DARK_WOOD);

    // == UPPER FACADE: dark navy (Image 2 upper floor) ==
    drawCuboid({  0,    GH, 3.9f },  NO_ROT, { 10, UH, 0.2f }, UPPER_WALL);  // front
    drawCuboid({  0,    GH, -3.9f }, NO_ROT, { 10, UH, 0.2f }, UPPER_WALL);  // back
    drawCuboid({ -4.9f, GH, 0 },    NO_ROT, { 0.2f, UH, 8 },  UPPER_WALL);  // left
    drawCuboid({  4.9f, GH, 0 },    NO_ROT, { 0.2f, UH, 8 },  UPPER_WALL);  // right

    // Full-height dark wood corner posts
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * 4.9f, 0, sz * 3.9f }, NO_ROT, { 0.3f, TH, 0.3f }, DARK_WOOD);

    // Horizontal dark beam at the very top of the front
    drawCuboid({ 0, TH - 0.1f, 4.02f }, NO_ROT, { 10.4f, 0.15f, 0.1f }, DARK_WOOD);
    // Horizontal beam dividing ground/upper on back wall
    drawCuboid({ 0, GH - 0.05f, -3.82f }, NO_ROT, { 10.2f, 0.12f, 0.1f }, DARK_WOOD);

    glPopMatrix();
}

// Roof: flat dark slab on top + zigzag awning over the open front (like reference Image 2).
// TH = 5.3 (top of the upper facade walls).
void drawRoof(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float TH = 5.3f;

    // Main flat roof slab with overhang
    drawCuboid({ 0, TH, 0 }, NO_ROT, { 11.2f, 0.18f, 9.2f }, DARK_GRAY);
    // Slight raised edge trim
    drawCuboid({ 0, TH + 0.18f, 4.6f },  NO_ROT, { 11.2f, 0.06f, 0.15f }, DARK_GRAY);
    drawCuboid({ 0, TH + 0.18f, -4.6f }, NO_ROT, { 11.2f, 0.06f, 0.15f }, DARK_GRAY);

    // Zigzag / wave awning across the open front between ground floor and upper
    // (the blue-white striped awning from Image 2, at y ~ 3.3, sticking out over the sidewalk)
    drawBoard({ 0, 3.22f, 4.7f }, { 12, 0, 0 }, { 10.6f, 1.2f, 1.4f }, AWNING_BLUE);
    // Zigzag teeth under the awning (triangular points hanging down)
    for (int i = -18; i <= 18; i++) {
        float x = i * 0.28f;
        drawCone({ x, 2.95f, 5.15f }, { 15, 0, 0 }, { 0.28f, 0.25f, 0.18f }, AWNING_BLUE);
    }

    // Dark eave/fascia trim under the main roof
    drawCuboid({ 0, TH - 0.02f, 4.55f }, NO_ROT, { 11.2f, 0.10f, 0.4f }, DARK_WOOD);
    drawCuboid({ 0, TH - 0.02f, -4.55f }, NO_ROT, { 11.2f, 0.10f, 0.4f }, DARK_WOOD);

    glPopMatrix();
}

// WINDOW: dark wood frame with a simple lattice (no glass). 2 wide, 1.6 tall, centred.
void drawWindow(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Glowing paper panel behind the lattice (warm interior glow like reference photos)
    setEmission(0.45f, 0.30f, 0.12f);
    drawCube({ 0, 0, -0.06f }, NO_ROT, { 1.9f, 1.5f, 0.02f }, PAPER);
    clearEmission();

    // Outer frame
    drawCube({ 0,  0.8f, 0 }, NO_ROT, { 2.1f, 0.1f, 0.25f }, DARK_WOOD);   // top
    drawCube({ 0, -0.8f, 0 }, NO_ROT, { 2.1f, 0.1f, 0.25f }, DARK_WOOD);   // bottom
    drawCube({ -1.0f, 0, 0 }, NO_ROT, { 0.1f, 1.7f, 0.25f }, DARK_WOOD);   // left
    drawCube({  1.0f, 0, 0 }, NO_ROT, { 0.1f, 1.7f, 0.25f }, DARK_WOOD);   // right

    // Lattice bars (shoji grid)
    for (int i = -1; i <= 1; i++)
        drawCube({ i * 0.5f, 0, 0 }, NO_ROT, { 0.04f, 1.6f, 0.05f }, DARK_WOOD);
    drawCube({ 0,  0.40f, 0 }, NO_ROT, { 2.0f, 0.04f, 0.05f }, DARK_WOOD);
    drawCube({ 0,  0.00f, 0 }, NO_ROT, { 2.0f, 0.04f, 0.05f }, DARK_WOOD);
    drawCube({ 0, -0.40f, 0 }, NO_ROT, { 2.0f, 0.04f, 0.05f }, DARK_WOOD);

    // Window sill sticking out
    drawCube({ 0, -0.88f, 0.12f }, NO_ROT, { 2.3f, 0.06f, 0.35f }, WOOD);

    glPopMatrix();
}

// NOREN CURTAIN: hanging cloth panels inside the shop (between customer area and kitchen).
// Rod across the top, multiple dark navy panels hanging down (like Image 1).
void drawNoren(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    // Rod
    drawCylinder({ -3.5f, 0, 0 }, { 0, 0, -90 }, { 0.04f, 7.0f, 0.04f }, DARK_WOOD);
    // Hanging cloth panels (dark, like Image 1)
    for (int i = -4; i <= 4; i++)
        drawBoard({ i * 0.72f, -0.65f, 0.02f }, { 90, 0, 0 }, { 0.68f, 0.6f, 1.3f }, NAVY);
    glPopMatrix();
}

// SIGN BOARD: white board with dark text in a wood frame (Image 2 top sign style).
void drawSignBoard(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    drawCube({ 0, 0, -0.02f }, NO_ROT, { 2.8f, 0.85f, 0.10f }, DARK_WOOD);   // frame
    setEmission(0.15f, 0.14f, 0.12f);   // slight self-illumination so it reads at night
    drawCube({ 0, 0,  0.03f }, NO_ROT, { 2.6f, 0.70f, 0.06f }, WHITE);       // white board
    clearEmission();

    // Letters
    const char* text = "RAMEN";
    float textWidth = (float)glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char*)text);
    float s = 1.8f / textWidth;
    glDisable(GL_LIGHTING);
    setColor(BLACK);
    glLineWidth(3);
    glPushMatrix();
    glTranslatef(-0.9f, -50 * s, 0.07f);
    glScalef(s, s, s);
    for (const char* p = text; *p; p++)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    glPopMatrix();
    glLineWidth(1.5f);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

// STREET: asphalt with dashed white centre line. 60 long (x), 8 wide (z), centred.
void drawStreet(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawPlane({ 0, 0.02f, 0 }, NO_ROT, { 60, 1, 8 }, ASPHALT);
    for (float x = -28; x <= 28; x += 4)
        drawPlane({ x, 0.03f, 0 }, NO_ROT, { 2, 1, 0.2f }, WHITE);
    glPopMatrix();
}

// SIDEWALK: raised grey slab with tile lines and a curb. 60 long, 3 deep, centred.
void drawSidewalk(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 60, 0.12f, 3 }, SIDEWALK);
    for (float x = -29; x <= 29; x += 1.5f)
        drawPlane({ x, 0.125f, 0 }, NO_ROT, { 0.03f, 1, 3 }, GRAY);      // tile joints
    drawCuboid({ 0, 0, 1.45f }, NO_ROT, { 60, 0.16f, 0.12f }, GRAY);    // curb
    glPopMatrix();
}

// STREET LAMP: base, pole, arm and a cone-shaped shade with a bulb. About 3.5 tall.
void drawLamp(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 },       NO_ROT, { 0.35f, 0.2f, 0.35f }, DARK_GRAY);  // base
    drawCylinder({ 0, 0.2f, 0 },    NO_ROT, { 0.12f, 3.3f, 0.12f }, DARK_GRAY);  // pole
    drawCube({ 0.35f, 3.45f, 0 },   NO_ROT, { 0.8f, 0.08f, 0.08f }, DARK_GRAY);  // arm
    drawCone({ 0.7f, 3.15f, 0 },    NO_ROT, { 0.5f, 0.3f, 0.5f },   DARK_GRAY);  // shade
    setEmission(0.9f, 0.7f, 0.3f);
    drawSphere({ 0.7f, 3.12f, 0 },  NO_ROT, { 0.2f, 0.2f, 0.2f },   GOLD);       // bulb
    clearEmission();
    glPopMatrix();
}

// PLANT: clay pot, soil, stem and a bushy top made of spheres. About 1.3 tall.
void drawPlant(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinderCustom({ 0, 0, 0 }, NO_ROT, ONE, CLAY_POT, 0.2f, 0.28f, 0.4f);   // pot
    drawCylinder({ 0, 0.40f, 0 }, NO_ROT, { 0.5f, 0.02f, 0.5f }, SOIL);         // soil
    drawCylinder({ 0, 0.42f, 0 }, NO_ROT, { 0.05f, 0.4f, 0.05f }, DARK_WOOD);   // stem
    drawSphere({  0.00f, 0.90f,  0.00f }, NO_ROT, { 0.55f, 0.50f, 0.55f }, LEAF);
    drawSphere({  0.18f, 1.08f,  0.05f }, NO_ROT, { 0.40f, 0.40f, 0.40f }, LEAF_LIGHT);
    drawSphere({ -0.15f, 1.05f, -0.08f }, NO_ROT, { 0.40f, 0.40f, 0.40f }, LEAF_LIGHT);
    drawSphere({  0.00f, 1.20f,  0.00f }, NO_ROT, { 0.30f, 0.30f, 0.30f }, LEAF);
    glPopMatrix();
}


// CYLINDRICAL LANTERN: tall glowing paper lantern for interior (Image 1 style).
// About 0.8 tall, 0.44 wide. pos = centre. Hangs from a string.
void drawCylinderLantern(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setEmission(0.90f, 0.50f, 0.10f);
    drawCylinderCustom({ 0, -0.4f, 0 }, NO_ROT, ONE, LANTERN_ORANGE, 0.22f, 0.22f, 0.8f);
    clearEmission();
    drawTorus({ 0, -0.40f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.02f, 0.23f);
    drawTorus({ 0, -0.13f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.015f, 0.23f);
    drawTorus({ 0,  0.13f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.015f, 0.23f);
    drawTorus({ 0,  0.40f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.02f, 0.23f);
    drawCylinder({ 0, 0.40f, 0 }, NO_ROT, { 0.02f, 0.7f, 0.02f }, BLACK);
    glPopMatrix();
}

// WOODEN FENCE: a 1-unit wide fence section with pickets and rails. pos = centre bottom.
void drawFence(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0.12f, 0 }, NO_ROT, { 1.0f, 0.06f, 0.06f }, FENCE_WOOD);
    drawCuboid({ 0, 0.52f, 0 }, NO_ROT, { 1.0f, 0.06f, 0.06f }, FENCE_WOOD);
    for (int i = -4; i <= 4; i++)
        drawCuboid({ i * 0.11f, 0, 0 }, NO_ROT, { 0.055f, 0.68f, 0.055f }, FENCE_WOOD);
    glPopMatrix();
}

// TREE: trunk + spherical foliage clusters. About 3.5 tall at scale 1.
void drawTree(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.15f, 1.8f, 0.15f }, TRUNK);
    drawSphere({ 0.0f, 2.2f,  0.0f }, NO_ROT, { 1.4f, 1.2f, 1.4f }, LEAF);
    drawSphere({ 0.4f, 2.6f,  0.2f }, NO_ROT, { 1.0f, 0.9f, 1.0f }, LEAF_LIGHT);
    drawSphere({ -0.3f, 2.5f, -0.2f }, NO_ROT, { 1.0f, 0.8f, 1.0f }, LEAF);
    drawSphere({ 0.0f, 3.0f,  0.0f }, NO_ROT, { 0.8f, 0.7f, 0.8f }, LEAF_LIGHT);
    glPopMatrix();
}

// GRID WINDOW: large multi-pane window (for upper facade, Image 2 style).
// w x h window, cols x rows grid, faces +Z, centred on pos.
void drawGridWindow(Vec3 pos, Vec3 rot, Vec3 scale, float w, float h, int cols, int rows)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCube({ 0, 0, -0.02f }, NO_ROT, { w + 0.12f, h + 0.12f, 0.12f }, DARK_WOOD);
    drawCube({ 0, 0, 0.02f },  NO_ROT, { w - 0.04f, h - 0.04f, 0.02f }, UPPER_WALL);
    float cellW = w / cols;
    float cellH = h / rows;
    for (int i = 0; i <= cols; i++) {
        float x = -w / 2 + i * cellW;
        drawCube({ x, 0, 0.04f }, NO_ROT, { 0.04f, h, 0.04f }, DARK_WOOD);
    }
    for (int j = 0; j <= rows; j++) {
        float y = -h / 2 + j * cellH;
        drawCube({ 0, y, 0.04f }, NO_ROT, { w, 0.04f, 0.04f }, DARK_WOOD);
    }
    glPopMatrix();
}


// ============================================================================
// 5. FURNITURE  (all made from cuboids / cylinders; pos = point on the floor)
// ============================================================================

// TABLE: top 1.2 x 0.8, 0.78 tall.
void drawTable(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0.72f, 0 }, NO_ROT, { 1.2f, 0.06f, 0.8f }, LIGHT_WOOD);   // top
    for (int sx = -1; sx <= 1; sx += 2)                                         // 4 legs
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * 0.52f, 0, sz * 0.32f }, NO_ROT, { 0.07f, 0.72f, 0.07f }, DARK_WOOD);
    glPopMatrix();
}

// CHAIR: seat, 4 legs, backrest. Faces +Z (the backrest is on the -Z side).
void drawChair(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0.42f, 0 }, NO_ROT, { 0.45f, 0.05f, 0.45f }, WOOD);       // seat
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * 0.19f, 0, sz * 0.19f }, NO_ROT, { 0.05f, 0.42f, 0.05f }, DARK_WOOD);
    drawCuboid({ -0.19f, 0.47f, -0.2f }, NO_ROT, { 0.05f, 0.5f, 0.05f }, DARK_WOOD);  // back posts
    drawCuboid({  0.19f, 0.47f, -0.2f }, NO_ROT, { 0.05f, 0.5f, 0.05f }, DARK_WOOD);
    drawCube({ 0, 0.85f, -0.2f }, NO_ROT, { 0.45f, 0.15f, 0.04f }, WOOD);      // backrest
    glPopMatrix();
}

// STOOL: round cushion, one metal leg, round base, foot ring. 0.78 tall.
void drawStool(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 },     NO_ROT, { 0.40f, 0.05f, 0.40f }, METAL);     // base
    drawCylinder({ 0, 0.05f, 0 }, NO_ROT, { 0.07f, 0.67f, 0.07f }, METAL);     // leg
    drawTorus({ 0, 0.30f, 0 },    NO_ROT, ONE, METAL, 0.02f, 0.18f);           // foot ring
    drawCylinder({ 0, 0.72f, 0 }, NO_ROT, { 0.38f, 0.06f, 0.38f }, CUSHION);   // seat
    glPopMatrix();
}

// BENCH: long seat on two slab legs. 1.8 long (along X).
void drawBench(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0.42f, 0 }, NO_ROT, { 1.8f, 0.06f, 0.4f }, WOOD);            // seat
    drawCuboid({ -0.75f, 0, 0 }, NO_ROT, { 0.06f, 0.42f, 0.36f }, DARK_WOOD);    // legs
    drawCuboid({  0.75f, 0, 0 }, NO_ROT, { 0.06f, 0.42f, 0.36f }, DARK_WOOD);
    drawCuboid({ 0, 0.15f, 0 }, NO_ROT, { 1.5f, 0.05f, 0.05f }, DARK_WOOD);      // stretcher bar
    glPopMatrix();
}


// ============================================================================
// 6. INTERIOR
// ============================================================================

// COUNTER: long wooden bar, 6 long (X), about 1.06 tall. The customer side is +Z.
void drawCounter(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 6, 1.0f, 0.6f }, WOOD);                      // body
    drawCuboid({ 0, 1.0f, 0.05f }, NO_ROT, { 6.2f, 0.06f, 0.8f }, LIGHT_WOOD);     // top
    for (float x = -2.75f; x <= 2.76f; x += 0.5f)                                  // decorative slats
        drawCuboid({ x, 0.05f, 0.31f }, NO_ROT, { 0.05f, 0.9f, 0.02f }, DARK_WOOD);
    glPopMatrix();
}

// SHELF: two side panels and two boards, 2 wide, 0.3 deep. pos = centre of the bottom board.
// Items are placed on it separately (boards are at local y = 0.04 and 0.64).
void drawShelf(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ -1.0f, 0, 0 }, NO_ROT, { 0.04f, 0.68f, 0.3f }, DARK_WOOD);
    drawCuboid({  1.0f, 0, 0 }, NO_ROT, { 0.04f, 0.68f, 0.3f }, DARK_WOOD);
    for (int i = 0; i < 2; i++)
        drawCuboid({ 0, i * 0.6f, 0 }, NO_ROT, { 2.04f, 0.04f, 0.3f }, WOOD);
    glPopMatrix();
}

// CABINET: kitchen base cabinet, 1 wide, 0.6 deep, 0.94 tall. Doors face +Z.
void drawCabinet(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0, 0 },    NO_ROT, { 1.0f, 0.9f, 0.6f },   LIGHT_WOOD);     // body
    drawCuboid({ 0, 0.9f, 0 }, NO_ROT, { 1.02f, 0.04f, 0.62f }, STEEL);         // steel top
    for (int side = -1; side <= 1; side += 2) {
        drawCube({ side * 0.245f, 0.45f, 0.305f }, NO_ROT, { 0.47f, 0.8f, 0.02f }, WOOD);    // door
        drawCube({ side * 0.06f,  0.60f, 0.32f  }, NO_ROT, { 0.03f, 0.15f, 0.03f }, METAL);  // handle
    }
    glPopMatrix();
}

// SINK: steel basin with a faucet. pos = on top of a cabinet.
void drawSink(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.7f, 0.02f, 0.5f }, STEEL);                 // base plate
    drawPlane({ 0, 0.021f, 0.02f }, NO_ROT, { 0.56f, 1, 0.36f }, DARK_GRAY);       // basin hole
    drawCuboid({ 0, 0,  0.24f }, NO_ROT, { 0.7f, 0.08f, 0.02f }, STEEL);           // rims
    drawCuboid({ 0, 0, -0.24f }, NO_ROT, { 0.7f, 0.08f, 0.02f }, STEEL);
    drawCuboid({ -0.34f, 0, 0 }, NO_ROT, { 0.02f, 0.08f, 0.5f }, STEEL);
    drawCuboid({  0.34f, 0, 0 }, NO_ROT, { 0.02f, 0.08f, 0.5f }, STEEL);
    drawCylinder({ 0, 0, -0.22f },     NO_ROT,     { 0.04f, 0.35f, 0.04f }, METAL); // faucet pipe
    drawCylinder({ 0, 0.33f, -0.22f }, { 90, 0, 0 }, { 0.04f, 0.2f, 0.04f }, METAL); // spout
    glPopMatrix();
}

// COOKING POT: steel pot with broth, rim and two handles. 0.5 wide, 0.45 tall.
void drawCookingPot(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 },      NO_ROT, { 0.5f, 0.45f, 0.5f },    STEEL);    // body
    drawCylinder({ 0, 0.45f, 0 },  NO_ROT, { 0.46f, 0.005f, 0.46f }, BROTH);    // broth surface
    drawTorus({ 0, 0.45f, 0 },     NO_ROT, ONE, STEEL, 0.02f, 0.25f);          // rim
    drawTorus({ -0.28f, 0.36f, 0 }, { 90, 0, 0 }, ONE, DARK_GRAY, 0.015f, 0.06f); // handles
    drawTorus({  0.28f, 0.36f, 0 }, { 90, 0, 0 }, ONE, DARK_GRAY, 0.015f, 0.06f);
    glPopMatrix();
}

// KITCHEN: a row of 7 cabinets with a stove, two pots, a sink, a cutting board and plates.
// pos = centre of the cabinet row on the floor. Cabinets face +Z.
void drawKitchen(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    for (int i = 0; i < 7; i++)
        drawCabinet({ -3.0f + i, 0, 0 });

    float top = 0.94f;   // height of the cabinet tops

    // Stove with two burner rings and two pots
    drawCuboid({ -1.5f, top, 0 }, NO_ROT, { 1.8f, 0.1f, 0.55f }, DARK_GRAY);
    drawTorus({ -1.95f, top + 0.1f, 0 }, NO_ROT, ONE, BLACK, 0.02f, 0.15f);
    drawTorus({ -1.05f, top + 0.1f, 0 }, NO_ROT, ONE, BLACK, 0.02f, 0.15f);
    drawCookingPot({ -1.95f, top + 0.1f, 0 });
    drawCookingPot({ -1.05f, top + 0.1f, 0 }, NO_ROT, { 0.8f, 0.8f, 0.8f });

    // Cutting board, sink and a stack of plates
    drawBoard({ 0.2f, top + 0.0125f, 0 }, { 0, 10, 0 }, { 0.5f, 0.5f, 0.35f }, LIGHT_WOOD);
    drawSink({ 1.5f, top, 0 });
    for (int i = 0; i < 4; i++)
        drawPlate({ 2.7f, top + i * 0.025f, 0 }, NO_ROT, { 0.3f, 0.3f, 0.3f }, PLATE_WHITE);

    glPopMatrix();
}


// ============================================================================
// 7. FOOD
//    The food parts are designed in "bowl units": the bowl is 1 wide and the
//    soup surface is at y = 0.3. drawRamenBowl() puts them all together,
//    then you scale the whole bowl (e.g. 0.22) to its real size.
// ============================================================================

// NOODLES: several flat rings of different sizes = a pile of noodles.
void drawRamenNoodles(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    for (int i = 0; i < 6; i++) {
        float x = 0.04f * (i % 3) - 0.04f;
        float z = 0.03f * (i % 2) - 0.015f;
        drawTorus({ x, 0.01f * (i % 2), z }, NO_ROT, ONE, NOODLE, 0.025f, 0.08f + 0.04f * i);
    }
    for (int i = 0; i < 3; i++)   // a few loose strands lying across
        drawCylinderCustom({ -0.05f, 0.03f, -0.1f + 0.1f * i }, { 0, 40.0f * i, -90 }, ONE,
                           NOODLE, 0.02f, 0.02f, 0.3f);
    glPopMatrix();
}

// EGG: half a boiled egg (flattened white sphere + yolk).
void drawEgg(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawSphere({ 0, 0, 0 },     NO_ROT, { 0.16f, 0.10f, 0.20f }, EGG_WHITE);
    drawSphere({ 0, 0.03f, 0 }, NO_ROT, { 0.09f, 0.05f, 0.11f }, YOLK);
    glPopMatrix();
}

// MEAT: a round chashu pork slice (flat cylinder with a darker edge ring).
void drawMeat(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.26f, 0.03f, 0.26f }, MEAT);
    drawTorus({ 0, 0.015f, 0 }, NO_ROT, ONE, MEAT_EDGE, 0.018f, 0.125f);
    glPopMatrix();
}

// SEAWEED: a standing sheet of nori (a thin board turned upright).
void drawSeaweed(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawBoard({ 0, 0, 0 }, { 90, 0, 0 }, { 0.25f, 0.3f, 0.3f }, NORI);
    glPopMatrix();
}

// GREEN ONION: a few tiny green rings scattered around pos.
void drawGreenOnion(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    float spots[6][2] = { { 0, 0 }, { 0.06f, 0.03f }, { -0.05f, 0.05f },
                          { 0.03f, -0.06f }, { -0.07f, -0.02f }, { 0.09f, -0.03f } };
    for (int i = 0; i < 6; i++)
        drawTorus({ spots[i][0], 0, spots[i][1] }, NO_ROT, ONE, ONION_GREEN, 0.008f, 0.018f);
    glPopMatrix();
}

// RAMEN BOWL: bowl + soup + noodles + toppings. 1 wide at scale 1 (use ~0.22 in the shop).
void drawRamenBowl(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawBowl({ 0, 0, 0 }, NO_ROT, ONE, BOWL_RED);
    drawCylinder({ 0, 0.29f, 0 }, NO_ROT, { 0.83f, 0.02f, 0.83f }, BROTH);   // soup
    drawRamenNoodles({ 0, 0.31f, 0 });
    drawMeat({ -0.17f, 0.33f, 0.12f }, { 10, 0, 0 });
    drawEgg({ 0.18f, 0.32f, 0.10f }, { 0, 30, 0 });
    drawSeaweed({ 0, 0.38f, -0.28f }, { -20, 0, 0 });
    drawGreenOnion({ 0.05f, 0.33f, -0.08f });
    glPopMatrix();
}

// CHOPSTICKS: two thin tapered cylinders lying along X. 1 long at scale 1.
void drawChopsticks(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinderCustom({ -0.5f, 0, -0.03f }, { 0, 0, -90 }, ONE, DARK_WOOD, 0.03f, 0.018f, 1.0f);
    drawCylinderCustom({ -0.5f, 0,  0.03f }, { 0, 0, -90 }, ONE, DARK_WOOD, 0.03f, 0.018f, 1.0f);
    glPopMatrix();
}

// SPOON: Japanese ramen spoon (renge): flattened sphere + a tilted handle.
void drawSpoon(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawSphere({ 0, 0.03f, 0 },    NO_ROT,         { 0.35f, 0.12f, 0.45f }, WHITE);
    drawCube({ 0, 0.07f, 0.33f }, { -15, 0, 0 },  { 0.10f, 0.04f, 0.35f }, WHITE);
    glPopMatrix();
}


// ============================================================================
// 8. DECORATIONS
//    (drawBottle, drawCup, drawPlate and drawPlant are above and reused here)
// ============================================================================

// LANTERN: red paper lantern with black caps, a string and a tassel. pos = its centre.
// The body glows using GL_EMISSION so it looks like a light source.
void drawLantern(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Glowing body (emissive warm red light)
    setEmission(0.85f, 0.25f, 0.08f);
    drawSphere({ 0, 0, 0 },          NO_ROT, { 0.5f, 0.65f, 0.5f },  RED);       // body
    drawTorus({ 0, 0, 0 },           NO_ROT, ONE, DARK_RED, 0.01f, 0.25f);      // rib
    // Second rib for tiered/ridged look (like reference Image 1)
    drawTorus({ 0, 0.15f, 0 },       NO_ROT, ONE, DARK_RED, 0.01f, 0.22f);
    drawTorus({ 0, -0.15f, 0 },      NO_ROT, ONE, DARK_RED, 0.01f, 0.22f);
    clearEmission();

    drawCylinder({ 0,  0.28f, 0 },   NO_ROT, { 0.25f, 0.06f, 0.25f }, BLACK);   // top cap
    drawCylinder({ 0, -0.34f, 0 },   NO_ROT, { 0.25f, 0.06f, 0.25f }, BLACK);   // bottom cap
    drawCylinder({ 0,  0.34f, 0 },   NO_ROT, { 0.02f, 0.5f, 0.02f },  BLACK);   // string
    drawCone({ 0, -0.55f, 0 },       NO_ROT, { 0.08f, 0.21f, 0.08f }, GOLD);    // tassel
    glPopMatrix();
}

// MENU BOARD: wooden board with 4 paper strips of "writing". 1.2 x 0.9, faces +Z, centred.
void drawMenuBoard(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCube({ 0, 0, 0 }, NO_ROT, { 1.2f, 0.9f, 0.05f }, DARK_WOOD);
    for (int i = 0; i < 4; i++) {
        float x = -0.42f + i * 0.28f;
        drawCube({ x, 0, 0.03f }, NO_ROT, { 0.2f, 0.75f, 0.01f }, PAPER);        // paper strip
        for (int j = 0; j < 3; j++)                                                // fake text
            drawCube({ x, 0.22f - j * 0.2f, 0.036f }, NO_ROT, { 0.04f, 0.12f, 0.005f }, BLACK);
    }
    glPopMatrix();
}

// WALL DECORATION: framed picture of a mountain and a red sun. 1 x 0.7, faces +Z, centred.
void drawWallDecoration(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCube({ 0, 0, 0 },     NO_ROT, { 1.0f, 0.70f, 0.04f }, DARK_WOOD);      // frame
    drawCube({ 0, 0, 0.02f }, NO_ROT, { 0.88f, 0.58f, 0.01f }, PAPER);         // canvas
    drawCylinder({ 0.22f, 0.12f, 0.026f }, { 90, 0, 0 }, { 0.18f, 0.005f, 0.18f }, RED);   // sun
    // Mountain = a wedge turned to face us; snow cap = a smaller white wedge on top
    drawWedge({ -0.1f, -0.25f, 0.027f }, { 0, 90, 0 }, { 0.005f, 0.40f, 0.60f }, MOUNTAIN);
    drawWedge({ -0.1f,  0.02f, 0.029f }, { 0, 90, 0 }, { 0.005f, 0.13f, 0.195f }, WHITE);
    glPopMatrix();
}


// ============================================================================
// 9. SCENE  -  this is where objects are placed in the world
// ============================================================================

void drawGround()
{
    // Dark ground plane (night scene)
    drawPlane({ 0, 0, 0 }, NO_ROT, { 80, 1, 80 }, { 0.12f, 0.14f, 0.10f });

    // Raised wooden platform the shop sits on (Image 2)
    drawCuboid({ 0, -0.20f, 0.5f }, NO_ROT, { 12.5f, 0.20f, 11.0f }, DARK_WOOD);
    // Front step
    drawCuboid({ 0, -0.10f, 5.5f }, NO_ROT, { 13.0f, 0.10f, 1.2f }, WOOD);

    // Sidewalk in front
    drawSidewalk({ 0, 0, 7.0f });
    // Street
    drawStreet({ 0, 0, 12.0f });
}

void drawExterior()
{
    // Building + roof
    drawShopBuilding({ 0, 0, 0 });
    if (showRoof) drawRoof({ 0, 0, 0 });

    // Large grid window on the upper front facade (Image 2 style)
    drawGridWindow({ 0, 4.3f, 4.05f }, NO_ROT, ONE, 8.5f, 1.5f, 12, 3);

    // Sign at the top of the building (Image 2: white "ラーメン" sign)
    drawSignBoard({ 0, 5.65f, 4.12f });

    // == Round CREAM lanterns on the front walls (Image 2) ==
    setEmission(0.70f, 0.55f, 0.30f);
    drawSphere({ -3.8f, 2.6f, 4.5f }, NO_ROT, { 0.55f, 0.65f, 0.55f }, LANTERN_CREAM);
    drawSphere({  3.8f, 2.6f, 4.5f }, NO_ROT, { 0.55f, 0.65f, 0.55f }, LANTERN_CREAM);
    clearEmission();
    drawCylinder({ -3.8f, 2.95f, 4.5f }, NO_ROT, { 0.02f, 0.4f, 0.02f }, BLACK);
    drawCylinder({  3.8f, 2.95f, 4.5f }, NO_ROT, { 0.02f, 0.4f, 0.02f }, BLACK);

    // == Floating paper lanterns in the sky (Image 2 top area) ==
    setEmission(0.55f, 0.45f, 0.28f);
    drawSphere({ -2.5f, 7.0f, 3.0f }, NO_ROT, { 0.40f, 0.48f, 0.40f }, LANTERN_CREAM);
    drawSphere({  2.0f, 7.5f, 2.5f }, NO_ROT, { 0.35f, 0.42f, 0.35f }, LANTERN_CREAM);
    drawSphere({  0.0f, 6.8f, 5.5f }, NO_ROT, { 0.38f, 0.44f, 0.38f }, LANTERN_CREAM);
    drawSphere({ -4.0f, 7.5f, 5.0f }, NO_ROT, { 0.32f, 0.38f, 0.32f }, LANTERN_CREAM);
    drawSphere({  4.5f, 7.0f, 4.0f }, NO_ROT, { 0.36f, 0.40f, 0.36f }, LANTERN_CREAM);
    clearEmission();

    // == Wooden fence across the front (Image 2) ==
    for (int i = -5; i <= 5; i++)
        drawFence({ i * 1.0f, 0, 5.8f });

    // == Plants near the entrance ==
    drawPlant({ -4.5f, 0, 4.8f }, NO_ROT, { 1.2f, 1.2f, 1.2f });
    drawPlant({  4.5f, 0, 4.8f }, NO_ROT, { 1.2f, 1.2f, 1.2f });
    drawPlant({ -2.5f, 0, 5.4f });
    drawPlant({  2.5f, 0, 5.4f });

    // == Trees on both sides (Image 2) ==
    drawTree({ -7.0f, 0, 2.0f }, NO_ROT, { 1.0f, 1.3f, 1.0f });
    drawTree({  7.0f, 0, 3.0f }, NO_ROT, { 1.2f, 1.5f, 1.2f });
    drawTree({ -6.5f, 0, -2.0f }, NO_ROT, { 0.8f, 1.1f, 0.8f });
    drawTree({  7.5f, 0, -1.5f });

    // == Street lamps ==
    drawLamp({ -7.0f, 0, 7.0f }, { 0, -90, 0 });
    drawLamp({  7.0f, 0, 7.0f }, { 0, -90, 0 });
}

void drawInterior()
{
    float FY = FLOOR_Y;   // 0.1

    // == COUNTER: long bar across the shop (Image 1) ==
    drawCounter({ 0, FY, -0.5f });
    float counterTop = FY + 1.06f;

    // == STOOLS: row of red-cushion stools in front of counter (Image 1) ==
    for (int i = 0; i < 6; i++) {
        float x = -2.8f + i * 1.12f;
        drawStool({ x, FY, 0.8f });
    }

    // == RAMEN BOWLS on the counter (Image 1 shows dark bowls with food) ==
    for (int i = 0; i < 4; i++) {
        float x = -2.2f + i * 1.5f;
        drawRamenBowl({ x, counterTop, -0.3f }, NO_ROT, { 0.24f, 0.24f, 0.24f });
        drawChopsticks({ x + 0.18f, counterTop + 0.005f, -0.12f },
                       { 0, 25, 0 }, { 0.24f, 0.24f, 0.24f });
    }
    // Condiments
    drawBottle({  3.5f, counterTop, -0.35f }, NO_ROT, { 0.3f, 0.3f, 0.3f }, SOY);
    drawBottle({  3.7f, counterTop, -0.25f }, NO_ROT, { 0.3f, 0.3f, 0.3f }, RED, GOLD);
    drawCup({ -3.2f, counterTop, -0.30f }, NO_ROT, { 0.1f, 0.1f, 0.1f }, CUP_GREEN);

    // == KITCHEN along the back wall (Image 1 shows shelves, pots, utensils) ==
    drawKitchen({ 0, FY, -3.2f });

    // == SHELVES with bowls, plates, pots on back wall (Image 1 steel shelving) ==
    float shelfY = FY + 1.7f;
    drawShelf({ -1.8f, shelfY, -3.63f });
    drawShelf({  1.8f, shelfY, -3.63f });
    for (int i = 0; i < 4; i++) {
        drawBowl({ -2.5f + i * 0.50f, shelfY + 0.04f, -3.63f },
                 NO_ROT, { 0.3f, 0.3f, 0.3f }, DARK_GRAY);
        drawPlate({ -2.5f + i * 0.50f, shelfY + 0.64f, -3.63f },
                  NO_ROT, { 0.35f, 0.35f, 0.35f }, PLATE_WHITE);
        drawCup({ 1.1f + i * 0.50f, shelfY + 0.04f, -3.63f },
                NO_ROT, { 0.12f, 0.12f, 0.12f }, STEEL);
        drawBottle({ 1.1f + i * 0.50f, shelfY + 0.64f, -3.63f },
                   NO_ROT, { 0.35f, 0.35f, 0.35f },
                   (i % 2 == 0) ? SOY : CUP_GREEN);
    }
    // Extra large cooking pot on display (Image 1 prominent pot)
    drawCookingPot({ 3.5f, shelfY + 0.04f, -3.63f }, NO_ROT, { 1.2f, 1.2f, 1.2f });

    // == CYLINDRICAL ORANGE LANTERNS hanging from ceiling (Image 1) ==
    drawCylinderLantern({ -1.8f, 2.9f, 0.0f }, NO_ROT, { 0.9f, 0.9f, 0.9f });
    drawCylinderLantern({  1.8f, 2.9f, 0.0f }, NO_ROT, { 0.9f, 0.9f, 0.9f });

    // == NOREN CURTAINS hanging from ceiling between counter and kitchen (Image 1) ==
    drawNoren({ 0, 3.0f, -1.3f });

    // == MENU BOARDS on back wall ==
    drawMenuBoard({ -3.2f, 2.8f, -3.77f });
    drawMenuBoard({  3.2f, 2.8f, -3.77f });

    // == WALL DECORATION on side walls ==
    drawWallDecoration({ -4.77f, 2.0f, 0.5f }, { 0,  90, 0 });
    drawWallDecoration({  4.77f, 2.0f, 1.0f }, { 0, -90, 0 });

    // == Small wooden step stool near counter (visible in Image 1) ==
    drawCuboid({ -3.8f, FY, 0.5f }, NO_ROT, { 0.4f, 0.35f, 0.3f }, WOOD);
    drawCuboid({ -3.8f, FY + 0.35f, 0.5f }, NO_ROT, { 0.45f, 0.04f, 0.35f }, LIGHT_WOOD);

    // Indoor plant
    drawPlant({ 4.3f, FY, 2.0f });
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera orbiting around the shop
    float a = camAngle * PI / 180.0f;
    gluLookAt(camDistance * sin(a), camHeight, camDistance * cos(a),   // eye
              0, 1.5, 1,                                               // look at
              0, 1, 0);                                                // up direction

    // Place lights in world space (after camera transform)
    GLfloat pos0[] = { 5.0f, 20.0f, 8.0f, 0.0f };     // directional moonlight
    GLfloat pos1[] = { -8.0f, 6.0f, -5.0f, 0.0f };    // fill from behind
    GLfloat pos2[] = { 0.0f, 2.8f, -0.2f, 1.0f };     // warm interior light (above counter)
    GLfloat pos3[] = { -3.8f, 2.6f, 4.5f, 1.0f };     // left cream lantern
    GLfloat pos4[] = {  3.8f, 2.6f, 4.5f, 1.0f };     // right cream lantern
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
    glLightfv(GL_LIGHT2, GL_POSITION, pos2);
    glLightfv(GL_LIGHT3, GL_POSITION, pos3);
    glLightfv(GL_LIGHT4, GL_POSITION, pos4);

    drawGround();
    drawExterior();
    drawInterior();

    glutSwapBuffers();
}


// ============================================================================
// 10. WINDOW, CAMERA KEYS, MAIN
// ============================================================================

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 0.5, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int, int)
{
    if (key == 'h' || key == 'H') showRoof = !showRoof;
    if (key == 'o' || key == 'O') showOutlines = !showOutlines;
    if (key == 27) exit(0);   // ESC
    glutPostRedisplay();
}

void specialKeys(int key, int, int)
{
    if (key == GLUT_KEY_LEFT)      camAngle -= 5;
    if (key == GLUT_KEY_RIGHT)     camAngle += 5;
    if (key == GLUT_KEY_UP   && camDistance > 3) camDistance -= 1;
    if (key == GLUT_KEY_DOWN)      camDistance += 1;
    if (key == GLUT_KEY_PAGE_UP)   camHeight += 0.5f;
    if (key == GLUT_KEY_PAGE_DOWN) camHeight -= 0.5f;
    glutPostRedisplay();
}

void init()
{
    glClearColor(SKY.r, SKY.g, SKY.b, 1.0f);
    glEnable(GL_DEPTH_TEST);                // near objects hide far ones
    glEnable(GL_POLYGON_OFFSET_FILL);       // push filled faces back a tiny bit
    glPolygonOffset(1.0f, 1.0f);            //   so outlines draw cleanly on top
    glLineWidth(1.5f);
    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);    // generate normals on GLU shapes

    // ---- Lighting (evening scene with warm interior glow) ----
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);                    // dim moonlight / ambient
    glEnable(GL_LIGHT1);                    // soft cool fill
    glEnable(GL_LIGHT2);                    // warm interior point light
    glEnable(GL_LIGHT3);                    // left lantern glow
    glEnable(GL_LIGHT4);                    // right lantern glow
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glShadeModel(GL_SMOOTH);

    // Global ambient (very dim, evening)
    GLfloat globalAmb[] = { 0.08f, 0.08f, 0.10f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

    // LIGHT0: dim directional moonlight from above
    GLfloat amb0[] = { 0.06f, 0.06f, 0.08f, 1.0f };
    GLfloat dif0[] = { 0.20f, 0.20f, 0.28f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif0);

    // LIGHT1: soft cool fill from the side
    GLfloat dif1[] = { 0.10f, 0.10f, 0.15f, 1.0f };
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  dif1);

    // LIGHT2: warm interior point light (inside the shop, above the counter)
    GLfloat amb2[] = { 0.10f, 0.08f, 0.04f, 1.0f };
    GLfloat dif2[] = { 0.90f, 0.65f, 0.30f, 1.0f };
    glLightfv(GL_LIGHT2, GL_AMBIENT,  amb2);
    glLightfv(GL_LIGHT2, GL_DIFFUSE,  dif2);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION,    0.05f);
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.01f);

    // LIGHT3 & LIGHT4: warm point lights at the front lantern positions
    GLfloat difLan[] = { 0.95f, 0.45f, 0.12f, 1.0f };
    for (int i = 3; i <= 4; i++) {
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, difLan);
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION,    0.12f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.03f);
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 800);
    glutCreateWindow("3D Ramen Shop");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}
