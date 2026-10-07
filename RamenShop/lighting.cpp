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
    glLightf(GL_LIGHT4, GL_SPOT_CUTOFF,    28.0f);
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
    GLfloat pos4[] = { -1.5f, 3.25f, -0.3f, 1.0f };
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
    if (isDayTime) {
        GLfloat dayAmb[] = { 0.42f, 0.42f, 0.46f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmbient ? dayAmb : ZERO4);
    } else {
        // Low but visible → dark areas maintain shape, pools of light stand out
        GLfloat nightAmb[] = { 0.04f, 0.04f, 0.06f, 1.0f };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lightAmbient ? nightAmb : ZERO4);
    }

    // ════════════════════════════════════════════════════════════════════
    //  OUTDOOR LIGHTING
    // ════════════════════════════════════════════════════════════════════

    // ── LIGHT0: Moon / Sun (Directional) ─────────────────────────────
    if (lightDirectional) {
        glEnable(GL_LIGHT0);
        if (isDayTime) {
            // Bright warm sunlight (~5500K warm gold)
            GLfloat a0[] = { 0.10f, 0.10f, 0.08f, 1.0f };
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
        float dayScale = isDayTime ? 0.30f : 1.0f;

        GLfloat a1[] = { 0.03f * dayScale, 0.02f * dayScale, 0.01f * dayScale, 1.0f };
        GLfloat d1[] = { 0.70f * flickPendant * dayScale,
                         0.48f * flickPendant * dayScale,
                         0.18f * flickPendant * dayScale, 1.0f };
        GLfloat s1[] = { 0.80f * dayScale, 0.65f * dayScale, 0.40f * dayScale, 1.0f };

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
        float daySpot = isDayTime ? 0.5f : 1.0f;

        GLfloat a4[] = { 0.01f * daySpot, 0.01f * daySpot, 0.01f * daySpot, 1.0f };
        GLfloat d4[] = { 0.90f * daySpot, 0.88f * daySpot, 0.78f * daySpot, 1.0f };
        GLfloat s4[] = { 0.95f * daySpot, 0.93f * daySpot, 0.85f * daySpot, 1.0f };

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
        float dayUp = isDayTime ? 0.25f : 1.0f;
        float flickUp = 1.0f + 0.03f * sinf(animTime * 3.0f);

        GLfloat a7[] = { 0.02f * dayUp, 0.015f * dayUp, 0.008f * dayUp, 1.0f };
        GLfloat d7[] = { 0.40f * dayUp * flickUp,
                         0.30f * dayUp * flickUp,
                         0.14f * dayUp * flickUp, 1.0f };
        GLfloat s7[] = { 0.25f * dayUp, 0.20f * dayUp, 0.10f * dayUp, 1.0f };

        glLightfv(GL_LIGHT7, GL_AMBIENT,  lightAmbient  ? a7 : ZERO4);
        glLightfv(GL_LIGHT7, GL_DIFFUSE,  lightDiffuse  ? d7 : ZERO4);
        glLightfv(GL_LIGHT7, GL_SPECULAR, lightSpecular ? s7 : ZERO4);
    } else {
        glDisable(GL_LIGHT7);
    }
}
