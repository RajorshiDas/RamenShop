#pragma once

#include "shapes.h"

// ════════════════════════════════════════════════════════════════════════════
//  GLASS REFRACTION  (Z toggles) - one empty drinking tumbler on the left customer table
//
//  SCREEN-SPACE APPROXIMATION, not ray tracing: after the whole scene is drawn, the part of
//  the frame behind the tumbler is copied into a texture.  The tumbler is then drawn with a
//  small GLSL 1.20 shader that reads that texture at an offset given by refract() with the
//  glass normal, adds a Fresnel reflection and the lamp highlights, and writes the result.
//  Back faces (far wall) are drawn first, then the frame is copied again and the front faces
//  (near wall) refract that.  Only what is already on screen can be seen through the glass,
//  and objects in front of the glass can bleed into the distortion at its edges.
//
//  Fallback: with Z off, or if the shader does not compile, the scene draws the project's
//  ordinary alpha-blended drawClearGlassTumbler() at the same place.
// ════════════════════════════════════════════════════════════════════════════

extern bool useGlassRefraction;          // Z : glass refraction on / off

extern const Vec3  TABLE_TUMBLER_POS;    // base centre of the tumbler (on the left customer table)
extern const float TABLE_TUMBLER_SCALE;  // same model units as drawClearGlassTumbler()

void initGlassRefraction();              // once, after glewInit()
bool glassRefractionActive();            // toggle on and shader ready
void drawRefractiveTumbler();            // after the whole scene (modelview = camera view), before the HUD

void        toggleGlassRefraction();
const char* getGlassStatus();            // short text for the window title
