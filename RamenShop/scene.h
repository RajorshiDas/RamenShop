#pragma once

#include "shapes.h"
#include "furniture.h"
#include "food.h"
#include "exterior.h"
#include "decorations.h"

// Scene feature toggles
extern bool showRoof;
extern bool showFog;
extern bool showSteam;
extern bool animPaused;

// Day/Night cycle
enum DayNightMode { DAY, NIGHT };
extern DayNightMode dayNightMode;
extern bool isDayTime;  // true = day, false = night

// Light component toggles (Ambient, Diffuse, Specular)
extern bool lightAmbient;
extern bool lightDiffuse;
extern bool lightSpecular;

// Light source toggles (Directional, Point, Spot, Area)
extern bool lightDirectional;
extern bool lightPoint;
extern bool lightSpot;
extern bool lightArea;

void toggleAmbient();
void toggleDiffuse();
void toggleSpecular();
void toggleDirectional();
void togglePointLights();
void toggleSpotLight();
void toggleAreaLight();
void toggleDayNight();  // Toggle between day and night
void cycleLightingPreset();
const char* getCurrentPresetName();
const char* getDayNightModeName();  // Returns "DAY" or "NIGHT"
void applyLightingParameters();

// Entrance door state (click to open/close)
extern bool doorOpen;
extern float doorAngle;

// Sliding shoji door state (click to open/close)
extern bool slideDoorOpen;
extern float slideDoorOffset;

// High-level scene functions
void drawSky();       // Sky dome with sun (day) or moon (night)
void drawGround();
void drawExterior();
void drawInterior();
