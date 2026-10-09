#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>

// ════════════════════════════════════════════════════════════════════════════
//  LIGHTING SYSTEM — Realistic Japanese Ramen Shop
//
//  8 GL lights mapped to visible scene objects:
//
//  ┌─────────────────────────────────────────────────────────────────────┐
//  │  OUTDOOR LIGHTING                                                  │
//  │    LIGHT0  Directional   Moon / Sun          (sky fill)            │
//  │    LIGHT5  Point         Street lamps        (sidewalk pools)      │
//  │                                                                    │
//  │  ENTRANCE LIGHTING                                                 │
//  │    LIGHT2  Point         Left chochin        (entrance left)       │
//  │    LIGHT3  Point         Right chochin       (entrance right)      │
//  │    LIGHT6  Point         Shop sign / awning  (storefront glow)     │
//  │                                                                    │
//  │  INTERIOR LIGHTING                                                 │
//  │    LIGHT1  Point         Dining pendants     (counter / dining)    │
//  │                                                                    │
//  │  KITCHEN LIGHTING                                                  │
//  │    LIGHT4  Spot          Ceiling spotlight   (chef prep station)   │
//  │                                                                    │
//  │  SECOND FLOOR LIGHTING                                             │
//  │    LIGHT7  Point         Ceiling paper dome  (tatami room)         │
//  └─────────────────────────────────────────────────────────────────────┘
//
//  Design principles:
//    • Each GL light corresponds to a visible object in the scene
//    • Warm color temperatures indoors (2200K–2700K amber/orange)
//    • Neutral kitchen task lighting (4000K white)
//    • Cool outdoor moonlight (6500K+ blue-silver)
//    • Strong attenuation creates visible light pools and dark areas
//    • Night: low global ambient → high contrast between lit/unlit
//    • Day: high global ambient → artificial lights subtle
// ════════════════════════════════════════════════════════════════════════════

// ─── Day/Night cycle ────────────────────────────────────────────────────────
enum DayNightMode { DAY, NIGHT };
extern DayNightMode dayNightMode;
extern bool isDayTime;

void toggleDayNight();
const char* getDayNightModeName();

// ─── Light component flags (Ambient / Diffuse / Specular) ──────────────────
extern bool lightAmbient;
extern bool lightDiffuse;
extern bool lightSpecular;

// ─── Light source group flags ──────────────────────────────────────────────
extern bool lightDirectional;   // LIGHT0: Moon/Sun
extern bool lightPoint;         // LIGHT1,2,3: dining + entrance chochins
extern bool lightSpot;          // LIGHT4: kitchen spotlight
extern bool lightArea;          // LIGHT5,6,7: street, sign, 2nd floor

// ─── Toggle functions ──────────────────────────────────────────────────────
void toggleAmbient();
void toggleDiffuse();
void toggleSpecular();
void toggleDirectional();
void togglePointLights();
void toggleSpotLight();
void toggleAreaLight();

// ─── Preset system ─────────────────────────────────────────────────────────
void cycleLightingPreset();
const char* getCurrentPresetName();

// ─── Core lighting API ─────────────────────────────────────────────────────
// Call once during init() after GL context and GLEW are ready.
void initLighting();

// Call every frame after camera transform to set light positions in eye space.
void placeLightsInWorldSpace();

// Call every frame to update light colors, intensities, flicker, day/night.
void applyLightingParameters();

// Fog state (shared with shader)
extern bool showFog;

// ── Individual light fixtures (each can be switched on its own) ─────────────
// Order matches the light tour in camera.cpp.  A fixture is lit when its group switch
// (lightPoint / lightSpot / lightArea) AND its own switch are on.
enum FixtureId { FX_PENDANTS, FX_SPOT, FX_PANEL, FX_BOX, FX_HANGING, FX_TOWER, FX_DOME, FX_COUNT };
extern bool fixtureOn[FX_COUNT];
// Share (0..1) of the shared "dining point light" supplied by the fixtures that are on
float pointFixtureShare();

// Reuse lights 2, 3, 5, 6 as interior fixtures while the interior is drawn (see lighting.cpp)
void applyInteriorFixtureLights();
void restoreExteriorFixtureLights();

// True while at least one interior fixture (keys 1-7) is switched on
bool anyInteriorLightOn();
