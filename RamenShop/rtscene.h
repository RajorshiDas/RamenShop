#pragma once

#include "rtcore.h"
#include "shapes.h"

// ════════════════════════════════════════════════════════════════════════════
//  THE SHOP AS THE RAY TRACER SEES IT
//
//  Featured objects (ray traced on screen):
//    - the two steel stock pots on the stove      reflections (metal)
//    - the four ramen bowls on the counter         reflections on the glaze and the soup
//    - a glass ball on the counter (new)           refraction + reflection
//    - the drinking glasses (left table, counter)  refraction through glass and water
//    - the glass water jug on the counter          refraction through glass and water
//    - a mirror in the upper tatami room (new)     reflection of the room
//  Everything around them that the rays can hit (counter, kitchen, hood, stools, tables,
//  noren, lamps, walls, floors; in the upper room the tatami, walls, tea table, chest,
//  futons, bed and lamp) is built from the same positions and sizes as the OpenGL models.
//  The outdoor scene is not included.
// ════════════════════════════════════════════════════════════════════════════

enum RtObject {
    RTO_POT_L, RTO_POT_R,
    RTO_BOWL_0, RTO_BOWL_1, RTO_BOWL_2, RTO_BOWL_3,
    RTO_GLASS_BALL, RTO_MIRROR,
    RTO_TABLE_GLASS,
    RTO_COUNTER_GLASS_0, RTO_COUNTER_GLASS_1, RTO_COUNTER_GLASS_2, RTO_COUNTER_GLASS_3,
    RTO_JUG,
    RTO_COUNT
};

struct RtObjectBounds { bool traced; rt::V3 bmin, bmax; };

// Once, after initTextures(): CPU copies of the textures the ray tracer samples
void rtLoadTextures();

// The ray-traced scene of this frame (without lights) and the world bounds of each featured
// object.  An object turned off its upright axis with the object keys is left rasterized.
void rtBuildShopScene(rt::Scene& s, RtObjectBounds bounds[RTO_COUNT]);

// New objects, drawn by OpenGL at the same places
void drawCounterGlassBall();   // glass ball on a small wooden stand (transparent pass)
void drawRoomMirror();         // framed mirror on the back wall of the upper room
