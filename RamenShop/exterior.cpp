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

    // Floor: gray ceramic tiles (skip in shadow pass so floor doesn't cast a bulk slab)
    if (!outdoorShadowPass) {
        setMaterialGloss(0.75f, 0.73f, 0.66f, 55.0f);      // glazed tiles: lamps leave shiny highlights
        drawTexturedBox({ 0, 0, 0 }, NO_ROT, { 10, 0.1f, 8 },
                        getTexID(TEX_TILE_FLOOR), WHITE, 2.0f);
        resetMaterialGloss();
    }

    // Ground floor walls (open front) — with window openings

    // Plaster walls are matte: almost no specular, very broad
    setMaterialGloss(0.04f, 0.04f, 0.04f, 8.0f);

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
        // Lower dark wood wainscot: satin varnish
        setMaterialGloss(0.30f, 0.29f, 0.26f, 50.0f);
        drawTexturedBox({ sx * 4.9f, 0, 0 }, NO_ROT, { 0.2f, 1.2f, 8 },
                        getTexID(TEX_DARK_WOOD), WHITE, 1.0f);
        setMaterialGloss(0.04f, 0.04f, 0.04f, 8.0f);

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

    resetMaterialGloss();

    // Front: structural wood posts
    drawCuboid({ -4.9f, 0, 3.9f }, NO_ROT, { 0.25f, GH, 0.25f }, DARK_WOOD);
    drawCuboid({  4.9f, 0, 3.9f }, NO_ROT, { 0.25f, GH, 0.25f }, DARK_WOOD);
    drawCuboid({ -1.8f, 0, 3.9f }, NO_ROT, { 0.15f, GH, 0.15f }, DARK_WOOD);
    drawCuboid({  1.8f, 0, 3.9f }, NO_ROT, { 0.15f, GH, 0.15f }, DARK_WOOD);
    drawCuboid({ 0, GH - 0.15f, 3.95f }, NO_ROT, { 10.2f, 0.18f, 0.14f }, DARK_WOOD);

    // Second floor walls — light cream plaster with dark wood frame
    {   // upper front wall with two window openings (x = +-[2.2, 4.2], y = [3.65, 4.95])
        GLuint wt = getTexID(TEX_WALL);
        const float wy0 = 3.65f, wy1 = 4.95f;
        drawTexturedBox({  0.0f, GH,  3.9f }, NO_ROT, { 10.0f, wy0 - GH,  0.2f }, wt, WHITE, 2.0f);   // below the windows
        drawTexturedBox({  0.0f, wy1, 3.9f }, NO_ROT, { 10.0f, TH - wy1, 0.2f }, wt, WHITE, 2.0f);   // above the windows
        drawTexturedBox({  0.0f, wy0, 3.9f }, NO_ROT, { 4.4f,  wy1 - wy0, 0.2f }, wt, WHITE, 2.0f);  // between them
        drawTexturedBox({ -4.6f, wy0, 3.9f }, NO_ROT, { 0.8f,  wy1 - wy0, 0.2f }, wt, WHITE, 2.0f);  // left end
        drawTexturedBox({  4.6f, wy0, 3.9f }, NO_ROT, { 0.8f,  wy1 - wy0, 0.2f }, wt, WHITE, 2.0f);  // right end
    }
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

    // Eave overhang slab (underside of roof eave; skip in shadow pass so gable roof wedge shapes the shadow)
    if (!outdoorShadowPass) {
        drawCuboid({ 0, TH, 0 }, NO_ROT, { 11.4f, 0.12f, 9.4f }, DARK_GRAY);
    }

    // Traditional Japanese gable roof (ridge runs E–W, slopes N and S)
    // Textured with corrugated hon-kawara tile pattern (dark brown ridges)
    drawTexturedWedge({ 0, TH + 0.12f, 0 }, NO_ROT, { 11.4f, 2.2f, 9.4f },
                      getTexID(TEX_ROOF_TILE), WHITE, 1.2f);

    // Ridge cap beam along the peak
    drawCuboid({ 0, TH + 2.24f, 0 }, NO_ROT, { 11.6f, 0.16f, 0.24f }, DARK_WOOD);

    // Eave fascia boards on all four sides
    drawCuboid({ 0,     TH + 0.02f,  4.7f }, NO_ROT, { 11.6f, 0.22f, 0.14f }, DARK_WOOD);
    drawCuboid({ 0,     TH + 0.02f, -4.7f }, NO_ROT, { 11.6f, 0.22f, 0.14f }, DARK_WOOD);
    drawCuboid({ -5.7f, TH + 0.02f,  0    }, NO_ROT, { 0.14f, 0.22f, 9.6f  }, DARK_WOOD);
    drawCuboid({  5.7f, TH + 0.02f,  0    }, NO_ROT, { 0.14f, 0.22f, 9.6f  }, DARK_WOOD);

    // Tiled lower awning over open front
    drawTexturedBox({ 0, 3.22f, 4.7f }, { 12, 0, 0 }, { 10.6f, 0.06f, 1.4f },
                    getTexID(TEX_ROOF_TILE), WHITE, 1.2f);
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

    // Road bed (runs out to the edge of the world on both sides)
    drawPlane({ 0, 0.015f, 0 }, NO_ROT, { 240, 1, 8 }, STONE_BASE);

    // Cobblestone pattern — rows of rectangular stones with grout lines
    float roadHalfW = 120.0f, roadHalfD = 4.0f;
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
    drawCuboid({ 0, 0.04f,  roadHalfD }, NO_ROT, { 240, 0.08f, 0.18f }, CURB);
    drawCuboid({ 0, 0.04f, -roadHalfD }, NO_ROT, { 240, 0.08f, 0.18f }, CURB);

    // Center line — inlaid darker stone strip (subtle, not painted)
    drawPlane({ 0, 0.026f, 0 }, NO_ROT, { 240, 1, 0.12f }, STONE_DARK);

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
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 240, 0.10f, 3 }, { 0.55f, 0.52f, 0.48f });

    // Flagstone tiles on top
    float walkHalf = 120.0f;
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
    drawCuboid({ 0, 0.05f, 1.45f }, NO_ROT, { 240, 0.12f, 0.10f }, { 0.50f, 0.48f, 0.44f });
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
    if (!isDayTime && !drawingShadow) {
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

// ─── Distance detail (LOD) ───────────────────────────────────────────────────
// Trees and bushes far from the camera use a lighter version of the same design:
// same main branches, one branch level fewer, fewer but slightly larger flowers /
// leaves.  updateOutdoorView() is called once per frame (main.cpp).
static float g_eyeX = 0.0f, g_eyeY = 0.0f, g_eyeZ = 0.0f;
static float g_objDist2 = 0.0f;                  // distance^2 of the object whose bushes are being drawn

static float g_fr[6][4];                         // view frustum planes (world space)
static bool  g_frOk = false;

void updateOutdoorView()
{
    float P[16], M[16], C[16];
    glGetFloatv(GL_PROJECTION_MATRIX, P);
    glGetFloatv(GL_MODELVIEW_MATRIX, M);
    g_eyeX = -(M[0] * M[12] + M[1] * M[13] + M[2] * M[14]);
    g_eyeY = -(M[4] * M[12] + M[5] * M[13] + M[6] * M[14]);
    g_eyeZ = -(M[8] * M[12] + M[9] * M[13] + M[10] * M[14]);
    for (int col = 0; col < 4; col++)                 // clip = P * M (column-major)
        for (int row = 0; row < 4; row++) {
            float v = 0.0f;
            for (int k = 0; k < 4; k++) v += P[k * 4 + row] * M[col * 4 + k];
            C[col * 4 + row] = v;
        }
    for (int i = 0; i < 6; i++) {
        int axis = i / 2;
        float sgn = (i % 2 == 0) ? 1.0f : -1.0f;
        for (int j = 0; j < 4; j++) g_fr[i][j] = C[j * 4 + 3] + sgn * C[j * 4 + axis];
        float l = sqrtf(g_fr[i][0] * g_fr[i][0] + g_fr[i][1] * g_fr[i][1] + g_fr[i][2] * g_fr[i][2]);
        if (l > 1e-6f) for (int j = 0; j < 4; j++) g_fr[i][j] /= l;
    }
    g_frOk = true;
}

static float outdoorDist2(float x, float z)
{
    float dx = x - g_eyeX, dz = z - g_eyeZ;
    return dx * dx + dz * dz;
}

// View culling: true when a world-space sphere is completely outside the view frustum,
// so the object is simply not drawn.  Changes nothing visually.  In the shadow pass the
// radius grows by the longest shadow so shadows of off-screen objects still appear.
// True when the camera is inside the shop (ground or upper floor)
bool cameraInsideShop()
{
    return fabsf(g_eyeX) < 4.8f && g_eyeZ > -3.8f && g_eyeZ < 4.6f && g_eyeY > 0.1f && g_eyeY < 5.4f;
}

static bool outdoorCulled(float x, float y, float z, float r)
{
    if (!g_frOk) return false;
    if (outdoorShadowPass) r += 14.0f;
    for (int i = 0; i < 6; i++)
        if (g_fr[i][0] * x + g_fr[i][1] * y + g_fr[i][2] * z + g_fr[i][3] < -r) return true;
    return false;
}

static bool cullObj(Vec3 pos, Vec3 scale, float hr)       // hr = object radius at scale 1
{
    float sc = fmaxf(scale.x, fmaxf(scale.y, scale.z));
    return outdoorCulled(pos.x, pos.y + hr * scale.y * 0.5f, pos.z, hr * sc);
}

static const float TREE_LOD_DIST  = 18.0f;       // trees farther than this use the lighter build
static const float BAMBOO_LOD_DIST = 24.0f;
static const float BUSH_LOD_DIST  = 18.0f;

// ─── Ground shadows and dark-grass decals ───────────────────────────────────
// Soft elliptical decals on the lawn (alpha fades to nothing at the rim).  The
// sun is the directional light in lighting.cpp (from +x, +y, -z), so shadows fall
// toward -x, +z.  Shadow directions are given in world space and converted into
// the object's local frame using its yaw.
// Solid tree silhouettes for the shadow pass: trunk + a few ellipsoids for the crown
static void shadowTrunk(float h, float r0, float r1)
{
    glPushMatrix();
    glRotatef(-90.0f, 1, 0, 0);
    gluCylinder(quad, r0, r1, h, 8, 1);
    glPopMatrix();
}

static void shadowEllipsoid(float cx, float cy, float cz, float rx, float ry, float rz)
{
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glScalef(rx, ry, rz);
    gluSphere(quad, 1.0, 12, 8);
    glPopMatrix();
}

bool outdoorShadowPass = false;
static const float SHADOW_DETAIL_Z = -6.0f;       // trees in front of this z (near the shop and road) cast detailed shadows; the forest behind uses cheap crown shapes
bool outdoorShadowDetail = true;                 // true: full-detail casters (sun/moon); false: cheap crown shapes (lamps)                  // true while scene.cpp flattens the outdoor scene into the shadow mask
static float g_shadowRot = 0.0f;                 // yaw of the object being drawn (set by callers)

static void groundBlob(float cx, float cz, float major, float minor, float ang,
                       Color col, float alpha, float y)
{
    if (drawingShadow || alpha <= 0.0f) return;
    setLighting(false);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    float ca = cosf(ang), sa = sinf(ang);
    const int N = 28;
    glBegin(GL_TRIANGLE_FAN);                    // solid core
    glColor4f(col.r, col.g, col.b, alpha);
    glVertex3f(cx, y, cz);
    for (int i = 0; i <= N; i++) {
        float t = i * 6.2832f / N, u = cosf(t) * major * 0.55f, v = sinf(t) * minor * 0.55f;
        glColor4f(col.r, col.g, col.b, alpha * 0.85f);
        glVertex3f(cx + u * ca - v * sa, y, cz + u * sa + v * ca);
    }
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);                  // soft falloff rim
    for (int i = 0; i <= N; i++) {
        float t = i * 6.2832f / N, cu = cosf(t), cv = sinf(t);
        float u0 = cu * major * 0.55f, v0 = cv * minor * 0.55f, u1 = cu * major, v1 = cv * minor;
        glColor4f(col.r, col.g, col.b, alpha * 0.85f);
        glVertex3f(cx + u0 * ca - v0 * sa, y, cz + u0 * sa + v0 * ca);
        glColor4f(col.r, col.g, col.b, 0.0f);
        glVertex3f(cx + u1 * ca - v1 * sa, y, cz + u1 * sa + v1 * ca);
    }
    glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    setLighting(true);
}

// ════════════════════════════════════════════════════════════════════════════
//  REALISTIC GRASS / BUSHES  (display lists, built once)
//  - domeBush : a round mound covered in thousands of fine blades (clipped-bush look)
//  - tuft     : arching ornamental-grass clump with tapered curved blades
// ════════════════════════════════════════════════════════════════════════════
static float fhash(int i, int k)
{
    float v = sinf(i * 127.1f + k * 311.7f) * 43758.5453f;
    return v - floorf(v);
}

static void normalize3(float& x, float& y, float& z)
{
    float l = sqrtf(x * x + y * y + z * z);
    if (l < 1e-6f) { x = 0; y = 1; z = 0; return; }
    x /= l; y /= l; z /= l;
}

static GLuint g_dome = 0, g_domeLod = 0;
static void buildDomeImpl(GLuint& list, int N, float widthMul, float lenMul)
{
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glDisable(GL_CULL_FACE);
    setColor({ 0.17f, 0.33f, 0.10f });
    glPushMatrix();
    glScalef(1.0f, 1.0f, 1.0f);
    gluSphere(quad, 0.90f, 18, 10);                    // dark core hides gaps between blades
    glPopMatrix();
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < N; i++) {
        float y = (i + 0.5f) / N;                       // upper hemisphere, even area
        float r = sqrtf(1.0f - y * y), a = i * 2.39996f;
        float nx = r * cosf(a), ny = y, nz = r * sinf(a);
        float h1 = fhash(i, 1), h2 = fhash(i, 2), h3 = fhash(i, 3), h4 = fhash(i, 4);
        float L = (0.15f + 0.14f * h1) * lenMul;
        float b = h2 * 6.2832f;
        float dx = nx + cosf(b) * 0.55f * (h3 - 0.3f), dy = ny + 0.30f, dz = nz + sinf(b) * 0.55f * (h3 - 0.3f);
        normalize3(dx, dy, dz);
        float sx = dy * nz - dz * ny, sy = dz * nx - dx * nz, sz = dx * ny - dy * nx;   // dir x n
        normalize3(sx, sy, sz);
        float w = (0.016f + 0.012f * h4) * widthMul;
        float px = nx * 0.93f, py = ny * 0.93f, pz = nz * 0.93f;
        float tint = 0.80f + 0.40f * h3;
        glNormal3f(nx, ny, nz);
        glColor3f(0.20f * tint, 0.40f * tint, 0.11f * tint);
        glVertex3f(px - sx * w, py - sy * w, pz - sz * w);
        glVertex3f(px + sx * w, py + sy * w, pz + sz * w);
        float tr = 0.40f + 0.12f * h4, tg = 0.64f + 0.12f * h1, tb = 0.20f;
        glColor3f(tr * tint, tg * tint, tb * tint);
        glVertex3f(px + dx * L, py + dy * L, pz + dz * L);
    }
    glEnd();
    glEndList();
}

static void buildDome()    { buildDomeImpl(g_dome, 260, 1.7f, 1.15f); }   // a little simpler: fewer, slightly broader blades
static void buildDomeLod() { buildDomeImpl(g_domeLod, 90, 2.6f, 1.2f); }   // far bushes: fewer, broader blades

// Cheap upper hemisphere used only as the bush silhouette in the shadow pass
static GLuint g_domeShadow = 0;
static void buildDomeShadow()
{
    g_domeShadow = glGenLists(1);
    glNewList(g_domeShadow, GL_COMPILE);
    const int R = 5, S = 12;
    for (int i = 0; i < R; i++) {
        float y0 = sinf(i * 1.5708f / R), y1 = sinf((i + 1) * 1.5708f / R);
        float r0 = cosf(i * 1.5708f / R), r1 = cosf((i + 1) * 1.5708f / R);
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= S; j++) {
            float a = j * 6.2832f / S;
            glVertex3f(cosf(a) * r0, y0, sinf(a) * r0);
            glVertex3f(cosf(a) * r1, y1, sinf(a) * r1);
        }
        glEnd();
    }
    glEndList();
}

// Fuzzy round bush (hemisphere radius 1 scaled to rx x ry x rx), gently swaying
static void domeBush(float x, float y, float z, float rx, float ry, float phase)
{
    if (outdoorShadowPass) {                          // silhouette only
        if (!g_domeShadow) buildDomeShadow();
        glPushMatrix();
        glTranslatef(x, y, z);
        glScalef(rx, ry, rx);
        glCallList(g_domeShadow);
        glPopMatrix();
        return;
    }
    bool farBush = g_objDist2 > BUSH_LOD_DIST * BUSH_LOD_DIST;
    if (farBush) { if (!g_domeLod) buildDomeLod(); } else if (!g_dome) buildDome();
    if (y < 0.3f) {                                   // bush standing on the lawn (not in a pot)
        if (isDayTime) groundBlob(x, z, rx * 1.9f, rx * 1.9f, 0.0f, { 0.10f, 0.22f, 0.06f }, 0.30f, 0.010f);   // slightly darker grass around the base (day only: it is drawn unlit, so at night it would glow)
    }
    float sw = 1.6f * sinf(animTime * 1.3f + phase) + 0.8f * sinf(animTime * 2.7f + phase * 1.9f);
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(sw, 0, 0, 1);
    glRotatef(sw * 0.6f, 1, 0, 0);
    glScalef(rx, ry, rx);
    glCallList(farBush ? g_domeLod : g_dome);
    glPopMatrix();
}

// Arching ornamental-grass clump (3 variants)
static GLuint g_tuft[3] = { 0, 0, 0 };
static void buildTuft(int v)
{
    g_tuft[v] = glGenLists(1);
    glNewList(g_tuft[v], GL_COMPILE);
    glDisable(GL_CULL_FACE);
    glBegin(GL_TRIANGLES);
    const int N = 30, SEG = 4;
    for (int i = 0; i < N; i++) {
        float h1 = fhash(i + v * 53, 11), h2 = fhash(i + v * 53, 12), h3 = fhash(i + v * 53, 13), h4 = fhash(i + v * 53, 14);
        float a = i * 2.39996f + v;
        float rr = 0.10f + 0.55f * sqrtf((i + 0.5f) / N) * (0.7f + 0.3f * h1);
        float bx = cosf(a) * rr * 0.55f, bz = sinf(a) * rr * 0.55f;
        float ox = cosf(a + (h2 - 0.5f) * 0.8f), oz = sinf(a + (h2 - 0.5f) * 0.8f);    // arch direction
        float H = 0.55f + 0.55f * h3 * (1.2f - rr);
        float k = 0.30f + 0.45f * h4;
        float tx = -oz, tz = ox;                                                       // blade width direction
        float tint = 0.75f + 0.50f * h2;
        float px[SEG + 1], py[SEG + 1], pz[SEG + 1], pw[SEG + 1];
        for (int sgi = 0; sgi <= SEG; sgi++) {
            float t = sgi / (float)SEG;
            px[sgi] = bx + ox * k * t * t * H;
            py[sgi] = H * t * (1.0f - 0.30f * t);
            pz[sgi] = bz + oz * k * t * t * H;
            pw[sgi] = 0.030f * powf(1.0f - t, 0.8f) + 0.0015f;
        }
        float nx = ox * 0.6f, ny = 0.8f, nz = oz * 0.6f;
        normalize3(nx, ny, nz);
        glNormal3f(nx, ny, nz);
        for (int sgi = 0; sgi < SEG; sgi++) {
            float t0 = sgi / (float)SEG, t1 = (sgi + 1) / (float)SEG;
            float c0r = (0.10f + 0.34f * t0) * tint, c0g = (0.28f + 0.34f * t0) * tint, c0b = (0.06f + 0.10f * t0) * tint;
            float c1r = (0.10f + 0.34f * t1) * tint, c1g = (0.28f + 0.34f * t1) * tint, c1b = (0.06f + 0.10f * t1) * tint;
            glColor3f(c0r, c0g, c0b);
            glVertex3f(px[sgi] - tx * pw[sgi], py[sgi], pz[sgi] - tz * pw[sgi]);
            glVertex3f(px[sgi] + tx * pw[sgi], py[sgi], pz[sgi] + tz * pw[sgi]);
            glColor3f(c1r, c1g, c1b);
            glVertex3f(px[sgi + 1] + tx * pw[sgi + 1], py[sgi + 1], pz[sgi + 1] + tz * pw[sgi + 1]);
            glColor3f(c0r, c0g, c0b);
            glVertex3f(px[sgi] - tx * pw[sgi], py[sgi], pz[sgi] - tz * pw[sgi]);
            glColor3f(c1r, c1g, c1b);
            glVertex3f(px[sgi + 1] + tx * pw[sgi + 1], py[sgi + 1], pz[sgi + 1] + tz * pw[sgi + 1]);
            glVertex3f(px[sgi + 1] - tx * pw[sgi + 1], py[sgi + 1], pz[sgi + 1] - tz * pw[sgi + 1]);
        }
    }
    glEnd();
    glEndList();
}

void drawPlant(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    g_objDist2 = outdoorDist2(pos.x, pos.z);
    drawCylinderCustom({ 0, 0, 0 }, NO_ROT, ONE, CLAY_POT, 0.2f, 0.28f, 0.4f);
    drawCylinder({ 0, 0.40f, 0 }, NO_ROT, { 0.5f, 0.02f, 0.5f }, SOIL);
    // fuzzy mound of fine blades with a tuft of tall grass on top
    domeBush(0.0f, 0.38f, 0.0f, 0.34f, 0.46f, pos.x * 0.7f + pos.z);
    if (!g_tuft[0]) buildTuft(0);
    glPushMatrix();
    glTranslatef(0.0f, 0.62f, 0.0f);
    glScalef(0.55f, 0.55f, 0.55f);
    glCallList(g_tuft[0]);
    glPopMatrix();
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

static float clamp01f(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// ════════════════════════════════════════════════════════════════════════════
//  BLOSSOM / AUTUMN TREES
//  - branching trunk -> main branches -> sub-branches -> twigs, built once into
//    display lists (one list per main-branch group so each group can sway)
//  - dense puffy clusters of small blossoms at the twig tips
//  - a gusting breeze rotates every group about its base, out of step
//  - falling petals / leaves (time driven, no state) and a carpet of fallen petals
// ════════════════════════════════════════════════════════════════════════════
static float blossomHash(int i, int k)           // deterministic pseudo-random 0..1
{
    float v = sinf(i * 127.1f + k * 311.7f) * 43758.5453f;
    return v - floorf(v);
}

// Gentle gusting breeze (about -1.7 .. 1.7); `phase` keeps trees/branches out of step
static float breeze(float phase)
{
    float t = animTime;
    return sinf(t * 1.1f + phase) + 0.45f * sinf(t * 2.3f + phase * 1.7f) + 0.25f * sinf(t * 0.37f + phase * 0.5f);
}

static Vec3 vAdd(Vec3 a, Vec3 b)  { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vec3 vMul(Vec3 a, float k) { return { a.x * k, a.y * k, a.z * k }; }
static Vec3 vCross(Vec3 a, Vec3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
static Vec3 vNorm(Vec3 a)
{
    float l = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    if (l < 1e-6f) return { 0.0f, 1.0f, 0.0f };
    return { a.x / l, a.y / l, a.z / l };
}

static int g_rndSeed = 0, g_rndCtr = 0;
static float rnd() { return blossomHash(g_rndSeed, g_rndCtr++); }

// Tapered tube from a to b (+ a ball at b so bends stay smooth)
static bool g_lod = false;                       // building the lighter distance version of a tree
static void branchTube(Vec3 a, Vec3 b, float r0, float r1, Color c)
{
    float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 1e-4f) return;
    setColor(c);
    glPushMatrix();
    glTranslatef(a.x, a.y, a.z);
    float ax = -dy, ay = dx;                         // z-axis x direction
    if (fabsf(ax) + fabsf(ay) > 1e-5f) glRotatef(acosf(dz / len) * 180.0f / PI, ax, ay, 0.0f);
    else if (dz < 0.0f) glRotatef(180.0f, 1, 0, 0);
    gluCylinder(quad, r0, r1, len, g_lod ? 5 : 7, 1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(b.x, b.y, b.z);
    gluSphere(quad, r1, g_lod ? 4 : 6, g_lod ? 3 : 4);
    glPopMatrix();
}

// One five-petal cherry blossom in the XY plane facing +Z: notched petals that are
// deep pink at the base and pale at the tips, cupped slightly toward the viewer,
// with a yellow centre.
static void drawFlowerShape(int style)
{
    static const float PV[6][3] = {                 // petal outline (x, y, z), length ~0.93
        { 0.00f, 0.06f, 0.00f }, { -0.34f, 0.42f, 0.08f }, { -0.23f, 0.93f, 0.20f },
        { 0.00f, 0.80f, 0.13f }, {  0.23f, 0.93f, 0.20f }, {  0.34f, 0.42f, 0.08f } };
    static const Color TIP[4]  = { { 1.00f, 0.88f, 0.93f }, { 1.00f, 0.80f, 0.88f },
                                   { 0.98f, 0.72f, 0.82f }, { 1.00f, 0.95f, 0.97f } };
    static const Color BASE[4] = { { 0.95f, 0.50f, 0.66f }, { 0.93f, 0.42f, 0.60f },
                                   { 0.90f, 0.36f, 0.55f }, { 0.98f, 0.62f, 0.74f } };
    const Color tip = TIP[style & 3], base = BASE[style & 3];
    const float vt[6] = { 0.0f, 0.55f, 1.0f, 0.8f, 1.0f, 0.55f };   // 0 = base colour, 1 = tip colour

    glBegin(GL_TRIANGLES);
    for (int p = 0; p < 5; p++) {
        float ang = p * 72.0f * PI / 180.0f, ca = cosf(ang), sa = sinf(ang);
        float vx[6], vy[6], vz[6];
        for (int i = 0; i < 6; i++) {
            vx[i] = PV[i][0] * ca - PV[i][1] * sa;
            vy[i] = PV[i][0] * sa + PV[i][1] * ca;
            vz[i] = PV[i][2];
        }
        float nx = (0.0f * ca - (-0.28f) * sa), ny = (0.0f * sa + (-0.28f) * ca), nz = 1.0f;   // cupped normal
        float nl = sqrtf(nx * nx + ny * ny + nz * nz);
        glNormal3f(nx / nl, ny / nl, nz / nl);
        static const int T[4][3] = { { 0, 1, 2 }, { 0, 2, 3 }, { 0, 3, 4 }, { 0, 4, 5 } };
        for (int t = 0; t < 4; t++)
            for (int k = 0; k < 3; k++) {
                int i = T[t][k];
                glColor3f(base.r + (tip.r - base.r) * vt[i], base.g + (tip.g - base.g) * vt[i],
                          base.b + (tip.b - base.b) * vt[i]);
                glVertex3f(vx[i], vy[i], vz[i]);
            }
    }
    glEnd();

    // yellow centre
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 0, 1);
    glColor3f(1.00f, 0.86f, 0.42f);
    glVertex3f(0, 0, 0.16f);
    glColor3f(0.96f, 0.62f, 0.40f);
    for (int i = 0; i <= 7; i++) {
        float a = i * 2.0f * PI / 7.0f;
        glVertex3f(cosf(a) * 0.11f, sinf(a) * 0.11f, 0.12f);
    }
    glEnd();
}

static void flowerAt(Vec3 p, Vec3 n, float r, float roll, int style)
{
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    float ax = -n.y, ay = n.x;                       // z-axis x n
    if (fabsf(ax) + fabsf(ay) > 1e-5f) glRotatef(acosf(n.z) * 180.0f / PI, ax, ay, 0.0f);
    else if (n.z < 0.0f) glRotatef(180.0f, 1, 0, 0);
    glRotatef(roll, 0, 0, 1);
    glScalef(r, r, r);
    drawFlowerShape(style);
    glPopMatrix();
}

// A bunch of individual blossoms facing outward, with a few buds and green leaves
static void blossomCluster(Vec3 c, float size)
{
    setEmission(0.14f, 0.07f, 0.10f);                // faint pink glow so they stay visible at night
    int n = (g_lod ? 3 : 5) + (int)(rnd() * (g_lod ? 2.0f : 2.0f));
    for (int i = 0; i < n; i++) {
        float a = rnd() * 6.2832f;
        float e = rnd() * 1.7f - 0.45f;              // biased upward
        float d = size * (0.35f + 0.65f * rnd());
        Vec3 off = { cosf(a) * d, e * d * 0.7f, sinf(a) * d };
        Vec3 nrm = vNorm({ off.x, off.y * 0.6f + 0.35f, off.z });         // outward and a little up
        float r = (0.17f + 0.08f * rnd()) * (g_lod ? 1.55f : 1.0f);
        flowerAt(vAdd(c, off), nrm, r, rnd() * 360.0f, (int)(rnd() * 4.0f) % 4);
    }
    // a couple of deeper-pink buds
    int nb = 1 + (int)(rnd() * 2.0f);
    for (int i = 0; i < nb; i++) {
        float a = rnd() * 6.2832f, d = size * (0.5f + 0.5f * rnd());
        setColor({ 0.92f, 0.36f, 0.56f });
        glPushMatrix();
        glTranslatef(c.x + cosf(a) * d, c.y + 0.1f * rnd(), c.z + sinf(a) * d);
        glScalef(0.8f, 1.2f, 0.8f);
        gluSphere(quad, 0.055f, 6, 4);
        glPopMatrix();
    }
    clearEmission();
    // small green leaves peeking out between the flowers
    int nl = 1 + (int)(rnd() * 2.0f);
    for (int i = 0; i < nl; i++) {
        float a = rnd() * 6.2832f, d = size * (0.55f + 0.5f * rnd());
        setColor({ 0.50f + 0.12f * rnd(), 0.74f + 0.08f * rnd(), 0.30f });
        glPushMatrix();
        glTranslatef(c.x + cosf(a) * d, c.y + 0.12f * rnd(), c.z + sinf(a) * d);
        glRotatef(-a * 180.0f / PI, 0, 1, 0);
        glRotatef(-25.0f - 25.0f * rnd(), 0, 0, 1);
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0.3f);
        glVertex3f(0, 0, 0); glVertex3f(0.09f, 0.03f, 0.06f); glVertex3f(0.22f, 0.0f, 0.0f); glVertex3f(0.09f, 0.03f, -0.06f);
        glEnd();
        glPopMatrix();
    }
}

// One maple leaf (five pointed lobes, tips curled up a little) in the XY plane, facing +Z
static void drawMapleLeafShape(float s)
{
    static const float R[5] = { 1.00f, 0.92f, 0.68f, 0.68f, 0.92f };     // tip radius per lobe
    float px[10], py[10], pz[10];
    for (int k = 0; k < 5; k++) {
        float a = (90.0f + 72.0f * k) * PI / 180.0f;
        px[2 * k] = cosf(a) * R[k];  py[2 * k] = sinf(a) * R[k];  pz[2 * k] = 0.14f;
        float b = a + 36.0f * PI / 180.0f;
        px[2 * k + 1] = cosf(b) * 0.42f;  py[2 * k + 1] = sinf(b) * 0.42f;  pz[2 * k + 1] = 0.0f;
    }
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 0.0f, 1.0f);
    for (int i = 0; i < 10; i++) {
        int j = (i + 1) % 10;
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(px[i] * s, py[i] * s, pz[i] * s);
        glVertex3f(px[j] * s, py[j] * s, pz[j] * s);
    }
    glEnd();
}

// Rotate so that +Z points along n (n must be normalised)
static void alignZ(Vec3 n)
{
    float ax = -n.y, ay = n.x;
    if (fabsf(ax) + fabsf(ay) > 1e-5f) glRotatef(acosf(n.z) * 180.0f / PI, ax, ay, 0.0f);
    else if (n.z < 0.0f) glRotatef(180.0f, 1, 0, 0);
}

// A loose bunch of small red / orange maple leaves at a twig tip
static void mapleLeafCluster(Vec3 c, float size)
{
    static const Color COL[6] = {
        { 0.88f, 0.20f, 0.10f }, { 0.95f, 0.34f, 0.12f }, { 0.93f, 0.50f, 0.14f },
        { 0.72f, 0.11f, 0.08f }, { 0.90f, 0.26f, 0.11f }, { 0.97f, 0.60f, 0.18f } };
    setEmission(0.10f, 0.03f, 0.01f);                // keeps the red readable at night
    int n = (g_lod ? 3 : 5) + (int)(rnd() * (g_lod ? 2.0f : 3.0f));
    for (int i = 0; i < n; i++) {
        float a = rnd() * 6.2832f;
        float e = rnd() * 1.7f - 0.5f;
        float d = size * (0.30f + 0.70f * rnd());
        Vec3 off = { cosf(a) * d, e * d * 0.7f, sinf(a) * d };
        Vec3 nrm = vNorm({ off.x, off.y * 0.6f + 0.45f, off.z });
        float r = (0.12f + 0.07f * rnd()) * (g_lod ? 1.7f : 1.25f);
        setColor(COL[(int)(rnd() * rnd() * 6.0f) % 6]);      // mostly the reds and oranges
        glPushMatrix();
        glTranslatef(c.x + off.x, c.y + off.y, c.z + off.z);
        alignZ(nrm);
        glRotatef(rnd() * 360.0f, 0, 0, 1);
        glScalef(r, r, r);
        drawMapleLeafShape(1.0f);
        glPopMatrix();
    }
    clearEmission();
}

// One broad green leaf (ovate, tip curled up), base at the origin, pointing along +Y,
// facing +Z; darker at the base, lighter toward the tip
static void drawLeafShape(Color c, float s)
{
    static const float LV[8][3] = {
        { 0.00f, 0.00f, 0.00f }, { -0.26f, 0.25f, 0.02f }, { -0.30f, 0.55f, 0.07f }, { -0.14f, 0.86f, 0.12f },
        { 0.00f, 1.00f, 0.14f }, {  0.14f, 0.86f, 0.12f }, {  0.30f, 0.55f, 0.07f }, {  0.26f, 0.25f, 0.02f } };
    const float dk = 0.62f;
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -0.15f, 1.0f);
    glColor3f(c.r * dk, c.g * dk, c.b * dk);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 1; i < 8; i++) {
        float k = dk + (1.0f - dk) * LV[i][1];
        glColor3f(c.r * k, c.g * k, c.b * k);
        glVertex3f(LV[i][0] * s, LV[i][1] * s, LV[i][2] * s);
    }
    glEnd();
}

// A pad of broad leaves lying roughly horizontal, giving the layered look of a broadleaf crown
static void greenLeafCluster(Vec3 c, float size, int count)
{
    static const Color COL[6] = {
        { 0.14f, 0.38f, 0.09f }, { 0.21f, 0.49f, 0.11f }, { 0.29f, 0.58f, 0.15f },
        { 0.39f, 0.67f, 0.19f }, { 0.52f, 0.76f, 0.25f }, { 0.11f, 0.31f, 0.08f } };
    int n = (g_lod ? (count * 2) / 5 : count) + (int)(rnd() * 4.0f);
    for (int i = 0; i < n; i++) {
        float a = rnd() * 6.2832f;
        float e = (rnd() - 0.5f) * 1.6f;                       // fuller vertically
        float d = size * (0.25f + 0.75f * rnd());
        Vec3 nrm = vNorm({ cosf(a) * d * 0.35f + (rnd() - 0.5f) * 0.5f, 0.85f, sinf(a) * d * 0.35f + (rnd() - 0.5f) * 0.5f });
        float sz = (0.24f + 0.12f * rnd()) * (g_lod ? 1.7f : 1.32f);   // fewer, slightly larger leaves
        glPushMatrix();
        glTranslatef(c.x + cosf(a) * d, c.y + e * d * 0.5f, c.z + sinf(a) * d);
        alignZ(nrm);
        glRotatef(rnd() * 360.0f, 0, 0, 1);
        drawLeafShape(COL[(int)(rnd() * 6.0f) % 6], sz);
        glPopMatrix();
    }
}

// One branch with its sub-branches (level 1 = main, 2 = sub, 3 = twig)
// Cluster centres recorded while a branch group is built; they become that group's
// shadow list (one small low-poly sphere per leaf/blossom cluster), so the cast
// shadow follows the real crown shape and sways with its branch group.
struct ClusterRec { float x, y, z, r; };
static ClusterRec g_rec[800];
static int  g_recN = 0;
static bool g_recOn = false;
static void recordCluster(Vec3 c, float r)
{
    if (g_recOn && g_recN < 800) g_rec[g_recN++] = { c.x, c.y, c.z, r };
}
static void emitShadowList(GLuint list)
{
    glNewList(list, GL_COMPILE);
    for (int i = 0; i < g_recN; i++) {
        glPushMatrix();
        glTranslatef(g_rec[i].x, g_rec[i].y, g_rec[i].z);
        gluSphere(quad, g_rec[i].r, 8, 4);
        glPopMatrix();
    }
    glEndList();
}

static bool g_mapleStyle = false;                // maple: finer branching, leaf clusters
static bool g_greenStyle = false;                // broadleaf: leaf pads on every branch tip
static int  g_maxLevel   = 3;                    // cherry 3 levels, maple 4

static void growBranch(Vec3 base, Vec3 dir, float len, float r, int level)
{
    static const Color BARK = { 0.40f, 0.28f, 0.20f }, BARK_DARK = { 0.30f, 0.21f, 0.15f };
    static const Color M_BARK = { 0.40f, 0.34f, 0.30f }, M_BARK_DARK = { 0.31f, 0.26f, 0.23f };
    static const Color G_BARK = { 0.36f, 0.29f, 0.22f }, G_BARK_DARK = { 0.28f, 0.22f, 0.17f };
    Color col = g_greenStyle ? ((level == 1) ? G_BARK_DARK : G_BARK)
              : g_mapleStyle ? ((level == 1) ? M_BARK_DARK : M_BARK) : ((level == 1) ? BARK_DARK : BARK);

    Vec3 p1 = vAdd(base, vMul(dir, len * 0.55f));
    Vec3 dir2 = vNorm({ dir.x + (rnd() - 0.5f) * 0.35f, dir.y + 0.22f, dir.z + (rnd() - 0.5f) * 0.35f });
    Vec3 p2 = vAdd(p1, vMul(dir2, len * 0.45f));
    branchTube(base, p1, r, r * 0.72f, col);
    branchTube(p1, p2, r * 0.72f, r * 0.48f, col);

    if (level < g_maxLevel) {
        int kids = (level == 1) ? 3 : 2;
        for (int k = 0; k < kids; k++) {
            float t = 0.45f + 0.50f * (k + rnd() * 0.5f) / kids;
            Vec3 start = vAdd(base, vMul(vAdd(p2, vMul(base, -1.0f)), t));
            Vec3 u = vNorm(vCross(dir, (fabsf(dir.y) > 0.95f) ? Vec3{ 1, 0, 0 } : Vec3{ 0, 1, 0 }));
            Vec3 w = vCross(dir, u);
            float th = 0.45f + 0.45f * rnd(), ph = rnd() * 6.2832f;
            Vec3 d = vNorm(vAdd(vAdd(vMul(dir, cosf(th)),
                                     vMul(vAdd(vMul(u, cosf(ph)), vMul(w, sinf(ph))), sinf(th))), { 0, 0.2f, 0 }));
            growBranch(start, d, len * (0.60f + 0.08f * rnd()), r * 0.55f, level + 1);
        }
        growBranch(p2, dir2, len * 0.55f, r * 0.45f, level + 1);   // the branch keeps going
    }
    if (g_greenStyle) {
        greenLeafCluster(p2, level == 1 ? 0.70f : (level == 2 ? 0.62f : 0.55f), 10);
        recordCluster(p2, (level == 1 ? 0.70f : (level == 2 ? 0.62f : 0.55f)) + 0.18f);
        if (level >= 2) { greenLeafCluster(p1, 0.48f, 6); recordCluster(p1, 0.48f + 0.18f); }   // extra pad part-way along the branch
    } else if (g_mapleStyle) {
        if (level >= g_maxLevel - 1) { mapleLeafCluster(p2, level == g_maxLevel ? 0.27f : 0.31f); recordCluster(p2, (level == g_maxLevel ? 0.27f : 0.31f) + 0.12f); }
    } else {
        blossomCluster(p2, level == 1 ? 0.40f : (level == 2 ? 0.34f : 0.29f));
        recordCluster(p2, (level == 1 ? 0.40f : (level == 2 ? 0.34f : 0.29f)) + 0.18f);
    }
}

// ---- cached geometry: trunk + 7 swaying groups, 3 variants ----
struct BlossomLists { bool built; GLuint base; Vec3 pivot[7]; };
static BlossomLists g_cherry[6] = {};                  // [0..2] full, [3..5] distance version

static void buildCherryVariant(int v)
{
    const bool lod = v >= 3;
    const int sv = lod ? v - 3 : v;
    BlossomLists& L = g_cherry[v];
    L.base = glGenLists(16);
    g_lod = lod; g_maxLevel = lod ? 2 : 3;

    // trunk + root flare
    g_rndSeed = 500 + sv * 17; g_rndCtr = 0;
    glNewList(L.base, GL_COMPILE);
    {
        static const Color BARK = { 0.36f, 0.25f, 0.17f }, BARK_D = { 0.27f, 0.19f, 0.13f };
        Vec3 a = { 0, 0, 0 }, b = { 0.05f, 0.9f, 0.03f }, c = { -0.04f, 1.55f, 0.0f }, d = { 0.0f, 2.05f, 0.0f };
        branchTube(a, b, 0.27f, 0.21f, BARK_D);
        branchTube(b, c, 0.21f, 0.16f, BARK);
        branchTube(c, d, 0.16f, 0.12f, BARK);
        for (int i = 0; i < 5; i++) {
            float ang = (i * 72.0f + rnd() * 25.0f) * PI / 180.0f;
            branchTube({ 0, 0.38f, 0 }, { cosf(ang) * 0.58f, 0.0f, sinf(ang) * 0.58f }, 0.13f, 0.035f, BARK_D);
        }
    }
    glEndList();

    for (int g = 0; g < 7; g++) {
        g_rndSeed = 900 + sv * 71 + g * 13; g_rndCtr = 0;
        float az, pitch, y0, len;
        if (g < 6) {
            az = (g * 60.0f + (rnd() - 0.5f) * 30.0f) * PI / 180.0f;
            pitch = (36.0f + 30.0f * rnd()) * PI / 180.0f;        // from vertical
            y0 = 1.30f + 0.65f * rnd();
            len = 1.25f + 0.25f * rnd();
        } else {                                                  // top leader
            az = rnd() * 6.2832f; pitch = 0.14f; y0 = 1.95f; len = 1.05f;
        }
        Vec3 dir = { sinf(pitch) * cosf(az), cosf(pitch), sinf(pitch) * sinf(az) };
        Vec3 base = { 0.0f, y0, 0.0f };
        L.pivot[g] = base;
        g_recN = 0; g_recOn = true;
        glNewList(L.base + 1 + g, GL_COMPILE);
        growBranch(base, dir, len, 0.085f, 1);
        glEndList();
        g_recOn = false;
        emitShadowList(L.base + 8 + g);
    }
    L.built = true;
    g_lod = false; g_maxLevel = 3;
}

static BlossomLists g_maple[6] = {};                   // [0..2] full, [3..5] distance version

// Slender maple: tall trunk with spiky roots, fine spreading branches (4 levels),
// red / orange leaf clusters at the twigs
static void buildMapleVariant(int v)
{
    const bool lod = v >= 3;
    const int sv = lod ? v - 3 : v;
    BlossomLists& L = g_maple[v];
    L.base = glGenLists(16);
    g_lod = lod;
    g_mapleStyle = true; g_maxLevel = lod ? 3 : 4;

    g_rndSeed = 1500 + sv * 23; g_rndCtr = 0;
    glNewList(L.base, GL_COMPILE);
    {
        static const Color BARK = { 0.40f, 0.34f, 0.30f }, BARK_D = { 0.30f, 0.25f, 0.22f };
        Vec3 a = { 0, 0, 0 }, b = { 0.10f, 1.0f, 0.05f }, c = { -0.06f, 1.9f, 0.0f }, d = { 0.0f, 2.5f, 0.0f };
        branchTube(a, b, 0.24f, 0.17f, BARK_D);
        branchTube(b, c, 0.17f, 0.13f, BARK);
        branchTube(c, d, 0.13f, 0.09f, BARK);
        for (int i = 0; i < 6; i++) {                              // long spiky roots
            float ang = (i * 60.0f + rnd() * 30.0f) * PI / 180.0f;
            branchTube({ 0, 0.38f, 0 }, { cosf(ang) * 0.90f, 0.0f, sinf(ang) * 0.90f }, 0.11f, 0.012f, BARK_D);
        }
    }
    glEndList();

    for (int g = 0; g < 7; g++) {
        g_rndSeed = 2100 + sv * 61 + g * 11; g_rndCtr = 0;
        float az, pitch, y0, len;
        if (g < 6) {
            az = (g * 60.0f + (rnd() - 0.5f) * 30.0f) * PI / 180.0f;
            pitch = (26.0f + 26.0f * rnd()) * PI / 180.0f;        // upright, spreading
            y0 = 1.55f + 0.85f * rnd();
            len = 1.55f + 0.35f * rnd();
        } else {
            az = rnd() * 6.2832f; pitch = 0.12f; y0 = 2.35f; len = 1.30f;
        }
        Vec3 dir = { sinf(pitch) * cosf(az), cosf(pitch), sinf(pitch) * sinf(az) };
        Vec3 base = { 0.0f, y0, 0.0f };
        L.pivot[g] = base;
        g_recN = 0; g_recOn = true;
        glNewList(L.base + 1 + g, GL_COMPILE);
        growBranch(base, dir, len, 0.062f, 1);
        glEndList();
        g_recOn = false;
        emitShadowList(L.base + 8 + g);
    }
    L.built = true;
    g_lod = false;
    g_mapleStyle = false; g_maxLevel = 3;
}

static BlossomLists g_green[7] = {};                   // [0..2] full, [3] jungle light, [4..6] distance version

// Broadleaf tree: short flared trunk, wide spreading branches, layered pads of leaves
static void buildGreenVariant(int v)
{
    const bool lod = v >= 4;
    const int sv = lod ? v - 4 : v;
    BlossomLists& L = g_green[v];
    L.base = glGenLists(16);
    g_lod = lod;
    g_greenStyle = true; g_maxLevel = (v == 3 || lod) ? 2 : 3;      // v == 3: jungle light variant; lod: distance version

    g_rndSeed = 3500 + sv * 29; g_rndCtr = 0;
    glNewList(L.base, GL_COMPILE);
    {
        static const Color BARK = { 0.36f, 0.29f, 0.22f }, BARK_D = { 0.28f, 0.22f, 0.17f };
        Vec3 a = { 0, 0, 0 }, b = { 0.03f, 1.3f, 0.02f }, c = { -0.03f, 2.3f, 0.0f }, d = { 0.0f, 3.1f, 0.0f };
        branchTube(a, b, 0.34f, 0.22f, BARK_D);                    // flared base
        branchTube(b, c, 0.22f, 0.17f, BARK);
        branchTube(c, d, 0.17f, 0.12f, BARK);
        for (int i = 0; i < 5; i++) {
            float ang = (i * 72.0f + rnd() * 25.0f) * PI / 180.0f;
            branchTube({ 0, 0.5f, 0 }, { cosf(ang) * 0.72f, 0.0f, sinf(ang) * 0.72f }, 0.17f, 0.04f, BARK_D);
        }
    }
    glEndList();

    for (int g = 0; g < 7; g++) {
        g_rndSeed = 3900 + sv * 53 + g * 17; g_rndCtr = 0;
        float az, pitch, y0, len;
        if (g < 6) {
            az = (g * 60.0f + (rnd() - 0.5f) * 30.0f) * PI / 180.0f;
            pitch = (14.0f + 24.0f * rnd()) * PI / 180.0f;        // steep: crown grows up, not out
            y0 = 1.9f + 1.3f * rnd();
            len = 1.9f + 0.6f * rnd();
        } else {
            az = rnd() * 6.2832f; pitch = 0.08f; y0 = 3.0f; len = 1.9f;
        }
        Vec3 dir = { sinf(pitch) * cosf(az), cosf(pitch), sinf(pitch) * sinf(az) };
        Vec3 base = { 0.0f, y0, 0.0f };
        L.pivot[g] = base;
        g_recN = 0; g_recOn = true;
        glNewList(L.base + 1 + g, GL_COMPILE);
        growBranch(base, dir, len, 0.085f, 1);
        glEndList();
        g_recOn = false;
        emitShadowList(L.base + 8 + g);
    }
    L.built = true;
    g_greenStyle = false;
    g_lod = false; g_maxLevel = 3;
}


// ─── Broadleaf tree ────────────────────────────────────────────────────────
void drawTree(Vec3 pos, Vec3 rot, Vec3 scale)
{
    if (cullObj(pos, scale, 6.0f)) return;
    int v = ((int)floorf(fabsf(pos.x) * 3.0f + fabsf(pos.z) * 5.0f)) % 3;
    if (!outdoorShadowPass && outdoorDist2(pos.x, pos.z) > TREE_LOD_DIST * TREE_LOD_DIST) v += 4;   // distance version
    BlossomLists& L = g_green[v];
    if (!L.built) buildGreenVariant(v);

    glPushMatrix();
    applyTransform(pos, rot, scale);
    glScalef(0.85f, 0.85f, 0.85f);

    float treePhase = pos.x * 0.29f + pos.z * 0.21f;
    glCallList(L.base);
    for (int g = 0; g < 7; g++) {
        float amp = (g < 6) ? 1.8f : 0.8f;
        float a = amp * breeze(treePhase + g * 0.6f);
        glPushMatrix();
        glTranslatef(L.pivot[g].x, L.pivot[g].y, L.pivot[g].z);
        glRotatef(a, 0, 0, 1);
        glRotatef(a * 0.45f, 1, 0, 0);
        glTranslatef(-L.pivot[g].x, -L.pivot[g].y, -L.pivot[g].z);
        glCallList(L.base + ((outdoorShadowPass && pos.z < SHADOW_DETAIL_Z) ? 8 : 1) + g);
        glPopMatrix();
    }
    glPopMatrix();
}

// Light broadleaf tree for the jungle patches (many of them, so far fewer triangles)
static void drawGreenTreeLOD(Vec3 p, float s, int idx)
{
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glRotatef(idx * 97.0f, 0, 1, 0);
    glScalef(s, s, s);
    BlossomLists& L = g_green[3];
    if (!L.built) buildGreenVariant(3);
    float treePhase = p.x * 0.29f + p.z * 0.21f + idx * 1.3f;
    glCallList(L.base);
    for (int g = 0; g < 7; g++) {
        float a = ((g < 6) ? 1.8f : 0.8f) * breeze(treePhase + g * 0.6f);
        glPushMatrix();
        glTranslatef(L.pivot[g].x, L.pivot[g].y, L.pivot[g].z);
        glRotatef(a, 0, 0, 1);
        glRotatef(a * 0.45f, 1, 0, 0);
        glTranslatef(-L.pivot[g].x, -L.pivot[g].y, -L.pivot[g].z);
        glCallList(L.base + ((outdoorShadowPass && p.z < SHADOW_DETAIL_Z) ? 8 : 1) + g);
        glPopMatrix();
    }
    glPopMatrix();
}

// ---- conifer: tall tapering trunk + stacked tiers of jagged needle skirts ----
static GLuint g_pine[3] = { 0, 0, 0 };

// One jagged skirt (triangle fan): light centre, darker drooping jagged rim
static void pineSkirt(float y, float R, float drop, Color c, int seed)
{
    const int N = 18;
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glColor3f(c.r * 1.25f, c.g * 1.25f, c.b * 1.2f);
    glVertex3f(0.0f, y + 0.22f, 0.0f);
    for (int i = 0; i <= N; i++) {
        int k = i % N;                                         // closing vertex equals the first one
        float a = k * 6.2832f / N + seed * 0.37f;
        float rr = R * ((k % 2 == 0) ? 1.0f : 0.70f) * (0.92f + 0.16f * blossomHash(seed * 31 + k, 7));
        float yy = y - drop - 0.10f * blossomHash(seed * 31 + k, 8);
        float nx = cosf(a) * 0.55f, nz = sinf(a) * 0.55f, nl = sqrtf(nx * nx + 0.64f + nz * nz);
        glNormal3f(nx / nl, 0.8f / nl, nz / nl);
        glColor3f(c.r * 0.72f, c.g * 0.72f, c.b * 0.72f);
        glVertex3f(cosf(a) * rr, yy, sinf(a) * rr);
    }
    glEnd();
}

static void buildPineVariant(int v)
{
    GLuint base = glGenLists(2);
    g_pine[v] = base;
    g_rndSeed = 4500 + v * 31; g_rndCtr = 0;

    glNewList(base, GL_COMPILE);                               // trunk
    {
        static const Color T = { 0.30f, 0.21f, 0.13f }, TD = { 0.23f, 0.16f, 0.10f };
        branchTube({ 0, 0, 0 }, { 0.03f, 1.4f, 0.0f }, 0.17f, 0.12f, TD);
        branchTube({ 0.03f, 1.4f, 0.0f }, { -0.02f, 3.3f, 0.0f }, 0.12f, 0.04f, T);
        for (int i = 0; i < 4; i++) {
            float ang = (i * 90.0f + rnd() * 30.0f) * PI / 180.0f;
            branchTube({ 0, 0.3f, 0 }, { cosf(ang) * 0.45f, 0.0f, sinf(ang) * 0.45f }, 0.09f, 0.03f, TD);
        }
    }
    glEndList();

    glNewList(base + 1, GL_COMPILE);                           // crown
    {
        const int tiers = 6;
        for (int t = 0; t < tiers; t++) {
            float u = t / (tiers - 1.0f);
            float y = 0.85f + u * 2.55f;
            float R = 1.15f * (1.0f - 0.84f * u) + 0.12f;
            float drop = 0.50f * (1.0f - 0.45f * u);
            float vary = 0.04f * (rnd() - 0.5f);
            Color c = { 0.06f + 0.08f * u + vary, 0.26f + 0.16f * u + vary, 0.11f + 0.06f * u };
            pineSkirt(y, R, drop, c, v * 17 + t);                                        // outer skirt
            pineSkirt(y + 0.14f, R * 0.70f, drop * 0.8f, { c.r * 1.15f, c.g * 1.15f, c.b * 1.1f }, v * 17 + t + 50);
        }
        // spire
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        glColor3f(0.18f, 0.46f, 0.20f);
        glVertex3f(0.0f, 3.75f, 0.0f);
        glColor3f(0.10f, 0.32f, 0.14f);
        for (int i = 0; i <= 8; i++) glVertex3f(cosf(i * 6.2832f / 8) * 0.14f, 3.25f, sinf(i * 6.2832f / 8) * 0.14f);
        glEnd();
    }
    glEndList();
}

// ---- falling petals / leaves and the carpet below the tree ----
static Color petalColor(bool leaf, int i)
{
    static const Color CHERRY[3] = { { 1.00f, 0.78f, 0.86f }, { 0.98f, 0.62f, 0.76f }, { 1.00f, 0.90f, 0.94f } };
    static const Color LEAF[3]   = { { 0.85f, 0.15f, 0.08f }, { 0.92f, 0.45f, 0.10f }, { 0.95f, 0.70f, 0.15f } };
    return leaf ? LEAF[i % 3] : CHERRY[i % 3];
}

static void petalQuad(float w, float h)          // flat kite shape in the XZ plane
{
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(0, 0, -h); glVertex3f(w, 0, 0); glVertex3f(0, 0, h); glVertex3f(-w, 0, 0);
    glEnd();
}

// Flat maple leaf in the XZ plane (for falling / fallen leaves)
static void mapleLeafFlat(float s)
{
    glPushMatrix();
    glRotatef(-90.0f, 1, 0, 0);
    drawMapleLeafShape(s);
    glPopMatrix();
}

// Flat notched cherry petal (same outline as the blossom petals) in the XZ plane
static void notchedPetal(float s)
{
    static const float PV[6][3] = {
        { 0.00f, 0.06f, 0.00f }, { -0.34f, 0.42f, 0.08f }, { -0.23f, 0.93f, 0.20f },
        { 0.00f, 0.80f, 0.13f }, {  0.23f, 0.93f, 0.20f }, {  0.34f, 0.42f, 0.08f } };
    static const int T[4][3] = { { 0, 1, 2 }, { 0, 2, 3 }, { 0, 3, 4 }, { 0, 4, 5 } };
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 1, 0);
    for (int t = 0; t < 4; t++)
        for (int k = 0; k < 3; k++) {
            int i = T[t][k];
            glVertex3f(PV[i][0] * s, PV[i][2] * s * 0.4f, (PV[i][1] - 0.45f) * s);
        }
    glEnd();
}

// Petals drift down from the canopy, tumble, get blown along the wind and
// shrink away once they have landed.  Purely a function of time.
static void drawFallingPetals(float radius, float top, int n, bool leaf, int seed)
{
    if (outdoorShadowPass) return;
    float w = leaf ? 0.075f : 0.045f, h = leaf ? 0.105f : 0.070f;
    for (int i = 0; i < n; i++) {
        float speed = 0.09f + 0.10f * blossomHash(seed + i, 1);          // cycles per second
        float u = fmodf(animTime * speed + blossomHash(seed + i, 2), 1.0f);
        float y = top * (1.0f - u * 1.04f);
        float scale = 1.0f;
        if (y < 0.03f) { y = 0.03f; scale = clamp01f((1.0f - u) / 0.04f); }   // landed: shrink out
        float a0 = blossomHash(seed + i, 3) * 6.2832f, rad = radius * sqrtf(blossomHash(seed + i, 4));
        float wind = 1.0f + 0.4f * breeze(seed * 0.1f + i * 0.3f);
        float drift = u * 1.4f * wind;
        float x = cosf(a0) * rad + drift + 0.25f * sinf(animTime * 1.3f + i);
        float z = sinf(a0) * rad + 0.45f * drift + 0.25f * cosf(animTime * 1.1f + i * 1.7f);
        setColor(petalColor(leaf, i));
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(animTime * (70.0f + 110.0f * blossomHash(seed + i, 5)) + blossomHash(seed + i, 6) * 360.0f, 0, 1, 0);
        if (y > 0.04f) glRotatef(38.0f * sinf(animTime * 3.1f + i * 2.1f) + 25.0f, 1, 0, 0);   // flutter
        glScalef(scale, scale, scale);
        if (leaf) mapleLeafFlat(0.085f); else notchedPetal(0.12f);
        glPopMatrix();
    }
}

// Carpet of fallen petals / leaves (static, cached)
static GLuint g_carpet[2] = { 0, 0 };
static void drawPetalCarpet(bool leaf, float radius, int n)
{
    if (outdoorShadowPass) return;
    int idx = leaf ? 1 : 0;
    if (!g_carpet[idx]) {
        g_carpet[idx] = glGenLists(1);
        glNewList(g_carpet[idx], GL_COMPILE);
        float w = leaf ? 0.075f : 0.045f, h = leaf ? 0.105f : 0.070f;
        for (int i = 0; i < n; i++) {
            float a = blossomHash(i, 41 + idx) * 6.2832f, r = radius * sqrtf(blossomHash(i, 43 + idx));
            setColor(petalColor(leaf, i));
            glPushMatrix();
            glTranslatef(cosf(a) * r, 0.02f + 0.0004f * (i % 7), sinf(a) * r);
            glRotatef(blossomHash(i, 47 + idx) * 360.0f, 0, 1, 0);
            glScalef(1.3f, 1.0f, 1.3f);
            if (leaf) mapleLeafFlat(0.085f); else notchedPetal(0.12f);
            glPopMatrix();
        }
        glEndList();
    }
    glCallList(g_carpet[idx]);
}

// ────── Cherry Blossom Tree ──────
void drawCherryBlossomTree(Vec3 pos, Vec3 rot, Vec3 scale)
{
    if (cullObj(pos, scale, 5.5f)) return;
    int v = ((int)floorf(fabsf(pos.x) * 3.0f + fabsf(pos.z) * 5.0f)) % 3;   // neighbours differ
    if (!outdoorShadowPass && outdoorDist2(pos.x, pos.z) > TREE_LOD_DIST * TREE_LOD_DIST) v += 3;   // distance version
    BlossomLists& L = g_cherry[v];
    if (!L.built) buildCherryVariant(v);

    glPushMatrix();
    applyTransform(pos, rot, scale);

    float treePhase = pos.x * 0.37f + pos.z * 0.23f;                        // each tree out of step
    setMaterialGloss(0.20f, 0.20f, 0.20f, 25.0f);
    glCallList(L.base);                                                     // trunk
    for (int g = 0; g < 7; g++) {                                           // swaying branch groups
        float amp = (g < 6) ? 2.8f : 1.2f;
        float a = amp * breeze(treePhase + g * 0.55f);
        glPushMatrix();
        glTranslatef(L.pivot[g].x, L.pivot[g].y, L.pivot[g].z);
        glRotatef(a, 0, 0, 1);
        glRotatef(a * 0.45f, 1, 0, 0);
        glTranslatef(-L.pivot[g].x, -L.pivot[g].y, -L.pivot[g].z);
        glCallList(L.base + ((outdoorShadowPass && pos.z < SHADOW_DETAIL_Z) ? 8 : 1) + g);
        glPopMatrix();
    }
    resetMaterialGloss();

    int seed = (int)(pos.x * 7.0f + pos.z * 13.0f);
    drawFallingPetals(2.1f, 3.7f, 14, false, seed);
    drawPetalCarpet(false, 2.8f, 110);

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
        drawCube({ 0, y, -0.045f }, NO_ROT,
                 { latticeW - 0.04f, 0.03f, 0.03f }, DARK_WOOD);   // lattice also on the inside face
    }

    // Vertical kumiko lattice bars
    for (int i = 1; i <= vBars; i++) {
        float x = -(pw * 0.5f - fw) + i * vStep;
        float barY = (latticeBot + latticeTop) * 0.5f;
        drawCube({ x, barY, 0.01f }, NO_ROT,
                 { 0.03f, latticeH, 0.03f }, DARK_WOOD);
        drawCube({ x, barY, -0.045f }, NO_ROT,
                 { 0.03f, latticeH, 0.03f }, DARK_WOOD);   // lattice also on the inside face
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
        drawCube({ 0, y, -0.045f }, NO_ROT,
                 { latticeW - 0.04f, 0.03f, 0.03f }, DARK_WOOD);   // lattice also on the inside face
    }

    // Vertical kumiko lattice bars
    for (int i = 1; i <= vBars; i++) {
        float x = -(pw * 0.5f - fw) + i * vStep;
        float barY = (latticeBot + latticeTop) * 0.5f;
        drawCube({ x, barY, 0.01f }, NO_ROT,
                 { 0.03f, latticeH, 0.03f }, DARK_WOOD);
        drawCube({ x, barY, -0.045f }, NO_ROT,
                 { 0.03f, latticeH, 0.03f }, DARK_WOOD);   // lattice also on the inside face
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
        drawCube({ 0, y, -0.045f }, NO_ROT, { latticeW - 0.02f, 0.025f, 0.025f }, DARK_WOOD);   // lattice also on the inside face
    }

    // Kumiko lattice (vertical bars)
    int vBars = (int)(latticeW / 0.35f);
    float vStep = latticeW / (vBars + 1);
    for (int i = 1; i <= vBars; i++) {
        float x = -hw + fw + i * vStep;
        float barCY = (latticeBot + latticeTop) * 0.5f;
        drawCube({ x, barCY, 0.008f }, NO_ROT, { 0.025f, latticeH, 0.025f }, DARK_WOOD);
        drawCube({ x, barCY, -0.045f }, NO_ROT, { 0.025f, latticeH, 0.025f }, DARK_WOOD);   // lattice also on the inside face
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
        drawCube({ 0, y, -0.045f }, NO_ROT, { width - 0.10f, 0.03f, 0.03f }, DARK_WOOD);   // lattice also on the inside face
    }

    // Vertical kumiko lattice bars
    int vBars = (int)(width / 0.40f);
    float vStep = (width - 0.12f) / (vBars + 1);
    for (int i = 1; i <= vBars; i++) {
        float x = -hw + 0.06f + i * vStep;
        drawCube({ x, 0, 0.01f }, NO_ROT, { 0.03f, height - 0.10f, 0.03f }, DARK_WOOD);
        drawCube({ x, 0, -0.045f }, NO_ROT, { 0.03f, height - 0.10f, 0.03f }, DARK_WOOD);   // lattice also on the inside face
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
        drawCube({ x, 0, -0.045f }, NO_ROT, { 0.025f, height - 0.10f, 0.025f }, DARK_WOOD);   // lattice also on the inside face
    }
    for (int j = 1; j <= rows; j++) {
        float y = -hh + 0.05f + j * cellH;
        drawCube({ 0, y, 0.01f }, NO_ROT, { width - 0.10f, 0.025f, 0.025f }, DARK_WOOD);
        drawCube({ 0, y, -0.045f }, NO_ROT, { width - 0.10f, 0.025f, 0.025f }, DARK_WOOD);   // lattice also on the inside face
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
    if (cullObj(pos, scale, 5.0f)) return;
    int v = ((int)floorf(fabsf(pos.x) * 3.0f + fabsf(pos.z) * 5.0f)) % 3;
    if (!g_pine[v]) buildPineVariant(v);

    glPushMatrix();
    applyTransform(pos, rot, scale);

    glCallList(g_pine[v]);                                     // trunk
    float a = 1.3f * breeze(pos.x * 0.33f + pos.z * 0.19f);    // the whole crown sways gently
    glPushMatrix();
    glRotatef(a, 0, 0, 1);
    glRotatef(a * 0.5f, 1, 0, 0);
    glCallList(g_pine[v] + 1);
    glPopMatrix();

    glPopMatrix();
}

// ─── Bamboo Grove ───────────────────────────────────────────────────────────
// Cluster of tall bamboo stalks with small leaf tufts at the top
// ─── Bamboo ─────────────────────────────────────────────────────────────────
// Each culm: slightly bent, tapering stalk made of internodes with swollen pale
// node bands, thin side branches near the top carrying drooping clusters of long
// narrow leaves.  Culms are cached in display lists (3 grove variants x 9 culms)
// and sway individually in the breeze.
static void bambooTube(Vec3 a, Vec3 b, float r0, float r1, int slices)
{
    float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 1e-5f) return;
    glPushMatrix();
    glTranslatef(a.x, a.y, a.z);
    float ax = -dy, ay = dx;
    if (fabsf(ax) + fabsf(ay) > 1e-5f) glRotatef(acosf(dz / len) * 180.0f / PI, ax, ay, 0.0f);
    else if (dz < 0.0f) glRotatef(180.0f, 1, 0, 0);
    gluCylinder(quad, r0, r1, len, slices, 1);
    glPopMatrix();
}

// One long narrow drooping leaf from `base` heading along unit `dir`
static void bambooLeaf(Vec3 base, Vec3 dir, float len, float w, float droop, float tone)
{
    float sx = dir.z, sy = 0.0f, sz = -dir.x;                     // dir x up
    float sl = sqrtf(sx * sx + sz * sz);
    if (sl < 0.05f) { sx = 1.0f; sz = 0.0f; sl = 1.0f; }
    sx /= sl; sz /= sl;
    float nx = -(dir.y * sz), ny = dir.z * sx - dir.x * sz, nz = dir.y * sx;   // side x dir
    if (ny < 0.0f) { nx = -nx; ny = -ny; nz = -nz; }
    normalize3(nx, ny, nz);
    const int SEG = 4;
    Vec3 pt[SEG + 1];
    float ww[SEG + 1];
    for (int i = 0; i <= SEG; i++) {
        float t = i / (float)SEG;
        pt[i] = { base.x + dir.x * len * t, base.y + dir.y * len * t - droop * len * t * t, base.z + dir.z * len * t };
        ww[i] = w * powf(sinf(3.14159f * (0.10f + 0.90f * t)), 0.8f);
    }
    glNormal3f(nx, ny, nz);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < SEG; i++) {
        float k0 = 0.60f + 0.55f * (i / (float)SEG), k1 = 0.60f + 0.55f * ((i + 1) / (float)SEG);
        glColor3f(0.10f * tone * k0, 0.34f * tone * k0, 0.08f * tone * k0);
        glVertex3f(pt[i].x - sx * ww[i], pt[i].y, pt[i].z - sz * ww[i]);
        glVertex3f(pt[i].x + sx * ww[i], pt[i].y, pt[i].z + sz * ww[i]);
        glColor3f(0.10f * tone * k1, 0.34f * tone * k1, 0.08f * tone * k1);
        glVertex3f(pt[i + 1].x + sx * ww[i + 1], pt[i + 1].y, pt[i + 1].z + sz * ww[i + 1]);
        glColor3f(0.10f * tone * k0, 0.34f * tone * k0, 0.08f * tone * k0);
        glVertex3f(pt[i].x - sx * ww[i], pt[i].y, pt[i].z - sz * ww[i]);
        glColor3f(0.10f * tone * k1, 0.34f * tone * k1, 0.08f * tone * k1);
        glVertex3f(pt[i + 1].x + sx * ww[i + 1], pt[i + 1].y, pt[i + 1].z + sz * ww[i + 1]);
        glVertex3f(pt[i + 1].x - sx * ww[i + 1], pt[i + 1].y, pt[i + 1].z - sz * ww[i + 1]);
    }
    glEnd();
}

// Fan of leaves at the end of a twig
static bool g_bambooLod = false;                 // building the lighter distance version of a bamboo grove
static void bambooLeafCluster(Vec3 c, float yawBase, float size)
{
    int n = (g_bambooLod ? 3 : 4) + (int)(rnd() * (g_bambooLod ? 2.0f : 3.0f));
    for (int i = 0; i < n; i++) {
        float yaw = yawBase + (i - (n - 1) * 0.5f) * 0.55f + (rnd() - 0.5f) * 0.3f;
        float up = 0.25f + 0.35f * rnd();
        Vec3 d = vNorm({ cosf(yaw), up, sinf(yaw) });
        bambooLeaf(c, d, size * (0.8f + 0.5f * rnd()), 0.05f + 0.025f * rnd(), 0.55f + 0.35f * rnd(), 0.8f + 0.5f * rnd());
    }
}

static GLuint g_bamboo[6] = { 0, 0, 0, 0, 0, 0 };          // [0..2] full, [3..5] distance version
static const int BAMBOO_CULMS = 9;
struct BambooPos { float x, z; };
static BambooPos g_bambooPos[6][BAMBOO_CULMS];

static void buildBambooVariant(int v)
{
    const bool lod = v >= 3;
    const int sv = lod ? v - 3 : v;
    g_bambooLod = lod;
    g_bamboo[v] = glGenLists(BAMBOO_CULMS);
    for (int c = 0; c < BAMBOO_CULMS; c++) {
        g_rndSeed = 7000 + sv * 101 + c * 13; g_rndCtr = 0;
        float ang = c * 2.39996f + sv, rad = 0.12f + 0.62f * sqrtf((c + 0.5f) / BAMBOO_CULMS);
        g_bambooPos[v][c] = { cosf(ang) * rad, sinf(ang) * rad };

        float H = 4.6f + 2.0f * rnd();
        float r0 = 0.055f + 0.025f * rnd();
        float lx = (rnd() - 0.5f) * 0.9f, lz = (rnd() - 0.5f) * 0.9f;        // lean
        float yellow = rnd();                                                  // older culms are yellower
        Color body = { 0.34f + 0.26f * yellow, 0.56f + 0.06f * yellow, 0.18f };
        int nodes = 9 + (int)(rnd() * 3.0f);

        glNewList(g_bamboo[v] + c, GL_COMPILE);
        glDisable(GL_CULL_FACE);
        // points at each node
        Vec3 np[16];
        float nr[16];
        for (int i = 0; i <= nodes; i++) {
            float t = i / (float)nodes;
            float tt = powf(t, 1.0f + 0.35f * (1.0f - t));                     // internodes get longer toward the top
            float y = H * (0.02f + 0.98f * tt);
            np[i] = { lx * t * t, y, lz * t * t };
            nr[i] = r0 * (1.0f - 0.52f * t);
        }
        for (int i = 0; i < nodes; i++) {
            float mid = 0.5f * (nr[i] + nr[i + 1]);
            float k = 0.92f + 0.10f * rnd();
            setColor({ body.r * k, body.g * k, body.b * k });
            bambooTube(np[i], np[i + 1], nr[i] * 0.97f, nr[i + 1] * 0.97f, lod ? 6 : 8);
            (void)mid;
            // swollen pale node band + thin dark ring below it
            Vec3 n0 = np[i + 1], n1 = { np[i + 1].x, np[i + 1].y + 0.045f, np[i + 1].z };
            setColor({ 0.66f, 0.68f, 0.34f });
            bambooTube(n0, n1, nr[i + 1] * 1.22f, nr[i + 1] * 1.12f, lod ? 6 : 8);
            Vec3 d0 = { np[i + 1].x, np[i + 1].y - 0.012f, np[i + 1].z };
            setColor({ 0.26f, 0.30f, 0.12f });
            bambooTube(d0, n0, nr[i + 1] * 1.15f, nr[i + 1] * 1.18f, 8);
        }
        // side branches + leaves on the upper nodes
        for (int i = nodes / 2; i <= nodes; i++) {
            int nb = (i == nodes || lod) ? 1 : 2;
            for (int q = 0; q < nb; q++) {
                float yaw = (rnd() * 6.2832f);
                float out = 0.35f + 0.45f * rnd();
                Vec3 st = np[i];
                Vec3 en = { st.x + cosf(yaw) * out, st.y + 0.12f + 0.18f * rnd(), st.z + sinf(yaw) * out };
                if (i == nodes) en = { st.x, st.y + 0.12f, st.z };
                setColor({ 0.28f, 0.42f, 0.16f });
                bambooTube(st, en, 0.014f, 0.007f, 5);
                bambooLeafCluster(en, yaw, (i == nodes) ? 0.55f : 0.45f);
            }
        }
        glEndList();
    }
    g_bambooLod = false;
}

void drawBambooGrove(Vec3 pos, Vec3 rot, Vec3 scale)
{
    if (cullObj(pos, scale, 8.0f)) return;
    int v = ((int)floorf(fabsf(pos.x) * 3.0f + fabsf(pos.z) * 5.0f)) % 3;
    const bool farGrove = !outdoorShadowPass && outdoorDist2(pos.x, pos.z) > BAMBOO_LOD_DIST * BAMBOO_LOD_DIST;
    if (farGrove) v += 3;                          // lighter distance version of the same grove
    if (!g_bamboo[v]) buildBambooVariant(v);

    glPushMatrix();
    applyTransform(pos, rot, scale);
    glScalef(1.0f, 1.0f, 1.0f);

    // grass fronds around the foot of the grove
    if (!outdoorShadowPass) {
        if (!g_tuft[1]) buildTuft(1);
        glPushMatrix();
        glScalef(1.1f, 0.9f, 1.1f);
        glCallList(g_tuft[1]);
        glPopMatrix();
    }

    float phase = pos.x * 0.3f + pos.z * 0.2f;
    int nCulms = (!outdoorShadowPass && outdoorDist2(pos.x, pos.z) > BAMBOO_LOD_DIST * BAMBOO_LOD_DIST) ? 7 : BAMBOO_CULMS;
    for (int c = 0; c < nCulms; c++) {
        float a = 1.6f * breeze(phase + c * 0.9f);                  // each culm sways on its own
        glPushMatrix();
        glTranslatef(g_bambooPos[v][c].x, 0.0f, g_bambooPos[v][c].z);
        glRotatef(a, 0, 0, 1);
        glRotatef(a * 0.6f, 1, 0, 0);
        glCallList(g_bamboo[v] + c);
        glPopMatrix();
    }
    glPopMatrix();
}

// ─── Maple Tree (Momiji) ────────────────────────────────────────────────────
// Slender maple with a fine branching crown of small red / orange leaves that
// sways in the breeze and drops leaves (same machinery as the cherry trees)
void drawMapleTree(Vec3 pos, Vec3 rot, Vec3 scale)
{
    if (cullObj(pos, scale, 6.0f)) return;
    int v = ((int)floorf(fabsf(pos.x) * 3.0f + fabsf(pos.z) * 5.0f)) % 3;
    if (!outdoorShadowPass && outdoorDist2(pos.x, pos.z) > TREE_LOD_DIST * TREE_LOD_DIST) v += 3;   // distance version
    BlossomLists& L = g_maple[v];
    if (!L.built) buildMapleVariant(v);

    glPushMatrix();
    applyTransform(pos, rot, scale);
    glScalef(0.9f, 0.9f, 0.9f);

    float treePhase = pos.x * 0.31f + pos.z * 0.27f;
    glCallList(L.base);                                                     // trunk + roots
    for (int g = 0; g < 7; g++) {
        float amp = (g < 6) ? 2.2f : 1.0f;
        float a = amp * breeze(treePhase + g * 0.6f);
        glPushMatrix();
        glTranslatef(L.pivot[g].x, L.pivot[g].y, L.pivot[g].z);
        glRotatef(a, 0, 0, 1);
        glRotatef(a * 0.45f, 1, 0, 0);
        glTranslatef(-L.pivot[g].x, -L.pivot[g].y, -L.pivot[g].z);
        glCallList(L.base + ((outdoorShadowPass && pos.z < SHADOW_DETAIL_Z) ? 8 : 1) + g);
        glPopMatrix();
    }

    int seed = (int)(pos.x * 5.0f + pos.z * 11.0f) + 400;
    drawFallingPetals(2.3f, 4.4f, 12, true, seed);
    drawPetalCarpet(true, 2.6f, 80);

    glPopMatrix();
}

// ─── Japanese Neighboring House ─────────────────────────────────────────────
// Traditional Japanese house with exposed rafters, veranda, red lanterns, foundation stones
void drawJapaneseHouse(Vec3 pos, Vec3 rot, Vec3 scale)
{
    if (cullObj(pos, scale, 6.5f)) return;
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float w = 4.0f, d = 3.5f, h = 2.8f;
    float verandaD = 1.0f;  // veranda depth extending from front
    const Color LANTERN_RED = { 0.85f, 0.15f, 0.10f };

    // ── Foundation stones under corner posts ──
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * w * 0.5f, 0, sz * d * 0.5f }, NO_ROT,
                       { 0.28f, 0.10f, 0.28f }, WHITE);
    // Foundation stones under veranda posts
    for (int sx = -1; sx <= 1; sx += 2)
        drawCuboid({ sx * w * 0.5f, 0, d * 0.5f + verandaD }, NO_ROT,
                   { 0.24f, 0.10f, 0.24f }, WHITE);
    drawCuboid({ 0, 0, d * 0.5f + verandaD }, NO_ROT, { 0.24f, 0.10f, 0.24f }, WHITE);

    // ── Walls — cream plaster ──
    GLuint wallTex = getTexID(TEX_WALL);
    // Back wall
    drawTexturedBox({ 0, 0.10f, -d * 0.5f }, NO_ROT, { w, h - 0.10f, 0.12f }, wallTex, WHITE, 2.0f);
    // Left wall
    drawTexturedBox({ -w * 0.5f, 0.10f, 0 }, NO_ROT, { 0.12f, h - 0.10f, d }, wallTex, WHITE, 2.0f);
    // Right wall
    drawTexturedBox({ w * 0.5f, 0.10f, 0 }, NO_ROT, { 0.12f, h - 0.10f, d }, wallTex, WHITE, 2.0f);
    // Front wall (shorter, above veranda level)
    drawTexturedBox({ 0, 0.10f, d * 0.5f }, NO_ROT, { w, h - 0.10f, 0.12f }, wallTex, WHITE, 2.0f);

    // ── Dark wood frame — corner posts ──
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * w * 0.5f, 0.10f, sz * d * 0.5f }, NO_ROT,
                       { 0.16f, h - 0.10f, 0.16f }, DARK_WOOD);

    // ── Horizontal frame beams — top and mid ──
    // Top beams (all 4 sides)
    drawCuboid({ 0, h - 0.06f,  d * 0.52f }, NO_ROT, { w + 0.2f, 0.10f, 0.08f }, DARK_WOOD);
    drawCuboid({ 0, h - 0.06f, -d * 0.52f }, NO_ROT, { w + 0.2f, 0.10f, 0.08f }, DARK_WOOD);
    drawCuboid({  w * 0.52f, h - 0.06f, 0 }, NO_ROT, { 0.08f, 0.10f, d + 0.2f }, DARK_WOOD);
    drawCuboid({ -w * 0.52f, h - 0.06f, 0 }, NO_ROT, { 0.08f, 0.10f, d + 0.2f }, DARK_WOOD);
    // Mid beams (sides only, not front)
    float midH = 1.4f;
    drawCuboid({  w * 0.52f, midH, 0 }, NO_ROT, { 0.06f, 0.08f, d + 0.2f }, DARK_WOOD);
    drawCuboid({ -w * 0.52f, midH, 0 }, NO_ROT, { 0.06f, 0.08f, d + 0.2f }, DARK_WOOD);

    // ── Shoji windows on front ──
    drawShojiWindow({ -0.8f, 1.5f, d * 0.5f + 0.08f }, NO_ROT, ONE, 1.2f, 1.0f);
    drawShojiWindow({  0.8f, 1.5f, d * 0.5f + 0.08f }, NO_ROT, ONE, 1.2f, 1.0f);

    // ── Door ──
    drawCuboid({ 0, 0.10f, d * 0.5f + 0.05f }, NO_ROT, { 0.7f, 1.8f, 0.06f }, WOOD);
    drawSphere({ 0.25f, 1.0f, d * 0.5f + 0.10f }, NO_ROT, { 0.04f, 0.04f, 0.04f }, DARK_GRAY);

    // ── Raised wooden veranda/engawa at front ──
    // Veranda deck
    drawTexturedBox({ 0, 0.28f, d * 0.5f + verandaD * 0.5f }, NO_ROT,
                    { w + 0.3f, 0.06f, verandaD + 0.1f },
                    getTexID(TEX_WOOD), LIGHT_WOOD, 1.0f);
    // Veranda support posts
    for (int sx = -1; sx <= 1; sx += 2) {
        drawCuboid({ sx * w * 0.5f, 0.10f, d * 0.5f + verandaD }, NO_ROT,
                   { 0.12f, h - 0.10f, 0.12f }, DARK_WOOD);
    }
    drawCuboid({ 0, 0.10f, d * 0.5f + verandaD }, NO_ROT,
               { 0.10f, 0.18f, 0.10f }, DARK_WOOD);

    // ── Gable roof with tile texture ──
    float roofW = w + 1.4f, roofD = d + 1.4f + verandaD;
    // Eave slab
    drawCuboid({ 0, h, verandaD * 0.3f }, NO_ROT, { roofW, 0.06f, roofD }, DARK_GRAY);
    // Tiled roof
    drawTexturedWedge({ 0, h + 0.06f, verandaD * 0.3f }, NO_ROT,
                      { roofW, 1.4f, roofD },
                      getTexID(TEX_ROOF_TILE), WHITE, 1.2f);
    // Ridge cap
    drawCuboid({ 0, h + 1.40f, verandaD * 0.3f }, NO_ROT,
               { roofW + 0.2f, 0.10f, 0.18f }, DARK_WOOD);

    // ── Exposed roof rafters under the eaves ──
    {
        int numRafters = 8;
        float rafterSpacing = roofW / (numRafters + 1);
        float eaveOverhang = (roofD - d) * 0.5f;
        // Front eave rafters
        for (int i = 1; i <= numRafters; i++) {
            float rx = -roofW * 0.5f + i * rafterSpacing;
            drawCuboid({ rx, h - 0.02f, d * 0.5f + verandaD * 0.3f + eaveOverhang * 0.5f },
                       NO_ROT, { 0.05f, 0.05f, eaveOverhang + 0.2f }, WOOD);
        }
        // Back eave rafters
        for (int i = 1; i <= numRafters; i++) {
            float rx = -roofW * 0.5f + i * rafterSpacing;
            drawCuboid({ rx, h - 0.02f, -d * 0.5f - eaveOverhang * 0.3f },
                       NO_ROT, { 0.05f, 0.05f, eaveOverhang + 0.2f }, WOOD);
        }
    }

    // ── Red paper lanterns on front wall ──
    {
        float lanternY = 2.2f;
        float lanternZ = d * 0.5f + 0.16f;
        float positions[] = { -1.4f, -0.5f, 0.5f, 1.4f };
        for (int i = 0; i < 4; i++) {
            // Bracket
            drawCuboid({ positions[i], lanternY + 0.18f, lanternZ - 0.04f }, NO_ROT,
                       { 0.03f, 0.04f, 0.08f }, DARK_WOOD);
            // Lantern body (oval)
            if (!isDayTime)
                setEmission(0.85f, 0.20f, 0.08f);
            else
                setEmission(0.06f, 0.01f, 0.01f);
            drawSphere({ positions[i], lanternY, lanternZ }, NO_ROT,
                       { 0.12f, 0.16f, 0.12f }, LANTERN_RED);
            clearEmission();
        }
    }

    // ── Night glow effects ──
    if (!isDayTime) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);

        // Window light spill on ground
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

// ─── Japanese House Variant 2 ───────────────────────────────────────────────
// Wider, lower house with large sliding doors, side porch, raised floor
void drawJapaneseHouse2(Vec3 pos, Vec3 rot, Vec3 scale)
{
    if (cullObj(pos, scale, 6.5f)) return;
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float w = 6.0f, d = 3.5f, h = 2.4f;
    float floorH = 0.35f;  // raised floor height

    // ── Foundation posts ──
    for (int i = 0; i < 3; i++) {
        float fx = -w * 0.4f + i * w * 0.4f;
        for (int sz = -1; sz <= 1; sz += 2) {
            drawCuboid({ fx, 0, sz * d * 0.45f }, NO_ROT,
                       { 0.14f, floorH, 0.14f }, DARK_WOOD);
            // Stone base
            drawCuboid({ fx, 0, sz * d * 0.45f }, NO_ROT,
                       { 0.22f, 0.06f, 0.22f }, WHITE);
        }
    }

    // ── Raised floor platform ──
    drawTexturedBox({ 0, floorH, 0 }, NO_ROT, { w + 0.2f, 0.08f, d + 0.2f },
                    getTexID(TEX_WOOD), LIGHT_WOOD, 1.5f);

    // ── Walls ──
    GLuint wallTex = getTexID(TEX_WALL);
    float wallBase = floorH + 0.08f;
    float wallH = h - wallBase;
    // Back wall
    drawTexturedBox({ 0, wallBase, -d * 0.5f }, NO_ROT, { w, wallH, 0.10f }, wallTex, WHITE, 2.0f);
    // Left wall (solid wood panel)
    drawTexturedBox({ -w * 0.5f, wallBase, 0 }, NO_ROT, { 0.10f, wallH, d },
                    getTexID(TEX_DARK_WOOD), WOOD, 1.5f);
    // Right wall (partial - has side entrance)
    drawTexturedBox({ w * 0.5f, wallBase, -d * 0.25f }, NO_ROT, { 0.10f, wallH, d * 0.5f },
                    getTexID(TEX_DARK_WOOD), WOOD, 1.5f);

    // ── Front: large sliding glass/shoji panels ──
    float panelW = w / 4.0f;
    for (int i = 0; i < 4; i++) {
        float px = -w * 0.5f + panelW * 0.5f + i * panelW;
        // Shoji window panel
        drawShojiWindow({ px, wallBase + wallH * 0.5f, d * 0.5f + 0.06f },
                        NO_ROT, ONE, panelW - 0.08f, wallH - 0.1f);
        // Vertical divider post
        if (i > 0) {
            drawCuboid({ px - panelW * 0.5f, wallBase, d * 0.5f }, NO_ROT,
                       { 0.08f, wallH, 0.08f }, DARK_WOOD);
        }
    }

    // ── Wood frame — corner posts ──
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * w * 0.5f, wallBase, sz * d * 0.5f }, NO_ROT,
                       { 0.14f, wallH, 0.14f }, DARK_WOOD);

    // ── Top beams ──
    drawCuboid({ 0, h - 0.05f,  d * 0.52f }, NO_ROT, { w + 0.2f, 0.08f, 0.07f }, DARK_WOOD);
    drawCuboid({ 0, h - 0.05f, -d * 0.52f }, NO_ROT, { w + 0.2f, 0.08f, 0.07f }, DARK_WOOD);
    drawCuboid({  w * 0.52f, h - 0.05f, 0 }, NO_ROT, { 0.07f, 0.08f, d + 0.2f }, DARK_WOOD);
    drawCuboid({ -w * 0.52f, h - 0.05f, 0 }, NO_ROT, { 0.07f, 0.08f, d + 0.2f }, DARK_WOOD);

    // ── Side porch on right ──
    float porchW = 1.2f, porchD = d;
    float porchX = w * 0.5f + porchW * 0.5f;
    // Porch floor
    drawTexturedBox({ porchX, floorH, 0 }, NO_ROT, { porchW, 0.06f, porchD },
                    getTexID(TEX_WOOD), LIGHT_WOOD, 1.0f);
    // Porch corner posts
    drawCuboid({ w * 0.5f + porchW, wallBase, d * 0.5f }, NO_ROT,
               { 0.10f, wallH, 0.10f }, DARK_WOOD);
    drawCuboid({ w * 0.5f + porchW, wallBase, -d * 0.5f }, NO_ROT,
               { 0.10f, wallH, 0.10f }, DARK_WOOD);
    // Porch foundation stones
    drawCuboid({ w * 0.5f + porchW, 0, d * 0.45f }, NO_ROT, { 0.20f, 0.06f, 0.20f }, WHITE);
    drawCuboid({ w * 0.5f + porchW, 0, -d * 0.45f }, NO_ROT, { 0.20f, 0.06f, 0.20f }, WHITE);

    // ── Roof (wider to cover porch) ──
    float roofW = w + porchW + 1.2f, roofD = d + 1.2f;
    float roofOffX = porchW * 0.3f;
    drawCuboid({ roofOffX, h, 0 }, NO_ROT, { roofW, 0.05f, roofD }, DARK_GRAY);
    drawTexturedWedge({ roofOffX, h + 0.05f, 0 }, NO_ROT, { roofW, 1.1f, roofD },
                      getTexID(TEX_ROOF_TILE), WHITE, 1.2f);
    drawCuboid({ roofOffX, h + 1.1f, 0 }, NO_ROT, { roofW + 0.2f, 0.08f, 0.16f }, DARK_WOOD);

    // ── Exposed rafters under front eave ──
    {
        int numRafters = 10;
        float eaveOver = (roofD - d) * 0.5f;
        for (int i = 1; i <= numRafters; i++) {
            float rx = -roofW * 0.5f + roofOffX + i * (roofW / (numRafters + 1));
            drawCuboid({ rx, h - 0.01f, d * 0.5f + eaveOver * 0.4f }, NO_ROT,
                       { 0.04f, 0.04f, eaveOver + 0.1f }, WOOD);
        }
    }

    // ── Red lanterns (2 on front) ──
    {
        float lanternZ = d * 0.5f + 0.14f;
        float lx[] = { -w * 0.25f, w * 0.25f };
        for (int i = 0; i < 2; i++) {
            drawCuboid({ lx[i], h - 0.12f, lanternZ }, NO_ROT,
                       { 0.03f, 0.10f, 0.06f }, DARK_WOOD);
            if (!isDayTime)
                setEmission(0.85f, 0.20f, 0.08f);
            else
                setEmission(0.06f, 0.01f, 0.01f);
            drawSphere({ lx[i], h - 0.30f, lanternZ }, NO_ROT,
                       { 0.12f, 0.16f, 0.12f }, { 0.85f, 0.15f, 0.10f });
            clearEmission();
        }
    }

    // ── Night glow ──
    if (!isDayTime) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        glColor4f(0.90f, 0.65f, 0.20f, 0.12f);
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
    if (cullObj(pos, scale, 2.0f)) return;
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
// Japanese arched bridge with red posts, dark railings, string lights
void drawBridge(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const Color BRIDGE_WOOD = { 0.35f, 0.22f, 0.13f };
    const Color POST_RED    = { 0.82f, 0.12f, 0.10f };
    const Color RAIL_DARK   = { 0.12f, 0.10f, 0.10f };
    const Color EDGE_METAL  = { 0.70f, 0.72f, 0.75f };
    const float bLen = 17.0f, bW = 1.6f, arch = 0.6f;
    const int   NP = 34;     // deck planks
    const int   NR = 8;      // railing post count per side

    // ── Metallic edge trim along bridge underside ──
    setMaterialConductive(EDGE_METAL, 80.0f);
    for (int i = 0; i < NP; i++) {
        float z = (i + 0.5f) / NP * bLen - bLen * 0.5f;
        float archY = arch * cosf(z * PI / bLen);
        for (int sx = -1; sx <= 1; sx += 2) {
            drawCuboid({ sx * bW * 0.52f, archY - 0.04f, z }, NO_ROT,
                       { 0.06f, 0.08f, bLen / NP + 0.01f }, EDGE_METAL);
        }
    }
    resetMaterialGloss();

    // ── Arched deck planks (dark wood) ──
    setMaterialPBR(Materials::WoodMatte, BRIDGE_WOOD);
    for (int i = 0; i < NP; i++) {
        float z = (i + 0.5f) / NP * bLen - bLen * 0.5f;
        float archY = arch * cosf(z * PI / bLen);
        drawCuboid({ 0, archY, z }, NO_ROT, { bW, 0.08f, bLen / NP + 0.01f }, BRIDGE_WOOD);
    }
    resetMaterialGloss();

    // ── Red posts with spherical finials ──
    for (int sx = -1; sx <= 1; sx += 2) {
        float rx = sx * bW * 0.5f;
        for (int i = 0; i <= NR; i++) {
            float z = i * (bLen / NR) - bLen * 0.5f;
            float archY = arch * cosf(z * PI / bLen);

            // Corner posts are taller
            bool isCorner = (i == 0 || i == NR);
            float postH = isCorner ? 1.4f : 0.8f;

            // Red post
            drawCuboid({ rx, archY, z }, NO_ROT, { 0.09f, postH, 0.09f }, POST_RED);

            // Red sphere finial on top
            drawSphere({ rx, archY + postH + 0.06f, z }, NO_ROT,
                       { 0.10f, 0.10f, 0.10f }, POST_RED);
        }

        // ── Dark horizontal railings (2 rails) ──
        for (int rail = 0; rail < 2; rail++) {
            float railOff = (rail == 0) ? 0.35f : 0.65f;
            for (int i = 0; i < NR; i++) {
                float z0 = i * (bLen / NR) - bLen * 0.5f;
                float z1 = (i + 1) * (bLen / NR) - bLen * 0.5f;
                float y0 = arch * cosf(z0 * PI / bLen) + railOff;
                float y1 = arch * cosf(z1 * PI / bLen) + railOff;
                float zc = (z0 + z1) * 0.5f;
                float len = sqrtf((z1 - z0) * (z1 - z0) + (y1 - y0) * (y1 - y0));
                float pitch = atanf((y1 - y0) / (z1 - z0)) * 180.0f / PI;
                drawCuboid({ rx, (y0 + y1) * 0.5f - 0.025f, zc }, { -pitch, 0, 0 },
                           { 0.05f, 0.05f, len + 0.02f }, RAIL_DARK);
            }
        }
    }

    // ── String lights between tall corner posts ──
    {
        // The 4 corner post tops
        float zFront = -bLen * 0.5f;
        float zBack  =  bLen * 0.5f;
        float yFront = arch * cosf(zFront * PI / bLen) + 1.4f + 0.06f;
        float yBack  = arch * cosf(zBack  * PI / bLen) + 1.4f + 0.06f;

        // String lights on each side
        for (int sx = -1; sx <= 1; sx += 2) {
            float rx = sx * bW * 0.5f;
            int numLights = 10;
            for (int i = 0; i <= numLights; i++) {
                float t = (float)i / numLights;
                float z = zFront + t * (zBack - zFront);
                float yLine = yFront + t * (yBack - yFront);
                // Catenary sag
                float sag = -0.4f * sinf(t * PI);
                float ly = yLine + sag;

                // Thin wire
                if (i < numLights) {
                    float zNext = zFront + (t + 1.0f / numLights) * (zBack - zFront);
                    float yNext = yFront + (t + 1.0f / numLights) * (yBack - yFront)
                                  - 0.4f * sinf((t + 1.0f / numLights) * PI);
                    float zc = (z + zNext) * 0.5f;
                    float yc = (ly + yNext) * 0.5f;
                    float segLen = sqrtf((zNext - z) * (zNext - z) + (yNext - ly) * (yNext - ly));
                    float pitch = atanf((yNext - ly) / (zNext - z + 0.001f)) * 180.0f / PI;
                    drawCuboid({ rx, yc - 0.005f, zc }, { -pitch, 0, 0 },
                               { 0.012f, 0.012f, segLen }, DARK_GRAY);
                }

                // Light bulb
                if (i > 0 && i < numLights) {
                    if (!isDayTime)
                        setEmission(1.0f, 0.85f, 0.5f);
                    else
                        setEmission(0.05f, 0.04f, 0.02f);
                    drawSphere({ rx, ly - 0.08f, z }, NO_ROT,
                               { 0.055f, 0.065f, 0.055f }, WHITE);
                    clearEmission();
                }
            }
        }
    }

    glPopMatrix();
}

// ─── Lake / Pond ────────────────────────────────────────────────────────────
// Reflective water surface with shore rocks and subtle wave animation
void drawLake(Vec3 pos, Vec3 rot, Vec3 scale)
{
    if (outdoorShadowPass) return;
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

    // Textured water surface with wave pattern and gradient
    GLuint waterTex = getTexID(TEX_WATER);
    if (waterTex) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, waterTex);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        shaderSetTexture(true);
    }

    glColor4f(waterCol.r * 1.4f, waterCol.g * 1.3f, waterCol.b * 1.1f, 0.88f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    // Center vertex
    float cy = 0.02f + 0.02f * sinf(waveT);
    // Animated UV scroll for moving water effect
    float uvOff = waveT * 0.04f;
    glTexCoord2f(0.5f + uvOff, 0.5f + uvOff * 0.3f);
    glVertex3f(0, cy, 0);
    // Outer ring
    int segs = 48;
    for (int i = 0; i <= segs; i++) {
        float a = (float)i * 2.0f * PI / segs;
        float rx = lakeR * cosf(a);
        float rz = lakeR * sinf(a);
        float wy = 0.02f + 0.015f * sinf(waveT + a * 3.0f);
        float tu = 0.5f + 0.5f * cosf(a) + uvOff;
        float tv = 0.5f + 0.5f * sinf(a) + uvOff * 0.3f;
        glTexCoord2f(tu, tv);
        glVertex3f(rx, wy, rz);
    }
    glEnd();

    if (waterTex) {
        shaderSetTexture(false);
        glDisable(GL_TEXTURE_2D);
    }

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
    if (cullObj(pos, scale, 1.8f) || outdoorDist2(pos.x, pos.z) > 32.0f * 32.0f) return;   // skip far tufts
    if (outdoorShadowPass) return;
    int v = ((int)floorf(fabsf(pos.x) * 3.0f + fabsf(pos.z) * 5.0f)) % 3;
    if (!g_tuft[v]) buildTuft(v);
    glPushMatrix();
    applyTransform(pos, rot, scale);
    float a = 2.5f * sinf(animTime * 1.5f + pos.x * 0.6f + pos.z * 0.4f) + 1.2f * sinf(animTime * 3.1f + pos.x);
    glRotatef(a, 0, 0, 1);
    glRotatef(a * 0.5f, 1, 0, 0);
    glCallList(g_tuft[v]);
    glPopMatrix();
}

// ─── Dense Jungle Vegetation ────────────────────────────────────────────────
// Thick cluster of tropical-looking plants, ferns, and small trees
void drawJungle(Vec3 pos, Vec3 rot, Vec3 scale)
{
    if (cullObj(pos, scale, 6.0f)) return;
    glPushMatrix();
    applyTransform(pos, rot, scale);
    g_objDist2 = outdoorDist2(pos.x, pos.z);
    g_shadowRot = rot.y;

    const Color JUNGLE_DARK  = { 0.10f, 0.35f, 0.12f };
    const Color JUNGLE_MID   = { 0.15f, 0.45f, 0.15f };
    const Color JUNGLE_LIGHT = { 0.22f, 0.55f, 0.20f };
    const Color FERN         = { 0.18f, 0.48f, 0.18f };

    // Dense undergrowth spheres (ground cover)
    domeBush( 0.0f, 0.0f,  0.0f, 1.25f, 0.70f, pos.x);
    domeBush( 1.2f, 0.0f,  0.8f, 1.00f, 0.65f, pos.x + 1.0f);
    domeBush(-1.0f, 0.0f, -0.6f, 0.90f, 0.65f, pos.z + 2.0f);

    // Mid-level bushes
    domeBush( 0.5f, 0.15f,  0.3f, 0.75f, 1.00f, pos.x + 3.0f);
    domeBush(-0.8f, 0.15f,  0.5f, 0.65f, 0.90f, pos.z + 4.0f);
    domeBush( 0.3f, 0.15f, -0.8f, 0.70f, 0.95f, pos.x + pos.z);

    // Tall jungle trees rising above the canopy
    // (the jungle patches no longer carry their own broadleaf trees: they are ground cover only,
    //  which keeps the forests light.  Trees are placed individually in scene.cpp.)

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

    g_shadowRot = 0.0f;
    glPopMatrix();
}

// ─── Fireflies ──────────────────────────────────────────────────────────────
// Animated glowing particles that drift and dim/brighten — visible at night only
void drawFireflies(Vec3 pos, float radius, int count)
{
    if (outdoorShadowPass) return;
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
    if (outdoorShadowPass) return;
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
    if (cullObj(pos, scale, 4.0f)) return;
    if (outdoorShadowPass) return;
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
// ─── Low-poly faceted mountains ─────────────────────────────────────────────
// Each peak is a radial heightfield: smooth cone profile + ridged noise + radial
// spurs, drawn with flat-shaded triangles.  Colour by height and slope: forest
// green at the foot, grey-brown / violet rock, white snow on high gentle facets.
static float mnoise(float x, float y)
{
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    float sx = fx * fx * (3.0f - 2.0f * fx), sy = fy * fy * (3.0f - 2.0f * fy);
    float a = fhash(x0, y0), b = fhash(x0 + 1, y0), c = fhash(x0, y0 + 1), d = fhash(x0 + 1, y0 + 1);
    return a + (b - a) * sx + (c - a) * sy + (a - b - c + d) * sx * sy;
}

static float mridged(float x, float y)
{
    float sum = 0.0f, amp = 0.5f, f = 1.0f;
    for (int i = 0; i < 4; i++) {
        float n = mnoise(x * f, y * f);
        sum += amp * (1.0f - fabsf(2.0f * n - 1.0f));
        amp *= 0.5f; f *= 2.1f;
    }
    return sum / 0.9375f;                           // ~0..1
}

static GLuint g_mountains = 0;

static void emitPeak(float cx, float cz, float R, float H, int seed)
{
    const int NR = 18, NS = 34;
    static float V[NR + 1][NS][3];
    for (int i = 0; i <= NR; i++) {
        float r = i / (float)NR;
        for (int j = 0; j < NS; j++) {
            float a = (j + 0.45f * (fhash(i * 7 + j, seed) - 0.5f)) * 6.2832f / NS;
            float rr = r * R * (i == 0 ? 0.0f : (0.92f + 0.16f * fhash(i + j * 13, seed + 5)));
            float x = cx + cosf(a) * rr, z = cz + sinf(a) * rr;
            float prof = powf(1.0f - r, 1.22f);
            float rid  = mridged(x * 0.075f + seed * 3.1f, z * 0.075f + seed * 1.7f);
            float spur = 0.5f + 0.5f * sinf(a * 5.0f + seed * 2.3f + 2.2f * mnoise(r * 3.0f + seed, (float)seed));
            float h = H * prof * (0.70f + 0.60f * rid) + H * 0.14f * prof * spur;
            if (i == NR) h = 0.0f;
            V[i][j][0] = x; V[i][j][1] = h; V[i][j][2] = z;
        }
    }
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < NR; i++)
        for (int j = 0; j < NS; j++) {
            int j2 = (j + 1) % NS;
            const float* q[4] = { V[i][j], V[i][j2], V[i + 1][j2], V[i + 1][j] };
            static const int T[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } };
            for (int t = 0; t < 2; t++) {
                const float *A = q[T[t][0]], *B = q[T[t][1]], *C = q[T[t][2]];
                float ux = B[0] - A[0], uy = B[1] - A[1], uz = B[2] - A[2];
                float vx = C[0] - A[0], vy = C[1] - A[1], vz = C[2] - A[2];
                float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
                if (ny < 0.0f) { nx = -nx; ny = -ny; nz = -nz; }
                normalize3(nx, ny, nz);
                float hy = (A[1] + B[1] + C[1]) / (3.0f * H);
                float cxm = (A[0] + B[0] + C[0]) / 3.0f, czm = (A[2] + B[2] + C[2]) / 3.0f;
                float facet = fhash(i * 31 + j * 2 + t, seed + 9);
                float nz2 = mnoise(cxm * 0.12f + seed, czm * 0.12f);
                float snowScore = hy * 1.25f + (ny - 0.45f) * 0.55f + (nz2 - 0.5f) * 0.45f;
                float cr, cg, cb;
                if (snowScore > 0.80f) {                              // snow, slightly blue in the shade
                    float k = 0.80f + 0.20f * facet;
                    cr = 0.90f * k; cg = 0.93f * k; cb = 1.00f * k;
                } else {
                    float m = facet;                                  // warm brown-grey .. cool violet-grey
                    cr = 0.52f + (0.30f - 0.52f) * m; cg = 0.44f + (0.29f - 0.44f) * m; cb = 0.40f + (0.36f - 0.40f) * m;
                    float shade = 0.70f + 0.45f * ny;                 // steep faces darker
                    cr *= shade; cg *= shade; cb *= shade;
                    if (hy < 0.20f) {                                 // forested foothills
                        float g = 1.0f - hy / 0.20f; g = g * g * (3.0f - 2.0f * g);
                        cr += (0.17f - cr) * g; cg += (0.30f - cg) * g; cb += (0.14f - cb) * g;
                    }
                }
                glNormal3f(nx, ny, nz);
                glColor3f(cr, cg, cb);
                glVertex3fv(A); glVertex3fv(B); glVertex3fv(C);
            }
        }
    glEnd();
}

static void buildMountains()
{
    g_mountains = glGenLists(1);
    glNewList(g_mountains, GL_COMPILE);
    glDisable(GL_CULL_FACE);
    emitPeak(-62.0f,  10.0f, 24.0f, 15.0f, 41);
    emitPeak( 60.0f,  12.0f, 22.0f, 14.0f, 52);
    emitPeak(-38.0f,   5.0f, 26.0f, 24.0f, 17);
    emitPeak( 34.0f,   8.0f, 24.0f, 20.0f, 29);
    emitPeak( 14.0f, -10.0f, 24.0f, 30.0f, 63);
    emitPeak(-14.0f, -12.0f, 22.0f, 27.0f, 74);
    emitPeak(  0.0f,   0.0f, 36.0f, 38.0f,  7);
    glEndList();
}

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

    // ── Faceted low-poly peaks (see emitPeak) ──
    if (!g_mountains) buildMountains();
    resetMaterialGloss();
    glCallList(g_mountains);

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
