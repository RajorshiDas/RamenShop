#pragma once

#include "shapes.h"
#include "furniture.h"
#include "food.h"
#include "exterior.h"
#include "decorations.h"

// Scene feature toggles
extern bool showRoof;
extern bool showSteam;
extern bool animPaused;

// Day/Night cycle
enum DayNightMode { DAY, NIGHT };
extern DayNightMode dayNightMode;
extern bool isDayTime;  // true = day, false = night

// Restaurant lights master switch (L key)
extern bool restaurantLightsOn;

void toggleRestaurantLights();
void toggleDayNight();
const char* getDayNightModeName();
void applyLightingParameters();

// Entrance door state (click to open/close)
extern bool doorOpen;
extern float doorAngle;

// Sliding shoji door state (click to open/close)
extern bool slideDoorOpen;
extern float slideDoorOffset;

// Upper room door state (click to open/close)
extern bool upperDoorOpen;
extern float upperDoorOffset;

// High-level scene functions
void drawSky();       // Sky dome with sun (day) or moon (night)
void drawGround();
void drawExterior();
void drawInterior();
