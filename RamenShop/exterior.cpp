#include "exterior.h"

void drawShopBuilding(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float GH = 3.3f;   // ground floor ceiling height
    float UH = 2.0f;   // upper facade height
    float TH = GH + UH; // 5.3 total

    // Floor: gray ceramic tiles
    drawTexturedBox({ 0, 0, 0 }, NO_ROT, { 10, 0.1f, 8 },
                    getTexID(TEX_TILE_FLOOR), WHITE, 2.0f);

    // Ground floor walls (open front)
    drawTexturedBox({ 0, 0, -3.9f }, NO_ROT, { 10, GH, 0.2f },
                    getTexID(TEX_WALL), WHITE, 2.0f);   // back wall
    for (int sx = -1; sx <= 1; sx += 2) {
        drawTexturedBox({ sx * 4.9f, 0, 0 },    NO_ROT, { 0.2f, 1.2f, 8 },
                        getTexID(TEX_DARK_WOOD), WHITE, 1.0f);
        drawTexturedBox({ sx * 4.9f, 1.2f, 0 }, NO_ROT, { 0.2f, GH - 1.2f, 8 },
                        getTexID(TEX_WALL), WHITE, 2.0f);
    }

    // Front: structural wood posts
    drawCuboid({ -4.9f, 0, 3.9f }, NO_ROT, { 0.25f, GH, 0.25f }, DARK_WOOD);
    drawCuboid({  4.9f, 0, 3.9f }, NO_ROT, { 0.25f, GH, 0.25f }, DARK_WOOD);
    drawCuboid({ -1.8f, 0, 3.9f }, NO_ROT, { 0.15f, GH, 0.15f }, DARK_WOOD);
    drawCuboid({  1.8f, 0, 3.9f }, NO_ROT, { 0.15f, GH, 0.15f }, DARK_WOOD);
    drawCuboid({ 0, GH - 0.15f, 3.95f }, NO_ROT, { 10.2f, 0.18f, 0.14f }, DARK_WOOD);

    // Upper facade: dark navy
    drawCuboid({  0,    GH, 3.9f },  NO_ROT, { 10, UH, 0.2f }, UPPER_WALL);
    drawCuboid({  0,    GH, -3.9f }, NO_ROT, { 10, UH, 0.2f }, UPPER_WALL);
    drawCuboid({ -4.9f, GH, 0 },    NO_ROT, { 0.2f, UH, 8 },  UPPER_WALL);
    drawCuboid({  4.9f, GH, 0 },    NO_ROT, { 0.2f, UH, 8 },  UPPER_WALL);

    // Corner posts
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * 4.9f, 0, sz * 3.9f }, NO_ROT, { 0.3f, TH, 0.3f }, DARK_WOOD);

    // Trim beams
    drawCuboid({ 0, TH - 0.1f, 4.02f }, NO_ROT, { 10.4f, 0.15f, 0.1f }, DARK_WOOD);
    drawCuboid({ 0, GH - 0.05f, -3.82f }, NO_ROT, { 10.2f, 0.12f, 0.1f }, DARK_WOOD);

    glPopMatrix();
}

void drawRoof(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float TH = 5.3f;

    // Main flat roof slab
    drawCuboid({ 0, TH, 0 }, NO_ROT, { 11.2f, 0.18f, 9.2f }, DARK_GRAY);
    drawCuboid({ 0, TH + 0.18f,  4.6f }, NO_ROT, { 11.2f, 0.06f, 0.15f }, DARK_GRAY);
    drawCuboid({ 0, TH + 0.18f, -4.6f }, NO_ROT, { 11.2f, 0.06f, 0.15f }, DARK_GRAY);

    // Zigzag blue awning over open front
    drawBoard({ 0, 3.22f, 4.7f }, { 12, 0, 0 }, { 10.6f, 1.2f, 1.4f }, AWNING_BLUE);
    for (int i = -18; i <= 18; i++) {
        float x = i * 0.28f;
        drawCone({ x, 2.95f, 5.15f }, { 15, 0, 0 }, { 0.28f, 0.25f, 0.18f }, AWNING_BLUE);
    }

    // Fascia trim
    drawCuboid({ 0, TH - 0.02f,  4.55f }, NO_ROT, { 11.2f, 0.10f, 0.4f }, DARK_WOOD);
    drawCuboid({ 0, TH - 0.02f, -4.55f }, NO_ROT, { 11.2f, 0.10f, 0.4f }, DARK_WOOD);

    glPopMatrix();
}

void drawWindow(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    setEmission(0.45f, 0.30f, 0.12f);
    drawCube({ 0, 0, -0.06f }, NO_ROT, { 1.9f, 1.5f, 0.02f }, PAPER);
    clearEmission();

    // Outer frame
    drawCube({ 0,  0.8f, 0 }, NO_ROT, { 2.1f, 0.1f, 0.25f }, DARK_WOOD);
    drawCube({ 0, -0.8f, 0 }, NO_ROT, { 2.1f, 0.1f, 0.25f }, DARK_WOOD);
    drawCube({ -1.0f, 0, 0 }, NO_ROT, { 0.1f, 1.7f, 0.25f }, DARK_WOOD);
    drawCube({  1.0f, 0, 0 }, NO_ROT, { 0.1f, 1.7f, 0.25f }, DARK_WOOD);

    // Lattice
    for (int i = -1; i <= 1; i++)
        drawCube({ i * 0.5f, 0, 0 }, NO_ROT, { 0.04f, 1.6f, 0.05f }, DARK_WOOD);
    drawCube({ 0,  0.40f, 0 }, NO_ROT, { 2.0f, 0.04f, 0.05f }, DARK_WOOD);
    drawCube({ 0,  0.00f, 0 }, NO_ROT, { 2.0f, 0.04f, 0.05f }, DARK_WOOD);
    drawCube({ 0, -0.40f, 0 }, NO_ROT, { 2.0f, 0.04f, 0.05f }, DARK_WOOD);

    // Sill
    drawCube({ 0, -0.88f, 0.12f }, NO_ROT, { 2.3f, 0.06f, 0.35f }, WOOD);

    glPopMatrix();
}

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

void drawStreet(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawTexturedPlane({ 0, 0.02f, 0 }, NO_ROT, { 60, 1, 8 },
                     getTexID(TEX_ASPHALT), WHITE, 5.0f);
    for (float x = -28; x <= 28; x += 4)
        drawPlane({ x, 0.03f, 0 }, NO_ROT, { 2, 1, 0.2f }, WHITE);
    glPopMatrix();
}

void drawSidewalk(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawTexturedBox({ 0, 0, 0 }, NO_ROT, { 60, 0.12f, 3 },
                    getTexID(TEX_CONCRETE), WHITE, 3.0f);
    drawSubdividedPlane({ 0, 0.121f, 0 }, NO_ROT, { 60, 1, 3 }, SIDEWALK, 30, 6);
    for (float x = -29; x <= 29; x += 1.5f)
        drawPlane({ x, 0.125f, 0 }, NO_ROT, { 0.03f, 1, 3 }, GRAY);
    drawCuboid({ 0, 0, 1.45f }, NO_ROT, { 60, 0.16f, 0.12f }, GRAY);
    glPopMatrix();
}

void drawLamp(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 },       NO_ROT, { 0.35f, 0.2f, 0.35f }, DARK_GRAY);
    drawCylinder({ 0, 0.2f, 0 },    NO_ROT, { 0.12f, 3.3f, 0.12f }, DARK_GRAY);
    drawCube({ 0.35f, 3.45f, 0 },   NO_ROT, { 0.8f, 0.08f, 0.08f }, DARK_GRAY);
    drawCone({ 0.7f, 3.15f, 0 },    NO_ROT, { 0.5f, 0.3f, 0.5f },   DARK_GRAY);
    setEmission(0.9f, 0.7f, 0.3f);
    drawSphere({ 0.7f, 3.12f, 0 },  NO_ROT, { 0.2f, 0.2f, 0.2f },   GOLD);
    clearEmission();
    glPopMatrix();
}

void drawPlant(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinderCustom({ 0, 0, 0 }, NO_ROT, ONE, CLAY_POT, 0.2f, 0.28f, 0.4f);
    drawCylinder({ 0, 0.40f, 0 }, NO_ROT, { 0.5f, 0.02f, 0.5f }, SOIL);
    drawCylinder({ 0, 0.42f, 0 }, NO_ROT, { 0.05f, 0.4f, 0.05f }, DARK_WOOD);
    drawSphere({  0.00f, 0.90f,  0.00f }, NO_ROT, { 0.55f, 0.50f, 0.55f }, LEAF);
    drawSphere({  0.18f, 1.08f,  0.05f }, NO_ROT, { 0.40f, 0.40f, 0.40f }, LEAF_LIGHT);
    drawSphere({ -0.15f, 1.05f, -0.08f }, NO_ROT, { 0.40f, 0.40f, 0.40f }, LEAF_LIGHT);
    drawSphere({  0.00f, 1.20f,  0.00f }, NO_ROT, { 0.30f, 0.30f, 0.30f }, LEAF);
    glPopMatrix();
}

void drawFence(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0.12f, 0 }, NO_ROT, { 1.0f, 0.06f, 0.06f }, FENCE_WOOD);
    drawCuboid({ 0, 0.52f, 0 }, NO_ROT, { 1.0f, 0.06f, 0.06f }, FENCE_WOOD);
    for (int i = -4; i <= 4; i++)
        drawCuboid({ i * 0.11f, 0, 0 }, NO_ROT, { 0.055f, 0.68f, 0.055f }, FENCE_WOOD);
    glPopMatrix();
}

void drawVendingMachine(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // ── Main body ──────────────────────────────────────────────────────────
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.90f, 1.85f, 0.56f }, { 0.18f, 0.18f, 0.20f });

    // ── Company stripe (red band near top) ────────────────────────────────
    drawCuboid({ 0, 1.72f, 0.282f }, NO_ROT, { 0.86f, 0.09f, 0.012f }, RED);

    // ── Illuminated product window ─────────────────────────────────────────
    setEmission(0.28f, 0.20f, 0.10f);
    drawCuboid({ 0, 1.18f, 0.282f }, NO_ROT, { 0.72f, 0.62f, 0.010f },
               { 0.95f, 0.85f, 0.65f });
    clearEmission();

    // ── Product cans (3 rows × 4 columns) ─────────────────────────────────
    const Color prodCol[4] = {
        RED,
        { 0.85f, 0.60f, 0.10f },
        { 0.20f, 0.55f, 0.20f },
        { 0.15f, 0.30f, 0.80f }
    };
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            float cx = -0.24f + col * 0.16f;
            float cy  =  0.90f + row * 0.19f;
            drawCylinder({ cx, cy, 0.268f }, { 90, 0, 0 },
                         { 0.08f, 0.04f, 0.08f }, prodCol[col]);
        }
    }

    // ── Selection buttons (one per column) ────────────────────────────────
    for (int col = 0; col < 4; col++) {
        float bx = -0.24f + col * 0.16f;
        setEmission(0.25f, 0.08f, 0.08f);
        drawCylinder({ bx, 0.68f, 0.283f }, { 90, 0, 0 },
                     { 0.048f, 0.010f, 0.048f }, RED);
        clearEmission();
    }

    // ── Coin / card slot ──────────────────────────────────────────────────
    drawCuboid({ 0.31f, 1.10f, 0.283f }, NO_ROT, { 0.034f, 0.011f, 0.012f }, DARK_GRAY);

    // ── Retrieval hatch ───────────────────────────────────────────────────
    drawCuboid({ 0, 0.13f, 0.283f }, NO_ROT, { 0.52f, 0.09f, 0.010f }, BLACK);

    // ── Base plate ────────────────────────────────────────────────────────
    drawCuboid({ 0, -0.01f, 0 }, NO_ROT, { 0.94f, 0.03f, 0.58f }, DARK_GRAY);

    glPopMatrix();
}

void drawTree(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.15f, 1.8f, 0.15f }, TRUNK);
    drawSphere({  0.0f, 2.2f,  0.0f }, NO_ROT, { 1.4f, 1.2f, 1.4f }, LEAF);
    drawSphere({  0.4f, 2.6f,  0.2f }, NO_ROT, { 1.0f, 0.9f, 1.0f }, LEAF_LIGHT);
    drawSphere({ -0.3f, 2.5f, -0.2f }, NO_ROT, { 1.0f, 0.8f, 1.0f }, LEAF);
    drawSphere({  0.0f, 3.0f,  0.0f }, NO_ROT, { 0.8f, 0.7f, 0.8f }, LEAF_LIGHT);
    glPopMatrix();
}

// ─── Shoji Sliding Door (Japanese paper door) ─────────────────────────────
// Translucent washi paper panel in a dark-wood lattice frame.
// Warm interior light glows through the paper.
void drawShojiDoor(Vec3 pos, Vec3 rot, Vec3 scale, float width, float height)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    // Washi paper panel (warm emissive glow from interior light)
    setEmission(0.35f, 0.25f, 0.10f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    setColor(PAPER);
    glVertex3f(-hw + 0.06f, -hh + 0.06f, -0.02f);
    glVertex3f( hw - 0.06f, -hh + 0.06f, -0.02f);
    glVertex3f( hw - 0.06f,  hh - 0.06f, -0.02f);
    glVertex3f(-hw + 0.06f,  hh - 0.06f, -0.02f);
    glEnd();
    clearEmission();

    // Outer dark-wood frame
    drawCube({ 0,  hh, 0 }, NO_ROT, { width + 0.08f, 0.08f, 0.10f }, DARK_WOOD);  // top
    drawCube({ 0, -hh, 0 }, NO_ROT, { width + 0.08f, 0.08f, 0.10f }, DARK_WOOD);  // bottom
    drawCube({ -hw, 0, 0 }, NO_ROT, { 0.08f, height, 0.10f }, DARK_WOOD);          // left
    drawCube({  hw, 0, 0 }, NO_ROT, { 0.08f, height, 0.10f }, DARK_WOOD);          // right

    // Horizontal kumiko lattice bars
    int hBars = (int)(height / 0.45f);
    float hStep = (height - 0.12f) / (hBars + 1);
    for (int i = 1; i <= hBars; i++) {
        float y = -hh + 0.06f + i * hStep;
        drawCube({ 0, y, 0.01f }, NO_ROT, { width - 0.10f, 0.03f, 0.03f }, DARK_WOOD);
    }

    // Vertical kumiko lattice bars
    int vBars = (int)(width / 0.40f);
    float vStep = (width - 0.12f) / (vBars + 1);
    for (int i = 1; i <= vBars; i++) {
        float x = -hw + 0.06f + i * vStep;
        drawCube({ x, 0, 0.01f }, NO_ROT, { 0.03f, height - 0.10f, 0.03f }, DARK_WOOD);
    }

    // Bottom kick panel (solid wood, lower portion of traditional shoji)
    float kickH = height * 0.18f;
    drawCube({ 0, -hh + kickH * 0.5f, 0 }, NO_ROT,
             { width - 0.04f, kickH, 0.06f }, WOOD);

    // Sliding rail groove at top and bottom
    drawCube({ 0,  hh + 0.04f, 0 }, NO_ROT, { width + 0.20f, 0.03f, 0.14f }, DARK_WOOD);
    drawCube({ 0, -hh - 0.03f, 0 }, NO_ROT, { width + 0.20f, 0.02f, 0.14f }, DARK_WOOD);

    // Small recessed finger pull (hikite)
    drawCube({ hw - 0.18f, -0.1f, 0.05f }, NO_ROT, { 0.08f, 0.18f, 0.02f }, DARK_WOOD);

    glPopMatrix();
}

// ─── Shoji Window (Japanese paper window) ─────────────────────────────────
// Similar to shoji door but fixed in place with a finer lattice grid.
void drawShojiWindow(Vec3 pos, Vec3 rot, Vec3 scale, float width, float height)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float hw = width * 0.5f;
    float hh = height * 0.5f;

    // Washi paper panel (warm glow)
    setEmission(0.40f, 0.28f, 0.10f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    setColor(PAPER);
    glVertex3f(-hw + 0.05f, -hh + 0.05f, -0.02f);
    glVertex3f( hw - 0.05f, -hh + 0.05f, -0.02f);
    glVertex3f( hw - 0.05f,  hh - 0.05f, -0.02f);
    glVertex3f(-hw + 0.05f,  hh - 0.05f, -0.02f);
    glEnd();
    clearEmission();

    // Outer frame
    drawCube({ 0,  hh, 0 }, NO_ROT, { width + 0.06f, 0.07f, 0.10f }, DARK_WOOD);
    drawCube({ 0, -hh, 0 }, NO_ROT, { width + 0.06f, 0.07f, 0.10f }, DARK_WOOD);
    drawCube({ -hw, 0, 0 }, NO_ROT, { 0.07f, height, 0.10f }, DARK_WOOD);
    drawCube({  hw, 0, 0 }, NO_ROT, { 0.07f, height, 0.10f }, DARK_WOOD);

    // Dense kumiko lattice grid
    int cols = (int)(width / 0.32f);
    int rows = (int)(height / 0.35f);
    float cellW = (width - 0.10f) / (cols + 1);
    float cellH = (height - 0.10f) / (rows + 1);

    for (int i = 1; i <= cols; i++) {
        float x = -hw + 0.05f + i * cellW;
        drawCube({ x, 0, 0.01f }, NO_ROT, { 0.025f, height - 0.10f, 0.025f }, DARK_WOOD);
    }
    for (int j = 1; j <= rows; j++) {
        float y = -hh + 0.05f + j * cellH;
        drawCube({ 0, y, 0.01f }, NO_ROT, { width - 0.10f, 0.025f, 0.025f }, DARK_WOOD);
    }

    // Sill
    drawCube({ 0, -hh - 0.05f, 0.08f }, NO_ROT, { width + 0.16f, 0.05f, 0.25f }, WOOD);

    glPopMatrix();
}
