#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>

// Phong shading toggle (press 'G' to switch Gouraud ↔ Phong)
extern bool usePhongShading;

// Initialise the Phong GLSL program.  Call once after glewInit().
void initPhongShader();

// Activate / deactivate the Phong shader for scene rendering.
void enablePhongShader();
void disablePhongShader();

// Push the current light-enable state into shader uniforms.
// Call once per frame after applyLightingParameters().
void updatePhongUniforms();

// Notify the shader that a texture is / is not bound (GL_TEXTURE_2D).
void shaderSetTexture(bool on);

// Wrapper: replaces every glEnable/Disable(GL_LIGHTING) in the codebase
// so the shader's lightingOn uniform stays in sync.
void setLighting(bool on);

// While true, only interior lights (and sun/moon) light the scene: entrance lanterns, street lamps
// and the shop-sign light are skipped (call false to restore).
void setInteriorLightScope(bool interior);
