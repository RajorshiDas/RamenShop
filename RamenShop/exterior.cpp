#include "exterior.h"
#include "scene.h"    // isDayTime for day/night conditional emission
#include "shader.h"   // setLighting for glow halos

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

    // Ground floor walls (open front) — with window openings

    // ── Back wall: two window openings at x=±3.5, y=[1.3,2.3], size 1.2×1.0 ──
    {
        GLuint wallTex = getTexID(TEX_WALL);
        const float wBot = 1.3f, wTop = 2.3f;  // window vertical range
        const float wInner = 2.9f, wOuter = 4.1f;  // window x edges (symmetric ±)

        // Bottom strip (full width, below windows)
        drawTexturedBox({ 0, 0, -3.9f }, NO_ROT, { 10, wBot, 0.2f }, wallTex, WHITE, 2.0f);
        // Top strip (full width, above windows)
        drawTexturedBox({ 0, wTop, -3.9f }, NO_ROT, { 10, GH - wTop, 0.2f }, wallTex, WHITE, 2.0f);
        // Center section (between the two windows)
        drawTexturedBox({ 0, wBot, -3.9f }, NO_ROT, { wInner * 2.0f, wTop - wBot, 0.2f }, wallTex, WHITE, 2.0f);
        // Far left (left of left window)
        drawTexturedBox({ -4.55f, wBot, -3.9f }, NO_ROT, { 0.9f, wTop - wBot, 0.2f }, wallTex, WHITE, 2.0f);
        // Far right (right of right window)
        drawTexturedBox({ 4.55f, wBot, -3.9f }, NO_ROT, { 0.9f, wTop - wBot, 0.2f }, wallTex, WHITE, 2.0f);
    }

    // ── Side walls: window openings for clear glass windows at z=0.5, 2.4×2.0 ──
    for (int sx = -1; sx <= 1; sx += 2) {
        // Lower dark wood wainscot (unchanged)
        drawTexturedBox({ sx * 4.9f, 0, 0 }, NO_ROT, { 0.2f, 1.2f, 8 },
                        getTexID(TEX_DARK_WOOD), WHITE, 1.0f);

        // Upper wall split around window opening: z=[-0.7,1.7], y=[1.2,3.2]
        GLuint wallTex = getTexID(TEX_WALL);
        float wallH = GH - 1.2f;  // 2.1
        // Section behind window (z < -0.7)
        drawTexturedBox({ sx * 4.9f, 1.2f, -2.35f }, NO_ROT, { 0.2f, wallH, 3.3f },
                        wallTex, WHITE, 2.0f);
        // Section in front of window (z > 1.7)
        drawTexturedBox({ sx * 4.9f, 1.2f, 2.85f }, NO_ROT, { 0.2f, wallH, 2.3f },
                        wallTex, WHITE, 2.0f);
        // Strip above window (y=[3.2,3.3] over window area)
        drawTexturedBox({ sx * 4.9f, 3.2f, 0.5f }, NO_ROT, { 0.2f, GH - 3.2f, 2.4f },
                        wallTex, WHITE, 2.0f);
    }

    // Front: structural wood posts
    drawCuboid({ -4.9f, 0, 3.9f }, NO_ROT, { 0.25f, GH, 0.25f }, DARK_WOOD);
    drawCuboid({  4.9f, 0, 3.9f }, NO_ROT, { 0.25f, GH, 0.25f }, DARK_WOOD);
    drawCuboid({ -1.8f, 0, 3.9f }, NO_ROT, { 0.15f, GH, 0.15f }, DARK_WOOD);
    drawCuboid({  1.8f, 0, 3.9f }, NO_ROT, { 0.15f, GH, 0.15f }, DARK_WOOD);
    drawCuboid({ 0, GH - 0.15f, 3.95f }, NO_ROT, { 10.2f, 0.18f, 0.14f }, DARK_WOOD);

    // Second floor walls — light cream plaster with dark wood frame
    drawTexturedBox({  0,    GH,  3.9f }, NO_ROT, { 10,   UH, 0.2f }, getTexID(TEX_WALL), WHITE, 2.0f);
    drawTexturedBox({  0,    GH, -3.9f }, NO_ROT, { 10,   UH, 0.2f }, getTexID(TEX_WALL), WHITE, 2.0f);
    drawTexturedBox({ -4.9f, GH,  0    }, NO_ROT, { 0.2f, UH, 8    }, getTexID(TEX_WALL), WHITE, 2.0f);
    drawTexturedBox({  4.9f, GH,  0    }, NO_ROT, { 0.2f, UH, 8    }, getTexID(TEX_WALL), WHITE, 2.0f);

    // Floor-separator beam between ground floor and second floor
    drawCuboid({ 0, GH, 3.95f }, NO_ROT, { 10.2f, 0.15f, 0.14f }, DARK_WOOD);

    // Vertical mid-posts on second floor front facade
    drawCuboid({ -1.8f, GH, 3.95f }, NO_ROT, { 0.12f, UH, 0.12f }, DARK_WOOD);
    drawCuboid({  1.8f, GH, 3.95f }, NO_ROT, { 0.12f, UH, 0.12f }, DARK_WOOD);

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

    // Eave overhang slab (underside of roof eave)
    drawCuboid({ 0, TH, 0 }, NO_ROT, { 11.4f, 0.12f, 9.4f }, DARK_GRAY);

    // Traditional Japanese gable roof (ridge runs E–W, slopes N and S)
    drawWedge({ 0, TH + 0.12f, 0 }, NO_ROT, { 11.4f, 2.2f, 9.4f }, ROOF_TILE);

    // Ridge cap beam along the peak
    drawCuboid({ 0, TH + 2.24f, 0 }, NO_ROT, { 11.6f, 0.16f, 0.24f }, DARK_WOOD);

    // Eave fascia boards on all four sides
    drawCuboid({ 0,     TH + 0.02f,  4.7f }, NO_ROT, { 11.6f, 0.22f, 0.14f }, DARK_WOOD);
    drawCuboid({ 0,     TH + 0.02f, -4.7f }, NO_ROT, { 11.6f, 0.22f, 0.14f }, DARK_WOOD);
    drawCuboid({ -5.7f, TH + 0.02f,  0    }, NO_ROT, { 0.14f, 0.22f, 9.6f  }, DARK_WOOD);
    drawCuboid({  5.7f, TH + 0.02f,  0    }, NO_ROT, { 0.14f, 0.22f, 9.6f  }, DARK_WOOD);

    // Zigzag blue awning over open front
    drawBoard({ 0, 3.22f, 4.7f }, { 12, 0, 0 }, { 10.6f, 1.2f, 1.4f }, AWNING_BLUE);
    for (int i = -18; i <= 18; i++) {
        float x = i * 0.28f;
        drawCone({ x, 2.95f, 5.15f }, { 15, 0, 0 }, { 0.28f, 0.25f, 0.18f }, AWNING_BLUE);
    }

    // (fascia trim superseded by eave fascia boards in gable roof above)

    glPopMatrix();
}

void drawWindow(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    if (!isDayTime)
        setEmission(0.40f, 0.28f, 0.10f);
    else
        setEmission(0.05f, 0.04f, 0.02f);
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

    // Japanese-style cobblestone road — warm grey stone base
    const Color STONE_BASE  = { 0.55f, 0.52f, 0.48f };
    const Color STONE_LIGHT = { 0.65f, 0.62f, 0.56f };
    const Color STONE_DARK  = { 0.42f, 0.40f, 0.36f };
    const Color GROUT       = { 0.35f, 0.32f, 0.28f };

    // Road bed
    drawPlane({ 0, 0.015f, 0 }, NO_ROT, { 60, 1, 8 }, STONE_BASE);

    // Cobblestone pattern — rows of rectangular stones with grout lines
    float roadHalfW = 30.0f, roadHalfD = 4.0f;
    float stoneW = 1.2f, stoneD = 0.8f, gap = 0.06f;

    for (float sx = -roadHalfW; sx < roadHalfW; sx += stoneW + gap) {
        for (float sz = -roadHalfD; sz < roadHalfD; sz += stoneD + gap) {
            // Alternate row offset for brick pattern
            int row = (int)((sz + roadHalfD) / (stoneD + gap));
            float xOff = (row % 2 == 0) ? 0.0f : stoneW * 0.5f;
            float px = sx + xOff;
            if (px > roadHalfW) continue;
            // Vary stone color slightly based on position
            float hash = sinf(px * 13.7f + sz * 29.3f) * 0.5f + 0.5f;
            Color sc = (hash > 0.6f) ? STONE_LIGHT : (hash < 0.3f) ? STONE_DARK : STONE_BASE;
            drawPlane({ px + stoneW * 0.5f, 0.025f, sz + stoneD * 0.5f },
                      NO_ROT, { stoneW - 0.04f, 1, stoneD - 0.04f }, sc);
        }
    }

    // Raised stone curb edges on both sides
    const Color CURB = { 0.50f, 0.48f, 0.44f };
    drawCuboid({ 0, 0.04f,  roadHalfD }, NO_ROT, { 60, 0.08f, 0.18f }, CURB);
    drawCuboid({ 0, 0.04f, -roadHalfD }, NO_ROT, { 60, 0.08f, 0.18f }, CURB);

    // Center line — inlaid darker stone strip (subtle, not painted)
    drawPlane({ 0, 0.026f, 0 }, NO_ROT, { 60, 1, 0.12f }, STONE_DARK);

    glPopMatrix();
}

void drawSidewalk(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Japanese-style flagstone walkway — large flat stones with grout gaps
    const Color FLAG_BASE = { 0.68f, 0.65f, 0.60f };
    const Color FLAG_ALT  = { 0.62f, 0.58f, 0.52f };

    // Raised walkway base
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 60, 0.10f, 3 }, { 0.55f, 0.52f, 0.48f });

    // Flagstone tiles on top
    float walkHalf = 30.0f;
    float tileW = 1.4f, tileD = 1.4f, gap = 0.05f;
    for (float sx = -walkHalf; sx < walkHalf; sx += tileW + gap) {
        for (float sz = -1.4f; sz < 1.4f; sz += tileD + gap) {
            float hash = sinf(sx * 17.3f + sz * 41.7f) * 0.5f + 0.5f;
            Color tc = (hash > 0.5f) ? FLAG_BASE : FLAG_ALT;
            drawPlane({ sx + tileW * 0.5f, 0.105f, sz + tileD * 0.5f },
                      NO_ROT, { tileW - 0.06f, 1, tileD - 0.06f }, tc);
        }
    }

    // Low stone edge border
    drawCuboid({ 0, 0.05f, 1.45f }, NO_ROT, { 60, 0.12f, 0.10f }, { 0.50f, 0.48f, 0.44f });
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

    // Lamp bulb — bright sodium-warm glow at night, subtle during day
    if (!isDayTime)
        setEmission(0.95f, 0.70f, 0.22f);
    else
        setEmission(0.12f, 0.10f, 0.04f);
    drawSphere({ 0.7f, 3.12f, 0 },  NO_ROT, { 0.2f, 0.2f, 0.2f }, GOLD);
    clearEmission();

    // Realistic lamp lighting (visible at night)
    if (!isDayTime) {
        // Tiny warm halo around the bulb — just a hint of scatter, not a ball
        glPushMatrix();
        glTranslatef(0.7f, 3.12f, 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);
        glColor4f(1.0f, 0.80f, 0.30f, 0.12f);
        gluSphere(quad, 0.25f, 10, 10);
        glDepthMask(GL_TRUE);
        setLighting(true);
        glDisable(GL_BLEND);
        glPopMatrix();

        // Downward light cone — semi-transparent cone from lamp shade to ground
        // Simulates the visible beam of directed light cast by the shade
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);
        float coneTopY = 3.0f;     // bottom of the lamp shade
        float coneBaseY = 0.04f;   // ground level
        float topR = 0.15f;        // narrow opening at shade
        float bottomR = 2.2f;      // spread on ground
        int segs = 24;
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= segs; i++) {
            float a = (float)i * 2.0f * PI / segs;
            float cs = cosf(a), sn = sinf(a);
            // Top ring (bright, near the bulb)
            glColor4f(1.0f, 0.80f, 0.30f, 0.06f);
            glVertex3f(0.7f + topR * cs, coneTopY, topR * sn);
            // Bottom ring (fades to transparent at ground)
            glColor4f(0.95f, 0.70f, 0.22f, 0.0f);
            glVertex3f(0.7f + bottomR * cs, coneBaseY, bottomR * sn);
        }
        glEnd();
        glDepthMask(GL_TRUE);
        setLighting(true);
        glDisable(GL_BLEND);

        // Ground light pool — soft radial gradient on the street
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        // Bright center directly below the lamp
        glColor4f(0.95f, 0.75f, 0.28f, 0.18f);
        glVertex3f(0.7f, 0.04f, 0.0f);
        // Inner ring — still somewhat bright
        for (int i = 0; i <= 32; i++) {
            float a = (float)i * 2.0f * PI / 32;
            float r = 2.5f;
            float t = (float)i / 32.0f;
            float fadeAlpha = 0.0f;
            glColor4f(0.90f, 0.65f, 0.18f, fadeAlpha);
            glVertex3f(0.7f + r * cosf(a), 0.04f, r * sinf(a));
        }
        glEnd();
        glDepthMask(GL_TRUE);
        setLighting(true);
        glDisable(GL_BLEND);
    }

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

    // ── Illuminated product window — brighter at night ──────────────────────
    if (!isDayTime)
        setEmission(0.40f, 0.30f, 0.15f);
    else
        setEmission(0.10f, 0.08f, 0.04f);
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

    // Washi paper panel — warm glow from interior light at night
    if (!isDayTime)
        setEmission(0.30f, 0.22f, 0.08f);
    else
        setEmission(0.04f, 0.03f, 0.01f);
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

    // Washi paper panel — warm glow at night
    if (!isDayTime)
        setEmission(0.30f, 0.22f, 0.08f);
    else
        setEmission(0.04f, 0.03f, 0.01f);
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

    // Washi paper (above kick panel) — warm glow at night
    if (!isDayTime)
        setEmission(0.30f, 0.22f, 0.08f);
    else
        setEmission(0.04f, 0.03f, 0.01f);
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

    // Washi paper panel — warm glow from interior light at night
    if (!isDayTime)
        setEmission(0.30f, 0.22f, 0.08f);
    else
        setEmission(0.04f, 0.03f, 0.01f);
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

    // Washi paper panel — warm glow at night (interior light through paper)
    if (!isDayTime)
        setEmission(0.35f, 0.24f, 0.08f);
    else
        setEmission(0.05f, 0.04f, 0.02f);   // subtle during day
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

// ════════════════════════════════════════════════════════════════════════════
//  EXTENDED OUTDOOR ENVIRONMENT
// ════════════════════════════════════════════════════════════════════════════

// ─── Japanese Pine Tree (Matsu) ─────────────────────────────────────────────
// Layered conical tiers of dark green foliage on a gnarled brown trunk
void drawJapanesePineTree(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Trunk — slightly bent / gnarled
    const Color PINE_TRUNK = { 0.28f, 0.18f, 0.10f };
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.12f, 2.0f, 0.12f }, PINE_TRUNK);
    drawCylinder({ 0.05f, 2.0f, 0 }, { 0, 0, 5 }, { 0.10f, 0.8f, 0.10f }, PINE_TRUNK);

    // Foliage tiers — dark pine green cones stacked
    const Color PINE_DARK  = { 0.08f, 0.30f, 0.12f };
    const Color PINE_LIGHT = { 0.12f, 0.38f, 0.15f };

    drawCone({ 0, 1.2f, 0 }, NO_ROT, { 1.8f, 1.0f, 1.8f }, PINE_DARK);
    drawCone({ 0, 1.8f, 0 }, NO_ROT, { 1.5f, 0.9f, 1.5f }, PINE_LIGHT);
    drawCone({ 0, 2.3f, 0 }, NO_ROT, { 1.2f, 0.8f, 1.2f }, PINE_DARK);
    drawCone({ 0, 2.7f, 0 }, NO_ROT, { 0.8f, 0.7f, 0.8f }, PINE_LIGHT);

    glPopMatrix();
}

// ─── Bamboo Grove ───────────────────────────────────────────────────────────
// Cluster of tall bamboo stalks with small leaf tufts at the top
void drawBambooGrove(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const Color BAMBOO_GREEN = { 0.35f, 0.55f, 0.25f };
    const Color BAMBOO_LIGHT = { 0.45f, 0.65f, 0.30f };
    const Color BAMBOO_LEAF  = { 0.25f, 0.50f, 0.18f };

    // 8 bamboo stalks at slightly random positions
    const float stalks[][3] = {
        { 0.0f, 0, 0.0f }, { 0.3f, 0, -0.2f }, { -0.25f, 0, 0.15f },
        { 0.15f, 0, 0.3f }, { -0.35f, 0, -0.1f }, { 0.4f, 0, 0.15f },
        { -0.1f, 0, -0.35f }, { 0.2f, 0, -0.4f }
    };
    const float heights[] = { 4.0f, 3.5f, 4.2f, 3.8f, 4.5f, 3.3f, 3.7f, 4.1f };

    for (int i = 0; i < 8; i++) {
        float h = heights[i];
        Color col = (i % 2 == 0) ? BAMBOO_GREEN : BAMBOO_LIGHT;

        // Main stalk
        drawCylinder({ stalks[i][0], 0, stalks[i][2] }, NO_ROT,
                     { 0.04f, h, 0.04f }, col);

        // Nodes (bamboo joints) every 0.6 units
        for (float y = 0.4f; y < h; y += 0.6f) {
            drawTorus({ stalks[i][0], y, stalks[i][2] }, { 90, 0, 0 },
                      ONE, col, 0.008f, 0.045f);
        }

        // Leaf tufts at top
        float sway = sinf(animTime * 1.5f + i * 0.8f) * 3.0f;
        glPushMatrix();
        glTranslatef(stalks[i][0], h, stalks[i][2]);
        glRotatef(sway, 0, 0, 1);
        for (int j = 0; j < 3; j++) {
            float angle = j * 120.0f + i * 30.0f;
            glPushMatrix();
            glRotatef(angle, 0, 1, 0);
            glRotatef(-35, 1, 0, 0);
            drawCube({ 0, 0, 0.15f }, NO_ROT, { 0.02f, 0.005f, 0.25f }, BAMBOO_LEAF);
            glPopMatrix();
        }
        glPopMatrix();
    }

    glPopMatrix();
}

// ─── Maple Tree (Momiji) ────────────────────────────────────────────────────
// Autumn-red maple tree with spreading canopy
void drawMapleTree(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Trunk with slight lean
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.15f, 1.6f, 0.15f }, TRUNK);
    // Major branch split
    drawCylinder({ 0.1f, 1.6f, 0 }, { 0, 0, 15 }, { 0.08f, 0.8f, 0.08f }, TRUNK);
    drawCylinder({ -0.1f, 1.6f, 0.05f }, { 0, 0, -12 }, { 0.07f, 0.7f, 0.07f }, TRUNK);

    // Autumn foliage — red/orange/gold maple leaves
    const Color MAPLE_RED    = { 0.85f, 0.15f, 0.08f };
    const Color MAPLE_ORANGE = { 0.92f, 0.45f, 0.10f };
    const Color MAPLE_GOLD   = { 0.95f, 0.70f, 0.15f };

    drawSphere({ 0.0f, 2.3f, 0.0f }, NO_ROT, { 1.6f, 1.3f, 1.6f }, MAPLE_RED);
    drawSphere({ 0.6f, 2.5f, 0.3f }, NO_ROT, { 1.2f, 1.0f, 1.2f }, MAPLE_ORANGE);
    drawSphere({ -0.5f, 2.4f, -0.2f }, NO_ROT, { 1.1f, 0.9f, 1.1f }, MAPLE_GOLD);
    drawSphere({ 0.2f, 2.8f, -0.1f }, NO_ROT, { 0.9f, 0.8f, 0.9f }, MAPLE_RED);
    drawSphere({ -0.3f, 2.0f, 0.4f }, NO_ROT, { 0.8f, 0.7f, 0.8f }, MAPLE_ORANGE);

    glPopMatrix();
}

// ─── Japanese Neighboring House ─────────────────────────────────────────────
// Simple traditional Japanese house with cream walls, dark wood frame, tiled roof
void drawJapaneseHouse(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float w = 4.0f, d = 3.5f, h = 2.8f;

    // Walls — cream plaster
    GLuint wallTex = getTexID(TEX_WALL);
    // Front wall
    drawTexturedBox({ 0, 0, d * 0.5f }, NO_ROT, { w, h, 0.15f }, wallTex, WHITE, 2.0f);
    // Back wall
    drawTexturedBox({ 0, 0, -d * 0.5f }, NO_ROT, { w, h, 0.15f }, wallTex, WHITE, 2.0f);
    // Left wall
    drawTexturedBox({ -w * 0.5f, 0, 0 }, NO_ROT, { 0.15f, h, d }, wallTex, WHITE, 2.0f);
    // Right wall
    drawTexturedBox({ w * 0.5f, 0, 0 }, NO_ROT, { 0.15f, h, d }, wallTex, WHITE, 2.0f);

    // Dark wood frame — corner posts
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * w * 0.5f, 0, sz * d * 0.5f }, NO_ROT,
                       { 0.20f, h, 0.20f }, DARK_WOOD);

    // Top beam
    drawCuboid({ 0, h - 0.08f, d * 0.52f }, NO_ROT, { w + 0.2f, 0.12f, 0.10f }, DARK_WOOD);
    drawCuboid({ 0, h - 0.08f, -d * 0.52f }, NO_ROT, { w + 0.2f, 0.12f, 0.10f }, DARK_WOOD);

    // Shoji windows on front — glowing at night
    drawShojiWindow({ -0.8f, 1.5f, d * 0.5f + 0.08f }, NO_ROT, ONE, 1.2f, 1.0f);
    drawShojiWindow({  0.8f, 1.5f, d * 0.5f + 0.08f }, NO_ROT, ONE, 1.2f, 1.0f);

    // Door
    drawCuboid({ 0, 0, d * 0.5f + 0.05f }, NO_ROT, { 0.7f, 1.8f, 0.06f }, WOOD);
    drawSphere({ 0.25f, 0.9f, d * 0.5f + 0.10f }, NO_ROT, { 0.04f, 0.04f, 0.04f }, DARK_GRAY);

    // Gable roof
    drawCuboid({ 0, h, 0 }, NO_ROT, { w + 0.8f, 0.08f, d + 0.8f }, DARK_GRAY);
    drawWedge({ 0, h + 0.08f, 0 }, NO_ROT, { w + 0.8f, 1.4f, d + 0.8f }, ROOF_TILE);

    // Ridge cap
    drawCuboid({ 0, h + 1.42f, 0 }, NO_ROT, { w + 1.0f, 0.10f, 0.18f }, DARK_WOOD);

    // Garden stone lantern near entrance (larger, brighter)
    drawStoneLantern({ w * 0.3f, 0, d * 0.5f + 0.8f }, NO_ROT, { 0.7f, 0.7f, 0.7f });

    // ── Wall-mounted entrance lantern (warm glow at night) ──
    // Lantern body above the door
    if (!isDayTime)
        setEmission(0.90f, 0.60f, 0.15f);
    else
        setEmission(0.08f, 0.06f, 0.02f);
    drawSphere({ 0, 2.3f, d * 0.5f + 0.14f }, NO_ROT,
               { 0.10f, 0.14f, 0.10f }, { 0.95f, 0.70f, 0.20f });
    clearEmission();
    // Lantern bracket
    drawCuboid({ 0, 2.5f, d * 0.5f + 0.10f }, NO_ROT,
               { 0.04f, 0.06f, 0.08f }, DARK_WOOD);

    // ── Night glow effects from the house ──
    if (!isDayTime) {
        // Glow halo around entrance lantern
        glPushMatrix();
        glTranslatef(0, 2.3f, d * 0.5f + 0.14f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);
        glColor4f(0.95f, 0.65f, 0.18f, 0.14f);
        gluSphere(quad, 0.6f, 12, 12);
        glColor4f(0.90f, 0.55f, 0.12f, 0.05f);
        gluSphere(quad, 1.5f, 12, 12);
        glPopMatrix();

        // Window light spill — warm glow pool on ground from windows
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        glColor4f(0.90f, 0.65f, 0.20f, 0.15f);
        glVertex3f(0, 0.03f, d * 0.5f + 1.0f);
        for (int i = 0; i <= 24; i++) {
            float a = (float)i * 2.0f * PI / 24;
            glColor4f(0.90f, 0.60f, 0.15f, 0.0f);
            glVertex3f(2.5f * cosf(a), 0.03f, d * 0.5f + 1.0f + 2.5f * sinf(a));
        }
        glEnd();

        glDepthMask(GL_TRUE);
        setLighting(true);
        glDisable(GL_BLEND);
    }

    glPopMatrix();
}

// ─── Stone Lantern (Toro) ───────────────────────────────────────────────────
// Traditional Japanese garden stone lantern
void drawStoneLantern(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const Color STONE = { 0.60f, 0.58f, 0.55f };
    const Color STONE_DARK = { 0.45f, 0.43f, 0.40f };

    // Base
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.5f, 0.15f, 0.5f }, STONE);
    // Pillar
    drawCylinder({ 0, 0.15f, 0 }, NO_ROT, { 0.10f, 0.8f, 0.10f }, STONE);
    // Light chamber
    drawCuboid({ 0, 0.95f, 0 }, NO_ROT, { 0.35f, 0.35f, 0.35f }, STONE_DARK);

    // Glowing chamber interior at night
    if (!isDayTime) {
        setEmission(0.70f, 0.45f, 0.12f);
        drawCuboid({ 0, 0.97f, 0 }, NO_ROT, { 0.28f, 0.25f, 0.28f }, { 0.95f, 0.75f, 0.30f });
        clearEmission();
    }

    // Roof cap — pagoda-style
    drawCuboid({ 0, 1.30f, 0 }, NO_ROT, { 0.50f, 0.06f, 0.50f }, STONE);
    drawCone({ 0, 1.36f, 0 }, NO_ROT, { 0.45f, 0.30f, 0.45f }, STONE_DARK);
    // Finial
    drawSphere({ 0, 1.66f, 0 }, NO_ROT, { 0.06f, 0.08f, 0.06f }, STONE);

    glPopMatrix();
}

// ─── Wooden Bridge ──────────────────────────────────────────────────────────
// Small arched wooden bridge over water
void drawBridge(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const Color BRIDGE_WOOD = { 0.45f, 0.28f, 0.15f };
    // Spans the whole lake (lake z-radius is 8 units, drawn at 1.0 scale)
    const float bLen = 17.0f, bW = 1.6f, arch = 0.6f;
    const int   NP = 34;     // deck planks
    const int   NR = 8;      // railing segments per side

    // Arched deck planks
    for (int i = 0; i < NP; i++) {
        float z = (i + 0.5f) / NP * bLen - bLen * 0.5f;
        float archY = arch * cosf(z * PI / bLen);
        drawCuboid({ 0, archY, z }, NO_ROT, { bW, 0.08f, bLen / NP + 0.01f }, BRIDGE_WOOD);
    }

    // Railings
    for (int sx = -1; sx <= 1; sx += 2) {
        float rx = sx * bW * 0.5f;
        // Posts
        for (int i = 0; i <= NR; i++) {
            float z = i * (bLen / NR) - bLen * 0.5f;
            float archY = arch * cosf(z * PI / bLen);
            drawCuboid({ rx, archY, z }, NO_ROT, { 0.08f, 0.7f, 0.08f }, DARK_WOOD);
        }
        // Top rail (curved approximation)
        for (int i = 0; i < NR; i++) {
            float z0 = i * (bLen / NR) - bLen * 0.5f;
            float z1 = (i + 1) * (bLen / NR) - bLen * 0.5f;
            float y0 = arch * cosf(z0 * PI / bLen) + 0.65f;
            float y1 = arch * cosf(z1 * PI / bLen) + 0.65f;
            float zc = (z0 + z1) * 0.5f;
            float len = sqrtf((z1 - z0) * (z1 - z0) + (y1 - y0) * (y1 - y0));
            float pitch = atanf((y1 - y0) / (z1 - z0)) * 180.0f / PI;
            drawCuboid({ rx, (y0 + y1) * 0.5f - 0.03f, zc }, { -pitch, 0, 0 },
                       { 0.06f, 0.06f, len + 0.02f }, DARK_WOOD);
        }
    }

    glPopMatrix();
}

// ─── Lake / Pond ────────────────────────────────────────────────────────────
// Reflective water surface with shore rocks and subtle wave animation
void drawLake(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float lakeR = 8.0f;

    // Water surface — semi-transparent reflective blue
    setMaterialDielectric(100.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Animated subtle wave using vertex displacement
    float waveT = animTime * 0.8f;
    Color waterCol = isDayTime
        ? Color{ 0.25f, 0.50f, 0.70f }
        : Color{ 0.08f, 0.15f, 0.30f };

    // Muddy bank under/around the water so the lake sits in a basin, not on the lawn
    {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        setColor({ 0.30f, 0.24f, 0.15f });
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        glVertex3f(0, -0.03f, 0);
        for (int i = 0; i <= 48; i++) {
            float a = (float)i * 2.0f * PI / 48;
            glVertex3f(lakeR * 1.10f * cosf(a), -0.03f, lakeR * 1.10f * sinf(a));
        }
        glEnd();
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
    }

    glColor4f(waterCol.r, waterCol.g, waterCol.b, 0.88f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    // Center vertex
    float cy = 0.02f + 0.02f * sinf(waveT);
    glVertex3f(0, cy, 0);
    // Outer ring
    int segs = 48;
    for (int i = 0; i <= segs; i++) {
        float a = (float)i * 2.0f * PI / segs;
        float rx = lakeR * cosf(a);
        float rz = lakeR * sinf(a);
        float wy = 0.02f + 0.015f * sinf(waveT + a * 3.0f);
        glVertex3f(rx, wy, rz);
    }
    glEnd();

    // Specular highlight shimmer layer
    glColor4f(0.70f, 0.80f, 0.95f, 0.12f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0, cy + 0.005f, 0);
    for (int i = 0; i <= segs; i++) {
        float a = (float)i * 2.0f * PI / segs;
        float rx = lakeR * 0.85f * cosf(a);
        float rz = lakeR * 0.85f * sinf(a);
        float wy = 0.025f + 0.01f * sinf(waveT * 1.3f + a * 5.0f);
        glVertex3f(rx, wy, rz);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

    // Shore rocks around the edge
    const Color ROCK = { 0.40f, 0.38f, 0.35f };
    const Color ROCK_DARK = { 0.30f, 0.28f, 0.25f };
    const float rocks[][4] = {
        // x, z, size, which_color
        { 7.5f, 1.0f, 0.5f, 0 }, { 7.0f, -2.0f, 0.4f, 1 },
        { -6.8f, 2.5f, 0.6f, 0 }, { -7.2f, -1.5f, 0.35f, 1 },
        { 5.0f, 5.5f, 0.45f, 0 }, { -4.5f, 6.0f, 0.5f, 1 },
        { 3.0f, -6.5f, 0.55f, 0 }, { -3.5f, -7.0f, 0.4f, 1 },
        { 6.5f, 4.0f, 0.3f, 0 }, { -6.0f, -4.5f, 0.35f, 1 },
        { 0.5f, 7.8f, 0.45f, 0 }, { -1.0f, -7.5f, 0.5f, 1 },
    };
    for (int i = 0; i < 12; i++) {
        Color rc = rocks[i][3] > 0.5f ? ROCK_DARK : ROCK;
        drawSphere({ rocks[i][0] * 1.05f, 0.0f, rocks[i][1] * 1.05f }, NO_ROT,
                   { rocks[i][2], rocks[i][2] * 0.5f, rocks[i][2] }, rc);
    }

    // Lily pads
    const Color LILY_GREEN = { 0.18f, 0.45f, 0.15f };
    const float lilies[][2] = { {2.0f, 1.5f}, {-1.5f, 3.0f}, {3.5f, -2.0f}, {-2.5f, -1.0f} };
    for (int i = 0; i < 4; i++) {
        float ly = 0.035f + 0.01f * sinf(waveT * 0.7f + i);
        drawCylinder({ lilies[i][0], ly, lilies[i][1] }, NO_ROT,
                     { 0.25f, 0.008f, 0.25f }, LILY_GREEN);
    }

    glPopMatrix();
}

// ─── Grass Patch ────────────────────────────────────────────────────────────
// Cluster of grass blade quads rising from the ground
void drawGrassPatch(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const Color GRASS_D = { 0.16f, 0.38f, 0.10f };   // blade root
    const Color GRASS_L = { 0.45f, 0.70f, 0.22f };   // blade tip

    // Tuft of tapered blades spread evenly (sunflower spiral) over a ~1 unit
    // radius disc; all blades go in one glBegin for speed.
    const int   N = 36;
    const float GOLDEN = 2.39996f;
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 1, 0);
    for (int i = 0; i < N; i++) {
        float r   = sqrtf((i + 0.5f) / N);
        float bx  = r * cosf(i * GOLDEN);
        float bz  = r * sinf(i * GOLDEN);
        float bh  = 0.35f + 0.25f * (0.5f + 0.5f * sinf(i * 1.7f));
        float yaw = i * 1.1f;
        float dx  = cosf(yaw) * 0.07f, dz = sinf(yaw) * 0.07f;
        float sway = sinf(animTime * 1.8f + i * 0.7f + bx * 2.0f) * 0.06f;
        float lean = 0.08f * sinf(i * 2.3f);

        setColor(GRASS_D);
        glVertex3f(bx - dx, 0.0f, bz - dz);
        glVertex3f(bx + dx, 0.0f, bz + dz);
        setColor(GRASS_L);
        glVertex3f(bx + sway + lean, bh, bz + sway * 0.5f);
    }
    glEnd();

    glPopMatrix();
}

// ─── Dense Jungle Vegetation ────────────────────────────────────────────────
// Thick cluster of tropical-looking plants, ferns, and small trees
void drawJungle(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const Color JUNGLE_DARK  = { 0.10f, 0.35f, 0.12f };
    const Color JUNGLE_MID   = { 0.15f, 0.45f, 0.15f };
    const Color JUNGLE_LIGHT = { 0.22f, 0.55f, 0.20f };
    const Color FERN         = { 0.18f, 0.48f, 0.18f };

    // Dense undergrowth spheres (ground cover)
    drawSphere({ 0.0f, 0.3f, 0.0f }, NO_ROT, { 2.5f, 0.6f, 2.5f }, JUNGLE_DARK);
    drawSphere({ 1.2f, 0.35f, 0.8f }, NO_ROT, { 2.0f, 0.5f, 2.0f }, JUNGLE_MID);
    drawSphere({ -1.0f, 0.3f, -0.6f }, NO_ROT, { 1.8f, 0.55f, 1.8f }, JUNGLE_LIGHT);

    // Mid-level bushes
    drawSphere({ 0.5f, 0.8f, 0.3f }, NO_ROT, { 1.5f, 1.2f, 1.5f }, JUNGLE_MID);
    drawSphere({ -0.8f, 0.7f, 0.5f }, NO_ROT, { 1.3f, 1.0f, 1.3f }, JUNGLE_DARK);
    drawSphere({ 0.3f, 0.9f, -0.8f }, NO_ROT, { 1.4f, 1.1f, 1.4f }, JUNGLE_LIGHT);

    // Tall jungle trees rising above the canopy
    for (int i = 0; i < 4; i++) {
        float tx = sinf(i * 1.8f) * 1.5f;
        float tz = cosf(i * 2.3f) * 1.5f;
        float th = 3.0f + i * 0.5f;
        drawCylinder({ tx, 0, tz }, NO_ROT, { 0.08f, th, 0.08f }, TRUNK);
        drawSphere({ tx, th + 0.5f, tz }, NO_ROT,
                   { 1.2f, 1.0f, 1.2f }, (i % 2) ? JUNGLE_MID : JUNGLE_DARK);
        drawSphere({ tx + 0.3f, th + 0.8f, tz - 0.2f }, NO_ROT,
                   { 0.8f, 0.7f, 0.8f }, JUNGLE_LIGHT);
    }

    // Fern fronds (flat leaf-like quads fanning out from ground)
    for (int i = 0; i < 6; i++) {
        float fx = sinf(i * 1.1f) * 1.0f;
        float fz = cosf(i * 1.5f) * 1.0f;
        float sway = sinf(animTime * 1.2f + i * 0.9f) * 3.0f;
        glPushMatrix();
        glTranslatef(fx, 0, fz);
        glRotatef(i * 60.0f, 0, 1, 0);
        glRotatef(-40 + sway, 1, 0, 0);
        drawCube({ 0, 0.3f, 0 }, NO_ROT, { 0.06f, 0.5f, 0.01f }, FERN);
        // Smaller sub-fronds
        for (int j = 0; j < 3; j++) {
            float fy = 0.15f + j * 0.12f;
            drawCube({ 0.08f, fy, 0 }, { 0, 0, -30 }, { 0.04f, 0.18f, 0.005f }, FERN);
            drawCube({ -0.08f, fy, 0 }, { 0, 0, 30 }, { 0.04f, 0.18f, 0.005f }, FERN);
        }
        glPopMatrix();
    }

    glPopMatrix();
}

// ─── Fireflies ──────────────────────────────────────────────────────────────
// Animated glowing particles that drift and dim/brighten — visible at night only
void drawFireflies(Vec3 pos, float radius, int count)
{
    if (isDayTime) return;   // only at night

    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive glow
    glDepthMask(GL_FALSE);
    setLighting(false);

    for (int i = 0; i < count; i++) {
        // Deterministic but organic-looking motion
        float seed  = (float)i * 1.618f;   // golden ratio spacing
        float phase = seed * 3.14159f;

        // Slow drifting orbit
        float speed = 0.3f + 0.15f * sinf(seed * 2.3f);
        float fx = radius * 0.6f * sinf(animTime * speed + phase)
                 + radius * 0.3f * cosf(animTime * speed * 0.7f + seed);
        float fy = 0.5f + 1.2f * (0.5f + 0.5f * sinf(seed * 1.7f))
                 + 0.3f * sinf(animTime * 0.5f + phase * 2.0f);
        float fz = radius * 0.6f * cosf(animTime * speed * 0.8f + phase * 1.3f)
                 + radius * 0.3f * sinf(animTime * speed * 0.6f + seed * 0.5f);

        // Pulsing glow — each firefly dims and brightens independently
        float pulse = 0.5f + 0.5f * sinf(animTime * (2.0f + seed * 0.5f) + phase);
        // Occasional complete dim-out (firefly "off" period)
        float onOff = sinf(animTime * (0.8f + seed * 0.2f) + seed * 5.0f);
        float brightness = pulse * (onOff > -0.3f ? 1.0f : 0.0f);

        if (brightness < 0.05f) continue;   // skip invisible fireflies

        // Warm yellow-green glow
        glColor4f(0.70f, 0.90f, 0.25f, brightness * 0.6f);

        glPushMatrix();
        glTranslatef(fx, fy, fz);

        // Small glowing point
        glPointSize(4.0f);
        glBegin(GL_POINTS);
        glVertex3f(0, 0, 0);
        glEnd();

        // Soft glow halo around each firefly
        glColor4f(0.65f, 0.85f, 0.20f, brightness * 0.08f);
        gluSphere(quad, 0.08f, 6, 6);

        glPopMatrix();
    }

    glPointSize(1.0f);
    glDepthMask(GL_TRUE);
    setLighting(true);
    glDisable(GL_BLEND);

    glPopMatrix();
}

// ─── Swimming Duck ──────────────────────────────────────────────────────────
// Animated duck that swims in a circle on the lake surface
void drawDuck(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Swim in a slow circle
    float swimAngle = animTime * 0.4f;
    float swimR = 3.5f;
    float dx = swimR * cosf(swimAngle);
    float dz = swimR * sinf(swimAngle);
    // Bob up and down on waves
    float bob = 0.03f * sinf(animTime * 2.0f);

    glTranslatef(dx, bob, dz);
    // Face direction of travel
    float facing = -swimAngle * (180.0f / PI) + 90.0f;
    glRotatef(facing, 0, 1, 0);

    const Color DUCK_WHITE  = { 0.95f, 0.95f, 0.90f };
    const Color DUCK_BROWN  = { 0.50f, 0.32f, 0.12f };
    const Color DUCK_GREEN  = { 0.15f, 0.40f, 0.18f };
    const Color DUCK_ORANGE = { 0.90f, 0.55f, 0.10f };
    const Color DUCK_YELLOW = { 0.95f, 0.85f, 0.20f };

    // Body — oval shape sitting on water
    setMaterialPBR(Materials::GlazedMatte, DUCK_WHITE);
    drawSphere({ 0, 0.08f, 0 }, NO_ROT, { 0.22f, 0.16f, 0.30f }, DUCK_WHITE);

    // Brown wing patches
    setMaterialPBR(Materials::GlazedMatte, DUCK_BROWN);
    drawSphere({ -0.12f, 0.12f, -0.02f }, { 0, 0, -15 }, { 0.08f, 0.06f, 0.18f }, DUCK_BROWN);
    drawSphere({  0.12f, 0.12f, -0.02f }, { 0, 0,  15 }, { 0.08f, 0.06f, 0.18f }, DUCK_BROWN);

    // Head — green iridescent (mallard)
    setMaterialPBR(Materials::GlazedMatte, DUCK_GREEN);
    drawSphere({ 0, 0.26f, 0.18f }, NO_ROT, { 0.10f, 0.10f, 0.10f }, DUCK_GREEN);

    // Beak
    setMaterialPBR(Materials::GlazedMatte, DUCK_ORANGE);
    drawCone({ 0, 0.24f, 0.28f }, { -90, 0, 0 }, { 0.04f, 0.10f, 0.03f }, DUCK_ORANGE);

    // Eyes — tiny black dots
    drawSphere({ -0.06f, 0.28f, 0.24f }, NO_ROT, { 0.015f, 0.015f, 0.015f }, BLACK);
    drawSphere({  0.06f, 0.28f, 0.24f }, NO_ROT, { 0.015f, 0.015f, 0.015f }, BLACK);

    // Tail feathers — small upward flick
    setMaterialPBR(Materials::GlazedMatte, DUCK_BROWN);
    drawCone({ 0, 0.16f, -0.28f }, { 45, 0, 0 }, { 0.06f, 0.12f, 0.05f }, DUCK_BROWN);

    // Wake ripples behind the duck (small translucent V)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    setLighting(false);
    float wakeAlpha = 0.18f;
    glColor4f(0.80f, 0.85f, 0.90f, wakeAlpha);
    glBegin(GL_TRIANGLES);
    glVertex3f( 0.0f,  0.01f, -0.30f);
    glVertex3f(-0.25f, 0.01f, -0.70f);
    glVertex3f( 0.0f,  0.01f, -0.50f);
    glVertex3f( 0.0f,  0.01f, -0.30f);
    glVertex3f( 0.25f, 0.01f, -0.70f);
    glVertex3f( 0.0f,  0.01f, -0.50f);
    glEnd();
    glDepthMask(GL_TRUE);
    setLighting(true);
    glDisable(GL_BLEND);

    resetMaterialGloss();
    glPopMatrix();
}

// ─── Flower Garden ──────────────────────────────────────────────────────────
// Patch of colorful Japanese flowers: chrysanthemums, cosmos, and lotuses
void drawFlowerGarden(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Flower colors — vibrant Japanese garden palette
    const Color COLORS[] = {
        { 0.90f, 0.20f, 0.30f },   // red
        { 0.95f, 0.45f, 0.55f },   // pink
        { 0.95f, 0.85f, 0.20f },   // yellow
        { 0.85f, 0.40f, 0.70f },   // magenta
        { 0.98f, 0.98f, 0.95f },   // white
        { 0.70f, 0.30f, 0.60f },   // purple
        { 0.95f, 0.60f, 0.15f },   // orange
    };
    const int NCOLORS = 7;
    const Color STEM_GREEN = { 0.20f, 0.45f, 0.12f };
    const Color LEAF_GREEN = { 0.25f, 0.55f, 0.18f };
    const Color CENTER_YEL = { 0.95f, 0.80f, 0.15f };

    // Scatter flowers in a sunflower-spiral disc of radius ~3
    const int NFLOWERS = 40;
    const float GOLDEN = 2.39996f;
    const float RADIUS = 3.0f;

    for (int i = 0; i < NFLOWERS; i++) {
        float r = RADIUS * sqrtf((i + 0.5f) / NFLOWERS);
        float fx = r * cosf(i * GOLDEN);
        float fz = r * sinf(i * GOLDEN);
        // Random-ish height and color
        float h = 0.3f + 0.25f * (0.5f + 0.5f * sinf(i * 3.7f));
        int ci = i % NCOLORS;
        float sway = sinf(animTime * 1.5f + i * 0.9f) * 2.5f;

        glPushMatrix();
        glTranslatef(fx, 0, fz);
        glRotatef(sway, 0, 0, 1);

        // Stem
        setMaterialPBR(Materials::WoodMatte, STEM_GREEN);
        drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.015f, h, 0.015f }, STEM_GREEN);

        // Leaves on stem (1-2 small leaves)
        setMaterialPBR(Materials::GlazedMatte, LEAF_GREEN);
        float leafY = h * 0.4f;
        drawSphere({ 0.06f, leafY, 0 }, { 0, 0, -30 }, { 0.08f, 0.02f, 0.04f }, LEAF_GREEN);
        if (i % 3 == 0)
            drawSphere({ -0.05f, leafY + 0.08f, 0 }, { 0, 0, 25 }, { 0.07f, 0.02f, 0.035f }, LEAF_GREEN);

        // Flower head — ring of petals around center
        glPushMatrix();
        glTranslatef(0, h, 0);
        // Slightly tilt flower outward
        float tilt = 10.0f + 8.0f * sinf(i * 2.1f);
        glRotatef(tilt, fx > 0 ? 1.0f : -1.0f, 0, fz > 0 ? 1.0f : -1.0f);

        setMaterialPBR(Materials::GlazedMatte, COLORS[ci]);
        int nPetals = 6 + (i % 3);
        float petalR = 0.06f + 0.02f * sinf(i * 1.3f);
        for (int p = 0; p < nPetals; p++) {
            float pa = (float)p * 360.0f / nPetals;
            float px2 = cosf(pa * PI / 180.0f) * petalR;
            float pz2 = sinf(pa * PI / 180.0f) * petalR;
            drawSphere({ px2, 0.01f, pz2 }, NO_ROT,
                       { 0.035f, 0.012f, 0.02f }, COLORS[ci]);
        }
        // Center (yellow pistil)
        setMaterialPBR(Materials::GlazedMatte, CENTER_YEL);
        drawSphere({ 0, 0.015f, 0 }, NO_ROT, { 0.025f, 0.02f, 0.025f }, CENTER_YEL);

        glPopMatrix();
        glPopMatrix();
    }

    resetMaterialGloss();
    glPopMatrix();
}

// ─── Torii Gate (Shinto shrine gate) ────────────────────────────────────
// Traditional red torii gate — two pillars with kasagi/nuki cross beams
void drawToriiGate(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const Color TORII_RED  = { 0.75f, 0.12f, 0.08f };
    const Color TORII_DARK = { 0.55f, 0.08f, 0.05f };
    const Color STONE_COL  = { 0.55f, 0.53f, 0.50f };

    // Two main pillars (hashira)
    drawCylinder({ -1.2f, 0, 0 }, NO_ROT, { 0.12f, 3.5f, 0.12f }, TORII_RED);
    drawCylinder({  1.2f, 0, 0 }, NO_ROT, { 0.12f, 3.5f, 0.12f }, TORII_RED);

    // Top beam (kasagi) — extends beyond pillars
    drawCuboid({ 0, 3.5f, 0 }, NO_ROT, { 3.2f, 0.15f, 0.18f }, TORII_DARK);
    // Slightly wider cap on top
    drawCuboid({ 0, 3.65f, 0 }, NO_ROT, { 3.4f, 0.08f, 0.22f }, TORII_DARK);

    // Lower cross beam (nuki)
    drawCuboid({ 0, 2.8f, 0 }, NO_ROT, { 2.8f, 0.10f, 0.12f }, TORII_RED);

    // Support wedges (kusabi) where nuki meets pillars
    drawCuboid({ -1.2f, 2.8f, 0.08f }, NO_ROT, { 0.14f, 0.08f, 0.06f }, TORII_DARK);
    drawCuboid({  1.2f, 2.8f, 0.08f }, NO_ROT, { 0.14f, 0.08f, 0.06f }, TORII_DARK);

    // Base stones (kamebara)
    drawCylinder({ -1.2f, 0, 0 }, NO_ROT, { 0.22f, 0.10f, 0.22f }, STONE_COL);
    drawCylinder({  1.2f, 0, 0 }, NO_ROT, { 0.22f, 0.10f, 0.22f }, STONE_COL);

    glPopMatrix();
}

// ─── Moss Ground Patch ─────────────────────────────────────────────────
// Mossy ground carpet with small mounds and rocks — bamboo grove floor
void drawMossGround(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const Color MOSS_DARK  = { 0.12f, 0.32f, 0.08f };
    const Color MOSS_MID   = { 0.18f, 0.42f, 0.12f };
    const Color MOSS_LIGHT = { 0.22f, 0.50f, 0.15f };
    const Color ROCK_MOSS  = { 0.38f, 0.36f, 0.33f };

    // Flat moss carpet
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 2.0f, 0.04f, 2.0f }, MOSS_DARK);

    // Scattered mossy mounds
    for (int i = 0; i < 8; i++) {
        float mx = sinf(i * 2.4f) * 1.4f;
        float mz = cosf(i * 3.1f) * 1.4f;
        float ms = 0.3f + 0.2f * sinf(i * 1.7f);
        Color mc = (i % 3 == 0) ? MOSS_LIGHT : (i % 3 == 1) ? MOSS_MID : MOSS_DARK;
        drawSphere({ mx, 0.02f, mz }, NO_ROT, { ms, 0.08f, ms }, mc);
    }

    // Small rocks nestled in moss
    drawSphere({  0.5f, 0,  0.3f }, NO_ROT, { 0.15f, 0.08f, 0.12f }, ROCK_MOSS);
    drawSphere({ -0.8f, 0, -0.4f }, NO_ROT, { 0.12f, 0.06f, 0.10f }, ROCK_MOSS);
    drawSphere({  0.2f, 0, -0.7f }, NO_ROT, { 0.10f, 0.05f, 0.08f }, ROCK_MOSS);

    glPopMatrix();
}

// ─── Falling Cherry Blossom Petals ─────────────────────────────────────
// Animated pink petals drifting and tumbling through the air
void drawFallingPetals(Vec3 pos, float radius, int count)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const Color PETAL_PINK  = { 0.95f, 0.75f, 0.80f };
    const Color PETAL_WHITE = { 1.0f, 0.92f, 0.95f };

    for (int i = 0; i < count; i++) {
        float seed  = (float)i * 1.618f;
        float phase = seed * 3.14159f;

        // Slow falling with lateral drift
        float fallSpeed = 0.3f + 0.2f * sinf(seed * 2.3f);
        float t = fmodf(animTime * fallSpeed + phase, 8.0f);
        float progress = t / 8.0f;

        float px = radius * 0.5f * sinf(phase + animTime * 0.2f)
                 + radius * 0.3f * cosf(seed * 1.3f);
        float py = -progress * 6.0f;
        float pz = radius * 0.5f * cosf(phase * 1.5f + animTime * 0.15f)
                 + radius * 0.2f * sinf(seed * 0.7f);

        // Tumble rotation
        float spin = animTime * (60.0f + seed * 20.0f);
        float tilt = sinf(animTime * 1.5f + phase) * 30.0f;

        Color col = (i % 2 == 0) ? PETAL_PINK : PETAL_WHITE;

        glPushMatrix();
        glTranslatef(px, py, pz);
        glRotatef(spin, 0, 1, 0);
        glRotatef(tilt, 1, 0, 0);
        drawCube({ 0, 0, 0 }, NO_ROT, { 0.04f, 0.005f, 0.03f }, col);
        glPopMatrix();
    }

    glDisable(GL_BLEND);
    glPopMatrix();
}

// ─── Mountain Range (Mt. Fuji-style background) ───────────────────────
// Snow-capped volcanic peak with green foothills — placed far behind the scene
void drawMountainRange(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Colors
    const Color MTN_BASE   = { 0.22f, 0.28f, 0.38f };  // blue-gray rock
    const Color MTN_MID    = { 0.28f, 0.35f, 0.44f };  // lighter mid slope
    const Color MTN_SNOW   = { 0.92f, 0.94f, 0.96f };  // white snow cap
    const Color HILL_GREEN = { 0.18f, 0.38f, 0.14f };  // dark forest green
    const Color HILL_LIGHT = { 0.24f, 0.45f, 0.18f };  // lighter foothill green

    // ── Main peak (Mt. Fuji style — tall symmetrical cone) ──
    // Large base cone — mountain body
    drawCone({ 0, 0, 0 }, NO_ROT, { 28.0f, 32.0f, 28.0f }, MTN_BASE);
    // Mid-slope layer for colour transition
    drawCone({ 0, 8.0f, 0 }, NO_ROT, { 20.0f, 24.0f, 20.0f }, MTN_MID);
    // Snow cap — upper portion
    drawCone({ 0, 20.0f, 0 }, NO_ROT, { 10.0f, 14.0f, 10.0f }, MTN_SNOW);
    // Snow tip — bright white peak
    drawCone({ 0, 28.0f, 0 }, NO_ROT, { 4.0f, 6.0f, 4.0f }, WHITE);

    // ── Secondary peak (smaller companion mountain to the left) ──
    drawCone({ -35.0f, 0, 5.0f }, NO_ROT, { 18.0f, 20.0f, 18.0f }, MTN_BASE);
    drawCone({ -35.0f, 10.0f, 5.0f }, NO_ROT, { 11.0f, 12.0f, 11.0f }, MTN_MID);
    drawCone({ -35.0f, 16.0f, 5.0f }, NO_ROT, { 5.0f, 6.0f, 5.0f }, MTN_SNOW);

    // ── Tertiary peak (smaller, to the right) ──
    drawCone({ 30.0f, 0, 8.0f }, NO_ROT, { 15.0f, 16.0f, 15.0f }, MTN_BASE);
    drawCone({ 30.0f, 8.0f, 8.0f }, NO_ROT, { 9.0f, 10.0f, 9.0f }, MTN_MID);
    drawCone({ 30.0f, 13.0f, 8.0f }, NO_ROT, { 4.0f, 5.0f, 4.0f }, MTN_SNOW);

    // ── Green foothills (overlapping spheres in front of mountains) ──
    // Front row — closest to viewer
    drawSphere({ -20.0f, 0, 18.0f }, NO_ROT, { 14.0f, 4.5f, 10.0f }, HILL_GREEN);
    drawSphere({   0.0f, 0, 20.0f }, NO_ROT, { 16.0f, 5.0f, 12.0f }, HILL_LIGHT);
    drawSphere({  22.0f, 0, 16.0f }, NO_ROT, { 13.0f, 4.0f,  9.0f }, HILL_GREEN);
    drawSphere({ -40.0f, 0, 15.0f }, NO_ROT, { 12.0f, 3.5f,  8.0f }, HILL_LIGHT);
    drawSphere({  42.0f, 0, 18.0f }, NO_ROT, { 11.0f, 3.8f,  9.0f }, HILL_GREEN);

    // Back row — taller, behind the front row
    drawSphere({ -10.0f, 0, 8.0f }, NO_ROT, { 18.0f, 6.0f, 12.0f }, HILL_GREEN);
    drawSphere({  15.0f, 0, 6.0f }, NO_ROT, { 15.0f, 5.5f, 10.0f }, HILL_LIGHT);
    drawSphere({ -35.0f, 0, 10.0f }, NO_ROT, { 14.0f, 5.0f, 10.0f }, HILL_GREEN);
    drawSphere({  38.0f, 0, 8.0f }, NO_ROT, { 13.0f, 4.5f,  9.0f }, HILL_LIGHT);

    glPopMatrix();
}
