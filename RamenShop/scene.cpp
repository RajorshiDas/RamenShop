#include "scene.h"
#include "lighting.h"
#include "objects.h"
#include "shader.h"

// ─── Scene feature flags ────────────────────────────────────────────────────
bool showRoof   = true;
bool showSteam  = true;
bool animPaused = false;

// Entrance door state
bool doorOpen   = false;
float doorAngle = 0.0f;

// Sliding shoji door state
bool slideDoorOpen   = false;
float slideDoorOffset = 0.0f;

// Upper room door state
bool upperDoorOpen   = false;
float upperDoorOffset = 0.0f;

// ─── Sky dome with Sun / Moon ──────────────────────────────────────────────
// Uses skybox technique: strips camera translation from the modelview matrix
// so celestial objects appear infinitely far away.  Drawn first with depth
// writes disabled; the scene then renders on top, naturally occluding the sky.
void drawSky()
{
    // ── Render state: sky is a background layer ────────────────────────
    setLighting(false);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Strip camera translation (keep rotation) → skybox effect
    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    glPushMatrix();
    mv[12] = mv[13] = mv[14] = 0.0f;
    glLoadMatrixf(mv);

    // ── Celestial body position on the sky sphere ──────────────────────
    // Fixed position: upper-right of the sky, visible above the roofline
    // from the default orbit camera.
    float skyR = 90.0f;
    float cx   =  skyR * 0.5f;       // to the right
    float cy   =  7.0f;              // just above roofline (~4° elevation)
    float cz   = -skyR * 0.7f;       // behind the shop (visible from front camera)

    if (isDayTime) {
        // ── Sun ────────────────────────────────────────────────────────
        // Glow halo (additive blend)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.95f, 0.5f, 0.18f);
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        gluSphere(quad, 8.0f, 16, 16);
        glPopMatrix();
        glDisable(GL_BLEND);

        // Sun body (bright golden-white)
        glColor3f(1.0f, 0.95f, 0.6f);
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        gluSphere(quad, 4.5f, 24, 24);
        glPopMatrix();

        // Hot core
        glColor3f(1.0f, 1.0f, 0.92f);
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        gluSphere(quad, 2.8f, 16, 16);
        glPopMatrix();
    } else {
        // ── Moon ───────────────────────────────────────────────────────
        // Glow halo
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(0.7f, 0.75f, 0.9f, 0.12f);
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        gluSphere(quad, 6.0f, 16, 16);
        glPopMatrix();
        glDisable(GL_BLEND);

        // Moon surface (pale silver)
        glColor3f(0.85f, 0.88f, 0.92f);
        glPushMatrix();
        glTranslatef(cx, cy, cz);
        gluSphere(quad, 3.5f, 24, 24);
        glPopMatrix();

        // Craters
        glColor3f(0.70f, 0.72f, 0.76f);
        glPushMatrix();
        glTranslatef(cx + 0.6f, cy + 0.9f, cz + 2.8f);
        gluSphere(quad, 0.7f, 10, 10);
        glPopMatrix();

        glColor3f(0.72f, 0.74f, 0.78f);
        glPushMatrix();
        glTranslatef(cx - 0.9f, cy - 0.5f, cz + 3.0f);
        gluSphere(quad, 0.55f, 10, 10);
        glPopMatrix();

        glColor3f(0.68f, 0.70f, 0.74f);
        glPushMatrix();
        glTranslatef(cx + 0.2f, cy - 1.1f, cz + 2.9f);
        gluSphere(quad, 0.45f, 10, 10);
        glPopMatrix();
    }

    // ── Stars (night only) ─────────────────────────────────────────────
    // Scattered across the sky hemisphere at varied elevations.
    // Low-elevation stars (Y ≈ 2-10) are visible in the default view;
    // higher ones appear when the camera tilts upward.
    if (!isDayTime) {
        glPointSize(2.5f);
        glBegin(GL_POINTS);
        const float stars[][3] = {
            // low-elevation band (visible by default)
            {  70,  4, -50 }, { -60,  3,  55 }, {  40,  6, -75 },
            { -80,  5,  30 }, {  55,  7, -60 }, { -45,  4,  70 },
            {  85,  3, -20 }, { -30,  8,  80 }, {  20,  5, -85 },
            { -70,  6,  45 }, {  65,  9, -40 }, { -50,  3,  65 },
            // mid-elevation (visible when looking up a bit)
            {  50, 18, -60 }, { -40, 22,  55 }, {  30, 20, -70 },
            { -65, 15,  35 }, {  45, 25, -50 }, { -55, 19,  40 },
            // high-elevation (visible in FPS mode looking up)
            {  20, 55, -30 }, { -25, 50,  20 }, {  10, 60, -15 },
            { -15, 45,  35 }, {  30, 48, -25 }, { -35, 52,  10 },
        };
        int n = sizeof(stars) / sizeof(stars[0]);
        for (int i = 0; i < n; i++) {
            float tw = 0.7f + 0.3f * sinf(animTime * (2.0f + i * 0.3f) + i * 1.7f);
            glColor3f(tw, tw, tw * 0.95f);
            glVertex3f(stars[i][0], stars[i][1], stars[i][2]);
        }
        glEnd();
        glPointSize(1.0f);
    }

    // ── Clouds (day only) ──────────────────────────────────────────────
    if (isDayTime) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float drift = animTime * 0.3f;

        struct Cloud { float x, y, z, sx, sy, sz; };
        Cloud clouds[] = {
            {  50 + drift,       6, -40, 12, 2.0f, 6 },
            { -45 + drift*0.7f,  8,  55, 14, 2.5f, 7 },
            {  30 + drift*0.5f,  5, -65, 10, 1.8f, 5 },
            { -60 + drift*0.8f,  7,  30, 11, 2.2f, 6 },
            {  70 + drift*0.6f,  9, -20, 15, 2.8f, 8 },
            { -30 + drift*0.9f,  5,  70, 10, 1.6f, 5 },
            {  15 + drift*0.4f, 10, -50, 13, 2.4f, 7 },
            { -75 + drift*0.5f,  6,  15,  9, 1.5f, 5 },
            {  40 + drift*0.3f,  7,  45, 11, 2.0f, 6 },
            { -20 + drift*0.8f,  8, -75, 16, 3.0f, 8 },
            {  80 + drift*0.4f,  5,  10,  8, 1.4f, 4 },
            { -55 + drift*0.6f, 11, -35, 12, 2.2f, 6 },
        };
        for (auto& c : clouds) {
            float wx = fmodf(c.x + 90, 180) - 90;
            glColor4f(1.0f, 1.0f, 1.0f, 0.50f);
            glPushMatrix();
            glTranslatef(wx, c.y, c.z);
            glScalef(c.sx, c.sy, c.sz);
            gluSphere(quad, 1.0f, 12, 12);
            glPopMatrix();
            glColor4f(1.0f, 1.0f, 1.0f, 0.40f);
            glPushMatrix();
            glTranslatef(wx + c.sx*0.35f, c.y + c.sy*0.3f, c.z);
            glScalef(c.sx*0.7f, c.sy*0.8f, c.sz*0.7f);
            gluSphere(quad, 1.0f, 10, 10);
            glPopMatrix();
        }
        glDisable(GL_BLEND);
    }

    glPopMatrix();   // restore full modelview (with translation)

    // ── Restore render state ───────────────────────────────────────────
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    setLighting(true);
    if (showFog) glEnable(GL_FOG);
}

// ─── Lighting is now in lighting.h / lighting.cpp ───────────────────────────

// ─── Floor Reflection (stencil-based ray-tracing approximation) ─────────────
// Renders key interior objects mirrored through y=0 as a semi-transparent
// warm overlay, simulating reflections on a polished wooden floor.
static void drawFloorReflection()
{
    disablePhongShader();   // uses special material overrides
    float FY         = FLOOR_Y;
    float counterTop = FY + 1.06f;

    // Only render where the floor platform was marked in the stencil buffer
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    // Blend on top of the already-drawn floor; disable depth test so the
    // reflected geometry isn't occluded by the floor surface it overlays.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    // Force all reflected geometry to a semi-transparent warm-wood silhouette
    glDisable(GL_COLOR_MATERIAL);
    GLfloat ra[] = { 0.50f, 0.38f, 0.24f, 0.28f };   // warm wood, 28% opaque
    GLfloat rs[] = { 0.00f, 0.00f, 0.00f, 0.00f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  ra);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  ra);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, rs);

    glPushMatrix();
    glScalef(1.0f, -1.0f, 1.0f);   // mirror through y = 0
    glFrontFace(GL_CW);             // correct face winding after mirror-flip

    drawCounter({ 0, FY, -0.5f });
    for (int i = 0; i < 6; i++)
        drawStool({ -2.8f + i * 1.12f, FY, 0.8f });
    for (int i = 0; i < 4; i++) {
        float x = -2.2f + i * 1.5f;
        drawRamenBowl({ x, counterTop, -0.3f }, NO_ROT, { 0.24f, 0.24f, 0.24f });
    }

    glFrontFace(GL_CCW);
    glPopMatrix();

    // Restore material and render state
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat noSpec[] = { 0.00f, 0.00f, 0.00f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    enablePhongShader();        // restore Phong after reflection pass
}

// ─── Projected Shadows (planar shadow-matrix projection) ────────────────────
// Projects the main interior objects onto the floor (y=0) from the warm
// overhead point light.  Uses a material override so all projected geometry
// renders as a dark, semi-transparent silhouette.
static void drawShadows()
{
    disablePhongShader();   // uses special material overrides
    float FY         = FLOOR_Y;
    float counterTop = FY + 1.06f;

    // Interior warm point-light position (matches GL_LIGHT1 dining pendant)
    const float lx = 0.3f, ly = 2.8f, lz = 0.0f;

    // Shadow projection matrix: project onto plane y = 0 (column-major)
    // Derived from M[i][j] = dot*δ[i][j] − L[i]*n_ext[j]
    // with n_ext = (0,1,0,0),  dot = ly = 2.8
    GLfloat shadowMat[16] = {
         ly,   0.0f,  0.0f,  0.0f,      // col 0
        -lx,   0.0f,  -lz,  -1.0f,      // col 1
         0.0f, 0.0f,   ly,   0.0f,      // col 2
         0.0f, 0.0f,  0.0f,   ly        // col 3
    };

    // Reuse the floor stencil (==1) and increment to prevent double-darkening
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Shadow darkness depends on time of day:
    //   Night: stronger shadows from artificial lights (45% darkening)
    //   Day:   softer diffused shadows (25% darkening)
    float shadowAlpha = isDayTime ? 0.25f : 0.45f;
    glDisable(GL_COLOR_MATERIAL);
    GLfloat sAmb[]  = { 0.0f, 0.0f, 0.0f, shadowAlpha };
    GLfloat sSpec[] = { 0.0f, 0.0f, 0.0f, 0.0f  };
    GLfloat sEm[]   = { 0.0f, 0.0f, 0.0f, 1.0f  };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  sAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  sAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, sEm);

    drawingShadow = true;     // prevents beginSphereReflect from firing

    glPushMatrix();
    glTranslatef(0.0f, 0.005f, 0.0f);   // nudge above floor to avoid z-fight
    glMultMatrixf(shadowMat);

    // Project key shadow-casting objects through the matrix
    drawCounter({ 0, FY, -0.5f });
    for (int i = 0; i < 6; i++)
        drawStool({ -2.8f + i * 1.12f, FY, 0.8f });
    for (int i = 0; i < 4; i++) {
        float x = -2.2f + i * 1.5f;
        drawRamenBowl({ x, counterTop, -0.3f }, NO_ROT, { 0.24f, 0.24f, 0.24f });
    }

    glPopMatrix();

    drawingShadow = false;

    // Restore render state
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat noSpec[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec);
    GLfloat noEm[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEm);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    enablePhongShader();        // restore Phong after shadow pass
}

// ─── Ground ─────────────────────────────────────────────────────────────────
void drawGround()
{
    drawSubdividedPlane({ 0, 0, 0 }, NO_ROT, { 80, 1, 80 }, { 0.12f, 0.14f, 0.10f }, 32, 32);

    // Draw floor platform and write stencil = 1 for every floor pixel
    // (used by drawFloorReflection to mask the reflection to the floor area)
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    drawTexturedBox({ 0, -0.20f, 0.5f }, NO_ROT, { 12.5f, 0.20f, 11.0f },
                    getTexID(TEX_DARK_WOOD), WHITE, 1.0f);
    glDisable(GL_STENCIL_TEST);

    drawCuboid({ 0, -0.10f, 5.5f }, NO_ROT, { 13.0f, 0.10f, 1.2f }, WOOD);
    drawSidewalk({ 0, 0, 7.0f });
    drawStreet({ 0, 0, 12.0f });

    // Polished-floor reflection overlay (stencil-masked to the floor platform)
    drawFloorReflection();
    // Projected shadows from the warm interior light (stencil prevents double-darkening)
    drawShadows();
}

// ─── Exterior ───────────────────────────────────────────────────────────────
void drawExterior()
{
    drawShopBuilding({ 0, 0, 0 });
    if (showRoof) drawRoof({ 0, 0, 0 });

    // Japanese paper (shoji) shopfront facade panels (replaces glass)
    drawShojiWindow({ -3.35f, 1.65f, 3.90f }, NO_ROT, ONE, 2.90f, 2.70f);
    drawShojiWindow({  3.35f, 1.65f, 3.90f }, NO_ROT, ONE, 2.90f, 2.70f);

    // Wooden entrance doors — sliding shoji (click to open/close)
    drawEntranceDoor(doorAngle);

    // Clear glass windows on side walls (left and right) — walls have matching openings
    drawClearGlassWindow({ -4.90f, 2.2f, 0.5f }, { 0, -90, 0 }, ONE, 2.40f, 2.00f);   // left wall center
    drawClearGlassWindow({  4.90f, 2.2f, 0.5f }, { 0,  90, 0 }, ONE, 2.40f, 2.00f);   // right wall center

    // Side wall window frames (wooden mullions around clear glass)
    for (int sx = -1; sx <= 1; sx += 2) {
        float fx = sx * 4.88f;
        drawCube({ fx, 3.22f, 0.5f }, NO_ROT, { 0.10f, 0.08f, 2.56f }, DARK_WOOD);   // top rail
        drawCube({ fx, 2.2f, -0.74f }, NO_ROT, { 0.10f, 2.08f, 0.08f }, DARK_WOOD);  // back post
        drawCube({ fx, 2.2f,  1.74f }, NO_ROT, { 0.10f, 2.08f, 0.08f }, DARK_WOOD);  // front post
        drawCube({ fx, 2.2f, 0.5f }, NO_ROT, { 0.06f, 2.0f, 0.04f }, DARK_WOOD);     // vertical mullion
        drawCube({ fx, 2.2f, 0.5f }, NO_ROT, { 0.06f, 0.04f, 2.4f }, DARK_WOOD);     // horizontal mullion
    }

    // Back wall clear glass windows with wooden frames (two windows at x=±3.5)
    for (int sx = -1; sx <= 1; sx += 2) {
        float wx = sx * 3.5f;
        float wy = 1.8f;
        float wz = -3.85f;
        float wW = 1.2f, wH = 1.0f;
        float hW = wW * 0.5f, hH = wH * 0.5f;
        float ft = 0.08f;

        // Glass pane
        drawClearGlassWindow({ wx, wy, wz }, NO_ROT, ONE, wW, wH);

        // Frame: top/bottom rails and left/right stiles
        drawCube({ wx, wy + hH + ft * 0.5f, wz }, NO_ROT,
                 { wW + ft * 2, ft, 0.12f }, DARK_WOOD);
        drawCube({ wx, wy - hH - ft * 0.5f, wz }, NO_ROT,
                 { wW + ft * 2, ft, 0.12f }, DARK_WOOD);
        drawCube({ wx - hW - ft * 0.5f, wy, wz }, NO_ROT,
                 { ft, wH, 0.12f }, DARK_WOOD);
        drawCube({ wx + hW + ft * 0.5f, wy, wz }, NO_ROT,
                 { ft, wH, 0.12f }, DARK_WOOD);
        // Cross mullions
        drawCube({ wx, wy, wz }, NO_ROT, { 0.04f, wH, 0.06f }, DARK_WOOD);
        drawCube({ wx, wy, wz }, NO_ROT, { wW, 0.04f, 0.06f }, DARK_WOOD);
        // Window sill
        drawCube({ wx, wy - hH - ft - 0.03f, wz + 0.06f }, NO_ROT,
                 { wW + 0.20f, 0.05f, 0.20f }, WOOD);
    }

    // Shoji windows on the side walls (rear sections)
    drawShojiWindow({ -4.90f, 2.2f, -2.0f }, { 0, -90, 0 }, ONE, 1.6f, 1.2f);   // left wall, rear
    drawShojiWindow({  4.90f, 2.2f, -2.0f }, { 0,  90, 0 }, ONE, 1.6f, 1.2f);   // right wall, rear

    // Sliding shoji door on the left side wall (click to open/close)
    drawSlidingShoji({ -4.90f, 1.5f, 2.8f }, { 0, -90, 0 }, slideDoorOffset, 2.4f, 2.8f);

    // Second floor front facade: two shoji windows flush with the outer wall face
    drawShojiWindow({ -3.2f, 4.3f, 4.02f }, NO_ROT, ONE, 2.0f, 1.3f);
    drawShojiWindow({  3.2f, 4.3f, 4.02f }, NO_ROT, ONE, 2.0f, 1.3f);
    // Ramen sign mounted above the roofline (above the gable peak)
    drawSignBoard({ 0, 7.6f, 4.05f });

    float flickL = 1.0f + 0.08f * sin(animTime * 4.9f);
    float flickR = 1.0f + 0.08f * sin(animTime * 4.6f + 1.5f);
    float swayL  = sin(animTime * 2.0f) * 2.5f;
    float swayR  = sin(animTime * 1.9f + 0.8f) * 2.5f;

    // Left lantern — selectable as OBJ_LANTERN_L (warm orange chochin)
    //   Emissive surface + additive glow halo → visible glowing lantern
    float dayLanScale = isDayTime ? 0.15f : 1.0f;
    glPushMatrix();
    glTranslatef(-3.8f, 2.95f, 4.5f);
    applyObjDelta(OBJ_LANTERN_L);
    glRotatef(swayL, 0, 0, 1);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.02f, 0.4f, 0.02f }, BLACK);
    // Glowing lantern body (warm orange-amber, matching LIGHT2 color)
    setEmission(0.95f * flickL * dayLanScale, 0.55f * flickL * dayLanScale, 0.10f * flickL * dayLanScale);
    drawSphere({ 0, -0.35f, 0 }, NO_ROT, { 0.55f, 0.65f, 0.55f }, { 0.95f, 0.65f, 0.15f });
    clearEmission();
    // Additive glow halo around lantern (visible light spill)
    if (!isDayTime) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);
        glColor4f(0.95f, 0.55f, 0.10f, 0.08f * flickL);
        gluSphere(quad, 0.55f, 12, 12);
        glDepthMask(GL_TRUE);
        setLighting(true);
        glDisable(GL_BLEND);
    }
    glPopMatrix();

    // Right lantern — selectable as OBJ_LANTERN_R (warm orange chochin)
    glPushMatrix();
    glTranslatef(3.8f, 2.95f, 4.5f);
    applyObjDelta(OBJ_LANTERN_R);
    glRotatef(swayR, 0, 0, 1);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.02f, 0.4f, 0.02f }, BLACK);
    setEmission(0.95f * flickR * dayLanScale, 0.55f * flickR * dayLanScale, 0.10f * flickR * dayLanScale);
    drawSphere({ 0, -0.35f, 0 }, NO_ROT, { 0.55f, 0.65f, 0.55f }, { 0.95f, 0.65f, 0.15f });
    clearEmission();
    if (!isDayTime) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);
        glColor4f(0.95f, 0.55f, 0.10f, 0.08f * flickR);
        gluSphere(quad, 0.55f, 12, 12);
        glDepthMask(GL_TRUE);
        setLighting(true);
        glDisable(GL_BLEND);
    }
    glPopMatrix();

    for (int i = -5; i <= 5; i++)
        drawFence({ i * 1.0f, 0, 5.8f });

    drawPlant({ -4.5f, 0, 4.8f }, NO_ROT, { 1.2f, 1.2f, 1.2f });
    drawPlant({  4.5f, 0, 4.8f }, NO_ROT, { 1.2f, 1.2f, 1.2f });
    drawPlant({ -2.5f, 0, 5.4f });
    drawPlant({  2.5f, 0, 5.4f });

    drawTree({ -7.0f, 0,  2.0f }, NO_ROT, { 1.0f, 1.3f, 1.0f });
    drawTree({  7.0f, 0,  3.0f }, NO_ROT, { 1.2f, 1.5f, 1.2f });
    drawTree({ -6.5f, 0, -2.0f }, NO_ROT, { 0.8f, 1.1f, 0.8f });
    drawTree({  7.5f, 0, -1.5f });

    // Cherry blossom tree prominently displayed to the left of the shop entrance
    drawCherryBlossomTree({ -5.5f, 0, 6.2f }, NO_ROT, { 1.0f, 1.1f, 1.0f });

    drawLamp({ -7.0f, 0, 7.0f }, { 0, -90, 0 });
    drawLamp({  7.0f, 0, 7.0f }, { 0, -90, 0 });

    // Vending machine to the right of the shop entrance, facing the sidewalk
    drawVendingMachine({ 8.5f, 0, 3.5f }, { 0, -90, 0 });

    // ── Extended Outdoor Environment ─────────────────────────────────────

    // Japanese pine trees scattered around the neighborhood
    drawJapanesePineTree({ -12.0f, 0,  8.0f }, NO_ROT, { 1.0f, 1.2f, 1.0f });
    drawJapanesePineTree({  14.0f, 0, -5.0f }, NO_ROT, { 0.9f, 1.0f, 0.9f });
    drawJapanesePineTree({ -15.0f, 0, -8.0f }, NO_ROT, { 1.1f, 1.4f, 1.1f });
    drawJapanesePineTree({  18.0f, 0, 10.0f }, NO_ROT, { 0.8f, 0.9f, 0.8f });

    // Maple trees with autumn foliage
    drawMapleTree({ -10.0f, 0,  12.0f }, NO_ROT, { 1.0f, 1.0f, 1.0f });
    drawMapleTree({  12.0f, 0,  14.0f }, NO_ROT, { 1.1f, 1.2f, 1.1f });
    drawMapleTree({ -18.0f, 0,  -3.0f }, NO_ROT, { 0.9f, 0.9f, 0.9f });

    // Bamboo groves flanking the path
    drawBambooGrove({ -14.0f, 0,  3.0f }, NO_ROT, { 1.0f, 1.0f, 1.0f });
    drawBambooGrove({  16.0f, 0,  6.0f }, { 0, 30, 0 }, { 0.9f, 1.1f, 0.9f });

    // Japanese houses along the street
    drawJapaneseHouse({ -20.0f, 0,  8.0f }, { 0, 45, 0 },  { 1.0f, 1.0f, 1.0f });
    drawJapaneseHouse({  22.0f, 0,  5.0f }, { 0, -90, 0 }, { 1.1f, 1.0f, 1.1f });
    drawJapaneseHouse({ -22.0f, 0, -6.0f }, { 0, 60, 0 },  { 0.9f, 0.9f, 0.9f });
    drawJapaneseHouse({  20.0f, 0, -8.0f }, { 0, -45, 0 }, { 1.0f, 1.1f, 1.0f });

    // Lake behind the shop with a bridge crossing it
    drawLake({ 0, -0.1f, -18.0f }, NO_ROT, { 3.0f, 1.0f, 2.5f });
    drawBridge({ 0, 0, -18.0f }, NO_ROT, { 1.2f, 1.0f, 1.0f });

    // Stone lanterns around the lake and garden paths
    drawStoneLantern({  4.0f, 0, -14.0f });
    drawStoneLantern({ -4.0f, 0, -14.0f });
    drawStoneLantern({  6.0f, 0, -20.0f });
    drawStoneLantern({ -6.0f, 0, -20.0f });

    // Grass patches scattered around the grounds
    drawGrassPatch({ -8.0f,  0.01f,  10.0f }, NO_ROT, { 2.0f, 1.0f, 2.0f });
    drawGrassPatch({  9.0f,  0.01f,  12.0f }, NO_ROT, { 1.5f, 1.0f, 1.5f });
    drawGrassPatch({ -6.0f,  0.01f, -10.0f }, NO_ROT, { 2.5f, 1.0f, 2.5f });
    drawGrassPatch({  5.0f,  0.01f, -12.0f }, NO_ROT, { 1.8f, 1.0f, 1.8f });
    drawGrassPatch({ -12.0f, 0.01f,  14.0f }, NO_ROT, { 2.0f, 1.0f, 2.0f });
    drawGrassPatch({  14.0f, 0.01f,  -2.0f }, NO_ROT, { 1.6f, 1.0f, 1.6f });
    drawGrassPatch({  0.0f,  0.01f, -14.0f }, NO_ROT, { 3.0f, 1.0f, 3.0f });
    drawGrassPatch({ -16.0f, 0.01f,   0.0f }, NO_ROT, { 2.2f, 1.0f, 2.2f });

    // Jungle area off to one side
    drawJungle({ -25.0f, 0, -15.0f }, NO_ROT, { 1.5f, 1.0f, 1.5f });
    drawJungle({  25.0f, 0, -12.0f }, { 0, 90, 0 }, { 1.2f, 1.0f, 1.2f });

    // Fireflies near the lake and around the garden (night only, handled inside the function)
    drawFireflies({ 0, 1.5f, -18.0f }, 8.0f, 25);
    drawFireflies({ -5.0f, 1.0f, -15.0f }, 5.0f, 15);
    drawFireflies({  5.0f, 1.0f, -15.0f }, 5.0f, 15);
    drawFireflies({ -10.0f, 1.2f, 8.0f }, 4.0f, 10);
    drawFireflies({  10.0f, 1.2f, 8.0f }, 4.0f, 10);
}

// ─── Second Floor ────────────────────────────────────────────────────────────
static void drawSecondFloor()
{
    const float GH = 3.3f;   // base y of second floor (top of ground floor)
    const float UH = 2.0f;   // upper floor wall height
    const float floorY2 = GH + 0.12f;  // 3.42 — walking surface

    // Floor/ceiling slab — split into 4 pieces, leaving a staircase opening
    //   Opening: x [3.5, 4.5], z [-0.5, 2.5]
    const Color SLAB_TINT = { 0.62f, 0.48f, 0.30f };
    GLuint slabTex = getTexID(TEX_DARK_WOOD);
    drawTexturedBox({ -0.7f, GH,  0.0f }, NO_ROT, { 8.4f, 0.12f, 7.8f }, slabTex, SLAB_TINT, 1.5f);
    drawTexturedBox({  4.7f, GH,  0.0f }, NO_ROT, { 0.4f, 0.12f, 7.8f }, slabTex, SLAB_TINT, 1.5f);
    drawTexturedBox({  4.0f, GH, -2.2f }, NO_ROT, { 1.0f, 0.12f, 3.4f }, slabTex, SLAB_TINT, 1.5f);
    drawTexturedBox({  4.0f, GH,  3.2f }, NO_ROT, { 1.0f, 0.12f, 1.4f }, slabTex, SLAB_TINT, 1.5f);

    // ── Clean polished light wood bedroom floor (single surface, no grid lines) ──
    const Color BEDROOM_FLOOR = { 0.90f, 0.82f, 0.65f };
    drawTexturedBox({ 0, floorY2, 0 }, NO_ROT, { 8.0f, 0.06f, 7.0f },
                    getTexID(TEX_DARK_WOOD), BEDROOM_FLOOR, 2.0f);

    // ══════════════════════════════════════════════════════════════════════
    //   BEDROOM  — Japanese futon bed, nightstand, wardrobe, folding screen
    // ══════════════════════════════════════════════════════════════════════
    const float TY = floorY2 + 0.06f;

    // ── Futon Bed (shikibuton + kakebuton + makura) ──
    // Shikibuton — thick cotton mattress pad
    const Color FUTON_WHITE = { 0.95f, 0.93f, 0.88f };
    setMaterialPBR(Materials::Fabric, FUTON_WHITE);
    drawCuboid({ 0.8f, TY, 0.0f }, NO_ROT, { 1.1f, 0.10f, 2.1f }, FUTON_WHITE);
    resetMaterialGloss();

    // Kakebuton — duvet / blanket (deep indigo blue)
    const Color FUTON_INDIGO = { 0.12f, 0.14f, 0.28f };
    setMaterialPBR(Materials::Fabric, FUTON_INDIGO);
    drawCuboid({ 0.8f, TY + 0.10f, -0.25f }, NO_ROT, { 1.05f, 0.06f, 1.5f }, FUTON_INDIGO);
    // Folded-back edge near pillow
    drawCuboid({ 0.8f, TY + 0.14f, 0.55f }, NO_ROT, { 1.0f, 0.04f, 0.20f }, FUTON_INDIGO);
    resetMaterialGloss();

    // Makura — buckwheat pillow
    const Color PILLOW_CREAM = { 0.94f, 0.91f, 0.84f };
    setMaterialPBR(Materials::Fabric, PILLOW_CREAM);
    drawCuboid({ 0.8f, TY + 0.08f, 0.85f }, NO_ROT, { 0.50f, 0.12f, 0.25f }, PILLOW_CREAM);
    resetMaterialGloss();

    // ── Bedside Tansu (nightstand) ──
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCuboid({ 2.0f, TY, 0.9f }, NO_ROT, { 0.55f, 0.40f, 0.45f }, DARK_WOOD);
    drawCuboid({ 2.0f, TY + 0.40f, 0.9f }, NO_ROT, { 0.60f, 0.03f, 0.50f }, WOOD);
    resetMaterialGloss();
    // Drawer divider
    drawCuboid({ 2.0f, TY + 0.19f, 1.125f }, NO_ROT, { 0.48f, 0.006f, 0.006f }, LIGHT_WOOD);
    // Drawer knobs
    setMaterialPBR(Materials::Gold, GOLD);
    drawSphere({ 2.0f, TY + 0.30f, 1.13f }, NO_ROT, { 0.025f, 0.025f, 0.025f }, GOLD);
    drawSphere({ 2.0f, TY + 0.10f, 1.13f }, NO_ROT, { 0.025f, 0.025f, 0.025f }, GOLD);
    resetMaterialGloss();
    // Book on nightstand
    drawCuboid({ 1.95f, TY + 0.43f, 0.85f }, { 0, 15, 0 }, { 0.18f, 0.025f, 0.13f }, DARK_RED);

    // ── Low Wardrobe Tansu (against back wall) ──
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCuboid({ 2.0f, TY, -3.35f }, NO_ROT, { 1.6f, 0.75f, 0.55f }, DARK_WOOD);
    drawCuboid({ 2.0f, TY + 0.75f, -3.35f }, NO_ROT, { 1.68f, 0.04f, 0.60f }, WOOD);
    resetMaterialGloss();
    // Metal corner brackets
    for (int sx = -1; sx <= 1; sx += 2)
        drawCuboid({ 2.0f + sx * 0.72f, TY + 0.74f, -3.07f }, NO_ROT,
                   { 0.08f, 0.06f, 0.02f }, METAL);
    // Drawer handles
    setMaterialPBR(Materials::Gold, GOLD);
    for (int r = 0; r < 2; r++)
        for (int sx = -1; sx <= 1; sx += 2)
            drawSphere({ 2.0f + sx * 0.40f, TY + 0.22f + r * 0.30f, -3.07f }, NO_ROT,
                       { 0.025f, 0.025f, 0.025f }, GOLD);
    resetMaterialGloss();

    // ── Zabuton cushion (beside tokonoma alcove) ──
    const Color ZABUTON = { 0.55f, 0.12f, 0.12f };
    setMaterialPBR(Materials::Fabric, ZABUTON);
    drawCuboid({ -3.0f, TY, -2.5f }, { 0, 15, 0 }, { 0.50f, 0.05f, 0.45f }, ZABUTON);
    resetMaterialGloss();
    // Tea cup near cushion
    drawCup({ -2.6f, TY, -2.2f }, NO_ROT, { 0.09f, 0.09f, 0.09f }, CUP_GREEN);

    // ── Folding Screen (byobu) — 3-panel room divider ──
    {
        const Color SCREEN_GOLD = { 0.88f, 0.82f, 0.62f };
        float scY = TY + 0.62f;
        float scH = 1.20f, scW = 0.65f;
        float px[] = { -2.6f, -1.95f, -1.3f };
        float pz[] = {  1.7f,  1.6f,   1.7f };
        float pa[] = {  12.0f, 0.0f, -12.0f };
        for (int i = 0; i < 3; i++) {
            if (!isDayTime)
                setEmission(0.05f, 0.04f, 0.02f);
            else
                setEmission(0.02f, 0.02f, 0.01f);
            drawCube({ px[i], scY, pz[i] }, { 0, pa[i], 0 },
                     { scW, scH, 0.03f }, SCREEN_GOLD);
            clearEmission();
            drawCube({ px[i], scY + scH * 0.5f + 0.02f, pz[i] }, { 0, pa[i], 0 },
                     { scW + 0.04f, 0.04f, 0.05f }, DARK_WOOD);
            drawCube({ px[i], scY - scH * 0.5f - 0.02f, pz[i] }, { 0, pa[i], 0 },
                     { scW + 0.04f, 0.04f, 0.05f }, DARK_WOOD);
        }
    }

    // ── Simple flush ceiling light (paper dome — no hanging cords) ──
    //   The dome itself glows visibly, matching LIGHT7 color temperature
    if (lightArea) {
        float dayDome = isDayTime ? 0.20f : 1.0f;
        float flickDome = 1.0f + 0.03f * sinf(animTime * 3.0f);
        setEmission(0.80f * dayDome * flickDome,
                    0.58f * dayDome * flickDome,
                    0.20f * dayDome * flickDome);
    }
    drawSphere({ 0.0f, GH + UH - 0.05f, 0.0f }, NO_ROT, { 0.30f, 0.10f, 0.30f }, PAPER);
    if (lightArea) clearEmission();

    // ── Tokonoma alcove shelf on the back wall ──
    drawCuboid({ -3.5f, floorY2 + 0.50f, -3.55f }, NO_ROT, { 1.8f, 0.06f, 0.5f }, DARK_WOOD);

    // Scroll painting above the tokonoma shelf (kakejiku)
    if (!isDayTime)
        setEmission(0.06f, 0.04f, 0.02f);
    else
        setEmission(0.02f, 0.02f, 0.01f);
    drawCuboid({ -3.5f, floorY2 + 1.20f, -3.72f }, NO_ROT, { 0.9f, 1.0f, 0.04f }, PAPER);
    clearEmission();
    drawCuboid({ -3.5f, floorY2 + 1.74f, -3.71f }, NO_ROT, { 1.0f, 0.06f, 0.06f }, DARK_WOOD);  // top rod
    drawCuboid({ -3.5f, floorY2 + 0.66f, -3.71f }, NO_ROT, { 1.0f, 0.06f, 0.06f }, DARK_WOOD);  // bottom rod

    // Small ikebana vase on the tokonoma shelf
    const Color VASE_BLUE = { 0.20f, 0.25f, 0.55f };
    setMaterialPBR(Materials::Ceramic, VASE_BLUE);
    drawCylinder({ -3.5f, floorY2 + 0.56f, -3.45f }, NO_ROT, { 0.06f, 0.18f, 0.06f }, VASE_BLUE);
    drawSphere({   -3.5f, floorY2 + 0.82f, -3.45f }, NO_ROT, { 0.04f, 0.06f, 0.04f }, LEAF);  // leaf
    resetMaterialGloss();

    // ── Wall decoration on the right-side inner wall ──
    drawWallDecoration({ 2.5f, floorY2 + 1.0f, -3.72f }, NO_ROT);

    // ── Floor lamp in the far corner ──
    drawJapaneseFloorLanternTower({ -4.1f, floorY2, -3.0f }, NO_ROT, { 0.7f, 0.7f, 0.7f }, lightPoint);

    // ── Clickable sliding shoji door at the staircase entrance ──
    {
        const float doorW = 1.2f, doorH = 1.9f;
        const float doorX = 3.50f;
        const float doorZ = -0.1f;

        // Frame posts
        drawCuboid({ doorX, floorY2, doorZ - doorW * 0.5f - 0.06f },
                   NO_ROT, { 0.10f, doorH + 0.10f, 0.10f }, DARK_WOOD);
        drawCuboid({ doorX, floorY2, doorZ + doorW * 0.5f + 0.06f },
                   NO_ROT, { 0.10f, doorH + 0.10f, 0.10f }, DARK_WOOD);
        // Lintel
        drawCuboid({ doorX, floorY2 + doorH, doorZ },
                   NO_ROT, { 0.10f, 0.15f, doorW + 0.24f }, DARK_WOOD);
        // Sliding shoji panels (click to open/close)
        drawSlidingShoji({ doorX, floorY2 + doorH * 0.5f, doorZ },
                         { 0, 90, 0 }, upperDoorOffset, doorW, doorH);
    }
}

// ─── Interior ───────────────────────────────────────────────────────────────
void drawInterior()
{
    float FY = FLOOR_Y;

    drawCounter({ 0, FY, -0.5f });
    float counterTop = FY + 1.06f;

    // 6 stools — first one is selectable as OBJ_STOOL
    for (int i = 0; i < 6; i++) {
        float x = -2.8f + i * 1.12f;
        if (i == 0) {
            glPushMatrix();
            glTranslatef(x, FY, 0.8f);
            applyObjDelta(OBJ_STOOL);
            drawStool({ 0, 0, 0 });
            glPopMatrix();
        } else {
            drawStool({ x, FY, 0.8f });
        }
    }

    // 4 ramen sets — first bowl + chopsticks selectable as OBJ_BOWL
    for (int i = 0; i < 4; i++) {
        float x = -2.2f + i * 1.5f;
        if (i == 0) {
            glPushMatrix();
            glTranslatef(x, counterTop, -0.3f);
            applyObjDelta(OBJ_BOWL);
            drawRamenBowl({ 0, 0, 0 }, NO_ROT, { 0.24f, 0.24f, 0.24f });
            drawChopsticks({ 0.18f, 0.005f, -0.12f }, { 0, 25, 0 }, { 0.24f, 0.24f, 0.24f });
            if (showSteam) drawSteam({ 0, 0.09f, 0 }, 0.22f, 0.0f);
            glPopMatrix();
        } else {
            drawRamenBowl({ x, counterTop, -0.3f }, NO_ROT, { 0.24f, 0.24f, 0.24f });
            drawChopsticks({ x + 0.18f, counterTop + 0.005f, -0.12f },
                           { 0, 25, 0 }, { 0.24f, 0.24f, 0.24f });
            if (showSteam) drawSteam({ x, counterTop + 0.09f, -0.3f }, 0.22f, (float)i * 1.35f);
        }
    }

    // ── Overhead Lighting Fixtures (Spotlight, Area Light, Pendant Glass Lamps) ──
    // Ceiling Track Spotlight aimed at the chef prep station
    drawSpotlightFixture({ -1.5f, 3.25f, -0.3f }, NO_ROT, ONE, lightSpot);
    if (lightSpot) {
        drawSpotlightBeam({ -1.5f, 3.25f, -0.3f }, 2.05f, 0.08f, 0.85f);
    }

    // Ceiling Rectangular Area Light Luminaire (Softbox) above kitchen and counter
    drawAreaLightFixture({ 0.0f, 3.22f, -1.2f }, NO_ROT, ONE, lightArea);

    // Clear Glass Pendant Lamps hanging above the dining counter
    drawPendantGlassLamp({ -1.2f, 2.85f, -0.2f }, NO_ROT, ONE, lightPoint);
    drawPendantGlassLamp({  1.2f, 2.85f, -0.2f }, NO_ROT, ONE, lightPoint);

    // Japanese Box Lantern Cluster — andon-style washi-paper lanterns above dining area
    drawJapaneseBoxLanternCluster({ 0.5f, 3.30f, 0.55f }, NO_ROT, ONE, lightPoint);

    // ── Clear Glass Drinking Tumblers at each dining seat (with water & ice) ──
    drawClearGlassTumbler({ -1.85f, counterTop + 0.06f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
    drawClearGlassTumbler({ -0.35f, counterTop + 0.06f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
    drawClearGlassTumbler({  1.15f, counterTop + 0.06f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
    drawClearGlassTumbler({  2.65f, counterTop + 0.06f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);

    // ── Clear Glass Water Pitcher / Carafe with fresh lemon slice ─────────────
    drawClearGlassPitcher({ 0.35f, counterTop + 0.06f, -0.36f }, NO_ROT, { 0.30f, 0.30f, 0.30f });

    // ── Clear Glass Condiment / Spice Jars ────────────────────────────────────
    drawClearGlassJar({ 1.65f, counterTop + 0.06f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.85f, 0.20f, 0.10f }); // shichimi chili
    drawClearGlassJar({ 1.90f, counterTop + 0.06f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.95f, 0.90f, 0.70f }); // pickled garlic
    drawClearGlassJar({ 2.15f, counterTop + 0.06f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.20f, 0.18f, 0.15f }); // sesame seeds

    drawBottle({  2.7f, counterTop, -0.35f }, NO_ROT, { 0.3f, 0.3f, 0.3f }, SOY);
    drawBottle({  2.9f, counterTop, -0.25f }, NO_ROT, { 0.3f, 0.3f, 0.3f }, RED, GOLD);
    drawCup({ -2.7f, counterTop, -0.30f }, NO_ROT, { 0.1f, 0.1f, 0.1f }, CUP_GREEN);

    // Cash register at the far-right end of the service counter
    drawCashRegister({ 3.1f, counterTop, -0.38f }, { 0, 180, 0 });

    drawKitchen({ 0, FY, -3.2f });

    // Steam above cooking pots (world coordinates matching centered pot positions)
    float kitchenTop = FY + 0.94f;
    if (showSteam) {
        drawSteam({ -0.85f, kitchenTop + 0.65f, -3.2f }, 0.40f, 0.4f);   // large pot left burner
        drawSteam({  0.0f,  kitchenTop + 0.65f, -3.2f }, 0.38f, 1.8f);   // large pot center burner
        drawSteam({  0.85f, kitchenTop + 0.42f, -3.2f }, 0.25f, 3.2f);   // small pot right burner
        drawSteam({  1.6f,  kitchenTop + 0.38f, -3.2f }, 0.30f, 5.0f);   // noodle station
    }

    drawHangingLantern({ -1.8f, 3.1f, 0.0f }, NO_ROT, { 0.8f, 0.8f, 0.8f });
    drawHangingLantern({  1.8f, 3.1f, 0.0f }, NO_ROT, { 0.8f, 0.8f, 0.8f });

    // Noren curtain — selectable as OBJ_NOREN
    glPushMatrix();
    glTranslatef(0, 3.0f, -1.3f);
    applyObjDelta(OBJ_NOREN);
    drawNoren({ 0, 0, 0 });
    glPopMatrix();

    // Wall clock centered on the back wall (below second floor boundary)
    drawWallClock({ 0.0f, 2.7f, -3.78f }, NO_ROT, { 0.55f, 0.55f, 0.55f });

    drawWallDecoration({ -4.77f, 2.0f, 2.8f }, { 0,  90, 0 });
    drawWallDecoration({  4.77f, 2.0f, 2.8f }, { 0, -90, 0 });

    drawCuboid({ -3.8f, FY, 0.5f },        NO_ROT, { 0.4f,  0.35f, 0.3f  }, WOOD);
    drawCuboid({ -3.8f, FY + 0.35f, 0.5f }, NO_ROT, { 0.45f, 0.04f, 0.35f }, LIGHT_WOOD);

    drawPlant({ 4.3f, FY, 2.0f });

    // Japanese floor lantern tower — left-front corner of the dining area
    drawJapaneseFloorLanternTower({ -4.1f, FY, 3.2f }, NO_ROT, ONE, lightPoint);

    // Staircase — right side of dining area, 10 solid-block steps rising to second floor
    {
        const float SW  = 1.0f;    // stair width  (x)
        const float SH  = 0.332f;  // riser height per step
        const float SD  = 0.30f;   // tread depth  (z)
        const float SX  = 4.0f;    // center X (near right wall)
        const float SZ0 = 2.5f;    // Z of front face of first step
        const int   NS  = 10;      // number of steps

        setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
        for (int i = 0; i < NS; i++) {
            // Each block fills from FY up to this step's tread level
            drawCuboid({ SX, FY, SZ0 - i * SD - SD * 0.5f }, NO_ROT,
                       { SW, (i + 1) * SH, SD }, DARK_WOOD);
        }
        resetMaterialGloss();
    }

    // Second floor: tatami room with low table and tea
    drawSecondFloor();
}
