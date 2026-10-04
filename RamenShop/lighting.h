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
