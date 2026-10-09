#include "lighting.h"
#include "shapes.h"
#include "shader.h"
#include <cmath>

// ════════════════════════════════════════════════════════════════════════════
//  LIGHTING SYSTEM — Realistic Japanese Ramen Shop
// ════════════════════════════════════════════════════════════════════════════

// ─── Day/Night state ───────────────────────────────────────────────────────
DayNightMode dayNightMode = NIGHT;
bool isDayTime = false;

// Sky colors
static const Color SKY_DAY   = { 0.64f, 0.79f, 0.93f };   // pale horizon haze (also the fog colour)
static const Color SKY_NIGHT = { 0.10f, 0.10f, 0.16f };

// ─── Fog state ─────────────────────────────────────────────────────────────
bool showFog = false;

// ─── Light component flags ─────────────────────────────────────────────────
bool lightAmbient   = true;
bool lightDiffuse   = true;
bool lightSpecular  = true;

// ─── Light source group flags ──────────────────────────────────────────────
bool lightDirectional = true;
bool lightPoint       = true;
bool lightSpot        = true;
bool lightArea        = true;

bool fixtureOn[FX_COUNT] = { true, true, true, true, true, true, true };

float pointFixtureShare()
{
    return fixtureOn[FX_PENDANTS] ? 1.0f : 0.0f;      // box / hanging lanterns and the tower have their own lights now
}

bool anyInteriorLightOn()
{
    for (int i = 0; i < FX_COUNT; i++) {
        bool grp = (i == FX_SPOT) ? lightSpot : ((i == FX_PANEL || i == FX_DOME) ? lightArea : lightPoint);
        if (grp && fixtureOn[i]) return true;
    }
    return false;
}

// ─── Interior light slots ────────────────────────────────────────────────────
// OpenGL has only 8 lights.  Inside the shop the four lights that normally serve the OUTSIDE
// (entrance lanterns 2 and 3, street lamps 5, shop sign 6) are reused for the other interior
// fixtures, so each interior fixture is a real light with its own ambient, diffuse and specular:
//   LIGHT1 pendants   LIGHT4 kitchen spot   LIGHT7 second-floor dome   (as before)
//   LIGHT2 hanging lanterns   LIGHT3 paper box lanterns   LIGHT5 ceiling panel   LIGHT6 floor tower
struct InteriorSlot { int light; int fixture; bool* group; float x, y, z; float r, g, b; float lin, quad; };
void applyInteriorFixtureLights()
{
    static const GLfloat ZERO4[] = { 0, 0, 0, 1 };
    // Slot LIGHT2: light from the entrance lanterns that gets through the thin paper walls (weaker, from outside).
    // Slot LIGHT3: the hanging lanterns and the paper box lanterns merged (they hang in the same area).
    // (the ceiling panel is merged into the same slot as the lanterns: they all hang in the same area)
    // Slot LIGHT5: the right customer-table lamp (the left one is LIGHT4).  Slot LIGHT6: floor lantern tower.
    float wH = (lightPoint && fixtureOn[FX_HANGING]) ? 1.0f : 0.0f, wB = (lightPoint && fixtureOn[FX_BOX]) ? 1.0f : 0.0f;
    float wP = (lightArea && fixtureOn[FX_PANEL]) ? 1.0f : 0.0f;
    float wS = wH + wB + wP;
    float bx = (wS > 0.0f) ? (0.0f * wH + 0.5f * wB + 0.0f * wP) / wS : 0.25f;
    float bz = (wS > 0.0f) ? (0.1f * wH + 0.6f * wB - 1.2f * wP) / wS : 0.35f;
    bool boxOn = wS > 0.0f;
    bool paperOn = lightPoint;                                   // entrance lanterns (group switch)
    static bool dummyOn = true;
    InteriorSlot slots[4] = {
        { GL_LIGHT2, -1,         &paperOn,     0.0f, 2.60f,  4.7f, 1.00f, 0.55f, 0.18f, 0.12f, 0.06f },   // through the paper
        { GL_LIGHT3, -2,         &boxOn,       bx,   3.05f,  bz,   1.00f, 0.82f, 0.56f, 0.15f, 0.07f },
        { GL_LIGHT5, FX_SPOT,    &lightSpot,   2.4f, 3.25f,  2.8f, 0.97f, 0.95f, 0.84f, 0.09f, 0.032f },  // right table spotlight
        { GL_LIGHT6, FX_TOWER,   &lightPoint, -4.1f, 1.40f,  3.2f, 1.00f, 0.82f, 0.56f, 0.28f, 0.13f },
    };
    (void)dummyOn;
    float day = isDayTime ? 0.40f : 1.0f;
    for (auto& sl : slots) {
        bool on = *sl.group && (sl.fixture >= 0 ? fixtureOn[sl.fixture] : true);
        float k = on ? day : 0.0f;
        if (sl.fixture == -1) k = on ? (isDayTime ? 0.12f : 0.34f) : 0.0f;                  // paper lets only some light through
        if (sl.fixture == -2) k = on ? (isDayTime ? 0.40f : 1.0f) * ((wS > 2.5f) ? 1.5f : ((wS > 1.5f) ? 1.25f : 1.0f)) : 0.0f;
        GLfloat pos[] = { sl.x, sl.y, sl.z, 1.0f };
        glLightfv(sl.light, GL_POSITION, pos);
        if (sl.light == GL_LIGHT5) {                       // the table spotlight: a real downward cone
            GLfloat dn[] = { 0.0f, -1.0f, 0.0f };
            glLightf(sl.light, GL_SPOT_CUTOFF, 28.0f);
            glLightf(sl.light, GL_SPOT_EXPONENT, 35.0f);
            glLightfv(sl.light, GL_SPOT_DIRECTION, dn);
        } else glLightf(sl.light, GL_SPOT_CUTOFF, 180.0f);
        glLightf(sl.light, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(sl.light, GL_LINEAR_ATTENUATION, sl.lin);
        glLightf(sl.light, GL_QUADRATIC_ATTENUATION, sl.quad);
        // Paper lanterns: a soft ambient glow (light scattered by the paper) plus normal
        // diffuse and specular, kept moderate so pale surfaces do not wash out to white.
        const bool lantern = (sl.light == GL_LIGHT3 || sl.light == GL_LIGHT6);
        const float ak = lantern ? 0.16f : 0.05f, dk = lantern ? 0.55f : 0.78f, sk = lantern ? 0.75f : 1.00f;
        GLfloat a[] = { ak * sl.r * k, ak * sl.g * k, ak * sl.b * k, 1.0f };
        GLfloat d[] = { dk * sl.r * k, dk * sl.g * k, dk * sl.b * k, 1.0f };
        GLfloat sp[] = { sk * sl.r * k, sk * sl.g * k, sk * sl.b * k, 1.0f };
        glLightfv(sl.light, GL_AMBIENT,  lightAmbient  ? a  : ZERO4);
        glLightfv(sl.light, GL_DIFFUSE,  lightDiffuse  ? d  : ZERO4);
        glLightfv(sl.light, GL_SPECULAR, lightSpecular ? sp : ZERO4);
        glEnable(sl.light);
    }
}

// Back to the outdoor roles of lights 2, 3, 5, 6
void restoreExteriorFixtureLights()
{
    for (int i = 2; i <= 3; i++) {
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION,    0.22f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.20f);
    }
    glLightf(GL_LIGHT5, GL_SPOT_CUTOFF, 180.0f);                 // street lamps are ordinary point lights again
    glLightf(GL_LIGHT5, GL_CONSTANT_ATTENUATION,  0.8f);
    glLightf(GL_LIGHT5, GL_LINEAR_ATTENUATION,    0.045f);
    glLightf(GL_LIGHT5, GL_QUADRATIC_ATTENUATION, 0.012f);
    glLightf(GL_LIGHT6, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT6, GL_LINEAR_ATTENUATION,    0.18f);
    glLightf(GL_LIGHT6, GL_QUADRATIC_ATTENUATION, 0.10f);
    applyLightingParameters();          // colours + enables for the outdoor roles
    placeLightsInWorldSpace();          // positions (current modelview = camera view)
}

// ─── Preset system ─────────────────────────────────────────────────────────
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

// ─── Toggle functions ──────────────────────────────────────────────────────
void toggleAmbient()     { lightAmbient     = !lightAmbient;     applyLightingParameters(); }
void toggleDiffuse()     { lightDiffuse     = !lightDiffuse;     applyLightingParameters(); }
void toggleSpecular()    { lightSpecular    = !lightSpecular;    applyLightingParameters(); }
void toggleDirectional() { lightDirectional = !lightDirectional; applyLightingParameters(); }
void togglePointLights() { lightPoint       = !lightPoint;       applyLightingParameters(); }
void toggleSpotLight()   { lightSpot        = !lightSpot;        applyLightingParameters(); }
void toggleAreaLight()   { lightArea        = !lightArea;        applyLightingParameters(); }

// ─── Day/Night toggle ──────────────────────────────────────────────────────
void toggleDayNight() {
    if (dayNightMode == NIGHT) {
        dayNightMode = DAY;
        isDayTime = true;
    } else {
        dayNightMode = NIGHT;
        isDayTime = false;
    }

    Color sky = isDayTime ? SKY_DAY : SKY_NIGHT;
    glClearColor(sky.r, sky.g, sky.b, 1.0f);

    GLfloat fogColor[] = { sky.r, sky.g, sky.b, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_DENSITY, isDayTime ? 0.012f : 0.018f);

    applyLightingParameters();
}

const char* getDayNightModeName() {
    return isDayTime ? "DAY" : "NIGHT";
}

// ════════════════════════════════════════════════════════════════════════════
//  LIGHT INITIALIZATION — called once from init()
// ════════════════════════════════════════════════════════════════════════════
void initLighting()
{
    setLighting(true);

    glEnable(GL_LIGHT0);   // Moon / Sun (directional)
    glEnable(GL_LIGHT1);   // Dining pendants (point)
    glEnable(GL_LIGHT2);   // Left entrance chochin (point)
    glEnable(GL_LIGHT3);   // Right entrance chochin (point)
    glEnable(GL_LIGHT4);   // Kitchen spotlight (spot)
    glEnable(GL_LIGHT5);   // Street lamps (point)
    glEnable(GL_LIGHT6);   // Shop sign / entrance glow (point)
    glEnable(GL_LIGHT7);   // Second floor ceiling (point)

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glShadeModel(GL_SMOOTH);

    // ════════════════════════════════════════════════════════════════════
    //  ATTENUATION — atten = 1 / (Kc + Kl*d + Kq*d²)
    //
    //  Values tuned per light based on realistic fixture range.
    //  The shop interior is ~10m wide, 8m deep.
    //  Strong quadratic falloff creates visible pools of light.
    // ════════════════════════════════════════════════════════════════════

    // ── INTERIOR: Dining pendants — warm pool ~4m radius ─────────────
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.14f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.07f);

    // ── ENTRANCE: Chochin lanterns — small warm pool ~2.5m radius ────
    for (int i = 2; i <= 3; i++) {
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION,    0.22f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.20f);
    }

    // ── KITCHEN: Spotlight — focused beam ~2.5m, narrow cone ─────────
    glLightf(GL_LIGHT4, GL_SPOT_CUTOFF,    28.0f);           // table spotlights (left = LIGHT4, right = LIGHT5)
    glLightf(GL_LIGHT4, GL_SPOT_EXPONENT,  35.0f);
    glLightf(GL_LIGHT4, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT4, GL_LINEAR_ATTENUATION,    0.09f);
    glLightf(GL_LIGHT4, GL_QUADRATIC_ATTENUATION, 0.032f);

    // ── OUTDOOR: Street lamps — wide pool ~8m radius ─────────────────
    glLightf(GL_LIGHT5, GL_CONSTANT_ATTENUATION,  0.8f);
    glLightf(GL_LIGHT5, GL_LINEAR_ATTENUATION,    0.045f);
    glLightf(GL_LIGHT5, GL_QUADRATIC_ATTENUATION, 0.012f);

    // ── ENTRANCE: Shop sign / awning glow — ~3m radius ──────────────
    glLightf(GL_LIGHT6, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT6, GL_LINEAR_ATTENUATION,    0.18f);
    glLightf(GL_LIGHT6, GL_QUADRATIC_ATTENUATION, 0.10f);

    // ── SECOND FLOOR: Paper dome — soft ~2.5m radius ─────────────────
    glLightf(GL_LIGHT7, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT7, GL_LINEAR_ATTENUATION,    0.22f);
    glLightf(GL_LIGHT7, GL_QUADRATIC_ATTENUATION, 0.20f);

    applyLightingParameters();
}

// ════════════════════════════════════════════════════════════════════════════
//  LIGHT POSITIONS — called every frame after camera transform
// ════════════════════════════════════════════════════════════════════════════
void placeLightsInWorldSpace()
{
    // ── OUTDOOR ──────────────────────────────────────────────────────────

    // LIGHT0: Directional sky light (w=0 → infinitely far, direction only)
    //   Sun in daytime comes from elevated angle; moon at night from lower atmospheric angle
    GLfloat pos0[4];
    if (isDayTime) {
        pos0[0] = 0.45f; pos0[1] = 0.70f; pos0[2] = -0.45f; pos0[3] = 0.0f;
    } else {
        pos0[0] = 0.50f; pos0[1] = 0.38f; pos0[2] = -0.70f; pos0[3] = 0.0f;
    }
    glLightfv(GL_LIGHT0, GL_POSITION, pos0);

    // LIGHT5: Street lamps — averaged position of the two lamp posts
    //   Lamp posts are at (±7, 0, 7); light placed high between them
    GLfloat pos5[] = { 0.0f, 3.40f, 7.0f, 1.0f };
    glLightfv(GL_LIGHT5, GL_POSITION, pos5);

    // ── ENTRANCE ─────────────────────────────────────────────────────────

    // LIGHT2: Left chochin lantern — hangs beside the entrance
    GLfloat pos2[] = { -3.8f, 2.50f, 4.5f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, pos2);

    // LIGHT3: Right chochin lantern — hangs beside the entrance
    GLfloat pos3[] = { 3.8f, 2.50f, 4.5f, 1.0f };
    glLightfv(GL_LIGHT3, GL_POSITION, pos3);

    // LIGHT6: Shop sign / entrance awning glow
    //   Positioned at the sign board above the entrance, illuminating
    //   the storefront and sidewalk below
    GLfloat pos6[] = { 0.0f, 4.5f, 4.2f, 1.0f };
    glLightfv(GL_LIGHT6, GL_POSITION, pos6);

    // ── INTERIOR ─────────────────────────────────────────────────────────

    // LIGHT1: Dining pendant lamps + box lantern cluster
    //   Centered above the counter/dining area
    GLfloat pos1[] = { 0.0f, 2.85f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);

    // ── KITCHEN ──────────────────────────────────────────────────────────

    // LIGHT4: Kitchen ceiling spotlight — points straight down at prep area
    GLfloat pos4[] = { -2.4f, 3.25f, 2.8f, 1.0f };         // left table spotlight (the right one is LIGHT5)
    GLfloat dir4[] = { 0.0f, -1.0f, 0.0f };
    glLightfv(GL_LIGHT4, GL_POSITION, pos4);
    glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, dir4);

    // ── SECOND FLOOR ─────────────────────────────────────────────────────

    // LIGHT7: Paper ceiling dome — soft overhead light in tatami room
    GLfloat pos7[] = { 0.0f, 5.15f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT7, GL_POSITION, pos7);
}

// ════════════════════════════════════════════════════════════════════════════
//  LIGHT PARAMETERS — called every frame to update colors, flicker, day/night
// ════════════════════════════════════════════════════════════════════════════
void applyLightingParameters()
{
    static const GLfloat ZERO4[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // ════════════════════════════════════════════════════════════════════
    //  GLOBAL AMBIENT
    //  Night: very low → dark areas remain dark, pools of light stand out
    //  Day:   moderate → fills shadows naturally like diffused skylight
    // ════════════════════════════════════════════════════════════════════
    // No scene-wide ambient: in the Phong model every light source contributes its OWN
    // ambient, diffuse and specular terms, so switching a light off removes all three.
    // (A constant global ambient would keep lighting objects even with every light off.)
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ZERO4);

    // ════════════════════════════════════════════════════════════════════
    //  OUTDOOR LIGHTING
    // ════════════════════════════════════════════════════════════════════

    // ── LIGHT0: Moon / Sun (Directional) ─────────────────────────────
    if (lightDirectional) {
        glEnable(GL_LIGHT0);
        if (isDayTime) {
            // Bright warm sunlight (~5500K warm gold)
            GLfloat a0[] = { 0.38f, 0.38f, 0.42f, 1.0f };      // skylight ambient belongs to the sun
            GLfloat d0[] = { 0.85f, 0.78f, 0.55f, 1.0f };
            GLfloat s0[] = { 0.95f, 0.90f, 0.75f, 1.0f };
            glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmbient  ? a0 : ZERO4);
            glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse  ? d0 : ZERO4);
            glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular ? s0 : ZERO4);
        } else {
            // Cool moonlight (~6500K blue-silver) — bright enough to see the landscape
            GLfloat a0[] = { 0.03f, 0.03f, 0.05f, 1.0f };
            GLfloat d0[] = { 0.14f, 0.16f, 0.25f, 1.0f };
            GLfloat s0[] = { 0.22f, 0.25f, 0.35f, 1.0f };
            glLightfv(GL_LIGHT0, GL_AMBIENT,  lightAmbient  ? a0 : ZERO4);
            glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightDiffuse  ? d0 : ZERO4);
            glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular ? s0 : ZERO4);
        }
    } else {
        glDisable(GL_LIGHT0);
    }

    // ── LIGHT5: Street lamps (~2500K sodium yellow) ──────────────────
    //   Bright at night for visible light pools on the street
    if (lightArea) {
        if (!isDayTime) {
            glEnable(GL_LIGHT5);
            GLfloat a5[] = { 0.06f, 0.04f, 0.01f, 1.0f };
            GLfloat d5[] = { 0.95f, 0.75f, 0.28f, 1.0f };
            GLfloat s5[] = { 0.70f, 0.55f, 0.20f, 1.0f };
            glLightfv(GL_LIGHT5, GL_AMBIENT,  lightAmbient  ? a5 : ZERO4);
            glLightfv(GL_LIGHT5, GL_DIFFUSE,  lightDiffuse  ? d5 : ZERO4);
            glLightfv(GL_LIGHT5, GL_SPECULAR, lightSpecular ? s5 : ZERO4);
        } else {
            glDisable(GL_LIGHT5);
        }
    } else {
        glDisable(GL_LIGHT5);
    }

    // ════════════════════════════════════════════════════════════════════
    //  ENTRANCE LIGHTING
    // ════════════════════════════════════════════════════════════════════

    // Organic candle flicker for entrance lanterns (independent phases)
    float flickLanL = 1.0f + 0.08f * sinf(animTime * 4.3f)
                           + 0.05f * cosf(animTime * 12.4f);
    float flickLanR = 1.0f + 0.08f * sinf(animTime * 4.1f + 1.2f)
                           + 0.05f * sinf(animTime * 13.1f);

    // ── LIGHT2 & LIGHT3: Entrance chochin lanterns (~2200K candle) ───
    if (lightPoint) {
        glEnable(GL_LIGHT2);
        glEnable(GL_LIGHT3);
        float dayLan = isDayTime ? 0.15f : 1.0f;

        GLfloat aLan[]  = { 0.01f * dayLan, 0.005f * dayLan, 0.0f, 1.0f };
        GLfloat dLanL[] = { 0.75f * flickLanL * dayLan, 0.32f * flickLanL * dayLan, 0.06f * flickLanL * dayLan, 1.0f };
        GLfloat dLanR[] = { 0.75f * flickLanR * dayLan, 0.32f * flickLanR * dayLan, 0.06f * flickLanR * dayLan, 1.0f };
        GLfloat sLan[]  = { 0.50f * dayLan, 0.22f * dayLan, 0.04f * dayLan, 1.0f };

        glLightfv(GL_LIGHT2, GL_AMBIENT,  lightAmbient  ? aLan  : ZERO4);
        glLightfv(GL_LIGHT2, GL_DIFFUSE,  lightDiffuse  ? dLanL : ZERO4);
        glLightfv(GL_LIGHT2, GL_SPECULAR, lightSpecular ? sLan  : ZERO4);

        glLightfv(GL_LIGHT3, GL_AMBIENT,  lightAmbient  ? aLan  : ZERO4);
        glLightfv(GL_LIGHT3, GL_DIFFUSE,  lightDiffuse  ? dLanR : ZERO4);
        glLightfv(GL_LIGHT3, GL_SPECULAR, lightSpecular ? sLan  : ZERO4);
    } else {
        glDisable(GL_LIGHT2);
        glDisable(GL_LIGHT3);
    }

    // ── LIGHT6: Shop sign / entrance awning (~3000K warm white) ──────
    //   Illuminates the storefront — sign glow at night, subtle during day
    if (lightArea) {
        glEnable(GL_LIGHT6);
        if (!isDayTime) {
            float flickSign = 1.0f + 0.03f * sinf(animTime * 2.8f);
            GLfloat a6[] = { 0.02f, 0.015f, 0.005f, 1.0f };
            GLfloat d6[] = { 0.50f * flickSign, 0.38f * flickSign, 0.15f * flickSign, 1.0f };
            GLfloat s6[] = { 0.35f, 0.28f, 0.12f, 1.0f };
            glLightfv(GL_LIGHT6, GL_AMBIENT,  lightAmbient  ? a6 : ZERO4);
            glLightfv(GL_LIGHT6, GL_DIFFUSE,  lightDiffuse  ? d6 : ZERO4);
            glLightfv(GL_LIGHT6, GL_SPECULAR, lightSpecular ? s6 : ZERO4);
        } else {
            // Subtle during day
            GLfloat a6d[] = { 0.01f, 0.01f, 0.005f, 1.0f };
            GLfloat d6d[] = { 0.10f, 0.08f, 0.04f, 1.0f };
            GLfloat s6d[] = { 0.05f, 0.04f, 0.02f, 1.0f };
            glLightfv(GL_LIGHT6, GL_AMBIENT,  lightAmbient  ? a6d : ZERO4);
            glLightfv(GL_LIGHT6, GL_DIFFUSE,  lightDiffuse  ? d6d : ZERO4);
            glLightfv(GL_LIGHT6, GL_SPECULAR, lightSpecular ? s6d : ZERO4);
        }
    } else {
        glDisable(GL_LIGHT6);
    }

    // ════════════════════════════════════════════════════════════════════
    //  INTERIOR LIGHTING
    // ════════════════════════════════════════════════════════════════════

    // Subtle incandescent flicker for dining pendants
    float flickPendant = 1.0f + 0.04f * sinf(animTime * 4.2f)
                               + 0.03f * sinf(animTime * 11.7f);

    // ── LIGHT1: Dining pendant lamps (~2700K warm amber) ─────────────
    //   Primary interior light — warm amber pool over the counter area
    if (lightPoint) {
        glEnable(GL_LIGHT1);
        float dayScale = (isDayTime ? 0.30f : 1.0f) * pointFixtureShare();

        GLfloat a1[] = { 0.03f * dayScale, 0.02f * dayScale, 0.01f * dayScale, 1.0f };
        GLfloat d1[] = { 0.70f * flickPendant * dayScale,
                         0.48f * flickPendant * dayScale,
                         0.18f * flickPendant * dayScale, 1.0f };
        GLfloat s1[] = { 1.00f * dayScale, 0.92f * dayScale, 0.76f * dayScale, 1.0f };

        glLightfv(GL_LIGHT1, GL_AMBIENT,  lightAmbient  ? a1 : ZERO4);
        glLightfv(GL_LIGHT1, GL_DIFFUSE,  lightDiffuse  ? d1 : ZERO4);
        glLightfv(GL_LIGHT1, GL_SPECULAR, lightSpecular ? s1 : ZERO4);
    } else {
        glDisable(GL_LIGHT1);
    }

    // ════════════════════════════════════════════════════════════════════
    //  KITCHEN LIGHTING
    // ════════════════════════════════════════════════════════════════════

    // ── LIGHT4: Kitchen spotlight (~4000K neutral white) ─────────────
    //   Functional task lighting — slightly warm neutral, focused cone
    if (lightSpot) {
        glEnable(GL_LIGHT4);
        float daySpot = (isDayTime ? 0.5f : 1.0f) * (fixtureOn[FX_SPOT] ? 1.0f : 0.0f);

        GLfloat a4[] = { 0.01f * daySpot, 0.01f * daySpot, 0.01f * daySpot, 1.0f };
        GLfloat d4[] = { 0.90f * daySpot, 0.88f * daySpot, 0.78f * daySpot, 1.0f };
        GLfloat s4[] = { 1.00f * daySpot, 1.00f * daySpot, 0.95f * daySpot, 1.0f };

        glLightfv(GL_LIGHT4, GL_AMBIENT,  lightAmbient  ? a4 : ZERO4);
        glLightfv(GL_LIGHT4, GL_DIFFUSE,  lightDiffuse  ? d4 : ZERO4);
        glLightfv(GL_LIGHT4, GL_SPECULAR, lightSpecular ? s4 : ZERO4);
    } else {
        glDisable(GL_LIGHT4);
    }

    // ════════════════════════════════════════════════════════════════════
    //  SECOND FLOOR LIGHTING
    // ════════════════════════════════════════════════════════════════════

    // ── LIGHT7: Paper ceiling dome (~3000K soft warm) ────────────────
    //   Gentle ambient for the tatami room — quiet, restful light
    if (lightArea) {
        glEnable(GL_LIGHT7);
        float dayUp = (isDayTime ? 0.25f : 1.0f) * (fixtureOn[FX_DOME] ? 1.0f : 0.0f);
        float flickUp = 1.0f + 0.03f * sinf(animTime * 3.0f);

        GLfloat a7[] = { 0.02f * dayUp, 0.015f * dayUp, 0.008f * dayUp, 1.0f };
        GLfloat d7[] = { 0.40f * dayUp * flickUp,
                         0.30f * dayUp * flickUp,
                         0.14f * dayUp * flickUp, 1.0f };
        GLfloat s7[] = { 0.60f * dayUp, 0.52f * dayUp, 0.36f * dayUp, 1.0f };

        glLightfv(GL_LIGHT7, GL_AMBIENT,  lightAmbient  ? a7 : ZERO4);
        glLightfv(GL_LIGHT7, GL_DIFFUSE,  lightDiffuse  ? d7 : ZERO4);
        glLightfv(GL_LIGHT7, GL_SPECULAR, lightSpecular ? s7 : ZERO4);
    } else {
        glDisable(GL_LIGHT7);
    }
}
