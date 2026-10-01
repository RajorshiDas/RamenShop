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

// ────── Cherry Blossom Tree ──────
// Elegant cherry blossom tree with pink flowering blossoms and graceful branches
void drawCherryBlossomTree(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Brown trunk with slight taper
    setMaterialPBR(Materials::WoodMatte, TRUNK);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.2f, 2.1f, 0.2f }, TRUNK);

    // Cherry blossom pink colors - soft and graceful
    Color cherryPink = { 0.95f, 0.70f, 0.75f };  // Soft pink for blossoms
    Color cherryPinkLight = { 1.0f, 0.85f, 0.90f };  // Lighter pink highlights

    // Set emissive glow for cherry blossoms - they catch light beautifully
    setEmission(0.15f, 0.08f, 0.10f);  // Subtle pink glow
    setMaterialPBR(Materials::GlazedMatte, cherryPink);

    // Central large canopy sphere
    drawSphere({ 0.0f, 2.4f, 0.0f }, NO_ROT, { 1.8f, 1.6f, 1.8f }, cherryPink);

    // Left side flowering cluster
    drawSphere({ -0.8f, 2.6f, -0.3f }, NO_ROT, { 1.2f, 1.4f, 1.2f }, cherryPinkLight);

    // Right side flowering cluster  
    drawSphere({ 0.7f, 2.7f, 0.4f }, NO_ROT, { 1.3f, 1.5f, 1.3f }, cherryPinkLight);

    // Top crown bloom
    drawSphere({ 0.0f, 3.5f, 0.0f }, NO_ROT, { 1.0f, 0.9f, 1.0f }, cherryPink);

    // Lower blooms
    drawSphere({ -0.5f, 1.8f, 0.3f }, NO_ROT, { 0.9f, 0.8f, 0.9f }, cherryPinkLight);
    drawSphere({ 0.6f, 1.9f, -0.2f }, NO_ROT, { 0.85f, 0.75f, 0.85f }, cherryPink);

    clearEmission();
    resetMaterialGloss();
    glPopMatrix();
}

// ─── Interactive Entrance Door (Sliding Shoji-style, click to open/close) ──
// Two Japanese shoji door panels that slide apart horizontally on tracks.
// Left panel slides further left, right panel slides further right.
// Translucent washi paper with kumiko lattice, warm interior glow, solid
// wood kick panel (koshi-ita), and recessed hikite finger pulls.
void drawEntranceDoor(float angle)
{
    float pw = 1.75f;        // panel width (each half)
    float ph = 3.05f;        // panel height (floor to header beam)
    float by = FLOOR_Y;      // base Y
    float hz = 3.92f;        // door Z (front wall plane)
    float fw = 0.08f;        // frame stile/rail width
    float kickH = ph * 0.18f; // solid wood kick panel height (koshi-ita)

    // Map angle (0-90) to normalized slide offset (0.0-1.0)
    float slideNorm = angle / 90.0f;
    float slideX = slideNorm * pw;  // how far each panel slides outward

    // Lattice dimensions (paper area above kick panel)
    float latticeBot = kickH;
    float latticeTop = ph - fw;
    float latticeH   = latticeTop - latticeBot;
    float latticeW   = pw - fw * 2;
    int hBars = (int)(latticeH / 0.45f);
    int vBars = (int)(latticeW / 0.40f);
    float hStep = latticeH / (hBars + 1);
    float vStep = latticeW / (vBars + 1);

    // Fixed door frame: top and bottom sliding tracks + side posts
    drawCube({ 0, by + ph + 0.02f, hz }, NO_ROT,
             { pw * 2 + 0.50f, 0.04f, 0.14f }, DARK_WOOD);          // top track
    drawCube({ 0, by - 0.015f, hz }, NO_ROT,
             { pw * 2 + 0.50f, 0.03f, 0.14f }, DARK_WOOD);          // bottom track
    drawCube({ -(pw + 0.08f), by + ph * 0.5f, hz }, NO_ROT,
             { 0.08f, ph + 0.10f, 0.14f }, DARK_WOOD);              // left post
    drawCube({  (pw + 0.08f), by + ph * 0.5f, hz }, NO_ROT,
             { 0.08f, ph + 0.10f, 0.14f }, DARK_WOOD);              // right post

    // Removed black aperture - interior is now visible when door opens
    if (false) {  // disabled aperture drawing

        float openW = slideX * 2.0f;
        glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);
        setColor(BLACK);
        glVertex3f(-openW * 0.5f, by + 0.02f, hz - 0.03f);
        glVertex3f( openW * 0.5f, by + 0.02f, hz - 0.03f);
        glVertex3f( openW * 0.5f, by + ph - 0.02f, hz - 0.03f);
        glVertex3f(-openW * 0.5f, by + ph - 0.02f, hz - 0.03f);
        glEnd();
    }

    // ── Left door panel ── slides left when opening (front track)
    glPushMatrix();
    glTranslatef(-pw * 0.5f - slideX, by, hz + 0.01f);

    // Washi paper panel (warm emissive glow from interior light)
    setEmission(0.35f, 0.25f, 0.10f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    setColor(PAPER);
    glVertex3f(fw - pw * 0.5f,      kickH,    -0.02f);
    glVertex3f(pw * 0.5f - fw,      kickH,    -0.02f);
    glVertex3f(pw * 0.5f - fw,      ph - fw,  -0.02f);
    glVertex3f(fw - pw * 0.5f,      ph - fw,  -0.02f);
    glEnd();
    clearEmission();

    // Outer dark-wood frame
    drawCube({ 0, ph - fw * 0.5f, 0 }, NO_ROT,
             { pw + 0.08f, fw, 0.10f }, DARK_WOOD);               // top rail
    drawCube({ 0, fw * 0.5f, 0 }, NO_ROT,
             { pw + 0.08f, fw, 0.10f }, DARK_WOOD);               // bottom rail
    drawCube({ -(pw * 0.5f - fw * 0.5f), ph * 0.5f, 0 }, NO_ROT,
             { fw, ph, 0.10f }, DARK_WOOD);                        // left stile
    drawCube({ (pw * 0.5f - fw * 0.5f), ph * 0.5f, 0 }, NO_ROT,
             { fw, ph, 0.10f }, DARK_WOOD);                        // right stile

    // Horizontal kumiko lattice bars
    for (int i = 1; i <= hBars; i++) {
        float y = latticeBot + i * hStep;
        drawCube({ 0, y, 0.01f }, NO_ROT,
                 { latticeW - 0.04f, 0.03f, 0.03f }, DARK_WOOD);
    }

    // Vertical kumiko lattice bars
    for (int i = 1; i <= vBars; i++) {
        float x = -(pw * 0.5f - fw) + i * vStep;
        float barY = (latticeBot + latticeTop) * 0.5f;
        drawCube({ x, barY, 0.01f }, NO_ROT,
                 { 0.03f, latticeH, 0.03f }, DARK_WOOD);
    }

    // Bottom kick panel — koshi-ita (solid wood)
    float koshiCY = (fw + kickH) * 0.5f;
    float koshiH  = kickH - fw + 0.02f;
    drawCube({ 0, koshiCY, 0 }, NO_ROT,
             { pw - fw * 2 + 0.02f, koshiH, 0.06f }, WOOD);

    // Hikite — recessed finger pull (on right stile side)
    drawCube({ pw * 0.5f - 0.20f, ph * 0.43f, 0.05f }, NO_ROT,
             { 0.07f, 0.14f, 0.02f }, DARK_WOOD);

    glPopMatrix();

    // ── Right door panel ── slides right when opening (rear track)
    glPushMatrix();
    glTranslatef(pw * 0.5f + slideX, by, hz - 0.01f);

    // Washi paper panel
    setEmission(0.35f, 0.25f, 0.10f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    setColor(PAPER);
    glVertex3f(-(pw * 0.5f - fw), kickH,    -0.02f);
    glVertex3f( (pw * 0.5f - fw), kickH,    -0.02f);
    glVertex3f( (pw * 0.5f - fw), ph - fw,  -0.02f);
    glVertex3f(-(pw * 0.5f - fw), ph - fw,  -0.02f);
    glEnd();
    clearEmission();

    // Outer dark-wood frame
    drawCube({ 0, ph - fw * 0.5f, 0 }, NO_ROT,
             { pw + 0.08f, fw, 0.10f }, DARK_WOOD);
    drawCube({ 0, fw * 0.5f, 0 }, NO_ROT,
             { pw + 0.08f, fw, 0.10f }, DARK_WOOD);
    drawCube({ -(pw * 0.5f - fw * 0.5f), ph * 0.5f, 0 }, NO_ROT,
             { fw, ph, 0.10f }, DARK_WOOD);
    drawCube({ (pw * 0.5f - fw * 0.5f), ph * 0.5f, 0 }, NO_ROT,
             { fw, ph, 0.10f }, DARK_WOOD);

    // Horizontal kumiko lattice bars
    for (int i = 1; i <= hBars; i++) {
        float y = latticeBot + i * hStep;
        drawCube({ 0, y, 0.01f }, NO_ROT,
                 { latticeW - 0.04f, 0.03f, 0.03f }, DARK_WOOD);
    }

    // Vertical kumiko lattice bars
    for (int i = 1; i <= vBars; i++) {
        float x = -(pw * 0.5f - fw) + i * vStep;
        float barY = (latticeBot + latticeTop) * 0.5f;
        drawCube({ x, barY, 0.01f }, NO_ROT,
                 { 0.03f, latticeH, 0.03f }, DARK_WOOD);
    }

    // Bottom kick panel — koshi-ita
    drawCube({ 0, koshiCY, 0 }, NO_ROT,
             { pw - fw * 2 + 0.02f, koshiH, 0.06f }, WOOD);

    // Hikite — recessed finger pull (on left stile side, mirrored)
    drawCube({ -(pw * 0.5f - 0.20f), ph * 0.43f, 0.05f }, NO_ROT,
             { 0.07f, 0.14f, 0.02f }, DARK_WOOD);

    glPopMatrix();
}

// ─── Helper: single centered shoji panel (used by sliding door) ───────────
static void drawShojiPanelCentered(float width, float height)
{
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    float fw = 0.06f;
    float kickH = height * 0.15f;

    // Washi paper (above kick panel)
    setEmission(0.35f, 0.25f, 0.10f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    setColor(PAPER);
    glVertex3f(-hw + fw, -hh + kickH, -0.015f);
    glVertex3f( hw - fw, -hh + kickH, -0.015f);
    glVertex3f( hw - fw,  hh - fw,    -0.015f);
    glVertex3f(-hw + fw,  hh - fw,    -0.015f);
    glEnd();
    clearEmission();

    // Panel frame
    drawCube({ 0,  hh - fw * 0.5f, 0 }, NO_ROT, { width, fw, 0.08f }, DARK_WOOD);  // top
    drawCube({ 0, -hh + fw * 0.5f, 0 }, NO_ROT, { width, fw, 0.08f }, DARK_WOOD);  // bottom
    drawCube({ -hw + fw * 0.5f, 0, 0 }, NO_ROT, { fw, height, 0.08f }, DARK_WOOD); // left
    drawCube({  hw - fw * 0.5f, 0, 0 }, NO_ROT, { fw, height, 0.08f }, DARK_WOOD); // right

    // Kumiko lattice (horizontal bars in paper area)
    float latticeBot = -hh + kickH;
    float latticeTop = hh - fw;
    float latticeH   = latticeTop - latticeBot;
    float latticeW   = width - fw * 2;
    int hBars = (int)(latticeH / 0.40f);
    float hStep = latticeH / (hBars + 1);
    for (int i = 1; i <= hBars; i++) {
        float y = latticeBot + i * hStep;
        drawCube({ 0, y, 0.008f }, NO_ROT, { latticeW - 0.02f, 0.025f, 0.025f }, DARK_WOOD);
    }

    // Kumiko lattice (vertical bars)
    int vBars = (int)(latticeW / 0.35f);
    float vStep = latticeW / (vBars + 1);
    for (int i = 1; i <= vBars; i++) {
        float x = -hw + fw + i * vStep;
        float barCY = (latticeBot + latticeTop) * 0.5f;
        drawCube({ x, barCY, 0.008f }, NO_ROT, { 0.025f, latticeH, 0.025f }, DARK_WOOD);
    }

    // Kick panel (solid wood — koshi-ita)
    float kickCY = -hh + (fw + kickH) * 0.5f;
    drawCube({ 0, kickCY, 0 }, NO_ROT,
             { width - fw * 2, kickH - fw, 0.05f }, WOOD);

    // Hikite (recessed finger pull)
    drawCube({ hw - 0.14f, 0, 0.04f }, NO_ROT, { 0.06f, 0.10f, 0.015f }, DARK_WOOD);
}

// ─── Interactive Sliding Shoji Door (click to open/close) ─────────────────
// Two shoji panels that slide apart horizontally on tracks.
// offset: 0 = closed, 1 = fully open.
void drawSlidingShoji(Vec3 pos, Vec3 rot, float offset, float totalW, float height)
{
    glPushMatrix();
    applyTransform(pos, rot, ONE);

    float panelW = totalW * 0.5f;
    float halfH  = height * 0.5f;
    float halfW  = totalW * 0.5f;
    float slide  = offset * panelW;

    // Removed black aperture - interior is now visible when shoji opens
    if (false) {  // disabled aperture drawing
        float openHW = halfW * offset;
        glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);
        setColor(BLACK);
        glVertex3f(-openHW, -halfH + 0.02f, -0.04f);
        glVertex3f( openHW, -halfH + 0.02f, -0.04f);
        glVertex3f( openHW,  halfH - 0.02f, -0.04f);
        glVertex3f(-openHW,  halfH - 0.02f, -0.04f);
        glEnd();
    }

    // Fixed door frame: top/bottom tracks and side posts
    drawCube({ 0, halfH + 0.03f, 0 }, NO_ROT,
             { totalW + 0.16f, 0.04f, 0.14f }, DARK_WOOD);          // top track
    drawCube({ 0, -halfH - 0.02f, 0 }, NO_ROT,
             { totalW + 0.16f, 0.03f, 0.14f }, DARK_WOOD);          // bottom track
    drawCube({ -(halfW + 0.04f), 0, 0 }, NO_ROT,
             { 0.06f, height + 0.04f, 0.12f }, DARK_WOOD);          // left post
    drawCube({  (halfW + 0.04f), 0, 0 }, NO_ROT,
             { 0.06f, height + 0.04f, 0.12f }, DARK_WOOD);          // right post

    // Left panel (slides left when opening, front track)
    glPushMatrix();
    glTranslatef(-panelW * 0.5f - slide, 0, 0.012f);
    drawShojiPanelCentered(panelW, height);
    glPopMatrix();

    // Right panel (slides right when opening, rear track)
    glPushMatrix();
    glTranslatef(panelW * 0.5f + slide, 0, -0.012f);
    drawShojiPanelCentered(panelW, height);
    glPopMatrix();

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
