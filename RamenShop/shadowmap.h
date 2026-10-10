#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>

// ════════════════════════════════════════════════════════════════════════════
//  SOFT SHADOWS - shadow mapping with PCF  (Y toggles)
//
//  One shadow-casting light: the dining pendants above the counter (GL_LIGHT1).
//  Every frame the counter, stools, tables, chairs and ramen bowls are drawn from that
//  light into a 512 x 512 depth texture.  The Phong shader (shader.cpp) compares each
//  pixel with it using 3 x 3 percentage-closer filtering, so light 1 fades out softly
//  behind those objects.  This is a rasterization technique (a depth comparison), not
//  ray tracing.
//
//  Fallback: in Gouraud mode (G), with the pendants switched off, or if the depth
//  framebuffer cannot be created, the original projected floor shadows are drawn.
// ════════════════════════════════════════════════════════════════════════════

extern bool useShadowMapping;      // Y : soft shadow mapping on / off

void initShadowMap();              // once, after glewInit()
void renderShadowMap();            // every frame, after the camera and the lights are placed
bool shadowMapActive();            // true when this frame's Phong pass uses the shadow map

// For the Phong shader
GLuint       shadowMapTexture();   // depth texture (compare mode on, for sampler2DShadow)
const float* shadowMapMatrix();    // camera eye space -> shadow-map texture space
float        shadowMapTexel();     // 1 / shadow-map size

void        toggleShadowMapping();
const char* getShadowStatus();     // short text for the window title
