#include "shadowmap.h"
#include "scene.h"
#include "shader.h"
#include "lighting.h"
#include <cstdio>
#include <cstring>

bool useShadowMapping = true;

static const int   SHADOW_MAP_SIZE = 512;     // 1024 gives sharper shadows if the GPU has time to spare
static const float SHADOW_FOV      = 120.0f;  // the light looks straight down; 120 deg covers the dining area
static const float SHADOW_NEAR     = 0.3f;
static const float SHADOW_FAR      = 6.0f;

static GLuint g_fbo      = 0;
static GLuint g_depthTex = 0;
static bool   g_ready    = false;             // framebuffer created and complete
static bool   g_active   = false;             // shadow map rendered for the current frame
static float  g_eyeToShadow[16];

// Column-major 4x4 product C = A * B
static void mul4(const float* A, const float* B, float* C)
{
    float R[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float v = 0.0f;
            for (int k = 0; k < 4; k++) v += A[k * 4 + r] * B[c * 4 + k];
            R[c * 4 + r] = v;
        }
    memcpy(C, R, sizeof(R));
}

// Inverse of a camera matrix (rotation + translation, as made by gluLookAt)
static void invertRigid(const float* M, float* I)
{
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++) I[c * 4 + r] = M[r * 4 + c];       // transpose of the rotation
    for (int r = 0; r < 3; r++)
        I[12 + r] = -(I[r] * M[12] + I[4 + r] * M[13] + I[8 + r] * M[14]);
    I[3] = I[7] = I[11] = 0.0f;
    I[15] = 1.0f;
}

void initShadowMap()
{
    glGenTextures(1, &g_depthTex);
    glBindTexture(GL_TEXTURE_2D, g_depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);     // linear + compare: 2x2 hardware PCF per tap
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };                 // outside the map: lit
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_depthTex, 0);
    glDrawBuffer(GL_NONE);                                              // depth only
    glReadBuffer(GL_NONE);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    g_ready = (status == GL_FRAMEBUFFER_COMPLETE);
    if (g_ready)
        fprintf(stderr, "[Shadow] %dx%d shadow map with PCF ready (dining pendant light).  Press Y to toggle.\n",
                SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    else
        fprintf(stderr, "[Shadow] Shadow-map framebuffer incomplete (0x%x): using the projected shadows instead.\n", status);
}

void renderShadowMap()
{
    g_active = false;
    if (!g_ready || !useShadowMapping || !usePhongShading) return;      // Gouraud mode keeps the projected shadows
    if (!(lightPoint && fixtureOn[FX_PENDANTS])) return;                 // pendants off: nothing to shadow

    // Pendant position: GL_LIGHT1 as just placed by placeLightsInWorldSpace() (eye space) -> world space
    float V[16], IV[16], pe[4];
    glGetFloatv(GL_MODELVIEW_MATRIX, V);
    invertRigid(V, IV);
    glGetLightfv(GL_LIGHT1, GL_POSITION, pe);
    if (pe[3] == 0.0f) return;
    float L[3];
    for (int r = 0; r < 3; r++)
        L[r] = (IV[r] * pe[0] + IV[4 + r] * pe[1] + IV[8 + r] * pe[2] + IV[12 + r] * pe[3]) / pe[3];

    glPushAttrib(GL_ALL_ATTRIB_BITS);                  // viewport, masks, polygon offset, enables, material
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(SHADOW_FOV, 1.0, SHADOW_NEAR, SHADOW_FAR);
    float P[16];
    glGetFloatv(GL_PROJECTION_MATRIX, P);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(L[0], L[1], L[2], L[0], L[1] - 1.0f, L[2], 0.0, 0.0, -1.0);
    float LV[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, LV);

    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);                       // depth bias against shadow acne

    drawingShadow = true;                              // skips sphere-map and glass passes (as the projected shadows do)
    drawShadowMapCasters();
    drawingShadow = false;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();

    // camera eye space -> shadow texture space:  Bias * P_light * View_light * inverse(View_camera)
    static const float BIAS[16] = { 0.5f, 0, 0, 0,   0, 0.5f, 0, 0,   0, 0, 0.5f, 0,   0.5f, 0.5f, 0.5f, 1.0f };
    float T[16];
    mul4(BIAS, P, T);
    mul4(T, LV, T);
    mul4(T, IV, g_eyeToShadow);
    g_active = true;
}

bool shadowMapActive()          { return g_active && useShadowMapping && usePhongShading; }
GLuint shadowMapTexture()       { return g_depthTex; }
const float* shadowMapMatrix()  { return g_eyeToShadow; }
float shadowMapTexel()          { return 1.0f / SHADOW_MAP_SIZE; }

void toggleShadowMapping()
{
    useShadowMapping = !useShadowMapping;
    if (useShadowMapping && !g_ready)
        fprintf(stderr, "[Shadow] Shadow mapping is not available; the projected shadows stay.\n");
}

const char* getShadowStatus()
{
    if (!g_ready)           return "n/a";
    if (!useShadowMapping)  return "off";
    if (!usePhongShading)   return "needs Phong";
    return "PCF";
}
