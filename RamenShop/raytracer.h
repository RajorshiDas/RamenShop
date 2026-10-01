#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>

// ─── Ray Tracing Configuration & State ─────────────────────────────────────
extern bool useRayTracing;         // Main toggle: true = Ray Traced, false = Rasterized
extern int  rayTraceBounces;        // Max reflection/refraction bounces (1 to 5)
extern bool rayTraceSoftShadows;    // Soft vs hard ray-traced shadows
extern bool rayTraceAO;             // Contact ambient occlusion

// ─── Public API ────────────────────────────────────────────────────────────
// Initialise GLSL Ray Tracing program. Call once after glewInit().
void initRayTracer();

// Render the entire Ramen Shop using real-time Ray Tracing.
// Takes current viewport width and height.
void renderRayTracedFrame(int width, int height);

// User toggle handlers
void toggleRayTracing();
void cycleRayBounces();
void toggleRayShadows();
void toggleRayAO();

// Status string formatted for window title or HUD
const char* getRayTracingStatusString();
