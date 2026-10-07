#include "scene.h"
#include "lighting.h"
#include "objects.h"
#include "shader.h"
#include "game.h"

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

    // ── Sky gradient dome ───────────────────────────────────────────────
    // Day: deep blue overhead fading to a pale hazy horizon (the horizon colour equals
    // the fog colour, so distant ground melts into it).  Night: dark navy overhead.
    {
        const float hz[3] = { isDayTime ? 0.64f : 0.10f, isDayTime ? 0.79f : 0.10f, isDayTime ? 0.93f : 0.16f };
        const float zn[3] = { isDayTime ? 0.17f : 0.015f, isDayTime ? 0.40f : 0.02f, isDayTime ? 0.80f : 0.07f };
        const int SL = 36, ST = 14;
        const float R = 150.0f;
        for (int st = 0; st < ST; st++) {
            glBegin(GL_TRIANGLE_STRIP);                                                   // one strip per ring
            float e0 = -0.17f + 1.74f * st / ST, e1 = -0.17f + 1.74f * (st + 1) / ST;     // radians, -10..90 deg
            float t0 = e0 <= 0.0f ? 0.0f : powf(e0 / 1.5708f, 0.55f), t1 = e1 <= 0.0f ? 0.0f : powf(e1 / 1.5708f, 0.55f);
            for (int j = 0; j <= SL; j++) {
                float a = j * 6.2832f / SL;
                for (int k = 0; k < 2; k++) {
                    float e = k ? e1 : e0, t = k ? t1 : t0;
                    glColor3f(hz[0] + (zn[0] - hz[0]) * t, hz[1] + (zn[1] - hz[1]) * t, hz[2] + (zn[2] - hz[2]) * t);
                    glVertex3f(R * cosf(e) * sinf(a), R * sinf(e), -R * cosf(e) * cosf(a));
                }
            }
            glEnd();
        }
    }

    // ── Sun / moon: bright core with soft glowing halos ────────────────
    // Fixed position: upper-right of the sky, visible above the roofline
    // from the default orbit camera.
    {
        float px = 45.0f, py = 7.0f, pz = -63.0f;
        float l = sqrtf(px * px + py * py + pz * pz);
        float dx = px / l, dy = py / l, dz = pz / l;
        float rx = dz, ry = 0.0f, rz = -dx;                        // right = up x dir (horizontal)
        float rl = sqrtf(rx * rx + rz * rz); rx /= rl; rz /= rl;
        float ux = dy * rz - dz * ry, uy = dz * rx - dx * rz, uz = dx * ry - dy * rx;   // up = dir x right

        // glowing disc (fan) in the plane facing the camera
        auto disc = [&](float rad, float r, float g, float b, float aCentre, float aEdge, float push) {
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(r, g, b, aCentre);
            glVertex3f(px + dx * push, py + dy * push, pz + dz * push);
            glColor4f(r, g, b, aEdge);
            for (int i = 0; i <= 40; i++) {
                float t = i * 6.2832f / 40.0f, cx_ = cosf(t) * rad, cy_ = sinf(t) * rad;
                glVertex3f(px + dx * push + rx * cx_ + ux * cy_, py + dy * push + ry * cx_ + uy * cy_, pz + dz * push + rz * cx_ + uz * cy_);
            }
            glEnd();
        };
        glEnable(GL_BLEND);
        if (isDayTime) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);                       // additive glow
            disc(26.0f, 1.00f, 0.92f, 0.65f, 0.12f, 0.0f, 0.0f);     // wide warm atmosphere glow
            disc(13.0f, 1.00f, 0.95f, 0.72f, 0.22f, 0.0f, 0.1f);
            disc(7.0f,  1.00f, 0.97f, 0.80f, 0.35f, 0.0f, 0.2f);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            disc(3.0f, 1.00f, 0.99f, 0.90f, 1.0f, 1.0f, 0.3f);       // bright core
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            disc(4.0f, 1.00f, 1.00f, 0.95f, 0.30f, 0.0f, 0.4f);      // soft bloom over the edge
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            disc(24.0f, 0.55f, 0.65f, 0.95f, 0.08f, 0.0f, 0.0f);     // cool wide glow
            disc(11.0f, 0.70f, 0.78f, 1.00f, 0.16f, 0.0f, 0.1f);
            disc(6.0f,  0.85f, 0.90f, 1.00f, 0.26f, 0.0f, 0.2f);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            disc(2.8f, 0.90f, 0.92f, 0.96f, 1.0f, 1.0f, 0.3f);       // moon disc
            // maria / craters: slightly darker patches on the disc
            const float cr[6][3] = { { -1.1f, 1.0f, 0.95f }, { 1.0f, 0.6f, 0.65f }, { 0.2f, -0.9f, 0.85f },
                                     { -0.6f, -0.2f, 0.45f }, { 1.2f, -0.8f, 0.4f }, { 0.4f, 1.6f, 0.4f } };
            for (int i = 0; i < 6; i++) {
                glBegin(GL_TRIANGLE_FAN);
                glColor4f(0.62f, 0.66f, 0.74f, 0.55f);
                float cx_ = cr[i][0] * 0.74f, cy_ = cr[i][1] * 0.74f, rr = cr[i][2] * 0.74f;
                glVertex3f(px + dx * 0.4f + rx * cx_ + ux * cy_, py + dy * 0.4f + ry * cx_ + uy * cy_, pz + dz * 0.4f + rz * cx_ + uz * cy_);
                glColor4f(0.62f, 0.66f, 0.74f, 0.0f);
                for (int j = 0; j <= 16; j++) {
                    float t = j * 6.2832f / 16.0f;
                    float ox = cx_ + cosf(t) * rr, oy = cy_ + sinf(t) * rr;
                    glVertex3f(px + dx * 0.4f + rx * ox + ux * oy, py + dy * 0.4f + ry * ox + uy * oy, pz + dz * 0.4f + rz * ox + uz * oy);
                }
                glEnd();
            }
        }
        glDisable(GL_BLEND);
    }

    // ── Stars (night only) ─────────────────────────────────────────────
    // Scattered across the sky hemisphere at varied elevations.
    // Low-elevation stars (Y ≈ 2-10) are visible in the default view;
    // higher ones appear when the camera tilts upward.
    if (!isDayTime) {
        // ── Bright stars (larger points) ──────────────────────────────
        const float brightStars[][3] = {
            // low-elevation band (visible by default)
            {  70,  4, -50 }, { -60,  3,  55 }, {  40,  6, -75 },
            { -80,  5,  30 }, {  55,  7, -60 }, { -45,  4,  70 },
            {  85,  3, -20 }, { -30,  8,  80 }, {  20,  5, -85 },
            { -70,  6,  45 }, {  65,  9, -40 }, { -50,  3,  65 },
            {  90,  5, -45 }, { -75,  7,  60 }, {  35,  4, -90 },
            { -85,  6,  15 }, {  60,  8, -35 }, { -40,  3,  85 },
            {  75,  5, -70 }, { -55,  9,  50 }, {  45,  3, -80 },
            { -90,  4,  25 }, {  80,  6, -55 }, { -65,  5,  75 },
            // mid-elevation
            {  50, 18, -60 }, { -40, 22,  55 }, {  30, 20, -70 },
            { -65, 15,  35 }, {  45, 25, -50 }, { -55, 19,  40 },
            {  60, 16, -45 }, { -35, 24,  65 }, {  25, 21, -55 },
            { -70, 17,  25 }, {  55, 23, -35 }, { -50, 14,  50 },
            {  40, 26, -65 }, { -60, 20,  30 }, {  35, 18, -40 },
            { -45, 22,  60 }, {  70, 15, -30 }, { -30, 25,  45 },
            // high-elevation
            {  20, 55, -30 }, { -25, 50,  20 }, {  10, 60, -15 },
            { -15, 45,  35 }, {  30, 48, -25 }, { -35, 52,  10 },
            {  15, 58, -20 }, { -20, 42,  30 }, {  25, 62, -10 },
            { -30, 47,  15 }, {  35, 53, -35 }, { -10, 56,  25 },
            {   5, 65, -5  }, { -40, 44,  20 }, {  40, 50, -15 },
        };
        int nBright = sizeof(brightStars) / sizeof(brightStars[0]);

        glPointSize(3.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < nBright; i++) {
            float tw = 0.7f + 0.3f * sinf(animTime * (2.0f + i * 0.3f) + i * 1.7f);
            glColor3f(tw, tw, tw * 0.95f);
            glVertex3f(brightStars[i][0], brightStars[i][1], brightStars[i][2]);
        }
        glEnd();

        // ── Dim stars (smaller points, fills the sky) ─────────────────
        const float dimStars[][3] = {
            // low-elevation
            {  78,  3, -42 }, { -52,  5,  68 }, {  33,  7, -82 },
            { -88,  4,  22 }, {  48,  6, -68 }, { -38,  3,  78 },
            {  92,  5, -15 }, { -22,  7,  88 }, {  15,  4, -92 },
            { -68,  8,  38 }, {  58,  3, -52 }, { -42,  6,  72 },
            {  82,  4, -28 }, { -78,  5,  42 }, {  28,  7, -78 },
            { -92,  3,  18 }, {  72,  6, -62 }, { -48,  4,  82 },
            {  62,  5, -48 }, { -82,  7,  32 }, {  42,  3, -88 },
            { -58,  6,  58 }, {  88,  4, -38 }, { -32,  5,  92 },
            {  52,  8, -72 }, { -72,  3,  48 }, {  38,  5, -58 },
            { -95,  4,  10 }, {  95,  3, -10 }, { -28,  6,  78 },
            // mid-elevation
            {  55, 12, -52 }, { -48, 16,  62 }, {  22, 14, -78 },
            { -72, 13,  28 }, {  38, 19, -42 }, { -58, 11,  48 },
            {  65, 17, -22 }, { -32, 21,  72 }, {  18, 13, -65 },
            { -62, 18,  38 }, {  48, 22, -32 }, { -42, 12,  58 },
            {  72, 14, -18 }, { -28, 23,  48 }, {  32, 16, -58 },
            { -52, 19,  22 }, {  58, 11, -42 }, { -38, 24,  32 },
            {  42, 20, -28 }, { -68, 15,  18 }, {  28, 17, -48 },
            { -45, 13,  42 }, {  52, 21, -55 }, { -35, 16,  68 },
            // high-elevation
            {  18, 40, -28 }, { -22, 48,  18 }, {   8, 55, -12 },
            { -12, 42,  28 }, {  28, 52, -18 }, { -32, 46,   8 },
            {  12, 58,  -8 }, { -18, 50,  22 }, {  22, 44, -32 },
            { -28, 54,  12 }, {   5, 62,  -5 }, { -35, 40,  18 },
            {  32, 46, -12 }, {  -8, 58,  15 }, {  15, 52, -22 },
            { -25, 43,  28 }, {  38, 48,  -8 }, { -42, 51,   5 },
            {   3, 64, -10 }, { -15, 57,  32 }, {  25, 60, -18 },
        };
        int nDim = sizeof(dimStars) / sizeof(dimStars[0]);

        glPointSize(1.5f);
        glBegin(GL_POINTS);
        for (int i = 0; i < nDim; i++) {
            float tw = 0.5f + 0.25f * sinf(animTime * (1.5f + i * 0.2f) + i * 2.3f);
            glColor3f(tw * 0.9f, tw * 0.9f, tw);
            glVertex3f(dimStars[i][0], dimStars[i][1], dimStars[i][2]);
        }
        glEnd();

        glPointSize(1.0f);
    }

    // ── Clouds: soft puffy billboards, flat at the base and domed on top ──
    {
        const float R = 118.0f;
        struct CloudDef { float az, el, w, h, speed; int seed; };
        const CloudDef defs[] = {
            {  20, 14, 17, 6.5f, 0.55f, 11 }, {  75, 24, 22, 7.5f, 0.40f, 23 }, { 130, 11, 15, 5.5f, 0.65f, 37 },
            { 185, 20, 20, 7.0f, 0.45f, 41 }, { 240, 27, 24, 8.0f, 0.35f, 53 }, { 300, 13, 16, 6.0f, 0.60f, 67 },
            { 340, 22, 19, 7.0f, 0.50f, 71 }, {  50, 38, 18, 6.0f, 0.30f, 83 }, { 160, 36, 21, 7.0f, 0.28f, 97 },
            { 270, 40, 17, 6.0f, 0.32f, 101 }, { 105, 8, 13, 4.5f, 0.70f, 113 }, { 215, 9, 14, 5.0f, 0.68f, 127 },
        };
        unsigned tex = getTexID(TEX_CLOUD);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        auto hsh = [](int i, int k) { float v = sinf(i * 127.1f + k * 311.7f) * 43758.5453f; return v - floorf(v); };
        for (const auto& cd : defs) {
            float az = (cd.az + animTime * cd.speed) * 0.0174533f, el = cd.el * 0.0174533f;
            float cxd = sinf(az) * cosf(el), cyd = sinf(el), czd = -cosf(az) * cosf(el);
            float rx = czd, rz = -cxd, rl = sqrtf(rx * rx + rz * rz);           // horizontal right at the cloud centre
            if (rl < 1e-4f) continue;
            rx /= rl; rz /= rl;
            float ux = cyd * rz, uy = czd * rx - cxd * rz, uz = -cyd * rx;      // up (dir x right)
            const int NP = 22;
            struct Puff { float u, v, s; } pf[NP];
            for (int i = 0; i < NP; i++) {
                float u = (hsh(cd.seed + i, 1) * 2.0f - 1.0f) * cd.w * 1.35f;
                float edge = fabsf(u) / (cd.w * 1.35f);
                float dome = powf(fmaxf(0.0f, 1.0f - edge * edge), 0.7f);
                float v = cd.h * (0.12f + 0.88f * hsh(cd.seed + i, 2)) * dome;      // flat-ish base, domed top
                float s = cd.h * 1.3f * (0.55f + 0.75f * hsh(cd.seed + i, 3)) * (1.0f - 0.45f * edge) + 1.0f;
                pf[i] = { u, v, s };
            }
            for (int i = 1; i < NP; i++)                                         // paint low puffs first, tops last
                for (int j = i; j > 0 && pf[j].v < pf[j - 1].v; j--) { Puff t = pf[j]; pf[j] = pf[j - 1]; pf[j - 1] = t; }
            for (int i = 0; i < NP; i++) {
                float px = cxd * R + rx * pf[i].u + ux * pf[i].v;
                float py = cyd * R + 0.0f * pf[i].u + uy * pf[i].v;
                float pz = czd * R + rz * pf[i].u + uz * pf[i].v;
                if (py < 1.0f) continue;                                             // never below the horizon
                float l = sqrtf(px * px + py * py + pz * pz);
                float dx = px / l, dy = py / l, dz = pz / l;
                float brx = dz, brz = -dx, brl = sqrtf(brx * brx + brz * brz);        // billboard facing the origin
                if (brl < 1e-4f) continue;
                brx /= brl; brz /= brl;
                float bux = dy * brz, buy = dz * brx - dx * brz, buz = -dy * brx;
                float t = pf[i].v / cd.h;                                            // 0 base .. 1 top
                float cr, cg, cb, ca;
                if (isDayTime) { cr = 0.76f + 0.24f * t; cg = 0.81f + 0.19f * t; cb = 0.90f + 0.10f * t; ca = 0.80f; }
                else           { cr = 0.15f + 0.07f * t; cg = 0.17f + 0.08f * t; cb = 0.27f + 0.10f * t; ca = 0.55f; }
                glColor4f(cr, cg, cb, ca);
                float sz = pf[i].s;
                glBegin(GL_QUADS);
                glTexCoord2f(0, 0); glVertex3f(px - brx * sz - bux * sz, py - buy * sz, pz - brz * sz - buz * sz);
                glTexCoord2f(1, 0); glVertex3f(px + brx * sz - bux * sz, py - buy * sz, pz + brz * sz - buz * sz);
                glTexCoord2f(1, 1); glVertex3f(px + brx * sz + bux * sz, py + buy * sz, pz + brz * sz + buz * sz);
                glTexCoord2f(0, 1); glVertex3f(px - brx * sz + bux * sz, py + buy * sz, pz - brz * sz + buz * sz);
                glEnd();
            }
        }
        glDisable(GL_TEXTURE_2D);
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
    // ── Lawn: realistic procedural grass texture (unlit; see texture.cpp) ───
    // Unlit so a big plane has no per-vertex lighting gradient; a night tint keeps
    // it dark but still textured.  A second, large-scale layer breaks up tiling.
    setLighting(false);
    {
        const float ext = 120.0f, tile = 5.0f, macro = 55.0f, gy = -0.005f;
        Color tint = isDayTime ? Color{ 1.0f, 1.0f, 1.0f } : Color{ 0.17f, 0.22f, 0.17f };
        auto lawnQuad = [&](float scale) {
            glBegin(GL_QUADS);
            glNormal3f(0, 1, 0);
            glTexCoord2f(-ext / scale, -ext / scale); glVertex3f(-ext, gy, -ext);
            glTexCoord2f( ext / scale, -ext / scale); glVertex3f( ext, gy, -ext);
            glTexCoord2f( ext / scale,  ext / scale); glVertex3f( ext, gy,  ext);
            glTexCoord2f(-ext / scale,  ext / scale); glVertex3f(-ext, gy,  ext);
            glEnd();
        };
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        shaderSetTexture(true);

        glBindTexture(GL_TEXTURE_2D, getTexID(TEX_GRASS));
        setColor(tint);
        lawnQuad(tile);

        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);          // dst * src * 2: 0.5 is neutral
        glDepthFunc(GL_LEQUAL);
        glBindTexture(GL_TEXTURE_2D, getTexID(TEX_GRASS_MACRO));
        setColor({ 1.0f, 1.0f, 1.0f });
        lawnQuad(macro);
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);

        shaderSetTexture(false);
        glDisable(GL_TEXTURE_2D);
    }
    setLighting(true);

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
    drawSidewalk({ 0, 0, 17.0f });   // far-side sidewalk across the street

    // Polished-floor reflection overlay (stencil-masked to the floor platform)
    drawFloorReflection();
    // Projected shadows from the warm interior light (stencil prevents double-darkening)
    drawShadows();
}

// ─── Exterior Shadows: real projected shadows ───────────────────────────────
// The whole outdoor scene (drawExteriorBody) is drawn a second time, flattened onto
// the ground along the sun direction, into the STENCIL buffer only.  One soft dark
// layer is then blended over the marked pixels, so every shadow has exactly the
// silhouette of its object (tree crowns, trunks, roofs ...) and overlaps do not
// double-darken.  Exterior ground only: the interior floor platform has stencil != 0.
static void drawExteriorBody();

// Projection onto the plane y = h.  mode 0: directional light given by the vector
// (lx, ly, lz) pointing TOWARD the light; mode 1: point light at (lx, ly, lz).
static void multShadowMatrix(int mode, float lx, float ly, float lz, float h)
{
    glTranslatef(0.0f, h, 0.0f);
    if (mode == 0) {
        GLfloat m[16] = { ly, 0, 0, 0,   -lx, 0, -lz, 0,   0, 0, ly, 0,   0, 0, 0, ly };
        glMultMatrixf(m);
    } else {
        float py = ly - h;
        GLfloat m[16] = { py, 0, 0, 0,   -lx, 0, -lz, -1,   0, 0, py, 0,   0, 0, 0, py };
        glTranslatef(0.0f, 0.0f, 0.0f);
        glMultMatrixf(m);
    }
    glTranslatef(0.0f, -h, 0.0f);
}

static void shadowRegion(bool sidewalk, float h)
{
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    if (!sidewalk) {
        glVertex3f(-130.0f, h, -130.0f); glVertex3f(130.0f, h, -130.0f);
        glVertex3f( 130.0f, h,  130.0f); glVertex3f(-130.0f, h,  130.0f);
    } else {                                              // the two sidewalk strips (z = 5.5..8.5 and 15.5..18.5)
        const float z0[2] = { 5.5f, 15.5f }, z1[2] = { 8.5f, 18.5f };
        for (int i = 0; i < 2; i++) {
            glVertex3f(-30.0f, h, z0[i]); glVertex3f(30.0f, h, z0[i]);
            glVertex3f( 30.0f, h, z1[i]); glVertex3f(-30.0f, h, z1[i]);
        }
    }
    glEnd();
}

// One shadow layer: flatten the outdoor scene onto plane y = h into the stencil
// buffer, then blend one soft dark layer over the marked pixels (and clear them).
//   lamp != 0 : restrict casters to a box around the point light (keeps it cheap)
static void shadowLayer(int mode, float lx, float ly, float lz, float h, bool sidewalk, float alpha, bool lamp)
{
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_EQUAL, 0, 0xFF);               // exterior ground only
    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);       // first fragment flips 0 -> 0xFF, later ones fail
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    drawingShadow = true;
    outdoorShadowPass = true;
    outdoorShadowDetail = false;                     // simplified shadows: cheap crown shapes (keeps movement smooth)

    if (lamp) {                                      // world-space clip box (camera matrix is current)
        const GLdouble pl[5][4] = {
            {  1, 0, 0, -(lx - 9.0) }, { -1, 0, 0, lx + 9.0 },
            {  0, 0, 1, 2.0 },         {  0, 0, -1, 14.0 },
            {  0, -1, 0, ly - 0.05 } };
        for (int i = 0; i < 5; i++) { glClipPlane(GL_CLIP_PLANE0 + i, pl[i]); glEnable(GL_CLIP_PLANE0 + i); }
    }
    glPushMatrix();
    multShadowMatrix(mode, lx, ly, lz, h);
    drawExteriorBody();
    glPopMatrix();
    if (lamp) for (int i = 0; i < 5; i++) glDisable(GL_CLIP_PLANE0 + i);

    drawingShadow = false;
    outdoorShadowPass = false;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glStencilFunc(GL_EQUAL, 0xFF, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);          // blend once, then clear the mask
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.02f, 0.05f, 0.02f, alpha);
    shadowRegion(sidewalk, h);
    glDisable(GL_BLEND);
}

static void drawExteriorShadows()
{
    if (!isDayTime) return;                            // no ground shadows at night (they only showed as blotches)

    const float HG = 0.03f, HW = 0.135f;             // lawn / street, and sidewalk-top planes
    // Sun / moon direction (vector toward the light): shadows fall toward -x, +z
    const float sx = isDayTime ? 0.62f : 0.80f, sy = 1.0f, sz = isDayTime ? -0.77f : -1.10f;
    const float sunA = isDayTime ? 0.42f : 0.20f;

    disablePhongShader();
    shadowLayer(0, sx, sy, sz, HG, false, sunA, false);
    shadowLayer(0, sx, sy, sz, HW, true,  sunA, false);

    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_LIGHTING);
    enablePhongShader();
}

// ─── Exterior ───────────────────────────────────────────────────────────────
void drawExterior()
{
    // Real shadows first (ground only), then the actual objects on top
    drawExteriorShadows();
    drawExteriorBody();
}

static void drawExteriorBody()
{
    drawShopBuilding({ 0, 0, 0 });
    if (showRoof) drawRoof({ 0, 0, 0 });

    // Japanese paper (shoji) shopfront facade panels (replaces glass)
    drawShojiWindow({ -3.35f, 1.65f, 3.90f }, NO_ROT, ONE, 2.90f, 2.70f);
    drawShojiWindow({  3.35f, 1.65f, 3.90f }, NO_ROT, ONE, 2.90f, 2.70f);

    // Wooden entrance doors — sliding shoji (click to open/close)
    drawEntranceDoor(doorAngle);

    // Clear glass windows on side walls (left and right) — walls have matching openings
    // (glass panes themselves are drawn in the transparent pass at the end of drawInterior)

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

    // ── Street lamp posts — near-side sidewalk (z≈7) ──────────────────
    drawLamp({ -7.0f, 0, 7.0f }, { 0, -90, 0 });
    drawLamp({  7.0f, 0, 7.0f }, { 0, -90, 0 });
    drawLamp({ -15.0f, 0, 7.0f }, { 0, -90, 0 });
    drawLamp({  15.0f, 0, 7.0f }, { 0, -90, 0 });
    drawLamp({ -23.0f, 0, 7.0f }, { 0, -90, 0 });
    drawLamp({  23.0f, 0, 7.0f }, { 0, -90, 0 });

    // ── Street lamp posts — far-side sidewalk (z≈17) ────────────────
    drawLamp({ -8.0f, 0, 17.0f }, { 0, 90, 0 });
    drawLamp({  8.0f, 0, 17.0f }, { 0, 90, 0 });
    drawLamp({ -16.0f, 0, 17.0f }, { 0, 90, 0 });
    drawLamp({  16.0f, 0, 17.0f }, { 0, 90, 0 });
    drawLamp({ -24.0f, 0, 17.0f }, { 0, 90, 0 });
    drawLamp({  24.0f, 0, 17.0f }, { 0, 90, 0 });

    // (vending machine removed)

    // ── Extended Outdoor Environment ─────────────────────────────────────
    // Layout: shop at origin facing +Z.  Street at z≈12.  Sidewalks at z≈7, z≈17.
    // Zone map (clear separation — no overlap with road z=5.5..18.5 or lake z=-16..-32):
    //   Dedicated Forest:  x<-14, z<-8   (left behind shop — dense pines/maples/jungle)
    //   Dedicated Bamboo:  x>14,  z<-8   (right behind shop — bamboo/torii/moss/petals)
    //   Lake:              center z=-24  (bridge + ducks, shore lanterns)
    //   Flower Gardens:    specific ground spots near shop & houses
    //   Far-side Forest:   z>25          (behind far houses — mixed forest)
    //   Perimeter Jungle:  world edges   (|x|>33 or |z|>36)

    // ── Near-side houses (same side as shop, facing +Z toward street) ────
    drawJapaneseHouse2({ -15.0f, 0, 2.15f }, NO_ROT, ONE);
    drawJapaneseHouse({ -23.0f, 0, 2.15f }, NO_ROT, ONE);
    drawJapaneseHouse({  15.0f, 0, 2.15f }, NO_ROT, ONE);
    drawJapaneseHouse2({  23.0f, 0, 2.15f }, NO_ROT, ONE);

    // ── Far-side houses (across the street, facing -Z back toward street) ─
    // House depth 3.5, front at z≈19, centre z = 19 + 1.75 = 20.75
    drawJapaneseHouse2({  -8.0f, 0, 20.75f }, { 0, 180, 0 }, ONE);
    drawJapaneseHouse({ -16.0f, 0, 20.75f }, { 0, 180, 0 }, ONE);
    drawJapaneseHouse2({ -24.0f, 0, 20.75f }, { 0, 180, 0 }, ONE);
    drawJapaneseHouse({   8.0f, 0, 20.75f }, { 0, 180, 0 }, ONE);
    drawJapaneseHouse2({  16.0f, 0, 20.75f }, { 0, 180, 0 }, ONE);
    drawJapaneseHouse({  24.0f, 0, 20.75f }, { 0, 180, 0 }, ONE);

    // ── Accent trees near the shop (not on road or lake) ──────────────
    drawMapleTree({ -10.5f, 0,   4.5f }, NO_ROT, { 1.0f, 1.0f, 1.0f });
    drawMapleTree({  10.5f, 0,   4.5f }, NO_ROT, { 1.1f, 1.2f, 1.1f });

    // ══════════════════════════════════════════════════════════════════════
    //  DEDICATED FOREST — left side behind shop (x < -14, z < -8)
    //  Dense cluster inspired by anime forest: tall trees, varied species,
    //  lush undergrowth.  Clear of lake (center z=-24) and road (z>5.5).
    // ══════════════════════════════════════════════════════════════════════

    // Tall pine trees (primary canopy)
    drawJapanesePineTree({ -16.0f, 0, -10.0f }, NO_ROT, { 1.2f, 1.5f, 1.2f });
    drawJapanesePineTree({ -19.0f, 0, -12.0f }, NO_ROT, { 1.1f, 1.8f, 1.1f });
    drawJapanesePineTree({ -25.0f, 0, -13.0f }, NO_ROT, { 1.3f, 1.7f, 1.3f });
    drawJapanesePineTree({ -28.0f, 0, -11.0f }, NO_ROT, { 0.9f, 1.4f, 0.9f });
    drawJapanesePineTree({ -17.0f, 0, -16.0f }, NO_ROT, { 1.0f, 1.9f, 1.0f });
    drawJapanesePineTree({ -21.0f, 0, -18.0f }, NO_ROT, { 1.2f, 1.6f, 1.2f });
    drawJapanesePineTree({ -30.0f, 0, -14.0f }, NO_ROT, { 1.0f, 1.3f, 1.0f });
    drawJapanesePineTree({ -15.0f, 0, -22.0f }, NO_ROT, { 1.3f, 1.8f, 1.3f });
    drawJapanesePineTree({ -29.0f, 0, -20.0f }, NO_ROT, { 0.9f, 1.4f, 0.9f });
    drawJapanesePineTree({ -18.0f, 0, -28.0f }, NO_ROT, { 1.1f, 1.6f, 1.1f });
    drawJapanesePineTree({ -27.0f, 0, -27.0f }, NO_ROT, { 1.2f, 1.8f, 1.2f });
    drawJapanesePineTree({ -32.0f, 0, -25.0f }, NO_ROT, { 1.0f, 1.3f, 1.0f });

    // Maple trees adding autumn colour variety
    drawMapleTree({ -17.0f, 0, -14.0f }, NO_ROT, { 1.1f, 1.3f, 1.1f });
    drawMapleTree({ -24.0f, 0, -16.0f }, NO_ROT, { 1.0f, 1.2f, 1.0f });
    drawMapleTree({ -20.0f, 0, -21.0f }, NO_ROT, { 1.2f, 1.4f, 1.2f });
    drawMapleTree({ -28.0f, 0, -23.0f }, NO_ROT, { 0.9f, 1.1f, 0.9f });
    drawMapleTree({ -16.0f, 0, -26.0f }, NO_ROT, { 1.0f, 1.2f, 1.0f });
    drawMapleTree({ -31.0f, 0, -18.0f }, NO_ROT, { 1.1f, 1.3f, 1.1f });

    // Regular broad-leaf trees filling gaps
    drawTree({ -18.0f, 0, -11.0f }, NO_ROT, { 1.0f, 1.4f, 1.0f });
    drawTree({ -23.0f, 0, -15.0f }, NO_ROT, { 1.2f, 1.6f, 1.2f });
    drawTree({ -27.0f, 0, -19.0f }, NO_ROT, { 1.1f, 1.5f, 1.1f });
    drawTree({ -21.0f, 0, -26.0f }, NO_ROT, { 1.0f, 1.4f, 1.0f });
    drawTree({ -30.0f, 0, -22.0f }, NO_ROT, { 1.2f, 1.6f, 1.2f });

    // Cherry blossom accents at forest edge
    drawCherryBlossomTree({ -25.0f, 0, -9.0f }, NO_ROT, { 1.0f, 1.1f, 1.0f });
    drawCherryBlossomTree({ -15.0f, 0, -14.0f }, NO_ROT, { 0.9f, 1.0f, 0.9f });

    // Dense jungle undergrowth throughout the forest floor
    drawJungle({ -20.0f, 0, -12.0f }, NO_ROT, { 1.5f, 0.8f, 1.5f });
    drawJungle({ -18.0f, 0, -20.0f }, { 0, 60, 0 }, { 1.6f, 0.9f, 1.6f });
    drawJungle({ -28.0f, 0, -22.0f }, { 0, 45, 0 }, { 1.4f, 0.8f, 1.4f });
    drawJungle({ -22.0f, 0, -28.0f }, { 0, 15, 0 }, { 1.5f, 0.7f, 1.5f });
    drawJungle({ -32.0f, 0, -16.0f }, { 0, -20, 0 }, { 1.3f, 0.7f, 1.3f });

    // ══════════════════════════════════════════════════════════════════════
    //  DEDICATED BAMBOO GROVE — right side behind shop (x > 14, z < -8)
    //  Mystical bamboo forest with stone lanterns, torii gate, cherry
    //  blossoms, and mossy ground — inspired by Arashiyama bamboo paths.
    // ══════════════════════════════════════════════════════════════════════

    // Dense bamboo clusters forming a continuous forest
    drawBambooGrove({ 17.0f, 0, -24.0f }, { 0, 45, 0 }, { 1.3f, 1.5f, 1.3f });
    drawBambooGrove({ 23.0f, 0, -26.0f }, { 0, 60, 0 }, { 1.2f, 1.3f, 1.2f });
    drawBambooGrove({ 16.0f, 0, -29.0f }, { 0, 75, 0 }, { 1.2f, 1.4f, 1.2f });
    drawBambooGrove({ 22.0f, 0, -30.0f }, { 0, -50, 0 }, { 1.3f, 1.5f, 1.3f });
    drawBambooGrove({ 28.0f, 0, -28.0f }, { 0, 15, 0 }, { 1.0f, 1.3f, 1.0f });

    // Cherry blossom trees at bamboo grove edges (blossoms cascading over bamboo)
    drawCherryBlossomTree({ 15.0f, 0, -9.0f }, NO_ROT, { 1.2f, 1.3f, 1.2f });
    drawCherryBlossomTree({ 22.0f, 0, -9.0f }, NO_ROT, { 1.0f, 1.1f, 1.0f });
    drawCherryBlossomTree({ 29.0f, 0, -10.0f }, NO_ROT, { 1.1f, 1.2f, 1.1f });
    drawCherryBlossomTree({ 14.0f, 0, -20.0f }, NO_ROT, { 0.9f, 1.0f, 0.9f });
    drawCherryBlossomTree({ 31.0f, 0, -20.0f }, NO_ROT, { 1.0f, 1.1f, 1.0f });

    // Red torii gate at the bamboo grove entrance
    drawToriiGate({ 20.0f, 0, -9.0f }, { 0, 90, 0 });

    // Stone lanterns along the bamboo path
    drawStoneLantern({ 15.0f, 0, -11.0f });
    drawStoneLantern({ 18.0f, 0, -15.0f });
    drawStoneLantern({ 21.0f, 0, -11.0f });
    drawStoneLantern({ 24.0f, 0, -16.0f });
    drawStoneLantern({ 27.0f, 0, -12.0f });
    drawStoneLantern({ 16.0f, 0, -21.0f });
    drawStoneLantern({ 20.0f, 0, -26.0f });
    drawStoneLantern({ 25.0f, 0, -23.0f });

    // Falling cherry blossom petals drifting through the bamboo canopy
    drawFallingPetals({ 22.0f, 5.0f, -18.0f }, 12.0f, 30);

    // ══════════════════════════════════════════════════════════════════════
    //  FAR-SIDE FOREST — behind the far houses (z > 25)
    //  Mixed forest: pines, maples, cherry blossoms, undergrowth
    // ══════════════════════════════════════════════════════════════════════

    // Pine trees (primary canopy)
    drawJapanesePineTree({ -12.0f, 0, 28.0f }, NO_ROT, { 1.0f, 1.3f, 1.0f });
    drawJapanesePineTree({ -20.0f, 0, 30.0f }, NO_ROT, { 1.2f, 1.5f, 1.2f });
    drawJapanesePineTree({ -28.0f, 0, 28.0f }, NO_ROT, { 1.0f, 1.4f, 1.0f });
    drawJapanesePineTree({  14.0f, 0, 27.0f }, NO_ROT, { 0.9f, 1.1f, 0.9f });
    drawJapanesePineTree({  22.0f, 0, 29.0f }, NO_ROT, { 1.0f, 1.3f, 1.0f });
    drawJapanesePineTree({  28.0f, 0, 27.0f }, NO_ROT, { 1.1f, 1.4f, 1.1f });
    drawJapanesePineTree({  -3.0f, 0, 32.0f }, NO_ROT, { 1.1f, 1.6f, 1.1f });
    drawJapanesePineTree({   5.0f, 0, 33.0f }, NO_ROT, { 0.8f, 1.2f, 0.8f });
    drawJapanesePineTree({ -16.0f, 0, 34.0f }, NO_ROT, { 1.0f, 1.5f, 1.0f });
    drawJapanesePineTree({  18.0f, 0, 35.0f }, NO_ROT, { 1.2f, 1.7f, 1.2f });
    drawJapanesePineTree({ -25.0f, 0, 33.0f }, NO_ROT, { 0.9f, 1.3f, 0.9f });
    drawJapanesePineTree({  25.0f, 0, 34.0f }, NO_ROT, { 1.0f, 1.4f, 1.0f });

    // Maple trees interspersed
    drawMapleTree({ -5.0f, 0, 26.0f }, NO_ROT, { 1.0f, 1.2f, 1.0f });
    drawMapleTree({  18.0f, 0, 28.0f }, NO_ROT, { 1.1f, 1.3f, 1.1f });
    drawMapleTree({ -25.0f, 0, 32.0f }, NO_ROT, { 0.9f, 1.1f, 0.9f });
    drawMapleTree({  26.0f, 0, 33.0f }, NO_ROT, { 1.0f, 1.2f, 1.0f });
    drawMapleTree({  -8.0f, 0, 34.0f }, NO_ROT, { 1.1f, 1.3f, 1.1f });
    drawMapleTree({  10.0f, 0, 32.0f }, NO_ROT, { 0.9f, 1.0f, 0.9f });

    // Cherry blossom trees for colour accents
    drawCherryBlossomTree({  3.0f, 0, 27.0f }, NO_ROT, { 1.0f, 1.1f, 1.0f });
    drawCherryBlossomTree({ -18.0f, 0, 33.0f }, NO_ROT, { 1.1f, 1.2f, 1.1f });
    drawCherryBlossomTree({  12.0f, 0, 30.0f }, NO_ROT, { 0.9f, 1.0f, 0.9f });

    // Dense undergrowth filling the far forest floor
    drawJungle({ -10.0f, 0, 30.0f }, { 0, 40, 0 }, { 1.5f, 0.8f, 1.5f });
    drawJungle({  10.0f, 0, 31.0f }, { 0, -20, 0 }, { 1.4f, 0.7f, 1.4f });
    drawJungle({ -22.0f, 0, 32.0f }, { 0, 60, 0 }, { 1.6f, 0.9f, 1.6f });
    drawJungle({  22.0f, 0, 30.0f }, { 0, -40, 0 }, { 1.3f, 0.8f, 1.3f });
    drawJungle({   0.0f, 0, 36.0f }, { 0, 30, 0 }, { 2.0f, 0.9f, 2.0f });
    drawJungle({ -15.0f, 0, 38.0f }, { 0, -15, 0 }, { 1.5f, 0.8f, 1.5f });
    drawJungle({  15.0f, 0, 37.0f }, { 0, 50, 0 }, { 1.4f, 0.7f, 1.4f });

    // ── Lake with swimming ducks (behind the shop, between houses) ──────
    drawLake({ 0, 0.04f, -24.0f }, NO_ROT, { 1.5f, 1.0f, 1.0f });
    drawBridge({ 0, 0.10f, -24.0f }, NO_ROT, ONE);
    // Two ducks swimming in circles on the lake
    drawDuck({ 0, 0.06f, -24.0f }, NO_ROT, ONE);
    drawDuck({ 0, 0.06f, -24.0f }, { 0, 120, 0 }, ONE);  // offset orbit phase via rotation

    // ══════════════════════════════════════════════════════════════════════
    //  DEDICATED FLOWER GARDENS — specific ground spots (not scattered)
    // ══════════════════════════════════════════════════════════════════════

    // Near the shop (on grass behind houses, not on the road)
    drawFlowerGarden({ -9.0f, 0, -2.0f }, NO_ROT, { 0.8f, 1.0f, 0.8f });
    drawFlowerGarden({  9.0f, 0, -2.0f }, NO_ROT, { 0.8f, 1.0f, 0.8f });
    // Between near-side houses (on grass side)
    drawFlowerGarden({ -19.0f, 0, -1.5f }, { 0, 15, 0 }, { 0.6f, 1.0f, 0.6f });
    drawFlowerGarden({  19.0f, 0, -1.5f }, { 0, -15, 0 }, { 0.6f, 1.0f, 0.6f });
    // Between far-side houses
    drawFlowerGarden({  0.0f, 0, 19.0f }, NO_ROT, { 1.0f, 1.0f, 1.0f });
    drawFlowerGarden({ -20.0f, 0, 19.0f }, { 0, 30, 0 }, { 0.7f, 1.0f, 0.7f });
    drawFlowerGarden({  20.0f, 0, 19.0f }, { 0, -30, 0 }, { 0.7f, 1.0f, 0.7f });
    // At forest entrance (village-to-forest transition)
    drawFlowerGarden({ -14.0f, 0, -7.0f }, { 0, 20, 0 }, { 0.7f, 1.0f, 0.7f });
    // At bamboo grove entrance
    drawFlowerGarden({  14.0f, 0, -7.0f }, { 0, -20, 0 }, { 0.7f, 1.0f, 0.7f });

    // ── Stone lanterns along village pathways ────────────────────────────
    drawStoneLantern({ -6.5f, 0,  6.2f });
    drawStoneLantern({  6.5f, 0,  6.2f });
    drawStoneLantern({ -6.5f, 0, 18.0f });
    drawStoneLantern({  6.5f, 0, 18.0f });
    // Near the lake shore (not in the water)
    drawStoneLantern({  10.0f, 0, -15.0f });
    drawStoneLantern({ -10.0f, 0, -15.0f });
    drawStoneLantern({   0.0f, 0, -14.0f });

    // ══════════════════════════════════════════════════════════════════════
    //  PERIMETER JUNGLE — dense forest wall at world edges
    //  Moved away from road (z>5.5) and lake (z~-24) zones
    // ══════════════════════════════════════════════════════════════════════
    drawJungle({ -35.0f, 0, -15.0f }, NO_ROT, { 2.0f, 1.2f, 2.0f });
    drawJungle({  35.0f, 0, -12.0f }, { 0, 90, 0 }, { 1.8f, 1.1f, 1.8f });
    drawJungle({ -35.0f, 0,  30.0f }, { 0, 45, 0 }, { 1.8f, 1.0f, 1.8f });
    drawJungle({  35.0f, 0,  28.0f }, { 0, -30, 0 }, { 1.7f, 1.1f, 1.7f });
    drawJungle({   0.0f, 0, -38.0f }, NO_ROT, { 3.0f, 1.2f, 2.0f });
    drawJungle({   0.0f, 0,  42.0f }, { 0, 90, 0 }, { 2.5f, 1.0f, 2.0f });
    drawJungle({ -35.0f, 0,   2.0f }, { 0, 20, 0 }, { 1.5f, 1.0f, 1.5f });
    drawJungle({  35.0f, 0,   2.0f }, { 0,-20, 0 }, { 1.5f, 1.0f, 1.5f });
    drawJungle({ -35.0f, 0, -30.0f }, { 0, -10, 0 }, { 1.8f, 1.0f, 1.8f });
    drawJungle({  35.0f, 0, -28.0f }, { 0, 15, 0 }, { 1.6f, 1.0f, 1.6f });
    drawJungle({ -20.0f, 0, -38.0f }, { 0, 30, 0 }, { 1.5f, 1.0f, 1.5f });
    drawJungle({  20.0f, 0, -38.0f }, { 0, -30, 0 }, { 1.5f, 1.0f, 1.5f });

    // ── Grass tufts scattered over both sides, skipping buildings/road ──
    for (float gx = -36.0f; gx <= 36.0f; gx += 3.2f) {
        for (float gz = -38.0f; gz <= 38.0f; gz += 3.2f) {
            float x = gx + sinf(gx * 12.9f + gz * 78.2f) * 1.2f;
            float z = gz + sinf(gx * 39.3f + gz * 11.1f) * 1.2f;
            // Skip shop + yard
            if (fabsf(x) < 7.0f && z > -5.5f && z < 6.0f) continue;
            // Skip near-side houses
            if (fabsf(x) > 12.0f && fabsf(x) < 26.0f && z > -0.5f && z < 5.0f) continue;
            // Skip road + sidewalks (z = 5.5 .. 18.5)
            if (z > 5.5f && z < 18.5f) continue;
            // Skip far-side houses
            if (fabsf(x) > 5.0f && fabsf(x) < 27.0f && z > 18.5f && z < 24.0f) continue;
            // Skip lake area
            float lx = x / 13.5f, lz = (z + 24.0f) / 9.5f;
            if (lx * lx + lz * lz < 1.0f) continue;
            float s = 0.9f + 0.4f * (0.5f + 0.5f * sinf(gx * 3.7f + gz * 5.3f));
            drawGrassPatch({ x, 0.0f, z }, { 0, gx * 17.0f + gz * 31.0f, 0 }, { s, 1.0f, s });
        }
    }

    // ── Mountain Range — distant background behind far-side forest ────
    // Mt. Fuji-style snow-capped peak with foothills, placed far back
    // (mountains removed)
    // drawMountainRange({ 0, 0, -85.0f });

    // ── Fireflies — near gardens, lake, forest and bamboo (night only) ──
    drawFireflies({ -9.0f, 1.2f, 5.0f }, 4.0f, 10);       // flower garden left
    drawFireflies({  9.0f, 1.2f, 5.0f }, 4.0f, 10);       // flower garden right
    drawFireflies({  0.0f, 1.5f, -24.0f }, 10.0f, 25);     // lake
    drawFireflies({ -22.0f, 2.0f, -18.0f }, 8.0f, 20);    // deep forest
    drawFireflies({  22.0f, 2.0f, -18.0f }, 8.0f, 20);    // bamboo grove
    drawFireflies({  0.0f, 1.5f, 30.0f }, 10.0f, 20);      // far-side forest
    drawFireflies({ -20.0f, 1.5f, 32.0f }, 6.0f, 12);     // far-side forest left
    drawFireflies({  20.0f, 1.5f, 32.0f }, 6.0f, 12);     // far-side forest right
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

// ─── Transparent pass ───────────────────────────────────────────────────────
// Glass and steam are alpha-blended and do not write depth, so they MUST be drawn
// after all opaque geometry (cabinets, walls, kitchen).  Drawn earlier, the opaque
// scenery behind them is painted over the glass and cuts it off.  Order is
// roughly far to near.
static void drawTransparentPass()
{
    float FY = FLOOR_Y;
    float counterTop = FY + 1.06f;

    // Window panes (back wall x2, side walls x2)
    for (int sx = -1; sx <= 1; sx += 2)
        drawClearGlassWindow({ sx * 3.5f, 1.8f, -3.85f }, NO_ROT, ONE, 1.2f, 1.0f);
    drawClearGlassWindow({ -4.90f, 2.2f, 0.5f }, { 0, -90, 0 }, ONE, 2.40f, 2.00f);
    drawClearGlassWindow({  4.90f, 2.2f, 0.5f }, { 0,  90, 0 }, ONE, 2.40f, 2.00f);

    // Steam above the cooking pots (far)
    float kitchenTop = FY + 0.94f;
    if (showSteam) {
        drawSteam({ -0.55f, kitchenTop + 0.65f, -3.2f }, 0.40f * gameSteamScale(0), 0.4f);   // large pot, left burner
        drawSteam({  0.55f, kitchenTop + 0.65f, -3.2f }, 0.38f * gameSteamScale(1), 1.8f);   // large pot, right burner
    }

    // Clear glass pendant lamps above the dining counter
    drawPendantGlassLamp({ -1.2f, 2.85f, -0.2f }, NO_ROT, ONE, lightPoint);
    drawPendantGlassLamp({  1.2f, 2.85f, -0.2f }, NO_ROT, ONE, lightPoint);

    // Clear glass condiment jars, water pitcher and tumblers on the counter
    drawClearGlassJar({ 1.65f, counterTop + 0.004f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.85f, 0.20f, 0.10f }); // shichimi chili
    drawClearGlassJar({ 1.90f, counterTop + 0.004f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.95f, 0.90f, 0.70f }); // pickled garlic
    drawClearGlassJar({ 2.15f, counterTop + 0.004f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.20f, 0.18f, 0.15f }); // sesame seeds
    drawClearGlassPitcher({ 0.35f, counterTop + 0.004f, -0.36f }, NO_ROT, { 0.30f, 0.30f, 0.30f });
    drawClearGlassTumbler({ -1.85f, counterTop + 0.004f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
    drawClearGlassTumbler({ -0.35f, counterTop + 0.004f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
    drawClearGlassTumbler({  1.15f, counterTop + 0.004f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
    drawClearGlassTumbler({  2.65f, counterTop + 0.004f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);

    // Steam above the ramen bowls (nearest; first bowl follows its OBJ_BOWL transform)
    if (showSteam) {
        for (int i = 0; i < 4; i++) {
            float x = -2.2f + i * 1.5f;
            if (i == 0) {
                glPushMatrix();
                glTranslatef(x, counterTop, -0.3f);
                applyObjDelta(OBJ_BOWL);
                drawSteam({ 0, 0.09f, 0 }, 0.22f, 0.0f);
                glPopMatrix();
            } else {
                drawSteam({ x, counterTop + 0.09f, -0.3f }, 0.22f, (float)i * 1.35f);
            }
        }
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
            glPopMatrix();
        } else {
            drawRamenBowl({ x, counterTop, -0.3f }, NO_ROT, { 0.24f, 0.24f, 0.24f });
            drawChopsticks({ x + 0.18f, counterTop + 0.005f, -0.12f },
                           { 0, 25, 0 }, { 0.24f, 0.24f, 0.24f });
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

    // Japanese Box Lantern Cluster — andon-style washi-paper lanterns above dining area
    drawJapaneseBoxLanternCluster({ 0.5f, 3.30f, 0.55f }, NO_ROT, ONE, lightPoint);

    drawBottle({  2.7f, counterTop, -0.35f }, NO_ROT, { 0.3f, 0.3f, 0.3f }, SOY);
    drawBottle({  2.9f, counterTop, -0.25f }, NO_ROT, { 0.3f, 0.3f, 0.3f }, RED, GOLD);
    drawCup({ -2.7f, counterTop, -0.30f }, NO_ROT, { 0.1f, 0.1f, 0.1f }, CUP_GREEN);

    // Cash register at the far-right end of the service counter
    drawCashRegister({ 3.1f, counterTop, -0.38f }, { 0, 180, 0 });

    drawKitchen({ 0, FY, -3.2f });

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

    // Game objects (customer, noodles in the pot, held item) - opaque, so before the glass
    drawGameWorld();

    // All glass and steam last (see drawTransparentPass)
    drawTransparentPass();
}
