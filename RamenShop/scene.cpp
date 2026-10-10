#include "scene.h"
#include "lighting.h"
#include "objects.h"
#include "shader.h"
#include "shadowmap.h"
#include "refraction.h"
#include "rtpass.h"
#include "rtscene.h"
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
[[maybe_unused]] static void drawFloorReflection()
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
[[maybe_unused]] static void drawShadows()
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

// ─── Interior shadows (projected onto the tiled floor) ─────────────────────
// Each interior light that is switched on projects the furniture under it onto the floor
// plane (planar shadow matrix for a point light).  The stencil buffer (floor platform = 1)
// keeps every shadow on the floor and stops overlapping parts darkening twice; the stencil
// is put back to 1 afterwards so the next light gets its own shadow layer.
static void multShadowMatrix(int mode, float lx, float ly, float lz, float h);
static void interiorShadowLayer(float lx, float ly, float lz, float alpha, void (*casters)())
{
    const float h = FLOOR_Y + 0.004f;
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);                 // first fragment: 1 -> 2, later ones fail
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    drawingShadow = true;
    glPushMatrix();
    multShadowMatrix(1, lx, ly, lz, h);
    casters();
    glPopMatrix();
    drawingShadow = false;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glStencilFunc(GL_EQUAL, 2, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);                 // darken once, then back to 1
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.02f, 0.015f, 0.01f, alpha);
    glBegin(GL_QUADS);
    glVertex3f(-4.8f, h, -3.8f); glVertex3f(4.8f, h, -3.8f);
    glVertex3f( 4.8f, h,  3.8f); glVertex3f(-4.8f, h,  3.8f);
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static float g_tableX = 0.0f;
static void tableCasters()
{
    drawTable({ g_tableX, FLOOR_Y, 2.8f }, NO_ROT, { 0.8f, 0.9f, 0.8f });
    drawChair({ g_tableX, FLOOR_Y, 2.8f - 0.55f }, NO_ROT, { 0.85f, 0.85f, 0.85f });
    drawChair({ g_tableX, FLOOR_Y, 2.8f + 0.55f }, { 0, 180, 0 }, { 0.85f, 0.85f, 0.85f });
}
static void diningCasters()
{
    drawCounter({ 0, FLOOR_Y, -0.5f });
    for (int i = 0; i < 6; i++) drawStool({ -2.8f + i * 1.12f, FLOOR_Y, 0.8f });
    for (int t = 0; t < 2; t++) { g_tableX = (t == 0) ? -2.4f : 2.4f; tableCasters(); }
}

static void drawInteriorShadows()
{
    disablePhongShader();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -2.0f);
    const float k = isDayTime ? 0.6f : 1.0f;                // softer when daylight fills the room
    // table lamps (key 2): sharp shadows of each table and its chairs straight below the lamp
    if (lightSpot && fixtureOn[FX_SPOT])
        for (int t = 0; t < 2; t++) {
            g_tableX = (t == 0) ? -2.4f : 2.4f;
            interiorShadowLayer(g_tableX, 2.75f, 2.8f, 0.42f * k, tableCasters);
        }
    // dining pendants (key 1): counter, stools and tables
    // (with soft shadow mapping on (Y), the Phong shader shadows this light instead - see shadowmap.cpp)
    if (lightPoint && fixtureOn[FX_PENDANTS] && !shadowMapActive())
        interiorShadowLayer(0.0f, 2.85f, 0.0f, 0.26f * k, diningCasters);
    // (the paper box lanterns cast no separate shadow: their light comes through large
    //  paper panels, so a real lantern shadow is so soft it is barely visible.  One clear
    //  shadow per object, from the pendant lamps.)
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    enablePhongShader();
}

// Shadow-map casters for the dining pendant light: the same objects and places as in
// drawInterior() (including the moved / rotated selectable stool and bowl)
void drawShadowMapCasters()
{
    const float FY = FLOOR_Y, counterTop = FY + 1.06f;
    drawCounter({ 0, FY, -0.5f });
    for (int i = 0; i < 6; i++) {
        float x = -2.8f + i * 1.12f;
        glPushMatrix();
        glTranslatef(x, FY, 0.8f);
        if (i == 0) applyObjDelta(OBJ_STOOL);
        drawStool({ 0, 0, 0 });
        glPopMatrix();
    }
    for (int t = 0; t < 2; t++) { g_tableX = (t == 0) ? -2.4f : 2.4f; tableCasters(); }
    for (int i = 0; i < 4; i++) {
        float x = -2.2f + i * 1.5f;
        glPushMatrix();
        glTranslatef(x, counterTop, -0.3f);
        if (i == 0) applyObjDelta(OBJ_BOWL);
        drawRamenBowl({ 0, 0, 0 }, NO_ROT, { 0.24f, 0.24f, 0.24f });
        glPopMatrix();
    }
}

// ─── Ground ─────────────────────────────────────────────────────────────────
void drawGround()
{
    // ── Lawn: realistic procedural grass texture (unlit; see texture.cpp) ───
    // Unlit so a big plane has no per-vertex lighting gradient; a night tint keeps
    // it dark but still textured.  A second, large-scale layer breaks up tiling.
    setLighting(false);
    {
        const float ext = 120.0f, tile = 14.0f, macro = 90.0f, gy = -0.005f;
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
    drawTexturedBox({ 0, -0.20f, 0.5f }, NO_ROT, { 12.5f, 0.20f, 11.0f },
                    getTexID(TEX_DARK_WOOD), WHITE, 3.0f);
    // stencil = 1 only for the shop's own floor (inside the walls): interior shadows go there, the
    // outdoor sun shadows everywhere else (porch and lawn around the shop included)
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glBegin(GL_QUADS);
    glVertex3f(-4.9f, 0.002f, -3.9f); glVertex3f(4.9f, 0.002f, -3.9f);
    glVertex3f( 4.9f, 0.002f,  3.9f); glVertex3f(-4.9f, 0.002f,  3.9f);
    glEnd();
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_STENCIL_TEST);

    drawCuboid({ 0, -0.108f, 5.5f }, NO_ROT, { 13.0f, 0.10f, 1.2f }, WOOD);   // top 0.008 below the porch surface (was coplanar: z-fighting stripes)
    drawSidewalk({ 0, 0, 7.0f });
    drawStreet({ 0, 0, 12.0f });
    drawSidewalk({ 0, 0, 17.0f });   // far-side sidewalk across the street

    // Polished-floor reflection overlay (stencil-masked to the floor platform)
    // (floor reflection removed: it leaked the stool reflections out onto the porch and street)
    // (interior shadows are drawn in drawInterior, on top of the tiled floor)
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
    if (!sidewalk) {                                      // everything except the two sidewalk strips
        const float zs[4][2] = { { -130.0f, 5.5f }, { 8.5f, 15.5f }, { 18.5f, 130.0f } };
        for (int i = 0; i < 3; i++) {
            glVertex3f(-130.0f, h, zs[i][0]); glVertex3f(130.0f, h, zs[i][0]);
            glVertex3f( 130.0f, h, zs[i][1]); glVertex3f(-130.0f, h, zs[i][1]);
        }
    } else {                                              // the two sidewalk strips (z = 5.5..8.5 and 15.5..18.5)
        const float z0[2] = { 5.5f, 15.5f }, z1[2] = { 8.5f, 18.5f };
        for (int i = 0; i < 2; i++) {
            glVertex3f(-120.0f, h, z0[i]); glVertex3f(120.0f, h, z0[i]);
            glVertex3f( 120.0f, h, z1[i]); glVertex3f(-120.0f, h, z1[i]);
        }
    }
    glEnd();
}

// One shadow layer: flatten the outdoor scene onto plane y = h into the stencil
// buffer, then blend one soft dark layer over the marked pixels (and clear them).
//   lamp != 0 : restrict casters to a box around the point light (keeps it cheap)
static void shadowLayer(int mode, float lx, float ly, float lz, float h, bool sidewalk, float alpha, bool lamp)
{
    // Only the ground has been drawn at this point, so the depth test is not needed: with it,
    // the shadow planes (a few mm above the paving) z-fight and come out streaky / speckled.
    glDisable(GL_DEPTH_TEST);
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
    glColor4f(0.01f, 0.03f, 0.01f, sidewalk ? alpha : alpha * 1.15f);
    shadowRegion(sidewalk, h);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Night: very simple, static shadows (soft dark shapes on the sidewalk) for the cherry
// blossom tree and the small posts beside the shop.  They do not move with the breeze.
static void nightBlob(float cx, float cz, float rx, float rz, float alpha)
{
    const float h = 0.14f;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(cx, h, cz);
    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);                    // soft edge
    for (int i = 0; i <= 24; i++) {
        float a = i * 6.2832f / 24.0f;
        glVertex3f(cx + cosf(a) * rx, h, cz + sinf(a) * rz);
    }
    glEnd();
}
static void nightStrip(float x0, float z0, float x1, float z1, float w, float alpha)
{
    const float h = 0.14f;
    float dx = x1 - x0, dz = z1 - z0, l = sqrtf(dx * dx + dz * dz);
    float nx = -dz / l * w * 0.5f, nz = dx / l * w * 0.5f;
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.0f, 0.0f, alpha);
    glVertex3f(x0 + nx, h, z0 + nz); glVertex3f(x0 - nx, h, z0 - nz);
    glColor4f(0.0f, 0.0f, 0.0f, alpha * 0.4f);          // fades toward the tip
    glVertex3f(x1 - nx, h, z1 - nz); glVertex3f(x1 + nx, h, z1 + nz);
    glEnd();
}
static void drawNightStaticShadows()
{
    disablePhongShader();
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0, 0xFF);                     // outside ground only (not the shop floor)
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDepthMask(GL_FALSE);
    GLboolean cull = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);                             // only the ground is drawn so far: nothing can hide them
    glDisable(GL_CULL_FACE);                              // flat shapes seen from above: draw both windings
    const float sx = -0.45f, sz = 0.55f;                  // shadows fall slightly toward -x, +z
    // cherry blossom tree at (-5.5, 6.2): crown and trunk
    nightBlob(-5.5f + sx * 1.6f, 6.2f + sz * 1.6f, 1.8f, 1.3f, 0.62f);
    nightStrip(-5.5f, 6.2f, -5.5f + sx * 1.8f, 6.2f + sz * 1.8f, 0.32f, 0.60f);
    // small stone lantern posts and street lamp posts beside the shop
    const float px[4] = { -6.5f, 6.5f, -7.0f, 7.0f }, pz[4] = { 6.2f, 6.2f, 7.0f, 7.0f };
    for (int i = 0; i < 4; i++) {
        float len = (i < 2) ? 0.9f : 1.6f;
        nightStrip(px[i], pz[i], px[i] + sx * len, pz[i] + sz * len, (i < 2) ? 0.24f : 0.14f, 0.58f);
        nightBlob(px[i] + sx * len, pz[i] + sz * len, (i < 2) ? 0.22f : 0.28f, (i < 2) ? 0.20f : 0.24f, 0.55f);
        nightBlob(px[i], pz[i], 0.28f, 0.28f, 0.50f);     // contact shadow at the base
    }
    if (cull) glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    enablePhongShader();
}

static void drawExteriorShadows()
{
    if (!isDayTime) { drawNightStaticShadows(); return; }

    const float HG = 0.03f, HW = 0.135f;             // lawn / street, and sidewalk-top planes
    // Sun / moon direction (vector toward the light): shadows fall toward -x, +z
    const float sx = 0.40f, sy = 1.0f, sz = -0.52f;      // steeper sun: shorter shadows that stay attached to their objects
    const float sunA = 0.52f;

    disablePhongShader();
    shadowLayer(0, sx, sy, sz, HG, false, sunA, false);
    shadowLayer(0, sx, sy, sz, HW, true,  sunA, false);

    glDepthMask(GL_TRUE);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_LIGHTING);
    enablePhongShader();
}

// ─── Round paper lantern (chochin) ──────────────────────────────────────────
// Glowing orange paper globe with fine horizontal ribs, two darker vertical bands, gold
// caps and collar, a fringe and a hanging tassel.  Pivot (0,0,0) is the hanging point;
// the globe centre is 0.35 below it.  `em` scales the glow (flicker, daytime dimming).
static void drawChochinLantern(float em)
{
    const float R = 0.30f, H = 0.33f;
    const Color GOLD_C = { 0.86f, 0.68f, 0.16f }, TASSEL = { 0.98f, 0.82f, 0.12f };
    glPushMatrix();
    glTranslatef(0.0f, -0.35f, 0.0f);

    // paper globe
    setEmission(0.95f * em, 0.55f * em, 0.10f * em);
    setColor({ 0.97f, 0.66f, 0.16f });
    glPushMatrix();
    glScalef(R, H, R);
    gluSphere(quad, 1.0, 20, 14);
    glPopMatrix();
    clearEmission();

    // ribs and bands drawn unlit, scaled with the glow so they dim with the lantern
    float k = 0.45f + 0.55f * (em > 1.0f ? 1.0f : em);
    setLighting(false);
    glLineWidth(1.0f);
    glColor3f(0.62f * k, 0.30f * k, 0.04f * k);
    for (int i = 1; i < 14; i++) {                              // horizontal ribs
        float lat = -1.5708f + 3.14159f * i / 14.0f;
        float y = sinf(lat) * H * 1.006f, r = cosf(lat) * R * 1.006f;
        glBegin(GL_LINE_LOOP);
        for (int j = 0; j < 20; j++) { float a = j * 6.2832f / 20.0f; glVertex3f(cosf(a) * r, y, sinf(a) * r); }
        glEnd();
    }
    glColor3f(0.80f * k, 0.38f * k, 0.05f * k);
    for (int side = 0; side < 4; side++) {                      // darker vertical bands (front, back, sides)
        float a0 = side * 1.5708f;
        const float half = (side % 2 == 0) ? 0.30f : 0.16f;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 12; i++) {
            float lat = -1.35f + 2.70f * i / 12.0f;
            float y = sinf(lat) * H * 1.01f, r = cosf(lat) * R * 1.01f;
            glVertex3f(cosf(a0 - half) * r, y, sinf(a0 - half) * r);
            glVertex3f(cosf(a0 + half) * r, y, sinf(a0 + half) * r);
        }
        glEnd();
    }
    setLighting(true);

    // gold caps, collar ring and gold ornament band
    setMaterialGloss(0.9f, 0.8f, 0.4f, 60.0f);
    drawCylinder({ 0, H * 0.90f, 0 }, NO_ROT, { 0.20f, 0.075f, 0.20f }, GOLD_C);
    drawCylinder({ 0, -H * 0.90f - 0.065f, 0 }, NO_ROT, { 0.19f, 0.075f, 0.19f }, GOLD_C);
    drawTorus({ 0, H * 0.80f, 0 }, NO_ROT, ONE, GOLD_C, 0.012f, 0.205f);
    drawTorus({ 0, -H * 0.80f, 0 }, NO_ROT, ONE, GOLD_C, 0.012f, 0.205f);
    resetMaterialGloss();

    // fringe and tassel under the bottom cap
    float fy = -H * 0.90f - 0.065f;
    for (int i = 0; i < 14; i++) {
        float a = i * 6.2832f / 14.0f;
        drawCylinderCustom({ cosf(a) * 0.075f, fy - 0.17f, sinf(a) * 0.075f }, NO_ROT, ONE, TASSEL, 0.007f, 0.004f, 0.17f);
    }
    drawCylinder({ 0, fy - 0.30f, 0 }, NO_ROT, { 0.03f, 0.30f, 0.03f }, TASSEL);
    drawSphere({ 0, fy - 0.07f, 0 }, NO_ROT, { 0.05f, 0.05f, 0.05f }, { 0.55f, 0.85f, 0.25f });
    glPopMatrix();

    // small gold hook ring where it hangs
    drawTorus({ 0, 0.0f, 0 }, { 90, 0, 0 }, ONE, GOLD_C, 0.008f, 0.03f);
}

// ─── Exterior ───────────────────────────────────────────────────────────────
void drawExterior()
{
    // Real shadows first (ground only), then the actual objects on top
    drawExteriorShadows();
    drawExteriorBody();
}

// ════════════════════════════════════════════════════════════════════════════
//  MOVING THINGS: bicycle on the road, walking cat, nobori banner, ceiling fan
// ════════════════════════════════════════════════════════════════════════════

// Cylinder from point a to point b (used for tubes, limbs, spokes)
static void limb(Vec3 a, Vec3 b, float r, Color c)
{
    float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 1e-4f) return;
    glPushMatrix();
    glTranslatef(a.x, a.y, a.z);
    float ax = dz, az = -dx, al = sqrtf(ax * ax + az * az);        // rotation axis = Y x d
    float ang = acosf(fmaxf(-1.0f, fminf(1.0f, dy / len))) * 57.29578f;
    if (al > 1e-5f) glRotatef(ang, ax, 0.0f, az);
    else if (dy < 0.0f) glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    drawCylinderCustom({ 0, 0, 0 }, NO_ROT, ONE, c, r, r, len);
    glPopMatrix();
}

// Two-bone leg: knee found from the hip and foot positions (bends toward +x)
static Vec3 kneeFor(Vec3 hip, Vec3 foot, float L1, float L2)
{
    float ux = foot.x - hip.x, uy = foot.y - hip.y;
    float d = sqrtf(ux * ux + uy * uy);
    d = fmaxf(0.05f, fminf(d, L1 + L2 - 0.001f));
    float a = (L1 * L1 - L2 * L2 + d * d) / (2.0f * d);
    float h = sqrtf(fmaxf(0.0f, L1 * L1 - a * a));
    ux /= d; uy /= d;
    return { hip.x + a * ux - h * uy, hip.y + a * uy + h * ux, hip.z };
}

// ── Bicycle with a pedalling rider ─────────────────────────────────────────
// Rides along the road; the wheels roll (angle = distance / radius), the cranks and
// the rider's legs follow the pedals, a headlight glows at night.
static void drawBicycle()
{
    // rides from the left end to the right end of the street in front of the shop, then
    // starts again on the left
    const float speed = 7.0f, span = 32.0f, R = 0.33f, SC = 1.4f;
    float dist = fmodf(animTime * speed, span);
    float x = -16.0f + dist;
    float wheelDeg = -(dist / (R * SC)) * 57.29578f;                      // rolling without slipping
    float crank = -(dist / (R * SC)) * 0.55f;                             // pedals turn slower than the wheels

    const Color FRAME = { 0.70f, 0.10f, 0.08f }, TYRE = { 0.06f, 0.06f, 0.06f };
    const Color SHIRT = { 0.20f, 0.42f, 0.70f }, PANTS = { 0.15f, 0.15f, 0.20f }, SKIN = { 0.92f, 0.74f, 0.58f };

    glPushMatrix();
    glTranslatef(x, 0.03f, 10.6f);                                 // near lane of the street, riding toward +x
    glScalef(SC, SC, SC);                                          // a bit bigger than life-size so it reads from afar

    // wheels: tyre, rim, spokes, hub
    for (int w = 0; w < 2; w++) {
        float wx = w ? 0.52f : -0.52f;
        glPushMatrix();
        glTranslatef(wx, R, 0.0f);
        glRotatef(wheelDeg, 0, 0, 1);
        setMaterialGloss(0.3f, 0.3f, 0.3f, 30.0f);
        drawTorus({ 0, 0, 0 }, { 90, 0, 0 }, ONE, TYRE, 0.030f, R - 0.03f);
        setMaterialConductive(STEEL, 80.0f);
        drawTorus({ 0, 0, 0 }, { 90, 0, 0 }, ONE, STEEL, 0.010f, R - 0.07f);
        for (int k = 0; k < 6; k++) {
            float a = k * 3.14159f / 3.0f;
            limb({ 0, 0, 0 }, { cosf(a) * (R - 0.07f), sinf(a) * (R - 0.07f), 0 }, 0.004f, STEEL);
        }
        drawSphere({ 0, 0, 0 }, NO_ROT, { 0.06f, 0.06f, 0.08f }, STEEL);
        glPopMatrix();
    }

    // frame
    Vec3 RH = { -0.52f, R, 0 }, FH = { 0.52f, R, 0 }, BB = { -0.02f, 0.30f, 0 };
    Vec3 ST = { -0.20f, 0.86f, 0 }, HT = { 0.38f, 0.84f, 0 }, HB = { 0.34f, 1.02f, 0 };
    setMaterialConductive(FRAME, 70.0f);
    limb(BB, ST, 0.022f, FRAME);  limb(ST, HT, 0.020f, FRAME);  limb(BB, HT, 0.024f, FRAME);
    limb(RH, BB, 0.016f, FRAME);  limb(RH, ST, 0.014f, FRAME);
    limb(FH, HT, 0.018f, FRAME);  limb(HT, HB, 0.016f, FRAME);
    setMaterialConductive(STEEL, 80.0f);
    limb({ HB.x, HB.y, -0.24f }, { HB.x, HB.y, 0.24f }, 0.014f, STEEL);   // handlebar
    resetMaterialGloss();
    drawCuboid({ -0.22f, 0.88f, 0 }, NO_ROT, { 0.22f, 0.04f, 0.10f }, TYRE);  // saddle

    // cranks and pedals
    Vec3 P[2];
    for (int sd = 0; sd < 2; sd++) {
        float a = crank + sd * 3.14159f, zz = sd ? -0.11f : 0.11f;
        P[sd] = { BB.x + cosf(a) * 0.17f, BB.y + sinf(a) * 0.17f, zz };
        limb({ BB.x, BB.y, zz }, P[sd], 0.010f, DARK_GRAY);
        drawCuboid({ P[sd].x, P[sd].y - 0.01f, zz }, NO_ROT, { 0.09f, 0.02f, 0.07f }, DARK_GRAY);
    }

    // rider
    Vec3 hip = { -0.22f, 0.98f, 0 }, sh = { 0.04f, 1.46f, 0 };
    limb(hip, sh, 0.12f, SHIRT);                                           // torso
    drawSphere({ 0.10f, 1.64f, 0 }, NO_ROT, { 0.20f, 0.22f, 0.20f }, SKIN); // head
    drawSphere({ 0.08f, 1.70f, 0 }, NO_ROT, { 0.22f, 0.14f, 0.22f }, { 0.12f, 0.10f, 0.08f });  // hair
    for (int sd = 0; sd < 2; sd++) {
        float zz = sd ? -0.11f : 0.11f;
        Vec3 hp = { hip.x, hip.y, zz }, ft = { P[sd].x, P[sd].y + 0.03f, zz };
        Vec3 kn = kneeFor(hp, ft, 0.47f, 0.47f);
        limb(hp, kn, 0.055f, PANTS);  limb(kn, ft, 0.045f, PANTS);          // thigh, shin
        limb({ sh.x, sh.y - 0.04f, zz * 1.6f }, { HB.x, HB.y, zz * 2.0f }, 0.035f, SHIRT);   // arm to the bar
    }

    // headlight (glows at night) and rear reflector
    if (!isDayTime) setEmission(1.0f, 0.95f, 0.75f);
    drawSphere({ 0.44f, 0.92f, 0 }, NO_ROT, { 0.07f, 0.07f, 0.07f }, { 1.0f, 0.95f, 0.8f });
    clearEmission();
    drawSphere({ -0.60f, 0.70f, 0 }, NO_ROT, { 0.04f, 0.04f, 0.04f }, RED);
    glPopMatrix();

    // headlight beam on the road at night (additive, cheap)
    if (!isDayTime && !drawingShadow) {
        glPushMatrix();
        glTranslatef(x, 0.0f, 10.6f);
        glScalef(SC, 1.0f, SC);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE); setLighting(false);
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.92f, 0.70f, 0.30f); glVertex3f(0.45f, 0.06f, 0.0f);
        glColor4f(1.0f, 0.92f, 0.70f, 0.00f); glVertex3f(3.6f, 0.06f, -1.1f);
        glColor4f(1.0f, 0.92f, 0.70f, 0.00f); glVertex3f(3.6f, 0.06f, 1.1f);
        glEnd();
        setLighting(true); glDepthMask(GL_TRUE); glDisable(GL_BLEND);
        glPopMatrix();
    }
}

// ── Cats ───────────────────────────────────────────────────────────────────
// Cat head in its own frame (x = where the face looks); tilt in degrees about z/x.
static void drawCatHead(Vec3 c, float tiltZ, float tiltX, bool eyesClosed, Color fur, Color furD)
{
    const Color PINK = { 0.95f, 0.62f, 0.62f }, MUZ = { 0.95f, 0.88f, 0.76f };
    glPushMatrix();
    glTranslatef(c.x, c.y, c.z);
    glRotatef(tiltZ, 0, 0, 1);
    glRotatef(tiltX, 1, 0, 0);
    drawSphere({ 0, 0, 0 }, NO_ROT, { 0.15f, 0.14f, 0.15f }, fur);
    drawSphere({ -0.01f, 0.035f, 0 }, NO_ROT, { 0.11f, 0.05f, 0.08f }, furD);          // darker crown
    drawSphere({ 0.07f, -0.02f, 0 }, NO_ROT, { 0.06f, 0.05f, 0.07f }, MUZ);            // muzzle
    drawSphere({ 0.10f, -0.005f, 0 }, NO_ROT, { 0.018f, 0.014f, 0.022f }, PINK);       // nose
    for (int sd = -1; sd <= 1; sd += 2) {
        drawCone({ -0.02f, 0.05f, sd * 0.045f }, { sd * 12.0f, 0, 0 }, { 0.05f, 0.07f, 0.04f }, fur);    // ears
        drawCone({ -0.012f, 0.055f, sd * 0.045f }, { sd * 12.0f, 0, 0 }, { 0.025f, 0.045f, 0.02f }, PINK);
        if (eyesClosed)                                                                        // ^ ^ sleepy eyes
            drawSphere({ 0.067f, 0.018f, sd * 0.037f }, NO_ROT, { 0.020f, 0.005f, 0.018f }, { 0.12f, 0.08f, 0.06f });
        else
            drawSphere({ 0.065f, 0.02f, sd * 0.037f }, NO_ROT, { 0.022f, 0.022f, 0.016f }, { 0.25f, 0.55f, 0.15f });
    }
    glPopMatrix();
}

// Walking shop cat.  It walks along the sidewalk; every third lap it stops in front of the shop, turns to
// face the street, sits down and washes its face (lick paw, wipe over the ear), looks
// around for a moment, then turns back and walks on.
static void drawWalkingCat()
{
    const float speed = 3.0f, SC = 1.8f, X0 = -10.0f, XS = -1.2f, X1 = 10.0f;   // start, sitting spot, end
    const float tWalk1 = (XS - X0) / speed, tSit = 7.0f, tWalk2 = (X1 - XS) / speed;
    // three laps in a row: the first two it just walks, on the third it stops to wash
    const float TW = (X1 - X0) / speed, TS = tWalk1 + tSit + tWalk2;
    const float T = 2.0f * TW + TS;
    float t = fmodf(animTime + 5.0f, T);
    bool sitting = false;
    float x, st = 0.0f;
    if (t < 2.0f * TW) {                                             // plain walking laps
        x = X0 + speed * fmodf(t, TW);
    } else {
        t -= 2.0f * TW;
        sitting = (t >= tWalk1 && t < tWalk1 + tSit);
        x = sitting ? XS : (t < tWalk1 ? X0 + speed * t : XS + speed * (t - tWalk1 - tSit));
        st = t - tWalk1;                                             // time since sitting down
    }

    const Color FUR = { 0.86f, 0.52f, 0.20f }, FUR_D = { 0.62f, 0.34f, 0.12f }, PINK = { 0.95f, 0.62f, 0.62f };
    glPushMatrix();
    glTranslatef(x, 0.115f, 8.05f);
    glScalef(SC, SC, SC);
    setMaterialGloss(0.15f, 0.15f, 0.15f, 20.0f);

    if (!sitting) {
        float phase = animTime * 15.0f;                              // step rate to match the speed
        float bob = 0.012f * fabsf(sinf(phase));
        glTranslatef(0.0f, bob, 0.0f);
        const float lx[4] = { 0.15f, 0.15f, -0.15f, -0.15f }, lz[4] = { 0.06f, -0.06f, 0.06f, -0.06f };
        const float ph[4] = { 0.0f, 3.14159f, 3.14159f, 0.0f };
        for (int i = 0; i < 4; i++) {                                // legs (diagonal pairs move together)
            float sw = sinf(phase + ph[i]) * 0.06f, lift = fmaxf(0.0f, cosf(phase + ph[i])) * 0.025f;
            limb({ lx[i], 0.20f, lz[i] }, { lx[i] + sw, lift, lz[i] }, 0.022f, FUR);
            drawSphere({ lx[i] + sw, lift + 0.01f, lz[i] }, NO_ROT, { 0.05f, 0.03f, 0.05f }, FUR);
        }
        drawSphere({ 0, 0.24f, 0 }, NO_ROT, { 0.46f, 0.17f, 0.17f }, FUR);
        drawSphere({ 0.02f, 0.29f, 0 }, NO_ROT, { 0.34f, 0.08f, 0.13f }, FUR_D);
        drawCatHead({ 0.27f, 0.34f + sinf(phase * 0.5f) * 0.01f, 0 }, 0.0f, 0.0f, false, FUR, FUR_D);
        Vec3 prev = { -0.22f, 0.26f, 0 };                            // tail up, swaying
        for (int k = 1; k <= 5; k++) {
            float u = k / 5.0f;
            Vec3 nx = { -0.22f - 0.10f * u, 0.26f + 0.24f * u * u, sinf(animTime * 2.5f + u * 1.5f) * 0.06f * u };
            limb(prev, nx, 0.022f - 0.003f * u, k > 3 ? FUR_D : FUR);
            prev = nx;
        }
    } else {
        // turn from the walking direction (+x) to face the street (+z), and back before leaving
        float turnIn = fminf(1.0f, st / 0.5f), turnOut = fminf(1.0f, (tSit - st) / 0.5f);
        float k = fminf(turnIn, turnOut);
        k = k * k * (3.0f - 2.0f * k);
        glRotatef(-90.0f * k, 0, 1, 0);
        float sitK = fminf(1.0f, fminf(st, tSit - st) / 0.35f);      // lower into / rise from the sitting pose
        glTranslatef(0.0f, -0.03f * (1.0f - sitK), 0.0f);

        // body: haunches on the ground, chest upright
        drawSphere({ -0.08f, 0.12f, 0 }, NO_ROT, { 0.26f, 0.22f, 0.25f }, FUR);           // haunch
        drawSphere({ 0.02f, 0.25f, 0 }, { 0, 0, -18 }, { 0.20f, 0.34f, 0.18f }, FUR);     // chest
        drawSphere({ -0.03f, 0.27f, 0 }, { 0, 0, -18 }, { 0.12f, 0.24f, 0.10f }, FUR_D);  // back stripe
        drawSphere({ 0.08f, 0.26f, 0 }, NO_ROT, { 0.06f, 0.16f, 0.10f }, { 0.97f, 0.92f, 0.82f });   // white bib
        for (int sd = -1; sd <= 1; sd += 2)                                                 // folded hind paws
            drawSphere({ 0.04f, 0.015f, sd * 0.11f }, NO_ROT, { 0.13f, 0.04f, 0.06f }, FUR);

        // washing: 0.6 s .. 5.0 s — lick the paw, then wipe it over the face and ear
        float wt = st - 0.6f;
        bool washing = (wt > 0.0f && wt < 4.4f);
        float lick = washing ? sinf(wt * 9.0f) : 0.0f;              // fast little licks
        float wipe = washing ? (0.5f + 0.5f * sinf(wt * 2.2f)) : 0.0f;   // slow up-and-over wipe
        // left front leg stays on the ground
        limb({ 0.08f, 0.22f, -0.05f }, { 0.11f, 0.02f, -0.05f }, 0.022f, FUR);
        drawSphere({ 0.12f, 0.015f, -0.05f }, NO_ROT, { 0.05f, 0.03f, 0.05f }, FUR);
        // right front leg: on the ground, or raised to the face
        Vec3 sh = { 0.08f, 0.30f, 0.05f };
        Vec3 paw = washing ? Vec3{ 0.15f - 0.06f * wipe, 0.42f + 0.07f * wipe + 0.01f * lick, 0.05f + 0.02f * wipe }
                           : Vec3{ 0.11f, 0.02f, 0.05f };
        limb(sh, paw, 0.022f, FUR);
        drawSphere(paw, NO_ROT, { 0.05f, 0.04f, 0.05f }, FUR);
        if (washing) drawSphere({ paw.x + 0.01f, paw.y + 0.01f, paw.z }, NO_ROT, { 0.02f, 0.012f, 0.02f }, PINK);   // pink paw pad

        // head: leans down to the paw while licking, tilts while wiping; otherwise looks around
        float headTiltZ = washing ? (-14.0f + 10.0f * wipe + 2.0f * lick) : 6.0f * sinf(animTime * 0.9f);
        float headTiltX = washing ? (12.0f * wipe) : 0.0f;
        float look = washing ? 0.0f : 25.0f * sinf(st * 0.8f);
        glPushMatrix();
        glTranslatef(0.06f, 0.47f, 0.0f);
        glRotatef(look, 0, 1, 0);
        drawCatHead({ 0, 0, 0 }, headTiltZ, headTiltX, washing && lick > 0.3f, FUR, FUR_D);
        glPopMatrix();

        // tail curled round the front paws, the tip flicking
        Vec3 prev = { -0.20f, 0.04f, 0.0f };
        for (int kk = 1; kk <= 7; kk++) {
            float u = kk / 7.0f, a = -2.6f + 2.4f * u;
            Vec3 nx = { -0.02f + cosf(a) * 0.20f, 0.03f + (kk == 7 ? 0.03f * (0.5f + 0.5f * sinf(animTime * 3.0f)) : 0.0f),
                        -sinf(a) * 0.20f };
            limb(prev, nx, 0.022f - 0.002f * u, kk > 5 ? FUR_D : FUR);
            prev = nx;
        }
    }
    resetMaterialGloss();
    glPopMatrix();
}

// ── Person walking on the sidewalk (right to left) ─────────────────────────
// Hierarchical model: each leg is hip -> thigh -> knee -> shin -> foot, each arm swings
// from the shoulder opposite to its leg; the body bobs twice per stride.
static void drawWalkingPerson()
{
    const float speed = 1.8f, span = 26.0f;
    float x = 13.0f - fmodf(animTime * speed + 4.0f, span);       // right -> left, then again from the right
    float ph = animTime * speed / 1.4f * 6.2832f;                  // one stride (two steps) every 1.4 m
    float bob = 0.035f * fabsf(cosf(ph));

    const Color JACKET = { 0.55f, 0.18f, 0.16f }, PANTS = { 0.18f, 0.20f, 0.26f };
    const Color SKIN = { 0.92f, 0.74f, 0.58f }, HAIR = { 0.10f, 0.08f, 0.07f }, SHOE = { 0.12f, 0.10f, 0.09f };

    glPushMatrix();
    glTranslatef(x, 0.115f + bob, 6.85f);                          // sidewalk, between the shop and the lamp posts
    glRotatef(180.0f, 0, 1, 0);                                    // local +x = walking direction (world -x)
    setMaterialGloss(0.12f, 0.12f, 0.12f, 18.0f);

    const float hipY = 0.92f, thigh = 0.46f, shin = 0.44f;
    for (int sd = 0; sd < 2; sd++) {
        float zz = sd ? -0.10f : 0.10f;
        float a = (sd ? -1.0f : 1.0f) * 28.0f * sinf(ph);          // thigh swing (degrees)
        float bend = 35.0f * fmaxf(0.0f, sinf(ph + (sd ? 3.14159f : 0.0f) + 1.2f));   // knee bends while the leg swings forward
        glPushMatrix();
        glTranslatef(0.0f, hipY, zz);
        glRotatef(a, 0, 0, 1);                                     // hip joint
        limb({ 0, 0, 0 }, { 0, -thigh, 0 }, 0.065f, PANTS);
        glTranslatef(0.0f, -thigh, 0.0f);
        glRotatef(-bend, 0, 0, 1);                                 // knee joint
        limb({ 0, 0, 0 }, { 0, -shin, 0 }, 0.055f, PANTS);
        drawCuboid({ 0.05f, -shin - 0.03f, 0 }, NO_ROT, { 0.24f, 0.07f, 0.10f }, SHOE);
        glPopMatrix();

        // arm on the same side swings opposite to the leg
        glPushMatrix();
        glTranslatef(0.0f, 1.42f, zz * 2.1f);
        glRotatef(-a * 0.8f, 0, 0, 1);                             // shoulder joint
        limb({ 0, 0, 0 }, { 0, -0.30f, 0 }, 0.048f, JACKET);
        glTranslatef(0.0f, -0.30f, 0.0f);
        glRotatef(15.0f, 0, 0, 1);                                 // relaxed elbow
        limb({ 0, 0, 0 }, { 0, -0.27f, 0 }, 0.040f, JACKET);
        drawSphere({ 0, -0.30f, 0 }, NO_ROT, { 0.08f, 0.09f, 0.08f }, SKIN);   // hand
        glPopMatrix();
    }
    // hips, torso, neck, head
    drawCuboid({ 0, hipY - 0.06f, 0 }, NO_ROT, { 0.22f, 0.14f, 0.32f }, PANTS);
    drawCylinderCustom({ 0, hipY + 0.04f, 0 }, NO_ROT, ONE, JACKET, 0.15f, 0.18f, 0.44f);
    drawSphere({ 0, 1.46f, 0 }, NO_ROT, { 0.30f, 0.14f, 0.44f }, JACKET);    // shoulders
    limb({ 0, 1.48f, 0 }, { 0, 1.58f, 0 }, 0.05f, SKIN);
    drawSphere({ 0.01f, 1.69f, 0 }, NO_ROT, { 0.21f, 0.24f, 0.20f }, SKIN);
    drawSphere({ -0.02f, 1.74f, 0 }, NO_ROT, { 0.22f, 0.17f, 0.21f }, HAIR);
    resetMaterialGloss();
    glPopMatrix();
}

// ── Nobori banner beside the door ──────────────────────────────────────────
// Tall vertical cloth flag on a pole.  The cloth is a grid of quads displaced by a
// travelling sine wave that grows away from the pole (it is held on that edge).
static void drawNoboriBanner(float px, float pz)
{
    const float poleH = 3.0f, top = 2.85f, bot = 0.75f, W = 0.55f;
    const Color POLE = { 0.85f, 0.82f, 0.74f };
    setMaterialConductive(DARK_GRAY, 40.0f);
    drawCylinder({ px, 0.0f, pz }, NO_ROT, { 0.34f, 0.10f, 0.34f }, DARK_GRAY);   // weighted stand
    resetMaterialGloss();
    drawCylinder({ px, 0.0f, pz }, NO_ROT, { 0.045f, poleH, 0.045f }, POLE);
    limb({ px, top + 0.03f, pz }, { px + W + 0.08f, top + 0.03f, pz }, 0.016f, POLE);   // top arm
    drawSphere({ px, poleH + 0.03f, pz }, NO_ROT, { 0.08f, 0.08f, 0.08f }, GOLD);

    const int NU = 12, NV = 28;
    const Color BASE = { 0.12f, 0.20f, 0.52f }, BORDER = { 0.92f, 0.90f, 0.84f }, MARK = { 0.82f, 0.14f, 0.10f };
    auto wave = [&](float u, float v) {                     // u: 0 at pole .. 1 free edge, v: 0 top .. 1 bottom
        return sinf(animTime * 3.2f - u * 4.0f + v * 1.5f) * 0.09f * u + sinf(animTime * 5.1f + v * 3.0f) * 0.015f * u;
    };
    auto colorAt = [&](float u, float v) -> Color {
        if (u < 0.08f || u > 0.92f || v < 0.04f) return BORDER;            // white edging
        float cu = (u - 0.5f) * W, cv = (v - 0.22f) * (top - bot);
        if (cu * cu + cv * cv < 0.16f * 0.16f) return MARK;                // red emblem disc
        if (v > 0.42f && v < 0.90f && fabsf(u - 0.5f) < 0.10f &&
            fmodf(v * 10.0f, 1.0f) < 0.55f) return BORDER;                  // simple brush-stroke marks
        return BASE;
    };
    GLboolean cull = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
    setMaterialGloss(0.05f, 0.05f, 0.05f, 10.0f);          // cloth: matte
    glBegin(GL_QUADS);
    for (int i = 0; i < NU; i++)
        for (int j = 0; j < NV; j++) {
            float us[4] = { (float)i / NU, (float)(i + 1) / NU, (float)(i + 1) / NU, (float)i / NU };
            float vs[4] = { (float)j / NV, (float)j / NV, (float)(j + 1) / NV, (float)(j + 1) / NV };
            Color c = colorAt((us[0] + us[1]) * 0.5f, (vs[0] + vs[2]) * 0.5f);
            glColor3f(c.r, c.g, c.b);
            for (int k = 0; k < 4; k++) {
                float u = us[k], v = vs[k], e = 0.01f;
                float dzdu = (wave(u + e, v) - wave(u, v)) / e;            // slope of the cloth -> normal
                glNormal3f(-dzdu / W, 0.0f, 1.0f);
                glVertex3f(px + 0.05f + u * W, top - v * (top - bot), pz + wave(u, v));
            }
        }
    glEnd();
    resetMaterialGloss();
    if (cull) glEnable(GL_CULL_FACE);
}

// ── Furin (glass wind chime) under the porch awning ────────────────────────
// Two pendulums driven by a gusty wind and integrated with real physics:
//   bell:            theta1'' = -(g/L1) sin(theta1) - c1 theta1' + kBell  * wind(t)
//   clapper + strip: theta2'' = -(g/L2) sin(theta2) - c2 theta2' + kStrip * wind(t)
// The paper strip (tanzaku) catches far more wind than the glass bell, so the clapper
// swings further and strikes the inside of the bell; each strike bounces it back and
// makes the glass glint.
static float g_fT1 = 0.0f, g_fW1 = 0.0f, g_fT2 = 0.0f, g_fW2 = 0.0f, g_fGlint = 0.0f, g_fLast = -1.0f;

static float furinWind(float t)
{
    float gust = 0.55f + 0.45f * sinf(t * 0.37f) * sinf(t * 0.23f + 1.1f);                 // slow gusts
    return gust * (0.9f * sinf(t * 1.7f) + 0.55f * sinf(t * 2.9f + 1.3f) + 0.3f * sinf(t * 5.3f + 0.4f) + 0.25f);
}

static void stepFurin()
{
    if (g_fLast < 0.0f) { g_fLast = animTime; return; }
    float dt = animTime - g_fLast;
    g_fLast = animTime;
    if (dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f;
    const float G = 9.81f, L1 = 0.12f, L2 = 0.20f, C1 = 2.2f, C2 = 1.6f, KB = 3.0f, KS = 22.0f;
    const float LIMIT = 0.42f;                                      // clapper touches the rim at this relative angle
    const int N = 8;
    const float h = dt / N;
    for (int i = 0; i < N; i++) {                                   // semi-implicit Euler substeps
        float t = g_fLast - dt + (i + 1) * h, w = furinWind(t);
        g_fW1 += (-(G / L1) * sinf(g_fT1) - C1 * g_fW1 + KB * w) * h;
        g_fW2 += (-(G / L2) * sinf(g_fT2) - C2 * g_fW2 + KS * w) * h;
        g_fT1 += g_fW1 * h;
        g_fT2 += g_fW2 * h;
        float rel = g_fT2 - g_fT1, relW = g_fW2 - g_fW1;
        if (fabsf(rel) > LIMIT && rel * relW > 0.0f) {              // strike: bounce back off the glass
            g_fT2 = g_fT1 + (rel > 0.0f ? LIMIT : -LIMIT);
            g_fW2 = g_fW1 - 0.45f * relW;
            g_fGlint = fminf(1.0f, g_fGlint + 0.8f * fminf(1.0f, fabsf(relW) * 0.5f) + 0.3f);
        }
    }
    g_fGlint *= expf(-dt * 3.0f);                                   // glint fades
}

static void drawFurin(float px, float py, float pz)
{
    if (!outdoorShadowPass) stepFurin();
    const float RAD = 57.29578f;
    const float side = 4.0f * sinf(animTime * 0.9f + 0.7f);        // a little sideways sway too

    glPushMatrix();
    glTranslatef(px, py, pz);
    // hanging cord from the awning to the bell
    limb({ 0, 0, 0 }, { 0, -0.02f, 0 }, 0.012f, { 0.30f, 0.20f, 0.12f });                     // small hook
    glRotatef(g_fT1 * RAD, 1, 0, 0);                                // bell swing (towards / away from the street)
    glRotatef(side, 0, 0, 1);
    limb({ 0, -0.02f, 0 }, { 0, -0.12f, 0 }, 0.003f, { 0.85f, 0.80f, 0.70f });

    // ── glass bell: thin dome, clear pale glass with painted goldfish, open at the bottom ──
    glPushMatrix();
    glTranslatef(0.0f, -0.12f, 0.0f);
    {
        const int SEG = 24, NP = 6;
        const float prof[NP][2] = { { 0.012f, 0.0f }, { 0.040f, -0.010f }, { 0.060f, -0.028f },
                                    { 0.070f, -0.050f }, { 0.074f, -0.072f }, { 0.075f, -0.085f } };   // {radius, y}
        GLboolean cull = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        setMaterialGloss(1.0f, 1.0f, 1.0f, 120.0f);                 // glass: sharp bright highlight (the glint)
        for (int i = 0; i + 1 < NP; i++) {
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= SEG; j++) {
                float a = j * 6.2832f / SEG, ca = cosf(a), sa = sinf(a);
                for (int k = 0; k < 2; k++) {
                    int m = i + k;
                    float dr = prof[(m + 1 < NP) ? m + 1 : m][0] - prof[m > 0 ? m - 1 : m][0];
                    float dy = prof[(m + 1 < NP) ? m + 1 : m][1] - prof[m > 0 ? m - 1 : m][1];
                    float nl = sqrtf(dr * dr + dy * dy), nr = -dy / nl, ny = dr / nl;
                    // painted goldfish: red patches on the lower half, small blue water lines
                    bool fish = (m >= 2 && m <= 4) && (fmodf(a + 0.3f, 2.0944f) < 0.55f);
                    bool water = (m == 4) && !fish && (j % 3 == 0);
                    if (fish)       glColor4f(0.90f, 0.18f, 0.12f, 0.95f);
                    else if (water) glColor4f(0.25f, 0.45f, 0.85f, 0.85f);
                    else            glColor4f(0.86f, 0.94f, 0.98f, 0.30f);
                    glNormal3f(nr * ca, ny, nr * sa);
                    glVertex3f(prof[m][0] * ca, prof[m][1], prof[m][0] * sa);
                }
            }
            glEnd();
        }
        // rim ring catches the light
        glColor4f(0.95f, 0.98f, 1.0f, 0.75f);
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= SEG; j++) {
            float a = j * 6.2832f / SEG;
            glNormal3f(cosf(a), -0.3f, sinf(a));
            glVertex3f(0.075f * cosf(a), -0.085f, 0.075f * sinf(a));
            glVertex3f(0.077f * cosf(a), -0.090f, 0.077f * sinf(a));
        }
        glEnd();
        resetMaterialGloss();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        if (cull) glEnable(GL_CULL_FACE);

        // ── clapper (zetsu) and paper strip (tanzaku): their own pendulum inside the bell ──
        glPushMatrix();
        glRotatef((g_fT2 - g_fT1) * RAD, 1, 0, 0);                  // relative to the bell
        limb({ 0, 0, 0 }, { 0, -0.07f, 0 }, 0.0025f, { 0.85f, 0.80f, 0.70f });
        setMaterialGloss(0.9f, 0.9f, 0.9f, 90.0f);
        drawCylinderCustom({ 0, -0.085f, 0 }, NO_ROT, ONE, { 0.75f, 0.85f, 0.90f }, 0.011f, 0.011f, 0.018f);   // glass clapper
        resetMaterialGloss();
        limb({ 0, -0.085f, 0 }, { 0, -0.20f, 0 }, 0.0025f, { 0.85f, 0.80f, 0.70f });
        // tanzaku: paper strip that twists and flutters in the wind
        glTranslatef(0.0f, -0.20f, 0.0f);
        glRotatef(25.0f * sinf(animTime * 2.3f) + 12.0f * furinWind(animTime), 0, 1, 0);
        glRotatef(8.0f * sinf(animTime * 6.1f), 1, 0, 0);
        {
            GLboolean cull2 = glIsEnabled(GL_CULL_FACE);
            glDisable(GL_CULL_FACE);
            setMaterialGloss(0.05f, 0.05f, 0.05f, 10.0f);
            const int NS = 8;
            const float W = 0.055f, Hs = 0.24f;
            glBegin(GL_QUAD_STRIP);
            for (int k = 0; k <= NS; k++) {
                float v = (float)k / NS;
                float bend = 0.03f * v * v * sinf(animTime * 4.0f - v * 3.0f);   // ripples down the strip
                glNormal3f(0.0f, 0.0f, 1.0f);
                if (k > 0 && k < NS && (k % 3 == 1)) glColor3f(0.15f, 0.12f, 0.10f);   // brush-written poem marks
                else glColor3f(0.92f, 0.30f, 0.32f);                                    // pink-red washi
                glVertex3f(-W * 0.5f, -v * Hs, bend);
                glVertex3f( W * 0.5f, -v * Hs, bend);
            }
            glEnd();
            resetMaterialGloss();
            if (cull2) glEnable(GL_CULL_FACE);
        }
        glPopMatrix();

        // ── glint: a small star flash on the rim when the clapper strikes ──
        if (g_fGlint > 0.05f && !outdoorShadowPass) {
            float m[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, m);
            Vec3 R = { m[0], m[4], m[8] }, U = { m[1], m[5], m[9] };            // camera right / up in this frame
            float sz = 0.06f * g_fGlint;
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            setLighting(false);
            glBegin(GL_TRIANGLES);
            for (int arm = 0; arm < 4; arm++) {
                float a = arm * 1.5708f + 0.4f;
                Vec3 d = { R.x * cosf(a) + U.x * sinf(a), R.y * cosf(a) + U.y * sinf(a), R.z * cosf(a) + U.z * sinf(a) };
                Vec3 q = { -d.y * 0.0f + (R.x * -sinf(a) + U.x * cosf(a)) * 0.12f,
                           (R.y * -sinf(a) + U.y * cosf(a)) * 0.12f,
                           (R.z * -sinf(a) + U.z * cosf(a)) * 0.12f };
                const float cx = 0.05f, cy = -0.08f, cz = 0.05f;
                glColor4f(1.0f, 1.0f, 0.95f, 0.9f * g_fGlint);
                glVertex3f(cx + q.x * sz, cy + q.y * sz, cz + q.z * sz);
                glVertex3f(cx - q.x * sz, cy - q.y * sz, cz - q.z * sz);
                glColor4f(1.0f, 1.0f, 0.95f, 0.0f);
                glVertex3f(cx + d.x * sz, cy + d.y * sz, cz + d.z * sz);
            }
            glEnd();
            setLighting(true);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }
    glPopMatrix();
    glPopMatrix();
}

// ── Ceiling fan (upstairs tatami room) ─────────────────────────────────────
static void drawCeilingFan(Vec3 p)
{
    const Color BRASS = { 0.78f, 0.60f, 0.26f };
    const float spin = animTime * 160.0f;                           // degrees: about 0.45 turns per second
    setMaterialConductive(DARK_GRAY, 60.0f);
    drawCylinder({ p.x, p.y - 0.06f, p.z }, NO_ROT, { 0.16f, 0.06f, 0.16f }, DARK_GRAY);   // ceiling canopy
    drawCylinder({ p.x, p.y - 0.36f, p.z }, NO_ROT, { 0.03f, 0.30f, 0.03f }, DARK_GRAY);   // down-rod
    drawCylinderCustom({ p.x, p.y - 0.52f, p.z }, NO_ROT, ONE, DARK_GRAY, 0.10f, 0.14f, 0.16f);  // motor
    resetMaterialGloss();
    glPushMatrix();
    glTranslatef(p.x, p.y - 0.50f, p.z);
    glRotatef(spin, 0, 1, 0);                                      // blades turn about the vertical axis
    for (int b = 0; b < 4; b++) {
        glPushMatrix();
        glRotatef(b * 90.0f, 0, 1, 0);
        setMaterialConductive(BRASS, 70.0f);
        drawCuboid({ 0.22f, 0.0f, 0.0f }, NO_ROT, { 0.16f, 0.02f, 0.05f }, BRASS);            // blade iron
        glRotatef(10.0f, 1, 0, 0);                                                            // blade pitch
        setMaterialGloss(0.45f, 0.42f, 0.36f, 60.0f);
        drawTexturedBox({ 0.62f, -0.01f, 0.0f }, NO_ROT, { 0.70f, 0.02f, 0.16f }, getTexID(TEX_DARK_WOOD), WHITE, 1.0f);
        resetMaterialGloss();
        glPopMatrix();
    }
    glPopMatrix();
    setMaterialConductive(BRASS, 70.0f);
    drawSphere({ p.x, p.y - 0.56f, p.z }, NO_ROT, { 0.10f, 0.08f, 0.10f }, BRASS);           // bottom cap
    resetMaterialGloss();
}

static void drawExteriorBody()
{
    // Seen from OUTSIDE the shell is lit by everything, as before (lantern-lit facade).  Seen from
    // INSIDE it is lit like the interior only: the lanterns / street lamps / sign light outside have
    // no occlusion in OpenGL, so they would light the inner faces of the walls and floor through
    // the building and a "light switched off" interior would still glow.
    const bool shellIndoors = !outdoorShadowPass && cameraInsideShop();
    if (shellIndoors) setInteriorLightScope(true);
    drawShopBuilding({ 0, 0, 0 });
    if (showRoof) drawRoof({ 0, 0, 0 });
    if (shellIndoors) setInteriorLightScope(false);

    // The shop's own paper windows and doors glow because of the lamps inside: no interior light, no glow
    emissionScale = anyInteriorLightOn() ? 1.0f : 0.10f;
    if (shellIndoors) setInteriorLightScope(true);          // seen from inside: lit like the interior
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
    if (shellIndoors) setInteriorLightScope(false);
    emissionScale = 1.0f;
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
    drawChochinLantern(flickL * dayLanScale);
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
    drawChochinLantern(flickR * dayLanScale);
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

    // ── Moving things: bicycle on the road, cat on the sidewalk, banner by the door ──
    drawBicycle();
    drawWalkingCat();
    drawWalkingPerson();
    drawNoboriBanner(2.35f, 5.25f);
    drawFurin(-1.05f, 3.13f, 5.0f);          // glass wind chime under the porch awning, left of the door

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

    // ── Cherry blossom trees along the road beside the houses ─────────────
    drawCherryBlossomTree({ -19.0f, 0,  4.6f }, NO_ROT, { 1.0f, 1.1f, 1.0f });
    drawCherryBlossomTree({  19.0f, 0,  4.6f }, NO_ROT, { 1.0f, 1.1f, 1.0f });
    drawCherryBlossomTree({ -12.0f, 0, 20.8f }, NO_ROT, { 1.0f, 1.1f, 1.0f });
    drawCherryBlossomTree({  12.0f, 0, 20.8f }, NO_ROT, { 1.0f, 1.1f, 1.0f });
    drawCherryBlossomTree({ -20.0f, 0, 21.0f }, NO_ROT, { 0.95f, 1.05f, 0.95f });
    drawCherryBlossomTree({  20.0f, 0, 21.0f }, NO_ROT, { 0.95f, 1.05f, 0.95f });

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
    drawJapanesePineTree({ -25.0f, 0, -13.0f }, NO_ROT, { 1.3f, 1.7f, 1.3f });

    // Maple trees adding autumn colour variety
    drawMapleTree({ -17.0f, 0, -14.0f }, NO_ROT, { 1.1f, 1.3f, 1.1f });
    drawMapleTree({ -24.0f, 0, -16.0f }, NO_ROT, { 1.0f, 1.2f, 1.0f });

    // Regular broad-leaf trees filling gaps
    drawTree({ -18.0f, 0, -11.0f }, NO_ROT, { 1.0f, 1.4f, 1.0f });
    drawTree({ -30.0f, 0, -22.0f }, NO_ROT, { 1.2f, 1.6f, 1.2f });

    // Cherry blossom accents at forest edge

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
    drawBambooGrove({ 22.0f, 0, -30.0f }, { 0, -50, 0 }, { 1.3f, 1.5f, 1.3f });

    // Cherry blossom trees at bamboo grove edges (blossoms cascading over bamboo)

    // Red torii gate at the bamboo grove entrance
    drawToriiGate({ 20.0f, 0, -9.0f }, { 0, 90, 0 });


    // Falling cherry blossom petals drifting through the bamboo canopy
    drawFallingPetals({ 22.0f, 5.0f, -18.0f }, 12.0f, 30);

    // ══════════════════════════════════════════════════════════════════════
    //  FAR-SIDE FOREST — behind the far houses (z > 25)
    //  Mixed forest: pines, maples, cherry blossoms, undergrowth
    // ══════════════════════════════════════════════════════════════════════

    // Pine trees (primary canopy)
    drawJapanesePineTree({ -12.0f, 0, 28.0f }, NO_ROT, { 1.0f, 1.3f, 1.0f });
    drawJapanesePineTree({  14.0f, 0, 27.0f }, NO_ROT, { 0.9f, 1.1f, 0.9f });
    drawJapanesePineTree({  18.0f, 0, 35.0f }, NO_ROT, { 1.2f, 1.7f, 1.2f });

    // Maple trees interspersed
    drawMapleTree({ -5.0f, 0, 26.0f }, NO_ROT, { 1.0f, 1.2f, 1.0f });
    drawMapleTree({  10.0f, 0, 32.0f }, NO_ROT, { 0.9f, 1.0f, 0.9f });

    // Cherry blossom trees for colour accents

    // Dense undergrowth filling the far forest floor
    drawJungle({ -10.0f, 0, 30.0f }, { 0, 40, 0 }, { 1.5f, 0.8f, 1.5f });
    drawJungle({ -22.0f, 0, 32.0f }, { 0, 60, 0 }, { 1.6f, 0.9f, 1.6f });
    drawJungle({   0.0f, 0, 36.0f }, { 0, 30, 0 }, { 2.0f, 0.9f, 2.0f });

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

    // ══════════════════════════════════════════════════════════════════════
    //  PERIMETER JUNGLE — dense forest wall at world edges
    //  Moved away from road (z>5.5) and lake (z~-24) zones
    // ══════════════════════════════════════════════════════════════════════

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
// ─── Wall pictures for the tatami room ──────────────────────────────────────
// Each picture is painted procedurally on a flat canvas in its own local frame
// (x right, y up, the canvas facing +z), out of simple 2D shapes.  Shapes are lit
// like the wall (normal +z), so the pictures get darker when the lights go off.
namespace pic {
static float zl = 0.0f;                                   // stacking offset so later shapes sit on top
static void layer() { zl += 0.0012f; }
static void col(Color c) { glColor3f(c.r, c.g, c.b); }
static void rect(float x0, float y0, float x1, float y1, Color c)
{
    layer(); col(c);
    glBegin(GL_QUADS); glNormal3f(0, 0, 1);
    glVertex3f(x0, y0, zl); glVertex3f(x1, y0, zl); glVertex3f(x1, y1, zl); glVertex3f(x0, y1, zl);
    glEnd();
}
static void grad(float x0, float y0, float x1, float y1, Color bot, Color topc)      // vertical gradient
{
    layer();
    glBegin(GL_QUADS); glNormal3f(0, 0, 1);
    col(bot);  glVertex3f(x0, y0, zl); glVertex3f(x1, y0, zl);
    col(topc); glVertex3f(x1, y1, zl); glVertex3f(x0, y1, zl);
    glEnd();
}
static void ellipse(float cx, float cy, float rx, float ry, Color c, float a0 = 0.0f, float a1 = 6.2832f)
{
    layer(); col(c);
    glBegin(GL_TRIANGLE_FAN); glNormal3f(0, 0, 1);
    glVertex3f(cx, cy, zl);
    for (int i = 0; i <= 28; i++) { float a = a0 + (a1 - a0) * i / 28.0f; glVertex3f(cx + cosf(a) * rx, cy + sinf(a) * ry, zl); }
    glEnd();
}
static void ring(float cx, float cy, float r, float w, Color c)
{
    layer(); col(c);
    glBegin(GL_QUAD_STRIP); glNormal3f(0, 0, 1);
    for (int i = 0; i <= 28; i++) {
        float a = 6.2832f * i / 28.0f;
        glVertex3f(cx + cosf(a) * (r - w), cy + sinf(a) * (r - w), zl);
        glVertex3f(cx + cosf(a) * r, cy + sinf(a) * r, zl);
    }
    glEnd();
}
static void tri(float x0, float y0, float x1, float y1, float x2, float y2, Color c)
{
    layer(); col(c);
    glBegin(GL_TRIANGLES); glNormal3f(0, 0, 1);
    glVertex3f(x0, y0, zl); glVertex3f(x1, y1, zl); glVertex3f(x2, y2, zl);
    glEnd();
}
static void line(float x0, float y0, float x1, float y1, float w, Color c)            // thick stroke
{
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    if (l < 1e-5f) return;
    float nx = -dy / l * w * 0.5f, ny = dx / l * w * 0.5f;
    layer(); col(c);
    glBegin(GL_QUADS); glNormal3f(0, 0, 1);
    glVertex3f(x0 + nx, y0 + ny, zl); glVertex3f(x0 - nx, y0 - ny, zl);
    glVertex3f(x1 - nx, y1 - ny, zl); glVertex3f(x1 + nx, y1 + ny, zl);
    glEnd();
}

// Triangle clipped to the rectangle [x0,x1] x [y0,y1] (Sutherland-Hodgman), so rays
// and other shapes never spill out of the picture
static void triClip(float ax, float ay, float bx, float by, float cx, float cy, Color c,
                    float x0, float y0, float x1, float y1)
{
    float px[12] = { ax, bx, cx }, py[12] = { ay, by, cy };
    int n = 3;
    for (int e = 0; e < 4 && n > 0; e++) {
        float qx[12], qy[12]; int m = 0;
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            auto inside = [&](float x, float y) { return e == 0 ? x >= x0 : e == 1 ? x <= x1 : e == 2 ? y >= y0 : y <= y1; };
            bool ii = inside(px[i], py[i]), jj = inside(px[j], py[j]);
            if (ii) { qx[m] = px[i]; qy[m] = py[i]; m++; }
            if (ii != jj) {
                float t = (e < 2) ? ((e == 0 ? x0 : x1) - px[i]) / (px[j] - px[i]) : ((e == 2 ? y0 : y1) - py[i]) / (py[j] - py[i]);
                qx[m] = px[i] + t * (px[j] - px[i]); qy[m] = py[i] + t * (py[j] - py[i]); m++;
            }
        }
        n = m; for (int i = 0; i < n; i++) { px[i] = qx[i]; py[i] = qy[i]; }
    }
    if (n < 3) return;
    layer(); col(c);
    glBegin(GL_POLYGON); glNormal3f(0, 0, 1);
    for (int i = 0; i < n; i++) glVertex3f(px[i], py[i], zl);
    glEnd();
}

// Frame (dark wood) + white mat around a w x h picture centred at the origin
static void frame(float w, float h, bool wood)
{
    const float m = wood ? 0.035f : 0.0f, b = 0.03f;
    if (wood) {
        setMaterialGloss(0.4f, 0.36f, 0.30f, 50.0f);
        const Color F = { 0.22f, 0.13f, 0.08f };
        drawCuboid({ 0, -h * 0.5f - m - b, 0.012f }, NO_ROT, { w + 2 * (m + b), b, 0.025f }, F);
        drawCuboid({ 0,  h * 0.5f + m,     0.012f }, NO_ROT, { w + 2 * (m + b), b, 0.025f }, F);
        drawCuboid({ -w * 0.5f - m - b * 0.5f, -h * 0.5f - m, 0.012f }, NO_ROT, { b, h + 2 * m, 0.025f }, F);
        drawCuboid({  w * 0.5f + m + b * 0.5f, -h * 0.5f - m, 0.012f }, NO_ROT, { b, h + 2 * m, 0.025f }, F);
        resetMaterialGloss();
        zl = 0.0f;
        rect(-w * 0.5f - m, -h * 0.5f - m, w * 0.5f + m, h * 0.5f + m, { 0.95f, 0.93f, 0.88f });   // mat
    } else {
        zl = 0.0f;
    }
}

// 1. Mt. Fuji with a red sun and cherry blossoms
static void fuji(float w, float h)
{
    frame(w, h, true);
    float x0 = -w * 0.5f, x1 = w * 0.5f, y0 = -h * 0.5f, y1 = h * 0.5f;
    grad(x0, y0, x1, y1, { 0.98f, 0.82f, 0.62f }, { 0.55f, 0.75f, 0.95f });
    ellipse(x1 - 0.12f, y1 - 0.09f, 0.055f, 0.055f, { 0.88f, 0.15f, 0.12f });
    tri(x0 + 0.02f, y0 + 0.05f, x1 - 0.02f, y0 + 0.05f, 0.0f, y0 + h * 0.72f, { 0.30f, 0.38f, 0.58f });
    tri(-0.075f, y0 + h * 0.56f, 0.075f, y0 + h * 0.56f, 0.0f, y0 + h * 0.72f, { 0.97f, 0.97f, 1.0f });
    tri(-0.075f, y0 + h * 0.56f, -0.03f, y0 + h * 0.56f, -0.05f, y0 + h * 0.50f, { 0.97f, 0.97f, 1.0f });
    tri(0.02f, y0 + h * 0.56f, 0.075f, y0 + h * 0.56f, 0.05f, y0 + h * 0.51f, { 0.97f, 0.97f, 1.0f });
    rect(x0, y0, x1, y0 + 0.06f, { 0.35f, 0.55f, 0.35f });
    for (int i = 0; i < 9; i++)
        ellipse(x0 + 0.04f + i * (w - 0.08f) / 8.0f, y0 + 0.05f + 0.02f * sinf(i * 2.1f), 0.028f, 0.022f, { 0.98f, 0.70f, 0.80f });
}

// 2. Maneki-neko, the beckoning lucky cat
static void luckyCat(float w, float h)
{
    frame(w, h, true);
    const Color WH = { 0.98f, 0.97f, 0.94f }, BK = { 0.08f, 0.08f, 0.08f }, RD = { 0.85f, 0.15f, 0.15f }, GD = { 0.95f, 0.75f, 0.20f };
    rect(-w * 0.5f, -h * 0.5f, w * 0.5f, h * 0.5f, { 0.98f, 0.86f, 0.40f });
    for (int i = 0; i < 12; i++)                                                         // sunburst
        triClip(0, 0.02f, cosf(i * 0.5236f) * 0.4f, 0.02f + sinf(i * 0.5236f) * 0.4f,
                cosf(i * 0.5236f + 0.26f) * 0.4f, 0.02f + sinf(i * 0.5236f + 0.26f) * 0.4f, { 1.0f, 0.92f, 0.55f },
                -w * 0.5f, -h * 0.5f, w * 0.5f, h * 0.5f);
    ellipse(0, -0.09f, 0.11f, 0.10f, WH);                                                 // body
    ellipse(0.0f, 0.06f, 0.10f, 0.085f, WH);                                              // head
    tri(-0.09f, 0.10f, -0.03f, 0.13f, -0.08f, 0.17f, WH); tri(0.09f, 0.10f, 0.03f, 0.13f, 0.08f, 0.17f, WH);   // ears
    tri(-0.08f, 0.115f, -0.045f, 0.13f, -0.075f, 0.155f, { 0.95f, 0.60f, 0.65f });
    tri(0.08f, 0.115f, 0.045f, 0.13f, 0.075f, 0.155f, { 0.95f, 0.60f, 0.65f });
    ellipse(0.12f, 0.08f, 0.035f, 0.05f, WH);                                            // raised paw
    line(0.10f, 0.03f, 0.12f, 0.06f, 0.05f, WH);
    ellipse(-0.035f, 0.065f, 0.015f, 0.006f, BK, 0.0f, 3.1416f);                         // happy closed eyes
    ellipse(0.035f, 0.065f, 0.015f, 0.006f, BK, 0.0f, 3.1416f);
    ellipse(0.0f, 0.040f, 0.007f, 0.005f, { 0.95f, 0.55f, 0.60f });                      // nose
    for (int k = -1; k <= 1; k += 2) { line(k * 0.04f, 0.035f, k * 0.09f, 0.045f, 0.003f, BK); line(k * 0.04f, 0.030f, k * 0.09f, 0.025f, 0.003f, BK); }
    rect(-0.08f, -0.005f, 0.08f, 0.012f, RD);                                            // collar
    ellipse(0.0f, -0.012f, 0.018f, 0.018f, GD);                                           // bell
    ellipse(0.0f, -0.10f, 0.05f, 0.032f, GD);                                             // koban coin
    rect(-0.025f, -0.105f, 0.025f, -0.095f, { 0.70f, 0.50f, 0.10f });
    ellipse(-0.09f, 0.04f, 0.018f, 0.010f, { 0.98f, 0.70f, 0.70f });                     // blush
    ellipse(0.09f, 0.04f, 0.018f, 0.010f, { 0.98f, 0.70f, 0.70f });
}

// 3. Kawaii onigiri (rice ball with a face)
static void onigiri(float w, float h)
{
    frame(w, h, true);
    const Color RICE = { 0.99f, 0.99f, 0.97f }, NORI = { 0.12f, 0.18f, 0.12f }, BK = { 0.10f, 0.08f, 0.08f };
    rect(-w * 0.5f, -h * 0.5f, w * 0.5f, h * 0.5f, { 0.99f, 0.80f, 0.85f });
    for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++)
        if ((i + j) % 2 == 0) ellipse(-w * 0.5f + 0.03f + i * (w - 0.06f) / 5.0f, -h * 0.5f + 0.03f + j * (h - 0.06f) / 5.0f, 0.008f, 0.008f, { 1.0f, 0.92f, 0.94f });
    tri(-0.115f, -0.10f, 0.115f, -0.10f, 0.0f, 0.105f, RICE);                            // rounded triangle:
    ellipse(-0.080f, -0.068f, 0.036f, 0.036f, RICE);                                      // corners rounded off
    ellipse( 0.080f, -0.068f, 0.036f, 0.036f, RICE);
    ellipse( 0.0f,    0.075f, 0.032f, 0.032f, RICE);
    rect(-0.05f, -0.105f, 0.05f, -0.03f, NORI);
    ellipse(-0.04f, 0.0f, 0.012f, 0.016f, BK); ellipse(0.04f, 0.0f, 0.012f, 0.016f, BK);
    ellipse(-0.036f, 0.006f, 0.004f, 0.005f, RICE); ellipse(0.044f, 0.006f, 0.004f, 0.005f, RICE);   // eye sparkle
    ellipse(-0.07f, -0.018f, 0.018f, 0.009f, { 0.98f, 0.65f, 0.70f }); ellipse(0.07f, -0.018f, 0.018f, 0.009f, { 0.98f, 0.65f, 0.70f });
    ellipse(0.0f, -0.012f, 0.012f, 0.009f, { 0.85f, 0.30f, 0.35f }, 3.1416f, 6.2832f);    // little smile
    ellipse(0.12f, 0.10f, 0.012f, 0.012f, { 0.98f, 0.55f, 0.65f });                      // hearts
    ellipse(0.135f, 0.10f, 0.012f, 0.012f, { 0.98f, 0.55f, 0.65f });
    tri(0.108f, 0.097f, 0.147f, 0.097f, 0.1275f, 0.075f, { 0.98f, 0.55f, 0.65f });
}

// 4. Koi swimming in a pond
static void koi(float w, float h)
{
    frame(w, h, true);
    grad(-w * 0.5f, -h * 0.5f, w * 0.5f, h * 0.5f, { 0.12f, 0.32f, 0.45f }, { 0.25f, 0.55f, 0.65f });
    ring(-0.06f, 0.12f, 0.07f, 0.004f, { 0.55f, 0.80f, 0.85f });
    ring(0.07f, -0.14f, 0.05f, 0.004f, { 0.55f, 0.80f, 0.85f });
    ellipse(0.08f, 0.15f, 0.04f, 0.03f, { 0.30f, 0.62f, 0.32f });                        // lily pad
    for (int f = 0; f < 2; f++) {
        float cx = f ? 0.04f : -0.04f, cy = f ? -0.06f : 0.04f, sgn = f ? -1.0f : 1.0f;
        Color body = f ? Color{ 0.98f, 0.96f, 0.92f } : Color{ 0.98f, 0.45f, 0.12f };
        ellipse(cx, cy, 0.035f, 0.075f, body);
        tri(cx, cy - sgn * 0.065f, cx - 0.04f, cy - sgn * 0.12f, cx + 0.04f, cy - sgn * 0.12f, body);   // tail
        ellipse(cx + 0.012f, cy + sgn * 0.02f, 0.015f, 0.02f, f ? Color{ 0.95f, 0.30f, 0.10f } : Color{ 0.98f, 0.95f, 0.90f });   // spots
        ellipse(cx - 0.012f, cy + sgn * 0.055f, 0.005f, 0.005f, { 0.05f, 0.05f, 0.05f });
        ellipse(cx + 0.012f, cy + sgn * 0.055f, 0.005f, 0.005f, { 0.05f, 0.05f, 0.05f });
    }
}

// 5. Big anime ramen poster: sunburst, bowl, noodles lifted by chopsticks, steam
static void ramenPoster(float w, float h)
{
    frame(w, h, false);
    const Color RD = { 0.86f, 0.16f, 0.13f }, RD2 = { 0.95f, 0.30f, 0.20f }, BK = { 0.08f, 0.06f, 0.06f };
    rect(-w * 0.5f, -h * 0.5f, w * 0.5f, h * 0.5f, RD);
    for (int i = 0; i < 16; i++) {                                                        // speed-line sunburst
        float a = i * 0.3927f;
        triClip(0, -0.02f, cosf(a) * 0.6f, -0.02f + sinf(a) * 0.6f, cosf(a + 0.17f) * 0.6f, -0.02f + sinf(a + 0.17f) * 0.6f, RD2,
                -w * 0.5f, -h * 0.5f, w * 0.5f, h * 0.5f);
    }
    ellipse(0.0f, -0.02f, 0.21f, 0.21f, { 1.0f, 0.85f, 0.30f });                         // yellow disc
    for (int k = 0; k < 3; k++)                                                           // steam curls
        for (int j = 0; j < 4; j++)
            ellipse(-0.08f + k * 0.08f + 0.02f * sinf(j * 1.7f + k), 0.06f + j * 0.04f, 0.022f, 0.016f, { 1.0f, 1.0f, 1.0f });
    ellipse(0.0f, -0.06f, 0.20f, 0.05f, { 0.95f, 0.75f, 0.40f });                         // broth surface
    ellipse(-0.06f, -0.055f, 0.045f, 0.022f, { 0.98f, 0.98f, 0.95f });                   // egg
    ellipse(-0.06f, -0.055f, 0.022f, 0.012f, { 0.98f, 0.62f, 0.10f });
    ellipse(0.07f, -0.055f, 0.03f, 0.016f, { 0.99f, 0.97f, 0.97f });                     // narutomaki
    ring(0.07f, -0.055f, 0.016f, 0.005f, { 0.95f, 0.45f, 0.60f });
    rect(0.0f, -0.07f, 0.04f, -0.02f, { 0.10f, 0.20f, 0.12f });                          // nori
    ellipse(0.0f, -0.07f, 0.20f, 0.14f, BK, 3.1416f, 6.2832f);                            // bowl
    ellipse(0.0f, -0.075f, 0.18f, 0.12f, { 0.70f, 0.10f, 0.08f }, 3.4f, 6.0f);
    rect(-0.20f, -0.075f, 0.20f, -0.06f, BK);
    for (int k = 0; k < 4; k++)                                                           // lifted noodles
        line(0.02f + k * 0.012f, -0.05f, 0.05f + k * 0.012f, 0.14f, 0.008f, { 1.0f, 0.88f, 0.45f });
    line(-0.02f, 0.22f, 0.10f, 0.12f, 0.012f, { 0.55f, 0.32f, 0.15f });                  // chopsticks
    line(0.00f, 0.23f, 0.12f, 0.14f, 0.012f, { 0.55f, 0.32f, 0.15f });
    // bold brush "lettering" bar at the bottom and a small badge
    rect(-0.30f, -h * 0.5f + 0.03f, 0.30f, -h * 0.5f + 0.09f, BK);
    for (int k = 0; k < 4; k++)
        rect(-0.24f + k * 0.13f, -h * 0.5f + 0.045f, -0.16f + k * 0.13f, -h * 0.5f + 0.075f, { 1.0f, 0.90f, 0.35f });
    ellipse(0.27f, 0.19f, 0.07f, 0.07f, { 1.0f, 0.90f, 0.20f });
    ring(0.27f, 0.19f, 0.07f, 0.008f, BK);
    ellipse(0.27f, 0.19f, 0.02f, 0.02f, RD);
    // thumbtacks
    ellipse(-w * 0.5f + 0.03f, h * 0.5f - 0.03f, 0.012f, 0.012f, { 0.2f, 0.4f, 0.9f });
    ellipse(w * 0.5f - 0.03f, h * 0.5f - 0.03f, 0.012f, 0.012f, { 0.2f, 0.4f, 0.9f });
}

// 6. Great wave
static void greatWave(float w, float h)
{
    frame(w, h, true);
    const Color CR = { 0.94f, 0.89f, 0.76f }, BL = { 0.12f, 0.24f, 0.48f }, BL2 = { 0.35f, 0.55f, 0.78f }, WH = { 0.98f, 0.98f, 0.96f };
    rect(-w * 0.5f, -h * 0.5f, w * 0.5f, h * 0.5f, CR);
    tri(0.08f, -0.10f, 0.20f, -0.10f, 0.14f, -0.03f, { 0.45f, 0.50f, 0.62f });         // tiny Fuji far away
    tri(0.125f, -0.045f, 0.155f, -0.045f, 0.14f, -0.03f, WH);
    // the big wave: a curling crest made of shrinking circles
    ellipse(-0.06f, -0.12f, 0.20f, 0.07f, BL);
    for (int i = 0; i < 9; i++) {
        float t = i / 8.0f, a = 3.4f - t * 3.6f;
        float cx = -0.12f + cosf(a) * 0.11f * (1.0f - 0.3f * t), cy = 0.0f + sinf(a) * 0.10f * (1.0f - 0.3f * t);
        ellipse(cx, cy, 0.05f * (1.0f - 0.6f * t), 0.05f * (1.0f - 0.6f * t), BL);
    }
    for (int i = 0; i < 9; i++) {                                                         // inner light band
        float t = i / 8.0f, a = 3.4f - t * 3.6f;
        float cx = -0.12f + cosf(a) * 0.09f * (1.0f - 0.3f * t), cy = 0.0f + sinf(a) * 0.08f * (1.0f - 0.3f * t);
        ellipse(cx, cy, 0.02f * (1.0f - 0.5f * t), 0.02f * (1.0f - 0.5f * t), BL2);
    }
    for (int i = 0; i < 10; i++) {                                                        // foam claws on the crest
        float a = 2.2f - i * 0.25f;
        ellipse(-0.12f + cosf(a) * 0.14f, sinf(a) * 0.12f, 0.014f, 0.014f, WH);
    }
    ellipse(0.12f, -0.15f, 0.12f, 0.035f, BL);                                            // small waves in front
    for (int i = 0; i < 5; i++) ellipse(0.05f + i * 0.035f, -0.12f, 0.01f, 0.01f, WH);
}
} // namespace pic

// Place one picture on the left wall (facing +x) or on the front wall (facing -z)
static void hangPicture(void (*paint)(float, float), float w, float h, Vec3 at, bool leftWall, float tilt)
{
    glPushMatrix();
    glTranslatef(at.x, at.y, at.z);
    glRotatef(leftWall ? 90.0f : 180.0f, 0, 1, 0);
    glRotatef(tilt, 0, 0, 1);
    setMaterialGloss(0.06f, 0.06f, 0.06f, 12.0f);          // printed paper: matte
    paint(w, h);
    resetMaterialGloss();
    glPopMatrix();
}

// Gloss presets for the tatami room
static void woodGlossTimber()  { setMaterialGloss(0.35f, 0.32f, 0.28f, 40.0f); }   // oiled dark timber
static void woodGlossLacquer() { setMaterialGloss(0.85f, 0.80f, 0.72f, 55.0f); }   // lacquered furniture

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

    // ══════════════════════════════════════════════════════════════════════
    //   COZY TATAMI ROOM — tatami floor, plaster walls with dark timber frame,
    //   fusuma sliding doors, wooden board ceiling, chabudai tea table, tansu chests,
    //   folded futons, a radio on the sideboard and an electric fan that swings round
    // ══════════════════════════════════════════════════════════════════════
    const float TY = floorY2 + 0.06f;                 // tatami surface (3.48)
    const float RX0 = -4.8f, RX1 = 3.45f;             // room interior in x (staircase partition at 3.5)
    const float RZ0 = -3.8f, RZ1 = 3.8f;              // room interior in z
    const Color TIMBER = { 0.24f, 0.15f, 0.09f };     // dark aged timber (posts, beams, frames)
    const Color HERI   = { 0.10f, 0.14f, 0.10f };     // dark green cloth border of the tatami

    // ── Tatami floor: mats laid in a running bond, each with its cloth border (heri) ──
    {
        GLuint tat = getTexID(TEX_TATAMI);
        const float ML = 1.90f, MW = 0.95f;           // mat length / width
        setMaterialGloss(0.10f, 0.10f, 0.08f, 12.0f); // woven rush: matte, faint sheen
        int row = 0;
        for (float z = RZ0; z < RZ1 - 0.01f; z += MW, row++) {
            float zw = fminf(MW, RZ1 - z);
            for (float x = RX0 - ((row & 1) ? ML * 0.5f : 0.0f); x < RX1 - 0.01f; x += ML) {
                float x0 = fmaxf(x, RX0), x1 = fminf(x + ML, RX1);
                if (x1 - x0 < 0.05f) continue;
                drawTexturedBox({ (x0 + x1) * 0.5f, floorY2, z + zw * 0.5f }, NO_ROT,
                                { x1 - x0, 0.06f, zw }, tat, WHITE, ML);
                // heri: cloth border along both long edges of the mat
                drawCuboid({ (x0 + x1) * 0.5f, TY, z + 0.02f },      NO_ROT, { x1 - x0, 0.004f, 0.04f }, HERI);
                drawCuboid({ (x0 + x1) * 0.5f, TY, z + zw - 0.02f }, NO_ROT, { x1 - x0, 0.004f, 0.04f }, HERI);
            }
        }
        resetMaterialGloss();
    }

    // ── Walls: plaster facing, dark posts (hashira), nageshi beam and skirting ──
    {
        GLuint wallTex = getTexID(TEX_WALL);
        const Color PLASTER = { 0.84f, 0.80f, 0.72f };            // warm grey-beige clay plaster
        setMaterialGloss(0.03f, 0.03f, 0.03f, 8.0f);
        drawTexturedBox({ 0.0f, TY, RZ0 + 0.005f }, NO_ROT, { 9.6f, 1.80f, 0.01f }, wallTex, PLASTER, 1.5f);   // back
        drawTexturedBox({ RX0 + 0.005f, TY, 0.0f }, NO_ROT, { 0.01f, 1.80f, 7.6f }, wallTex, PLASTER, 1.5f);   // left
        resetMaterialGloss();

        woodGlossTimber();
        const float NY = TY + 1.82f;                                // nageshi (head beam) height
        // posts
        const float postX[] = { RX0 + 0.06f, -2.35f, 1.05f, RX1 - 0.06f };
        for (float px : postX) drawCuboid({ px, TY, RZ0 + 0.06f }, NO_ROT, { 0.11f, 1.82f, 0.11f }, TIMBER);
        const float postZ[] = { -1.3f, 1.3f, RZ1 - 0.06f };
        for (float pz : postZ) drawCuboid({ RX0 + 0.06f, TY, pz }, NO_ROT, { 0.11f, 1.82f, 0.11f }, TIMBER);
        // head beams and skirting boards along the back and left walls
        drawCuboid({ 0.0f, NY, RZ0 + 0.06f }, NO_ROT, { 9.6f, 0.10f, 0.12f }, TIMBER);
        drawCuboid({ RX0 + 0.06f, NY, 0.0f }, NO_ROT, { 0.12f, 0.10f, 7.6f }, TIMBER);
        drawCuboid({ 0.0f, NY, RZ1 - 0.06f }, NO_ROT, { 9.6f, 0.10f, 0.12f }, TIMBER);   // front wall
        drawCuboid({ 0.0f, TY, RZ0 + 0.02f }, NO_ROT, { 9.6f, 0.07f, 0.04f }, TIMBER);
        drawCuboid({ RX0 + 0.02f, TY, 0.0f }, NO_ROT, { 0.04f, 0.07f, 7.6f }, TIMBER);
        resetMaterialGloss();
    }

    // ── Fusuma: four paper sliding doors in the back wall, checkered lower band ──
    {
        const float fx0 = -2.30f, fw = 0.83f, fh = 1.80f, fz = RZ0 + 0.035f;
        for (int i = 0; i < 4; i++) {
            float cx = fx0 + fw * (i + 0.5f);
            // paper face (vertex colours: soft grey washi, an ichimatsu checker band near the bottom)
            setMaterialGloss(0.04f, 0.04f, 0.04f, 8.0f);
            glBegin(GL_QUADS);
            glNormal3f(0, 0, 1);
            const int NXc = 6, NYc = 18;
            for (int gx = 0; gx < NXc; gx++)
                for (int gy = 0; gy < NYc; gy++) {
                    float u0 = (float)gx / NXc, u1 = (float)(gx + 1) / NXc, v0 = (float)gy / NYc, v1 = (float)(gy + 1) / NYc;
                    float tone = 0.86f + 0.04f * sinf(cx * 7.0f + gx * 1.3f + gy * 0.7f);
                    Color pc = { tone, tone * 0.98f, tone * 0.93f };
                    if (gy >= 2 && gy < 5) {                                   // checker band
                        bool dark = ((gx + gy) & 1) != 0;
                        pc = dark ? Color{ 0.52f, 0.55f, 0.50f } : Color{ 0.80f, 0.80f, 0.76f };
                    }
                    glColor3f(pc.r, pc.g, pc.b);
                    float xa = cx - fw * 0.5f + 0.03f + u0 * (fw - 0.06f), xb = cx - fw * 0.5f + 0.03f + u1 * (fw - 0.06f);
                    float ya = TY + 0.03f + v0 * (fh - 0.06f), yb = TY + 0.03f + v1 * (fh - 0.06f);
                    glVertex3f(xa, ya, fz); glVertex3f(xb, ya, fz); glVertex3f(xb, yb, fz); glVertex3f(xa, yb, fz);
                }
            glEnd();
            // lacquered frame and a small round recessed pull
            woodGlossTimber();
            drawCuboid({ cx - fw * 0.5f + 0.015f, TY, fz }, NO_ROT, { 0.03f, fh, 0.03f }, TIMBER);
            drawCuboid({ cx + fw * 0.5f - 0.015f, TY, fz }, NO_ROT, { 0.03f, fh, 0.03f }, TIMBER);
            drawCuboid({ cx, TY + fh - 0.03f, fz }, NO_ROT, { fw, 0.03f, 0.03f }, TIMBER);
            drawCuboid({ cx, TY, fz }, NO_ROT, { fw, 0.03f, 0.03f }, TIMBER);
            setMaterialConductive({ 0.25f, 0.22f, 0.18f }, 60.0f);
            float hx = (i & 1) ? cx - fw * 0.5f + 0.10f : cx + fw * 0.5f - 0.10f;
            drawCylinder({ hx, TY + 0.85f, fz + 0.012f }, { 90, 0, 0 }, { 0.06f, 0.006f, 0.06f }, { 0.25f, 0.22f, 0.18f });
            resetMaterialGloss();
        }
        // threshold rail and lintel of the fusuma opening
        woodGlossTimber();
        drawCuboid({ fx0 + fw * 2.0f, TY + fh, RZ0 + 0.05f }, NO_ROT, { fw * 4.0f + 0.1f, 0.05f, 0.08f }, TIMBER);
        resetMaterialGloss();
    }

    // ── Wooden board ceiling with dark battens ──
    {
        const float CY = GH + UH - 0.03f;
        setMaterialGloss(0.15f, 0.14f, 0.12f, 30.0f);
        drawTexturedBox({ -0.65f, CY, 0.0f }, NO_ROT, { 8.3f, 0.02f, 7.6f }, getTexID(TEX_WOOD), { 0.78f, 0.66f, 0.50f }, 1.6f);
        for (float bx = RX0 + 0.6f; bx < RX1; bx += 0.9f)
            drawCuboid({ bx, CY - 0.04f, 0.0f }, NO_ROT, { 0.05f, 0.04f, 7.6f }, TIMBER);
        resetMaterialGloss();
    }

    // ── Square washi ceiling lamp with a pull cord (this is the room light, key 7) ──
    {
        const bool on = lightArea && fixtureOn[FX_DOME];
        const float LY = GH + UH - 0.36f, s2 = 0.30f;
        float dayDome = isDayTime ? 0.25f : 1.0f;
        drawCylinder({ 0.0f, LY + 0.14f, 0.0f }, NO_ROT, { 0.01f, 0.20f, 0.01f }, TIMBER);       // hanging rod
        if (on) setEmission(0.85f * dayDome, 0.70f * dayDome, 0.42f * dayDome);
        drawCuboid({ 0.0f, LY, 0.0f }, NO_ROT, { s2 * 2.0f - 0.02f, 0.14f, s2 * 2.0f - 0.02f }, PAPER);   // glowing paper box
        clearEmission();
        woodGlossTimber();
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                drawCuboid({ sx * s2, LY - 0.01f, sz * s2 }, NO_ROT, { 0.03f, 0.17f, 0.03f }, TIMBER);   // corner posts
        for (int k = -1; k <= 1; k++) {                                                                  // lattice bars
            drawCuboid({ k * s2 * 0.5f, LY + 0.145f, 0.0f }, NO_ROT, { 0.02f, 0.02f, s2 * 2.0f }, TIMBER);
            drawCuboid({ 0.0f, LY + 0.145f, k * s2 * 0.5f }, NO_ROT, { s2 * 2.0f, 0.02f, 0.02f }, TIMBER);
        }
        resetMaterialGloss();
        drawCylinder({ 0.12f, LY - 0.62f, 0.10f }, NO_ROT, { 0.006f, 0.62f, 0.006f }, { 0.20f, 0.18f, 0.16f });   // pull cord
        drawSphere({ 0.12f, LY - 0.64f, 0.10f }, NO_ROT, { 0.03f, 0.04f, 0.03f }, { 0.75f, 0.20f, 0.18f });
    }

    // ── Ceiling fan, turning slowly (away from the room light) ──
    drawCeilingFan({ -1.6f, GH + UH - 0.03f, -0.8f });

    // ── Chabudai: low wooden tea table with a tea set ──
    {
        const float tx = -0.6f, tz = 0.7f, th = 0.32f;
        woodGlossLacquer();
        drawTexturedBox({ tx, TY + th - 0.04f, tz }, NO_ROT, { 1.10f, 0.04f, 0.68f }, getTexID(TEX_WOOD), { 0.92f, 0.72f, 0.50f }, 1.2f);
        drawCuboid({ tx, TY + th - 0.09f, tz }, NO_ROT, { 0.98f, 0.05f, 0.56f }, { 0.55f, 0.36f, 0.20f });   // apron
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                drawCuboid({ tx + sx * 0.46f, TY, tz + sz * 0.25f }, NO_ROT, { 0.05f, th - 0.04f, 0.05f }, { 0.55f, 0.36f, 0.20f });
        resetMaterialGloss();
        const float top = TY + th;
        // round lacquered tray
        setMaterialGloss(0.8f, 0.6f, 0.5f, 70.0f);
        drawCylinder({ tx - 0.18f, top, tz + 0.02f }, NO_ROT, { 0.34f, 0.012f, 0.34f }, { 0.45f, 0.10f, 0.08f });
        // kyusu teapot: dark clay body, spout and side handle
        drawSphere({ tx - 0.22f, top + 0.065f, tz + 0.02f }, NO_ROT, { 0.13f, 0.11f, 0.13f }, { 0.36f, 0.16f, 0.10f });
        drawCylinder({ tx - 0.22f, top + 0.11f, tz + 0.02f }, NO_ROT, { 0.07f, 0.02f, 0.07f }, { 0.32f, 0.14f, 0.09f });
        drawSphere({ tx - 0.22f, top + 0.135f, tz + 0.02f }, NO_ROT, { 0.02f, 0.02f, 0.02f }, { 0.32f, 0.14f, 0.09f });
        limb({ tx - 0.16f, top + 0.06f, tz + 0.02f }, { tx - 0.08f, top + 0.10f, tz + 0.02f }, 0.012f, { 0.36f, 0.16f, 0.10f });
        limb({ tx - 0.22f, top + 0.06f, tz - 0.04f }, { tx - 0.22f, top + 0.07f, tz - 0.16f }, 0.014f, { 0.36f, 0.16f, 0.10f });
        // two yunomi tea cups (glazed)
        setMaterialDielectric(90.0f);
        drawCylinderCustom({ tx + 0.10f, top, tz - 0.10f }, NO_ROT, ONE, { 0.30f, 0.45f, 0.32f }, 0.032f, 0.036f, 0.085f);
        drawCylinderCustom({ tx + 0.24f, top, tz + 0.12f }, NO_ROT, ONE, { 0.82f, 0.80f, 0.72f }, 0.032f, 0.036f, 0.085f);
        // vacuum pot (thermos): enamel body with a flower band, dark lid and handle
        drawCylinderCustom({ tx + 0.36f, top, tz - 0.08f }, NO_ROT, ONE, { 0.93f, 0.90f, 0.88f }, 0.075f, 0.070f, 0.24f);
        drawCylinderCustom({ tx + 0.36f, top + 0.09f, tz - 0.08f }, NO_ROT, ONE, { 0.88f, 0.55f, 0.60f }, 0.077f, 0.076f, 0.06f);
        drawCylinderCustom({ tx + 0.36f, top + 0.24f, tz - 0.08f }, NO_ROT, ONE, { 0.20f, 0.18f, 0.18f }, 0.060f, 0.045f, 0.05f);
        limb({ tx + 0.29f, top + 0.27f, tz - 0.08f }, { tx + 0.43f, top + 0.27f, tz - 0.08f }, 0.008f, { 0.30f, 0.30f, 0.32f });
        resetMaterialGloss();
    }

    // ── Zabuton floor cushions around the table (soft, with a centre tuft) ──
    {
        const Color ZAB = { 0.50f, 0.56f, 0.44f }, ZAB2 = { 0.58f, 0.22f, 0.18f };
        const float zx[3] = { -0.6f, -0.6f, -1.55f }, zz[3] = { 1.45f, -0.05f, 0.7f }, zr[3] = { 4.0f, -6.0f, 88.0f };
        for (int i = 0; i < 3; i++) {
            Color cc = (i == 2) ? ZAB2 : ZAB;
            setMaterialGloss(0.05f, 0.05f, 0.05f, 10.0f);
            drawCuboid({ zx[i], TY, zz[i] }, { 0, zr[i], 0 }, { 0.56f, 0.05f, 0.56f }, cc);
            drawCuboid({ zx[i], TY + 0.05f, zz[i] }, { 0, zr[i], 0 }, { 0.50f, 0.025f, 0.50f }, cc);   // puffed centre
            drawSphere({ zx[i], TY + 0.078f, zz[i] }, NO_ROT, { 0.03f, 0.015f, 0.03f }, { cc.r * 0.6f, cc.g * 0.6f, cc.b * 0.6f });   // tuft
            resetMaterialGloss();
        }
    }

    // ── Tall tansu chest against the left wall, with books on top ──
    {
        const float cx = RX0 + 0.26f, cz = -0.2f, H = 1.45f, Wd = 1.05f, D = 0.45f;
        const Color CH = { 0.30f, 0.20f, 0.12f }, CH2 = { 0.36f, 0.25f, 0.15f };
        woodGlossLacquer();
        drawTexturedBox({ cx, TY, cz }, NO_ROT, { D, H, Wd }, getTexID(TEX_DARK_WOOD), { 1.0f, 0.95f, 0.9f }, 1.0f);
        // drawer fronts (face +x) and a two-door cupboard at the top
        for (int r = 0; r < 4; r++) {
            float y = TY + 0.06f + r * 0.24f;
            drawCuboid({ cx + D * 0.5f + 0.008f, y, cz }, NO_ROT, { 0.016f, 0.21f, Wd - 0.08f }, CH2);
            setMaterialConductive({ 0.20f, 0.18f, 0.15f }, 60.0f);
            for (int k = -1; k <= 1; k += 2)
                limb({ cx + D * 0.5f + 0.03f, y + 0.12f, cz + k * 0.22f - 0.06f }, { cx + D * 0.5f + 0.03f, y + 0.12f, cz + k * 0.22f + 0.06f }, 0.008f, { 0.18f, 0.16f, 0.14f });
            woodGlossLacquer();
        }
        for (int k = -1; k <= 1; k += 2)
            drawCuboid({ cx + D * 0.5f + 0.008f, TY + 1.03f, cz + k * (Wd * 0.25f - 0.01f) }, NO_ROT, { 0.016f, 0.38f, Wd * 0.5f - 0.06f }, CH2);
        resetMaterialGloss();
        // books and a box on top
        const Color BK[4] = { { 0.70f, 0.15f, 0.12f }, { 0.85f, 0.68f, 0.20f }, { 0.20f, 0.30f, 0.50f }, { 0.90f, 0.88f, 0.82f } };
        for (int i = 0; i < 4; i++)
            drawCuboid({ cx, TY + H, cz - 0.35f + i * 0.055f }, NO_ROT, { 0.30f, 0.26f - 0.02f * (i & 1), 0.045f }, BK[i]);
        drawCuboid({ cx, TY + H, cz + 0.20f }, { 0, 6, 0 }, { 0.32f, 0.20f, 0.30f }, { 0.25f, 0.32f, 0.36f });
        drawCuboid({ cx - 0.02f, TY + H, cz + 0.40f }, NO_ROT, { 0.28f, 0.03f, 0.22f }, { 0.92f, 0.90f, 0.85f });   // magazine stack
    }

    // ── Folded futons stacked beside the chest ──
    {
        const float fx = RX0 + 0.55f, fz = 1.55f;
        const Color F1 = { 0.92f, 0.90f, 0.84f }, F2 = { 0.68f, 0.20f, 0.22f }, F3 = { 0.82f, 0.82f, 0.86f };
        setMaterialGloss(0.05f, 0.05f, 0.05f, 10.0f);
        drawCuboid({ fx, TY, fz }, NO_ROT, { 0.95f, 0.13f, 0.70f }, F3);
        drawCuboid({ fx, TY + 0.13f, fz }, NO_ROT, { 0.93f, 0.12f, 0.69f }, F1);
        drawCuboid({ fx, TY + 0.25f, fz }, NO_ROT, { 0.94f, 0.13f, 0.70f }, F2);
        drawCuboid({ fx, TY + 0.38f, fz }, NO_ROT, { 0.90f, 0.10f, 0.66f }, F1);
        for (int i = 0; i < 4; i++)                                                  // printed pattern on the red quilt
            drawCuboid({ fx - 0.30f + i * 0.2f, TY + 0.381f + 0.10f, fz - 0.15f + (i & 1) * 0.2f }, NO_ROT, { 0.10f, 0.003f, 0.10f }, { 0.95f, 0.85f, 0.75f });
        drawCuboid({ fx + 0.10f, TY + 0.48f, fz + 0.05f }, NO_ROT, { 0.42f, 0.10f, 0.26f }, { 0.94f, 0.92f, 0.86f });   // pillow
        resetMaterialGloss();
    }

    // ── Bed: a simple old-style futon laid straight on the tatami (like in anime) ──
    //    thin white shikibuton mattress, a thick red quilted kakebuton with little white
    //    flowers turned back at the top, a small buckwheat makura pillow, and a paper
    //    andon floor lamp beside it.
    {
        const float bx = 2.15f, z0 = 1.55f, z1 = 3.50f;          // futon along z, pillow end at z1
        const float W = 1.05f, zc = (z0 + z1) * 0.5f, L = z1 - z0;
        setMaterialGloss(0.05f, 0.05f, 0.05f, 10.0f);             // cotton: matte

        // shikibuton: thin white mattress with soft rounded long edges
        const float mH = 0.07f;
        drawCuboid({ bx, TY, zc }, NO_ROT, { W, mH, L }, { 0.96f, 0.95f, 0.91f });
        for (int k = -1; k <= 1; k += 2)
            drawCylinder({ bx + k * W * 0.5f, TY + mH * 0.5f, z0 }, { 90, 0, 0 }, { mH, L, mH }, { 0.96f, 0.95f, 0.91f });
        const float top = TY + mH;

        // kakebuton: thick quilted cover, soft red with small white flowers, slightly puffy
        const float dz0 = z0 - 0.04f, dz1 = z1 - 0.55f, dL = dz1 - dz0, dzc = (dz0 + dz1) * 0.5f;
        const Color BLUE = { 0.74f, 0.27f, 0.28f }, CREAM = { 0.95f, 0.92f, 0.84f };   // soft red quilt
        drawCuboid({ bx, top, dzc }, NO_ROT, { W + 0.10f, 0.11f, dL }, BLUE);
        for (int k = -1; k <= 1; k += 2)                                                     // puffy rolled sides
            drawCylinder({ bx + k * (W * 0.5f + 0.05f), top + 0.055f, dz0 }, { 90, 0, 0 }, { 0.11f, dL, 0.11f }, BLUE);
        // small white five-petal flowers scattered over the quilt
        for (int f = 0; f < 22; f++) {
            float u = 0.08f + 0.84f * (0.5f + 0.5f * sinf(f * 12.9898f)), v = 0.06f + 0.88f * (0.5f + 0.5f * sinf(f * 78.233f + 1.3f));
            float fx = bx - (W + 0.10f) * 0.5f + u * (W + 0.10f), fz = dz0 + v * dL;
            for (int pt = 0; pt < 5; pt++) {
                float a = pt * 1.2566f + f;
                drawSphere({ fx + cosf(a) * 0.022f, top + 0.111f, fz + sinf(a) * 0.022f }, NO_ROT, { 0.026f, 0.004f, 0.026f }, CREAM);
            }
            drawSphere({ fx, top + 0.113f, fz }, NO_ROT, { 0.014f, 0.004f, 0.014f }, { 0.95f, 0.78f, 0.25f });
        }
        // top edge turned back, showing the plain cream lining
        drawCuboid({ bx, top + 0.11f, dz1 - 0.12f }, NO_ROT, { W + 0.10f, 0.06f, 0.24f }, CREAM);
        drawCylinder({ bx - (W + 0.10f) * 0.5f, top + 0.08f, dz1 + 0.01f }, { 0, 0, -90 }, { 0.10f, W + 0.10f, 0.10f }, CREAM);

        // makura: small rectangular buckwheat pillow with a white cover
        drawSphere({ bx, top + 0.06f, z1 - 0.24f }, NO_ROT, { 0.46f, 0.13f, 0.25f }, { 0.97f, 0.97f, 0.94f });
        drawCylinder({ bx - 0.23f, top + 0.06f, z1 - 0.24f }, { 0, 0, -90 }, { 0.05f, 0.46f, 0.05f }, { 0.55f, 0.65f, 0.80f });   // pillow piping
        resetMaterialGloss();

        // andon floor lamp beside the pillow (paper box in a wooden frame)
        const float lx = bx + 0.95f, lz = z1 - 0.25f;
        const bool lampOn = lightArea && fixtureOn[FX_DOME];
        if (lampOn) setEmission(isDayTime ? 0.25f : 0.85f, isDayTime ? 0.20f : 0.62f, isDayTime ? 0.10f : 0.30f);
        drawCuboid({ lx, TY + 0.10f, lz }, NO_ROT, { 0.22f, 0.40f, 0.22f }, PAPER);
        clearEmission();
        woodGlossTimber();
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                drawCuboid({ lx + sx * 0.12f, TY, lz + sz * 0.12f }, NO_ROT, { 0.025f, 0.55f, 0.025f }, TIMBER);
        drawCuboid({ lx, TY + 0.53f, lz }, NO_ROT, { 0.27f, 0.025f, 0.27f }, TIMBER);
        drawCuboid({ lx, TY + 0.08f, lz }, NO_ROT, { 0.27f, 0.025f, 0.27f }, TIMBER);
        resetMaterialGloss();
    }

    // ── Low sideboard (back wall, right) with a radio-cassette player and an uchiwa fan ──
    {
        const float sx = 2.25f, sz = RZ0 + 0.30f, H = 0.72f;
        woodGlossLacquer();
        drawTexturedBox({ sx, TY, sz }, NO_ROT, { 1.80f, H, 0.48f }, getTexID(TEX_DARK_WOOD), WHITE, 1.0f);
        for (int i = 0; i < 3; i++)
            drawCuboid({ sx - 0.60f + i * 0.60f, TY + 0.05f, sz + 0.245f }, NO_ROT, { 0.56f, H - 0.10f, 0.012f }, { 0.36f, 0.24f, 0.14f });
        setMaterialConductive({ 0.25f, 0.22f, 0.18f }, 60.0f);
        for (int i = 0; i < 3; i++)
            drawCuboid({ sx - 0.60f + i * 0.60f, TY + H * 0.55f, sz + 0.26f }, NO_ROT, { 0.10f, 0.015f, 0.015f }, { 0.20f, 0.18f, 0.15f });
        resetMaterialGloss();
        const float top = TY + H;
        // radio-cassette: silver body, two speaker grilles, cassette deck, handle, aerial
        setMaterialConductive({ 0.70f, 0.70f, 0.72f }, 70.0f);
        drawCuboid({ sx + 0.15f, top, sz + 0.02f }, NO_ROT, { 0.56f, 0.24f, 0.14f }, { 0.68f, 0.68f, 0.70f });
        resetMaterialGloss();
        for (int k = -1; k <= 1; k += 2) {
            drawCylinder({ sx + 0.15f + k * 0.18f, top + 0.12f, sz + 0.09f }, { 90, 0, 0 }, { 0.15f, 0.01f, 0.15f }, { 0.10f, 0.10f, 0.11f });
            drawCylinder({ sx + 0.15f + k * 0.18f, top + 0.12f, sz + 0.10f }, { 90, 0, 0 }, { 0.05f, 0.01f, 0.05f }, { 0.30f, 0.30f, 0.32f });
        }
        drawCuboid({ sx + 0.15f, top + 0.07f, sz + 0.09f }, NO_ROT, { 0.16f, 0.10f, 0.01f }, { 0.18f, 0.18f, 0.20f });   // cassette door
        limb({ sx - 0.08f, top + 0.24f, sz + 0.02f }, { sx - 0.04f, top + 0.31f, sz + 0.02f }, 0.012f, { 0.15f, 0.15f, 0.16f });
        limb({ sx + 0.34f, top + 0.31f, sz + 0.02f }, { sx + 0.38f, top + 0.24f, sz + 0.02f }, 0.012f, { 0.15f, 0.15f, 0.16f });
        limb({ sx - 0.04f, top + 0.31f, sz + 0.02f }, { sx + 0.34f, top + 0.31f, sz + 0.02f }, 0.012f, { 0.15f, 0.15f, 0.16f });
        limb({ sx + 0.40f, top + 0.24f, sz - 0.03f }, { sx + 0.55f, top + 0.62f, sz - 0.03f }, 0.004f, { 0.75f, 0.75f, 0.78f });   // aerial
        // uchiwa (round paper fan) leaning on the wall
        drawCylinder({ sx + 0.72f, top + 0.12f, sz - 0.15f }, { 75, 0, 0 }, { 0.30f, 0.008f, 0.30f }, { 0.85f, 0.18f, 0.15f });
        limb({ sx + 0.72f, top, sz - 0.12f }, { sx + 0.72f, top + 0.12f, sz - 0.16f }, 0.008f, { 0.80f, 0.70f, 0.45f });
        // a small potted plant at the other end
        drawCylinderCustom({ sx - 0.62f, top, sz }, NO_ROT, ONE, { 0.55f, 0.30f, 0.20f }, 0.07f, 0.09f, 0.14f);
        drawSphere({ sx - 0.62f, top + 0.22f, sz }, NO_ROT, { 0.22f, 0.20f, 0.22f }, LEAF);
    }

    // ── Wall things: calligraphy scroll, calendar, a handwritten notice ──
    {
        // calligraphy strip between the post and the fusuma
        drawCuboid({ -2.62f, TY + 0.95f, RZ0 + 0.02f }, NO_ROT, { 0.26f, 0.72f, 0.01f }, { 0.94f, 0.92f, 0.86f });
        for (int i = 0; i < 4; i++)
            drawCuboid({ -2.62f + 0.02f * ((i & 1) ? 1 : -1), TY + 1.50f - i * 0.15f, RZ0 + 0.027f }, NO_ROT, { 0.10f - 0.02f * (i % 3), 0.08f, 0.004f }, { 0.08f, 0.08f, 0.08f });
        drawCuboid({ -2.62f, TY + 1.67f, RZ0 + 0.03f }, NO_ROT, { 0.30f, 0.025f, 0.025f }, TIMBER);
        // calendar above the sideboard: picture page on top, date grid below
        drawCuboid({ 2.95f, TY + 1.05f, RZ0 + 0.02f }, NO_ROT, { 0.42f, 0.60f, 0.01f }, { 0.95f, 0.95f, 0.93f });
        drawCuboid({ 2.95f, TY + 1.36f, RZ0 + 0.027f }, NO_ROT, { 0.38f, 0.26f, 0.004f }, { 0.20f, 0.36f, 0.48f });
        for (int r = 0; r < 4; r++)
            for (int k = 0; k < 7; k++)
                drawCuboid({ 2.80f + k * 0.05f, TY + 1.10f + r * 0.055f, RZ0 + 0.027f }, NO_ROT, { 0.025f, 0.025f, 0.003f },
                           (k == 0) ? Color{ 0.80f, 0.15f, 0.12f } : Color{ 0.35f, 0.35f, 0.38f });
        // handwritten notice
        drawCuboid({ 1.75f, TY + 1.15f, RZ0 + 0.02f }, NO_ROT, { 0.55f, 0.42f, 0.01f }, { 0.90f, 0.86f, 0.74f });
        for (int i = 0; i < 6; i++)
            drawCuboid({ 1.53f + i * 0.085f, TY + 1.20f, RZ0 + 0.027f }, NO_ROT, { 0.02f, 0.30f, 0.003f }, { 0.25f, 0.22f, 0.20f });
    }

    // ── Tokonoma alcove (left of the fusuma): shelf, hanging scroll and ikebana ──
    drawCuboid({ -3.55f, floorY2 + 0.30f, -3.55f }, NO_ROT, { 2.0f, 0.10f, 0.45f }, TIMBER);   // raised alcove floor (toko-ita)
    if (!isDayTime) setEmission(0.04f, 0.03f, 0.02f); else setEmission(0.02f, 0.02f, 0.01f);
    drawCuboid({ -3.55f, floorY2 + 0.80f, -3.75f }, NO_ROT, { 0.6f, 1.05f, 0.02f }, PAPER);    // kakejiku
    clearEmission();
    for (int i = 0; i < 3; i++)
        drawCuboid({ -3.55f, floorY2 + 1.55f - i * 0.22f, -3.737f }, NO_ROT, { 0.18f, 0.12f, 0.004f }, { 0.08f, 0.08f, 0.08f });
    drawCuboid({ -3.55f, floorY2 + 1.87f, -3.73f }, NO_ROT, { 0.68f, 0.04f, 0.04f }, TIMBER);
    drawCuboid({ -3.55f, floorY2 + 0.78f, -3.73f }, NO_ROT, { 0.68f, 0.04f, 0.04f }, TIMBER);
    {
        const Color VASE_BLUE = { 0.20f, 0.25f, 0.55f };
        setMaterialPBR(Materials::Ceramic, VASE_BLUE);
        drawCylinderCustom({ -3.05f, floorY2 + 0.40f, -3.50f }, NO_ROT, ONE, VASE_BLUE, 0.07f, 0.05f, 0.24f);
        resetMaterialGloss();
        for (int i = 0; i < 3; i++)
            limb({ -3.05f, floorY2 + 0.62f, -3.50f }, { -3.05f + (i - 1) * 0.12f, floorY2 + 0.95f + i * 0.04f, -3.48f }, 0.006f, { 0.25f, 0.40f, 0.18f });
        drawSphere({ -3.17f, floorY2 + 0.96f, -3.48f }, NO_ROT, { 0.06f, 0.05f, 0.06f }, { 0.92f, 0.50f, 0.62f });
        drawSphere({ -2.93f, floorY2 + 1.03f, -3.48f }, NO_ROT, { 0.05f, 0.05f, 0.05f }, { 0.95f, 0.88f, 0.90f });
    }

    // ── Electric stand fan: the head swings left and right, the blades spin ──
    {
        const float bx = -3.3f, bz = 2.6f;
        const float swing = 35.0f * sinf(animTime * 0.6f);
        setMaterialGloss(0.6f, 0.6f, 0.6f, 60.0f);
        drawCylinderCustom({ bx, TY, bz }, NO_ROT, ONE, { 0.82f, 0.84f, 0.82f }, 0.20f, 0.17f, 0.05f);   // base
        drawCylinder({ bx, TY + 0.05f, bz }, NO_ROT, { 0.05f, 0.80f, 0.05f }, { 0.75f, 0.78f, 0.76f });  // pole
        glPushMatrix();
        glTranslatef(bx, TY + 0.90f, bz);
        glRotatef(30.0f + swing, 0, 1, 0);                          // oscillation (faces into the room, +x)
        drawSphere({ -0.06f, 0, 0 }, NO_ROT, { 0.20f, 0.16f, 0.16f }, { 0.80f, 0.82f, 0.80f });   // motor housing
        setMaterialConductive(STEEL, 70.0f);
        // wire cage: rings and spokes, front faces +x
        drawTorus({ 0.10f, 0, 0 }, { 0, 0, 90 }, ONE, { 0.70f, 0.72f, 0.74f }, 0.006f, 0.20f);
        drawTorus({ 0.16f, 0, 0 }, { 0, 0, 90 }, ONE, { 0.70f, 0.72f, 0.74f }, 0.005f, 0.13f);
        for (int k = 0; k < 10; k++) {
            float a = k * 6.2832f / 10.0f;
            limb({ 0.04f, 0, 0 }, { 0.10f, cosf(a) * 0.20f, sinf(a) * 0.20f }, 0.003f, { 0.70f, 0.72f, 0.74f });
            limb({ 0.10f, cosf(a) * 0.20f, sinf(a) * 0.20f }, { 0.17f, cosf(a) * 0.05f, sinf(a) * 0.05f }, 0.003f, { 0.70f, 0.72f, 0.74f });
        }
        resetMaterialGloss();
        // blades (translucent-looking pale blue plastic), spinning about the fan axis
        glPushMatrix();
        glTranslatef(0.09f, 0, 0);
        glRotatef(animTime * 900.0f, 1, 0, 0);
        setMaterialGloss(0.5f, 0.5f, 0.5f, 50.0f);
        for (int k = 0; k < 3; k++) {
            glPushMatrix();
            glRotatef(k * 120.0f, 1, 0, 0);
            drawSphere({ 0, 0.10f, 0 }, { 25, 0, 0 }, { 0.02f, 0.17f, 0.10f }, { 0.62f, 0.78f, 0.86f });
            glPopMatrix();
        }
        drawSphere({ 0.02f, 0, 0 }, NO_ROT, { 0.05f, 0.05f, 0.05f }, { 0.80f, 0.82f, 0.80f });   // spinner cap
        resetMaterialGloss();
        glPopMatrix();
        glPopMatrix();
    }

    // ── Fun pictures on the walls ──
    hangPicture(pic::fuji,     0.50f, 0.36f, { RX0 + 0.02f, TY + 1.30f, -2.90f }, true,  0.0f);
    hangPicture(pic::luckyCat, 0.36f, 0.44f, { RX0 + 0.02f, TY + 1.28f, -2.00f }, true,  1.5f);
    hangPicture(pic::onigiri,  0.32f, 0.32f, { RX0 + 0.02f, TY + 1.25f,  2.05f }, true, -2.0f);
    hangPicture(pic::koi,      0.30f, 0.48f, { RX0 + 0.02f, TY + 1.30f,  2.85f }, true,  0.0f);
    hangPicture(pic::ramenPoster, 0.80f, 0.60f, { 0.10f, TY + 1.15f, RZ1 - 0.02f }, false, -1.0f);
    hangPicture(pic::greatWave,   0.50f, 0.36f, { -1.35f, TY + 1.30f, RZ1 - 0.02f }, false, 0.0f);

    // ── The two front windows seen from inside: deep wooden reveal, a pair of shoji
    //    panels whose paper is lit by daylight (or glows warm from the room lamp at night) ──
    {
        const float wy0 = 3.65f, wy1 = 4.95f, zi = RZ1 + 0.005f;         // inner wall face
        const bool lampOn = lightArea && fixtureOn[FX_DOME];
        const float paperLit = isDayTime ? 0.62f : (lampOn ? 0.16f : 0.05f);
        for (int w = -1; w <= 1; w += 2) {
            const float x0 = w * 2.2f, x1 = w * 4.2f, cx = (x0 + x1) * 0.5f, ww = fabsf(x1 - x0);
            // reveal: boards lining the wall opening (sill, head, jambs)
            woodGlossTimber();
            drawCuboid({ cx, wy0 - 0.03f, RZ1 + 0.10f }, NO_ROT, { ww + 0.10f, 0.05f, 0.30f }, { 0.36f, 0.24f, 0.14f });   // sill
            drawCuboid({ cx, wy1 - 0.02f, RZ1 + 0.10f }, NO_ROT, { ww + 0.10f, 0.05f, 0.24f }, TIMBER);                    // head
            drawCuboid({ x0, wy0, RZ1 + 0.10f }, NO_ROT, { 0.06f, wy1 - wy0, 0.24f }, TIMBER);
            drawCuboid({ x1, wy0, RZ1 + 0.10f }, NO_ROT, { 0.06f, wy1 - wy0, 0.24f }, TIMBER);
            resetMaterialGloss();
            // two sliding shoji panels (the right one slightly open to show the overlap)
            for (int p = 0; p < 2; p++) {
                const float pw = ww * 0.5f + 0.03f;
                const float pcx = fminf(x0, x1) + pw * 0.5f + p * (ww - pw);
                const float pz = zi + 0.03f + p * 0.035f;
                // washi paper: emissive by day (daylight shining through), soft warm at night
                setEmission(paperLit, paperLit * 0.96f, paperLit * (isDayTime ? 0.90f : 0.70f));
                setMaterialGloss(0.0f, 0.0f, 0.0f, 1.0f);
                glBegin(GL_QUADS);
                glNormal3f(0, 0, -1);
                setColor(PAPER);
                glVertex3f(pcx + pw * 0.5f, wy0 + 0.02f, pz); glVertex3f(pcx - pw * 0.5f, wy0 + 0.02f, pz);
                glVertex3f(pcx - pw * 0.5f, wy1 - 0.02f, pz); glVertex3f(pcx + pw * 0.5f, wy1 - 0.02f, pz);
                glEnd();
                clearEmission();
                resetMaterialGloss();
                // panel frame and kumiko lattice on the room side of the paper
                woodGlossTimber();
                const float fz = pz - 0.018f;
                drawCuboid({ pcx - pw * 0.5f + 0.02f, wy0, fz }, NO_ROT, { 0.04f, wy1 - wy0, 0.035f }, TIMBER);
                drawCuboid({ pcx + pw * 0.5f - 0.02f, wy0, fz }, NO_ROT, { 0.04f, wy1 - wy0, 0.035f }, TIMBER);
                drawCuboid({ pcx, wy1 - 0.04f, fz }, NO_ROT, { pw, 0.04f, 0.035f }, TIMBER);
                drawCuboid({ pcx, wy0, fz }, NO_ROT, { pw, 0.05f, 0.035f }, TIMBER);
                const int cols = 3, rows = 5;
                for (int i = 1; i < cols; i++)
                    drawCuboid({ pcx - pw * 0.5f + i * pw / cols, wy0, fz }, NO_ROT, { 0.016f, wy1 - wy0, 0.02f }, TIMBER);
                for (int j = 1; j < rows; j++)
                    drawCuboid({ pcx, wy0 + j * (wy1 - wy0) / rows, fz }, NO_ROT, { pw - 0.04f, 0.016f, 0.02f }, TIMBER);
                resetMaterialGloss();
            }
        }
    }

    // ── Floor lantern tower in the front-left corner ──
    drawJapaneseFloorLanternTower({ RX0 + 0.35f, floorY2, RZ1 - 0.40f }, NO_ROT, { 0.7f, 0.7f, 0.7f }, lightPoint && fixtureOn[FX_TOWER]);

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
    drawPendantGlassLamp({ -1.2f, 2.85f, -0.2f }, NO_ROT, ONE, lightPoint && fixtureOn[FX_PENDANTS]);
    drawPendantGlassLamp({  1.2f, 2.85f, -0.2f }, NO_ROT, ONE, lightPoint && fixtureOn[FX_PENDANTS]);

    // Clear glass condiment jars, water pitcher and tumblers on the counter
    drawClearGlassJar({ 1.65f, counterTop + 0.004f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.85f, 0.20f, 0.10f }); // shichimi chili
    drawClearGlassJar({ 1.90f, counterTop + 0.004f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.95f, 0.90f, 0.70f }); // pickled garlic
    drawClearGlassJar({ 2.15f, counterTop + 0.004f, -0.36f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.20f, 0.18f, 0.15f }); // sesame seeds
    if (!rayTracingActive()) {            // with ray tracing on (R) the jug and the glasses are ray traced instead
        drawClearGlassPitcher({ 0.35f, counterTop + 0.004f, -0.36f }, NO_ROT, { 0.30f, 0.30f, 0.30f });
        drawClearGlassTumbler({ -1.85f, counterTop + 0.004f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
        drawClearGlassTumbler({ -0.35f, counterTop + 0.004f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
        drawClearGlassTumbler({  1.15f, counterTop + 0.004f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
        drawClearGlassTumbler({  2.65f, counterTop + 0.004f, -0.22f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, true, true);
    }

    // Empty tumbler on the left customer table.  With glass refraction on (Z) it is drawn at the
    // end of the frame by refraction.cpp, with ray tracing on (R) by the ray tracer; this ordinary
    // blended version is the fallback.
    if (!glassRefractionActive() && !rayTracingActive())
        drawClearGlassTumbler(TABLE_TUMBLER_POS, NO_ROT, { TABLE_TUMBLER_SCALE, TABLE_TUMBLER_SCALE, TABLE_TUMBLER_SCALE }, false, false);

    // Glass ball on the counter (ray traced with R, see rtscene.cpp)
    drawCounterGlassBall();

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

    setInteriorLightScope(true);          // interior is lit only by its own fixtures (+ sun / moon through the windows)
    clearEmission();                      // nothing from the exterior pass may leak a glow into the interior
    drawInteriorShadows();                // shadows on the floor first, the furniture on top
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

    // Two small wooden tables for customers, each with two wooden chairs facing each other
    for (int t = 0; t < 2; t++) {
        float tx = (t == 0) ? -2.4f : 2.4f;
        drawTable({ tx, FY, 2.8f }, NO_ROT, { 0.8f, 0.9f, 0.8f });
        drawChair({ tx, FY, 2.8f - 0.55f }, NO_ROT, { 0.85f, 0.85f, 0.85f });                 // facing the table (+z)
        drawChair({ tx, FY, 2.8f + 0.55f }, { 0, 180, 0 }, { 0.85f, 0.85f, 0.85f });
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
    // Ceiling spotlights over the two customer tables (key 2); they are real spot lights pointing down
    {
        bool tl = lightSpot && fixtureOn[FX_SPOT];
        for (int t = 0; t < 2; t++) {
            float tx = (t == 0) ? -2.4f : 2.4f;
            drawSpotlightFixture({ tx, 3.25f, 2.8f }, NO_ROT, { 1.3f, 1.3f, 1.3f }, tl, true);
            if (tl) drawSpotlightBeam({ tx, 2.60f, 2.8f }, 1.41f, 0.22f, 0.80f);
        }
    }

    // Ceiling Rectangular Area Light Luminaire (Softbox) above kitchen and counter
    drawAreaLightFixture({ 0.0f, 3.22f, -1.2f }, NO_ROT, ONE, lightArea && fixtureOn[FX_PANEL]);

    // Japanese Box Lantern Cluster — andon-style washi-paper lanterns above dining area
    drawJapaneseBoxLanternCluster({ 0.5f, 3.30f, 0.55f }, NO_ROT, ONE, lightPoint && fixtureOn[FX_BOX]);

    drawBottle({  2.7f, counterTop, -0.35f }, NO_ROT, { 0.3f, 0.3f, 0.3f }, SOY);
    drawBottle({  2.9f, counterTop, -0.25f }, NO_ROT, { 0.3f, 0.3f, 0.3f }, RED, GOLD);
    drawCup({ -2.7f, counterTop, -0.30f }, NO_ROT, { 0.1f, 0.1f, 0.1f }, CUP_GREEN);

    // (cash register removed)

    drawKitchen({ 0, FY, -3.2f });

    drawHangingLantern({ -1.8f, 2.95f, 0.0f }, NO_ROT, { 0.8f, 0.8f, 0.8f });   // top ring just under the ceiling (3.30)
    drawHangingLantern({  1.8f, 2.95f, 0.0f }, NO_ROT, { 0.8f, 0.8f, 0.8f });

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


    // Japanese floor lantern tower — left-front corner of the dining area
    drawJapaneseFloorLanternTower({ -4.1f, FY, 3.2f }, NO_ROT, ONE, lightPoint && fixtureOn[FX_TOWER]);

    // Second floor: tatami room with low table and tea
    drawSecondFloor();
    drawRoomMirror();                     // mirror on the upper room's back wall (ray traced with R)

    // Game objects (customer, noodles in the pot, held item) - opaque, so before the glass
    drawGameWorld();

    // All glass and steam last (see drawTransparentPass)
    drawTransparentPass();
    setInteriorLightScope(false);
}

