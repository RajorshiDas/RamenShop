#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>

// ════════════════════════════════════════════════════════════════════════════
//  RAY TRACING PASS  (R toggles; off at start)
//
//  Real ray tracing on the CPU (rtcore.cpp) for a few objects, from any camera:
//  the steel pots, the ramen bowls (glaze and soup), the glass ball on the counter and
//  the mirror in the upper room (rtscene.cpp).  Each frame the screen rectangle of every
//  visible one is traced on all CPU threads, then the result is composited over the
//  OpenGL frame where its depth buffer shows that same surface.  The rest of the shop
//  stays rasterized; with R off nothing here runs.
// ════════════════════════════════════════════════════════════════════════════

extern bool useRayTracing;           // R

void initRayTracing();               // once, after initTextures()
bool rayTracingActive();             // toggle on and ready
void renderRayTracing();             // after the whole scene (modelview = camera view), before the HUD

void        toggleRayTracing();
const char* getRayTracingStatus();   // short text for the window title
