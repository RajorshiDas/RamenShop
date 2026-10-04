#pragma once

#include "shapes.h"
#include "furniture.h"
#include "food.h"
#include "exterior.h"
#include "decorations.h"
#include "lighting.h"

// Scene feature toggles
extern bool showRoof;
extern bool showSteam;
extern bool animPaused;

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
