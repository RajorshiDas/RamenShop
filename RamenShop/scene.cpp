#include "scene.h"
#include "objects.h"
#include "shader.h"

// ─── Scene feature flags ────────────────────────────────────────────────────
bool showRoof   = true;
bool showFog    = true;
bool showSteam  = true;
bool animPaused = false;

// ─── Day/Night cycle ────────────────────────────────────────────────────────
DayNightMode dayNightMode = NIGHT;
bool isDayTime = false;

// Sky colors for each mode
static const Color SKY_DAY   = { 0.45f, 0.65f, 0.90f };   // clear blue sky
static const Color SKY_NIGHT = { 0.10f, 0.10f, 0.16f };   // dark evening sky (original)

// ─── Light component flags (Ambient, Diffuse, Specular) ────────────────────
bool lightAmbient     = true;
bool lightDiffuse     = true;
bool lightSpecular    = true;

// ─── Light source flags (Directional, Point, Spot, Area) ─────────────────────
bool lightDirectional = true;
bool lightPoint       = true;
bool lightSpot        = true;
bool lightArea        = true;

// ─── Preset System ──────────────────────────────────────────────────────────
static int currentPresetIdx = 0;
struct LightPreset {
    const char* name;
    bool amb, dif, spec, dir, pt, spot, area;
};

static const LightPreset PRESETS[] = {
    { "All Lights On (Full Realism)",   true,  true,  true,  true,  true,  true,  true  },
    { "Spotlight Focus (Chef Station)", true,  true,  true,  false, false, true,  false },
    { "Area Light Softbox (Diffused)",  true,  true,  true,  false, false, false, true  },
    { "Cozy Night (Lanterns & Moon)",   true,  true,  true,  true,  true,  false, false },
    { "Specular Highlights Only",       false, false, true,  true,  true,  true,  true  },
    { "Diffuse Shading Only",           true,  true,  false, true,  true,  true,  true  },
    { "Ambient Base Only",              true,  false, false, false, false, false, false }
};

void cycleLightingPreset() {
    currentPresetIdx = (currentPresetIdx + 1) % 7;
    const auto& p = PRESETS[currentPresetIdx];
    lightAmbient     = p.amb;
    lightDiffuse     = p.dif;
    lightSpecular    = p.spec;
    lightDirectional = p.dir;
    lightPoint       = p.pt;
    lightSpot        = p.spot;
    lightArea        = p.area;
    applyLightingParameters();
}

const char* getCurrentPresetName() {
    return PRESETS[currentPresetIdx].name;
}

void toggleAmbient() {
    lightAmbient = !lightAmbient;
    applyLightingParameters();
}
void toggleDiffuse() {
    lightDiffuse = !lightDiffuse;
    applyLightingParameters();
}
void toggleSpecular() {
    lightSpecular = !lightSpecular;
    applyLightingParameters();
}
void toggleDirectional() {
    lightDirectional = !lightDirectional;
    applyLightingParameters();
}
void togglePointLights() {
    lightPoint = !lightPoint;
    applyLightingParameters();
}
void toggleSpotLight() {
    lightSpot = !lightSpot;
    applyLightingParameters();
}
void toggleAreaLight() {
    lightArea = !lightArea;
    applyLightingParameters();
}

void toggleDayNight() {
    if (dayNightMode == NIGHT) {
        dayNightMode = DAY;
        isDayTime = true;
    } else {
        dayNightMode = NIGHT;
        isDayTime = false;
    }

    // Update sky / clear color
    Color sky = isDayTime ? SKY_DAY : SKY_NIGHT;
    glClearColor(sky.r, sky.g, sky.b, 1.0f);

    // Update fog color to match sky
    GLfloat fogColor[] = { sky.r, sky.g, sky.b, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    // Day uses lighter fog, night uses denser fog
    glFogf(GL_FOG_DENSITY, isDayTime ? 0.012f : 0.022f);

    applyLightingParameters();
}

const char* getDayNightModeName() {
    return isDayTime ? "DAY" : "NIGHT";
}

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

void applyLightingParameters() {
    // Zero vector for disabled light components
    static const GLfloat ZERO4[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // 1. Global Ambient Illumination
    if (isDayTime) {
        // Daytime: bright warm ambient
        GLfloat DAY_AMB[] = { 0.30f, 0.30f, 0.28f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmbient ? DAY_AMB : ZERO4);
    } else {
        // Nighttime: low-energy fill light (original)
        static const GLfloat NIGHT_AMB[] = { 0.06f, 0.06f, 0.08f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmbient ? NIGHT_AMB : ZERO4);
    }

    // 2. Directional Light (GL_LIGHT0) - Sunlight (day) or Moonlight (night)
    if (lightDirectional) {
        glEnable(GL_LIGHT0);
        if (isDayTime) {
            // Warm sunlight
            GLfloat a0[] = { 0.15f, 0.14f, 0.10f, 1.0f };
            GLfloat d0[] = { 0.85f, 0.80f, 0.65f, 1.0f };
            GLfloat s0[] = { 1.00f, 0.95f, 0.80f, 1.0f };
            glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmbient  ? a0 : ZERO4);
            glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse  ? d0 : ZERO4);
            glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular ? s0 : ZERO4);
        } else {
            // Cool blue moonlight (original)
            GLfloat a0[] = { 0.02f, 0.02f, 0.04f, 1.0f };
            GLfloat d0[] = { 0.18f, 0.18f, 0.28f, 1.0f };
            GLfloat s0[] = { 0.40f, 0.40f, 0.50f, 1.0f };
            glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmbient  ? a0 : ZERO4);
            glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse  ? d0 : ZERO4);
            glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular ? s0 : ZERO4);
        }
    } else {
        glDisable(GL_LIGHT0);
    }

    // 3. Directional Back Fill (GL_LIGHT1) - Separation light
    if (lightDirectional) {
        glEnable(GL_LIGHT1);
        if (isDayTime) {
            // Warm sky bounce fill
            GLfloat a1[] = { 0.08f, 0.08f, 0.06f, 1.0f };
            GLfloat d1[] = { 0.35f, 0.38f, 0.45f, 1.0f };  // sky-blue bounce
            glLightfv(GL_LIGHT1, GL_AMBIENT,  lightAmbient ? a1 : ZERO4);
            glLightfv(GL_LIGHT1, GL_DIFFUSE,  lightDiffuse ? d1 : ZERO4);
        } else {
            // Cool back fill (original)
            GLfloat a1[] = { 0.01f, 0.01f, 0.02f, 1.0f };
            GLfloat d1[] = { 0.08f, 0.08f, 0.12f, 1.0f };
            glLightfv(GL_LIGHT1, GL_AMBIENT,  lightAmbient ? a1 : ZERO4);
            glLightfv(GL_LIGHT1, GL_DIFFUSE,  lightDiffuse ? d1 : ZERO4);
        }
        glLightfv(GL_LIGHT1, GL_SPECULAR, ZERO4);
    } else {
        glDisable(GL_LIGHT1);
    }

    // Organic flickers for point lights (simulate incandescent bulb flicker)
    float flickInt  = 1.0f + 0.06f * sinf(animTime * 4.2f) + 0.04f * sinf(animTime * 11.7f);
    float flickLanL = 1.0f + 0.08f * sinf(animTime * 4.3f) + 0.04f * cosf(animTime * 12.4f);
    float flickLanR = 1.0f + 0.08f * sinf(animTime * 4.1f + 1.2f) + 0.04f * sinf(animTime * 13.1f);

    // 4. Point Light: Warm Interior Central Pendant (GL_LIGHT2) - Tungsten warm
    if (lightPoint) {
        glEnable(GL_LIGHT2);
        GLfloat a2[] = { 0.06f, 0.04f, 0.02f, 1.0f };   // warm, dim ambient
        GLfloat d2[] = { 0.85f * flickInt, 0.60f * flickInt, 0.25f * flickInt, 1.0f };  // warm diffuse
        GLfloat s2[] = { 0.95f, 0.85f, 0.65f, 1.0f };   // warm specular
        glLightfv(GL_LIGHT2, GL_AMBIENT,  lightAmbient  ? a2 : ZERO4);
        glLightfv(GL_LIGHT2, GL_DIFFUSE,  lightDiffuse  ? d2 : ZERO4);
        glLightfv(GL_LIGHT2, GL_SPECULAR, lightSpecular ? s2 : ZERO4);
    } else {
        glDisable(GL_LIGHT2);
    }

    // 5. Point Lights: Left & Right Exterior Lanterns (GL_LIGHT3, GL_LIGHT4)
    if (lightPoint) {
        glEnable(GL_LIGHT3);
        glEnable(GL_LIGHT4);
        GLfloat aLan[]  = { 0.02f, 0.01f, 0.00f, 1.0f };  // warm amber ambient
        GLfloat dLanL[] = { 0.88f * flickLanL, 0.40f * flickLanL, 0.08f * flickLanL, 1.0f };
        GLfloat dLanR[] = { 0.88f * flickLanR, 0.40f * flickLanR, 0.08f * flickLanR, 1.0f };
        GLfloat sLan[]  = { 0.65f, 0.30f, 0.08f, 1.0f };  // warm glossy highlights

        glLightfv(GL_LIGHT3, GL_AMBIENT,  lightAmbient  ? aLan  : ZERO4);
        glLightfv(GL_LIGHT3, GL_DIFFUSE,  lightDiffuse  ? dLanL : ZERO4);
        glLightfv(GL_LIGHT3, GL_SPECULAR, lightSpecular ? sLan  : ZERO4);

        glLightfv(GL_LIGHT4, GL_AMBIENT,  lightAmbient  ? aLan  : ZERO4);
        glLightfv(GL_LIGHT4, GL_DIFFUSE,  lightDiffuse  ? dLanR : ZERO4);
        glLightfv(GL_LIGHT4, GL_SPECULAR, lightSpecular ? sLan  : ZERO4);
    } else {
        glDisable(GL_LIGHT3);
        glDisable(GL_LIGHT4);
    }

    // 6. Focused Spot Light: Counter Station Downlight (GL_LIGHT5) - Task lighting
    if (lightSpot) {
        glEnable(GL_LIGHT5);
        GLfloat a5[] = { 0.02f, 0.02f, 0.01f, 1.0f };
        GLfloat d5[] = { 1.00f, 0.96f, 0.85f, 1.0f };    // neutral white, slightly warm
        GLfloat s5[] = { 1.00f, 0.98f, 0.95f, 1.0f };    // almost pure white specular
        glLightfv(GL_LIGHT5, GL_AMBIENT,  lightAmbient  ? a5 : ZERO4);
        glLightfv(GL_LIGHT5, GL_DIFFUSE,  lightDiffuse  ? d5 : ZERO4);
        glLightfv(GL_LIGHT5, GL_SPECULAR, lightSpecular ? s5 : ZERO4);
    } else {
        glDisable(GL_LIGHT5);
    }

    // 7. Area Light: Ceiling Rectangular Luminaire Softbox (GL_LIGHT6, GL_LIGHT7)
    if (lightArea) {
        glEnable(GL_LIGHT6);
        glEnable(GL_LIGHT7);
        GLfloat aArea[] = { 0.03f, 0.04f, 0.05f, 1.0f };  // neutral-cool ambient
        GLfloat dArea[] = { 0.62f, 0.66f, 0.75f, 1.0f };  // neutral-cool diffuse
        GLfloat sArea[] = { 0.50f, 0.52f, 0.58f, 1.0f };  // soft specular (diffuse source)
        glLightfv(GL_LIGHT6, GL_AMBIENT,  lightAmbient  ? aArea : ZERO4);
        glLightfv(GL_LIGHT6, GL_DIFFUSE,  lightDiffuse  ? dArea : ZERO4);
        glLightfv(GL_LIGHT6, GL_SPECULAR, lightSpecular ? sArea : ZERO4);

        glLightfv(GL_LIGHT7, GL_AMBIENT,  lightAmbient  ? aArea : ZERO4);
        glLightfv(GL_LIGHT7, GL_DIFFUSE,  lightDiffuse  ? dArea : ZERO4);
        glLightfv(GL_LIGHT7, GL_SPECULAR, lightSpecular ? sArea : ZERO4);
    } else {
        glDisable(GL_LIGHT6);
        glDisable(GL_LIGHT7);
    }
}

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

    // Interior warm point-light position (same as GL_LIGHT2 in display())
    const float lx = 0.0f, ly = 2.8f, lz = -0.2f;

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

    // Override all materials to black + 35% alpha → darkens floor by 35%
    glDisable(GL_COLOR_MATERIAL);
    GLfloat sAmb[]  = { 0.0f, 0.0f, 0.0f, 0.35f };
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

    // Clear glass shopfront facade panels and sliding entrance doors
    drawClearGlassWindow({ -3.35f, 1.65f, 3.90f }, NO_ROT, ONE, 2.90f, 2.70f);
    drawClearGlassWindow({  3.35f, 1.65f, 3.90f }, NO_ROT, ONE, 2.90f, 2.70f);
    drawClearGlassWindow({ -0.90f, 1.60f, 3.92f }, NO_ROT, ONE, 1.60f, 2.60f);
    drawClearGlassWindow({  0.90f, 1.60f, 3.92f }, NO_ROT, ONE, 1.60f, 2.60f);

    drawGridWindow({ 0, 4.3f, 4.05f }, NO_ROT, ONE, 8.5f, 1.5f, 12, 3);
    drawSignBoard({ 0, 5.65f, 4.12f });

    float flickL = 1.0f + 0.08f * sin(animTime * 4.9f);
    float flickR = 1.0f + 0.08f * sin(animTime * 4.6f + 1.5f);
    float swayL  = sin(animTime * 2.0f) * 2.5f;
    float swayR  = sin(animTime * 1.9f + 0.8f) * 2.5f;

    // Left lantern — selectable as OBJ_LANTERN_L
    glPushMatrix();
    glTranslatef(-3.8f, 2.95f, 4.5f);
    applyObjDelta(OBJ_LANTERN_L);          // user transform applied at pivot
    glRotatef(swayL, 0, 0, 1);            // breeze sway on top
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.02f, 0.4f, 0.02f }, BLACK);
    setEmission(0.75f * flickL, 0.60f * flickL, 0.35f * flickL);
    drawSphere({ 0, -0.35f, 0 }, NO_ROT, { 0.55f, 0.65f, 0.55f }, LANTERN_CREAM);
    clearEmission();
    glPopMatrix();

    // Right lantern — selectable as OBJ_LANTERN_R
    glPushMatrix();
    glTranslatef(3.8f, 2.95f, 4.5f);
    applyObjDelta(OBJ_LANTERN_R);
    glRotatef(swayR, 0, 0, 1);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.02f, 0.4f, 0.02f }, BLACK);
    setEmission(0.75f * flickR, 0.60f * flickR, 0.35f * flickR);
    drawSphere({ 0, -0.35f, 0 }, NO_ROT, { 0.55f, 0.65f, 0.55f }, LANTERN_CREAM);
    clearEmission();
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

    drawLamp({ -7.0f, 0, 7.0f }, { 0, -90, 0 });
    drawLamp({  7.0f, 0, 7.0f }, { 0, -90, 0 });

    // Vending machine to the right of the shop entrance, facing the sidewalk
    drawVendingMachine({ 8.5f, 0, 3.5f }, { 0, -90, 0 });
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
            drawChopsticks({ 0.18f, 0.005f, 0.12f }, { 0, 25, 0 }, { 0.24f, 0.24f, 0.24f });
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

    float kitchenTop = FY + 0.94f;
    if (showSteam) {
        drawSteam({ -1.95f, kitchenTop + 0.46f, -3.2f }, 0.36f, 0.4f);
        drawSteam({ -1.05f, kitchenTop + 0.38f, -3.2f }, 0.28f, 1.8f);
    }

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
    drawCookingPot({ 3.5f, shelfY + 0.04f, -3.63f }, NO_ROT, { 1.2f, 1.2f, 1.2f });
    if (showSteam) drawSteam({ 3.5f, shelfY + 0.04f + 0.54f, -3.63f }, 0.44f, 3.1f);

    // Clear glass spice jars on the wall shelf
    drawClearGlassJar({ -0.85f, shelfY + 0.04f, -3.63f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.85f, 0.25f, 0.10f });
    drawClearGlassJar({ -0.55f, shelfY + 0.04f, -3.63f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, { 0.15f, 0.45f, 0.15f });

    // Ladle hanging on a hook between the two wall shelves
    drawLadle({ 0.0f, shelfY + 0.56f, -3.60f }, { 0, 0, 90 });

    drawCylinderLantern({ -1.8f, 2.9f, 0.0f }, NO_ROT, { 0.9f, 0.9f, 0.9f });
    drawCylinderLantern({  1.8f, 2.9f, 0.0f }, NO_ROT, { 0.9f, 0.9f, 0.9f });

    // Noren curtain — selectable as OBJ_NOREN
    glPushMatrix();
    glTranslatef(0, 3.0f, -1.3f);
    applyObjDelta(OBJ_NOREN);
    drawNoren({ 0, 0, 0 });
    glPopMatrix();

    drawMenuBoard({ -3.2f, 2.8f, -3.77f });
    drawMenuBoard({  3.2f, 2.8f, -3.77f });

    // Wall clock centered on the back wall between the two menu boards
    drawWallClock({ 0.0f, 3.3f, -3.78f });

    drawWallDecoration({ -4.77f, 2.0f, 0.5f }, { 0,  90, 0 });
    drawWallDecoration({  4.77f, 2.0f, 1.0f }, { 0, -90, 0 });

    drawCuboid({ -3.8f, FY, 0.5f },        NO_ROT, { 0.4f,  0.35f, 0.3f  }, WOOD);
    drawCuboid({ -3.8f, FY + 0.35f, 0.5f }, NO_ROT, { 0.45f, 0.04f, 0.35f }, LIGHT_WOOD);

    drawPlant({ 4.3f, FY, 2.0f });
}
