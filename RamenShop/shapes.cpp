#include "shapes.h"
#include "shader.h"

// Define colors
const Color SKY          = { 0.10f, 0.10f, 0.16f };   // dark evening sky
const Color GRASS        = { 0.40f, 0.62f, 0.33f };
const Color ASPHALT      = { 0.25f, 0.25f, 0.27f };
const Color SIDEWALK     = { 0.72f, 0.72f, 0.70f };
const Color GRAY         = { 0.50f, 0.50f, 0.50f };
const Color DARK_GRAY    = { 0.22f, 0.22f, 0.24f };
const Color WHITE        = { 1.00f, 1.00f, 1.00f };
const Color BLACK        = { 0.08f, 0.08f, 0.08f };
const Color WALL         = { 0.93f, 0.88f, 0.76f };   // cream plaster
const Color DARK_WOOD    = { 0.35f, 0.20f, 0.10f };
const Color WOOD         = { 0.60f, 0.40f, 0.22f };
const Color LIGHT_WOOD   = { 0.82f, 0.64f, 0.42f };
const Color FLOOR_WOOD   = { 0.55f, 0.42f, 0.30f };
const Color ROOF_TILE    = { 0.27f, 0.31f, 0.38f };
const Color RED          = { 0.80f, 0.10f, 0.10f };
const Color DARK_RED     = { 0.50f, 0.05f, 0.05f };
const Color NAVY         = { 0.10f, 0.15f, 0.35f };
const Color GOLD         = { 0.95f, 0.80f, 0.25f };
const Color PAPER        = { 0.97f, 0.95f, 0.88f };
const Color METAL        = { 0.45f, 0.45f, 0.50f };
const Color STEEL        = { 0.78f, 0.78f, 0.80f };
const Color CUSHION      = { 0.70f, 0.12f, 0.12f };
const Color CLAY_POT     = { 0.72f, 0.40f, 0.25f };
const Color SOIL         = { 0.30f, 0.20f, 0.10f };
const Color LEAF         = { 0.20f, 0.55f, 0.22f };
const Color LEAF_LIGHT   = { 0.35f, 0.68f, 0.30f };
const Color BOWL_RED     = { 0.78f, 0.18f, 0.14f };
const Color PLATE_WHITE  = { 0.92f, 0.94f, 0.97f };
const Color CUP_GREEN    = { 0.55f, 0.70f, 0.55f };
const Color SOY          = { 0.28f, 0.12f, 0.06f };
const Color BROTH        = { 0.87f, 0.62f, 0.32f };
const Color NOODLE       = { 0.98f, 0.88f, 0.50f };
const Color EGG_WHITE    = { 1.00f, 0.99f, 0.94f };
const Color YOLK         = { 1.00f, 0.60f, 0.10f };
const Color MEAT         = { 0.80f, 0.52f, 0.42f };
const Color MEAT_EDGE    = { 0.55f, 0.30f, 0.20f };
const Color NORI         = { 0.08f, 0.18f, 0.10f };
const Color ONION_GREEN  = { 0.40f, 0.80f, 0.30f };
const Color MOUNTAIN     = { 0.35f, 0.45f, 0.65f };
const Color TILE_FLOOR   = { 0.55f, 0.53f, 0.50f };   // gray tile floor
const Color UPPER_WALL   = { 0.15f, 0.18f, 0.28f };   // dark navy upper facade
const Color LANTERN_CREAM = { 0.95f, 0.90f, 0.75f };  // cream paper lanterns
const Color LANTERN_ORANGE = { 0.95f, 0.55f, 0.15f }; // orange cylinder lanterns
const Color FENCE_WOOD   = { 0.28f, 0.16f, 0.08f };   // dark fence pickets
const Color TRUNK        = { 0.30f, 0.22f, 0.12f };   // tree trunk
const Color AWNING_BLUE  = { 0.18f, 0.22f, 0.35f };   // zigzag awning
const Color CHROME       = { 0.88f, 0.90f, 0.94f };   // polished chrome
const Color CLEAR_GLASS  = { 0.90f, 0.95f, 1.00f };   // clear glass tint
const Color GLASS_WATER  = { 0.65f, 0.84f, 0.96f };   // drinking water

// Global variables definition
GLUquadric* quad = nullptr;
bool showOutlines = false;
float animTime = 0.0f;
bool drawingShadow = false;

void applyTransform(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(rot.x, 1, 0, 0);    // tilt forward/back
    glRotatef(rot.y, 0, 1, 0);    // turn left/right
    glRotatef(rot.z, 0, 0, 1);    // roll sideways
    glScalef(scale.x, scale.y, scale.z);
}

void setColor(Color c)
{
    glColor3f(c.r, c.g, c.b);
}

Color darker(Color c)
{
    return { c.r * 0.55f, c.g * 0.55f, c.b * 0.55f };
}

float emissionScale = 1.0f;   // 0 = every glow is off (used when no interior light is on)

void setEmission(float r, float g, float b)
{
    GLfloat em[] = { r * emissionScale, g * emissionScale, b * emissionScale, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, em);
}

void clearEmission()
{
    setEmission(0, 0, 0);
}

void setMaterialGloss(float r, float g, float b, float shininess)
{
    GLfloat spec[] = { r, g, b, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void resetMaterialGloss()
{
    // Default surface: a soft satin sheen, so every object picks up a light highlight and
    // shaded falloff from the lamps and the sun, instead of looking flat.
    GLfloat softSpec[] = { 0.14f, 0.14f, 0.14f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, softSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 24.0f);
}

// Dielectric: white specular — the highlight colour is always white,
// independent of the surface colour (ceramic, wood, plastic, fabric …).
void setMaterialDielectric(float shininess)
{
    setMaterialGloss(1.0f, 1.0f, 1.0f, shininess);
}

// Conductive: tinted specular — the highlight colour is tinted by the
// surface colour (metals reflect light in their own hue).
void setMaterialConductive(Color c, float shininess)
{
    // Blend surface colour toward white: bright metals are near-white,
    // coloured metals retain a strong hue tint.
    const float t = 0.65f;   // how strongly surface colour tints the specular
    setMaterialGloss(c.r * t + (1.0f - t),
                     c.g * t + (1.0f - t),
                     c.b * t + (1.0f - t),
                     shininess);
}

// ========== PBR MATERIAL SYSTEM ==========
// Convert roughness (0=smooth, 1=matte) to an OpenGL shininess exponent.
// Smooth surfaces get a tight highlight, rough ones a broad soft one.  The old
// curve gave ~120 for everything below roughness 0.5, i.e. pin-point glints that
// per-vertex lighting could not show.
static float roughnessToShininess(float roughness)
{
    float k = 1.0f - roughness;
    return 4.0f + 124.0f * k * k;          // r=0.1 -> 105, 0.3 -> 65, 0.6 -> 24, 0.9 -> 5
}

// Apply PBR material properties to currently set color
void setMaterialPBR(const MaterialPBR& mat, const Color& surfaceColor)
{
    // Non-metallic dielectric (ceramic, wood, plastic, fabric ...): white specular.
    // Glossier (low roughness) surfaces reflect noticeably more of the light.
    float shininess = roughnessToShininess(mat.roughness);
    float specIntensity = 0.08f + (1.0f - mat.roughness) * 0.55f + mat.metallic * 0.30f;
    if (specIntensity > 1.0f) specIntensity = 1.0f;

    setMaterialGloss(specIntensity, specIntensity, specIntensity, shininess);
}

// Apply metallic PBR material with color tinting
void setMaterialPBRMetallic(const MaterialPBR& mat, const Color& metalColor)
{
    // Metals reflect strongly and tint the highlight with their own colour
    float shininess = roughnessToShininess(mat.roughness);
    float specIntensity = 0.45f + (1.0f - mat.roughness) * 0.50f;
    if (specIntensity > 1.0f) specIntensity = 1.0f;

    setMaterialGloss(
        metalColor.r * specIntensity,
        metalColor.g * specIntensity,
        metalColor.b * specIntensity,
        shininess
    );
}

// Sphere-map environment reflection — adds a warm interior glow on top of the
// Phong-lit metal surface, approximating ray-traced reflections.
static bool g_sphereSkipped = false;

void beginSphereReflect()
{
    // The fake "warm interior" reflection is only light that comes from the lamps: with every
    // interior light off (emissionScale == 0) there is nothing to reflect, so skip it.
    g_sphereSkipped = (drawingShadow || emissionScale <= 0.0f);
    if (g_sphereSkipped) return;
    disablePhongShader();        // sphere-map texgen needs fixed-function
    GLuint env = getTexID(TEX_ENV_MAP);
    if (!env) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, env);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
}

void endSphereReflect()
{
    if (g_sphereSkipped) { g_sphereSkipped = false; return; }
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_TEXTURE_2D);
    enablePhongShader();         // restore Phong after sphere-map pass
}

// Internal raw GLU helpers
static void rawTube(float bottomRadius, float topRadius, float height, float y0 = 0)
{
    glPushMatrix();
    glTranslatef(0, y0, 0);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(quad, bottomRadius, topRadius, height, SLICES, 1);
    glPopMatrix();
}

static void rawDisk(float radius, float y)
{
    glPushMatrix();
    glTranslatef(0, y, 0);
    glRotatef(-90, 1, 0, 0);
    gluDisk(quad, 0, radius, SLICES, 1);
    glPopMatrix();
}

static void rawCircleOutline(float radius, float y, Color c)
{
    if (!showOutlines || radius <= 0) return;
    setLighting(false);
    setColor(darker(c));
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < SLICES; i++) {
        float a = 2 * PI * i / SLICES;
        glVertex3f(radius * cos(a), y, radius * sin(a));
    }
    glEnd();
    setLighting(true);
    setColor(c);
}

// --- Glass reflection pass ---------------------------------------------------
// Clear glass is drawn at very low alpha (14-28%).  Ordinary Phong highlights are
// added to the surface colour and then scaled by that alpha, so they all but
// vanish.  This pass re-draws the same shell ADDITIVELY (alpha = 1) so that
//   1) light glints : black diffuse + white specular, only the Phong highlight
//                     of every light (pendants, spot, lanterns, sun/moon) stays
//   2) environment  : sphere-mapped warm-room reflection, scaled by `env`
// are added on top of whatever is seen through the glass, the way real glass
// reflects light.  Call it with blending on and depth writes off, before the
// caller restores state.  `shell` must only issue geometry (no colour calls).
template <typename Shell>
static void glassReflections(float spec, float shininess, float env, Shell shell)
{
    if (drawingShadow) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    // 1) Light glints: a tight bright core plus a broad soft sheen
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    setMaterialGloss(spec, spec, spec, shininess);
    shell();
    setMaterialGloss(spec * 0.35f, spec * 0.35f, spec * 0.35f, shininess * 0.18f);
    shell();
    resetMaterialGloss();

    // 2) Environment reflection (fixed-function sphere map, unlit, scaled)
    if (env > 0.0f && getTexID(TEX_ENV_MAP)) {
        beginSphereReflect();
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDisable(GL_LIGHTING);
        glColor4f(env, env, env, 1.0f);
        shell();
        glEnable(GL_LIGHTING);
        endSphereReflect();          // restores the Phong program
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Flat glass pane made of a grid of quads (so per-vertex lighting can still
// show a highlight in the middle of the pane when the Phong shader is off).
static void glassPane(float x0, float y0, float x1, float y1, float z, float nz,
                      int nx = 14, int ny = 8)
{
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, nz);
    for (int i = 0; i < nx; i++) {
        float xa = x0 + (x1 - x0) * i / nx, xb = x0 + (x1 - x0) * (i + 1) / nx;
        for (int j = 0; j < ny; j++) {
            float ya = y0 + (y1 - y0) * j / ny, yb = y0 + (y1 - y0) * (j + 1) / ny;
            if (nz > 0) {
                glVertex3f(xa, ya, z); glVertex3f(xb, ya, z);
                glVertex3f(xb, yb, z); glVertex3f(xa, yb, z);
            } else {
                glVertex3f(xb, ya, z); glVertex3f(xa, ya, z);
                glVertex3f(xa, yb, z); glVertex3f(xb, yb, z);
            }
        }
    }
    glEnd();
}

// Soft diagonal light streaks across a big pane (window / sneeze guard)
static void glassStreaks(float halfW, float yBot, float yTop, float z)
{
    if (drawingShadow) return;
    setLighting(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    float h = yTop - yBot;
    const float streaks[3][2] = { { -0.55f, 0.14f }, { -0.15f, 0.07f }, { 0.35f, 0.10f } };
    for (int k = 0; k < 3; k++) {
        float cx = streaks[k][0] * halfW, w = streaks[k][1] * halfW, slant = 0.30f * h;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= 2; i++) {
            float t = i * 0.5f;                       // 0, 0.5, 1 up the pane
            float a = (i == 1) ? 0.16f : 0.0f;        // brightest mid-pane
            float x = cx + slant * (t - 0.5f);
            glColor4f(1.0f, 0.98f, 0.92f, a);
            glVertex3f(x - w, yBot + h * t, z);
            glVertex3f(x + w, yBot + h * t, z);
        }
        glEnd();
    }
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setLighting(true);
}

void drawSteam(Vec3 pos, float scale, float timeOffset)
{
    if (!quad) return;

    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    setLighting(false);

    const int PUFFS = 7;
    for (int i = 0; i < PUFFS; i++) {
        float phase = fmod(animTime * 0.65f + timeOffset + (float)i / PUFFS, 1.0f);
        float y = phase * 1.5f * scale;
        float driftX = sin(phase * 4.5f + timeOffset + (float)i) * 0.14f * scale * (0.4f + phase);
        float driftZ = cos(phase * 3.8f + timeOffset * 1.2f + (float)i) * 0.11f * scale * (0.4f + phase);
        float r = (0.09f + 0.32f * phase) * scale;
        float alpha = sin(phase * PI) * 0.28f;

        glColor4f(0.96f, 0.94f, 0.90f, alpha);

        glPushMatrix();
        glTranslatef(driftX, y, driftZ);
        gluSphere(quad, r, 10, 8);
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    setLighting(true);
    glDisable(GL_BLEND);

    glPopMatrix();
}

void drawCube(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    glutSolidCube(1.0);
    if (showOutlines) {
        setLighting(false);
        setColor(darker(c));
        glutWireCube(1.0);
        setLighting(true);
    }
    glPopMatrix();
}

void drawCuboid(Vec3 pos, Vec3 rot, Vec3 size, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, ONE);
    drawCube({ 0, size.y / 2, 0 }, NO_ROT, size, c);
    glPopMatrix();
}

void drawCylinderCustom(Vec3 pos, Vec3 rot, Vec3 scale, Color c,
                        float bottomRadius, float topRadius, float height)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    rawTube(bottomRadius, topRadius, height);
    rawDisk(bottomRadius, 0);
    rawDisk(topRadius, height);
    rawCircleOutline(bottomRadius, 0, c);
    rawCircleOutline(topRadius, height, c);
    glPopMatrix();
}

void drawCylinder(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    drawCylinderCustom(pos, rot, scale, c, 0.5f, 0.5f, 1.0f);
}

void drawCone(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    drawCylinderCustom(pos, rot, scale, c, 0.5f, 0.0f, 1.0f);
}

void drawSphere(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    gluSphere(quad, 0.5, SLICES, STACKS);
    glPopMatrix();
}

void drawTorus(Vec3 pos, Vec3 rot, Vec3 scale, Color c, float tubeRadius, float ringRadius)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    glRotatef(90, 1, 0, 0);
    glutSolidTorus(tubeRadius, ringRadius, 12, SLICES);
    glPopMatrix();
}

void drawPlane(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-0.5f, 0, -0.5f);
    glVertex3f( 0.5f, 0, -0.5f);
    glVertex3f( 0.5f, 0,  0.5f);
    glVertex3f(-0.5f, 0,  0.5f);
    glEnd();
    glPopMatrix();
}

void drawSubdividedPlane(Vec3 pos, Vec3 rot, Vec3 scale, Color c, int gridX, int gridZ)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    float stepX = 1.0f / gridX;
    float stepZ = 1.0f / gridZ;
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    for (int i = 0; i < gridX; i++) {
        float x0 = -0.5f + i * stepX;
        float x1 = x0 + stepX;
        for (int j = 0; j < gridZ; j++) {
            float z0 = -0.5f + j * stepZ;
            float z1 = z0 + stepZ;
            glVertex3f(x0, 0, z0);
            glVertex3f(x1, 0, z0);
            glVertex3f(x1, 0, z1);
            glVertex3f(x0, 0, z1);
        }
    }
    glEnd();
    glPopMatrix();
}

void drawBoard(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    drawCube(pos, rot, { scale.x, scale.y * 0.05f, scale.z }, c);
}

void drawWedge(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    glBegin(GL_TRIANGLES);
    glNormal3f(-1, 0, 0);
    glVertex3f(-0.5f, 0, -0.5f); glVertex3f(-0.5f, 0, 0.5f); glVertex3f(-0.5f, 1, 0);
    glNormal3f( 1, 0, 0);
    glVertex3f( 0.5f, 0, -0.5f); glVertex3f( 0.5f, 0, 0.5f); glVertex3f( 0.5f, 1, 0);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(0, -1, 0);
    glVertex3f(-0.5f, 0, -0.5f); glVertex3f(0.5f, 0, -0.5f); glVertex3f(0.5f, 0, 0.5f); glVertex3f(-0.5f, 0, 0.5f);
    glNormal3f(0, 1, 2);
    glVertex3f(-0.5f, 0,  0.5f); glVertex3f(0.5f, 0,  0.5f); glVertex3f(0.5f, 1, 0);    glVertex3f(-0.5f, 1, 0);
    glNormal3f(0, 1, -2);
    glVertex3f(-0.5f, 0, -0.5f); glVertex3f(-0.5f, 1, 0);    glVertex3f(0.5f, 1, 0);    glVertex3f(0.5f, 0, -0.5f);
    glEnd();
    glPopMatrix();
}

void drawBowl(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    rawTube(0.22f, 0.22f, 0.06f);
    rawTube(0.25f, 0.50f, 0.34f, 0.06f);
    rawDisk(0.25f, 0.06f);
    glPushMatrix();
    glTranslatef(0, 0.4f, 0);
    glRotatef(90, 1, 0, 0);
    glutSolidTorus(0.015, 0.5, 8, SLICES);
    glPopMatrix();
    rawCircleOutline(0.5f, 0.4f, c);
    rawCircleOutline(0.25f, 0.06f, c);
    glPopMatrix();
}

void drawPlate(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    rawTube(0.30f, 0.50f, 0.08f);
    rawDisk(0.30f, 0.005f);
    rawCircleOutline(0.5f, 0.08f, c);
    rawCircleOutline(0.3f, 0.005f, c);
    glPopMatrix();
}

void drawCup(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setColor(c);
    rawTube(0.30f, 0.35f, 1.0f);
    rawDisk(0.30f, 0.0f);
    glPushMatrix();
    glTranslatef(0.42f, 0.5f, 0);
    glutSolidTorus(0.05, 0.18, 8, 16);
    glPopMatrix();
    rawCircleOutline(0.35f, 1.0f, c);
    glPopMatrix();
}

void drawBottle(Vec3 pos, Vec3 rot, Vec3 scale, Color c, Color capColor)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setMaterialGloss(0.95f, 0.95f, 0.95f, 120.0f);      // glossy glass bottle
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.4f, 0.55f, 0.4f }, c);
    drawCylinderCustom({ 0, 0.55f, 0 }, NO_ROT, ONE, c, 0.2f, 0.07f, 0.15f);
    drawCylinder({ 0, 0.70f, 0 }, NO_ROT, { 0.14f, 0.2f, 0.14f }, c);
    setMaterialGloss(0.60f, 0.60f, 0.60f, 70.0f);       // plastic/metal cap
    drawCylinder({ 0, 0.90f, 0 }, NO_ROT, { 0.17f, 0.1f, 0.17f }, capColor);
    resetMaterialGloss();
    glPopMatrix();
}

void drawGlass(Vec3 pos, Vec3 rot, Vec3 scale, Color c)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    setMaterialDielectric(115.0f);     // sharp white specular highlight on clear glass

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glColor4f(c.r, c.g, c.b, 0.18f); // 18% opaque — pristine clear glass
    rawTube(0.30f, 0.33f, 1.0f);     // body
    rawDisk(0.30f, 0.0f);            // bottom
    rawCircleOutline(0.33f, 1.0f, c); // rim

    glassReflections(1.0f, 150.0f, 0.30f, [] {
        rawTube(0.30f, 0.33f, 1.0f);
        rawDisk(0.30f, 0.0f);
    });

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

    glPopMatrix();
}

// ─── Realistic Clear Glass Components ───────────────────────────────────────

// Realistic Clear Glass Tumbler: heavy solid glass base, cylindrical body,
// inner cavity, optional clear water with meniscus and floating faceted ice cubes.
void drawClearGlassTumbler(Vec3 pos, Vec3 rot, Vec3 scale, bool hasWater, bool hasIce)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // 1. Water contents (drawn before outer glass for correct alpha blending)
    if (hasWater) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        setMaterialGloss(0.85f, 0.95f, 1.0f, 95.0f);
        glColor4f(GLASS_WATER.r, GLASS_WATER.g, GLASS_WATER.b, 0.35f);

        // Water column inside cavity
        rawTube(0.24f, 0.26f, 0.65f, 0.08f);
        rawDisk(0.24f, 0.08f);
        // Water surface meniscus
        rawDisk(0.26f, 0.73f);

        // Floating Ice Cubes inside the water
        if (hasIce) {
            setMaterialDielectric(100.0f);
            glColor4f(0.92f, 0.96f, 1.0f, 0.42f);

            // Ice Cube 1
            glPushMatrix();
            glTranslatef(-0.06f, 0.60f, -0.04f);
            glRotatef(22.0f, 0, 1, 0);
            glRotatef(15.0f, 1, 0, 0);
            glutSolidCube(0.14);
            glPopMatrix();

            // Ice Cube 2
            glPushMatrix();
            glTranslatef(0.07f, 0.63f, 0.05f);
            glRotatef(-35.0f, 0, 1, 0);
            glRotatef(-12.0f, 0, 0, 1);
            glutSolidCube(0.13);
            glPopMatrix();
        }

        // Bright glint on the water surface
        glassReflections(1.0f, 120.0f, 0.25f, [] { rawDisk(0.26f, 0.73f); });

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        resetMaterialGloss();
    }

    // 2. Clear Glass Body (high refractive/specular dielectric glass)
    setMaterialDielectric(125.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Heavy solid glass base (sham) - slightly more visible clear glass thickness
    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.28f);
    rawTube(0.28f, 0.285f, 0.08f, 0.0f);
    rawDisk(0.28f, 0.0f);

    // Ultra-clear glass wall (16% opacity)
    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.16f);
    rawTube(0.285f, 0.32f, 0.85f, 0.08f);   // outer wall
    rawTube(0.24f,  0.275f, 0.85f, 0.08f);  // inner wall cavity

    // Rounded polished glass rim
    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.30f);
    glPushMatrix();
    glTranslatef(0, 0.93f, 0);
    glRotatef(90, 1, 0, 0);
    glutSolidTorus(0.02, 0.295, 12, SLICES);
    glPopMatrix();

    rawCircleOutline(0.32f, 0.93f, CLEAR_GLASS);

    // Light glints + reflection on the glass walls, base and polished rim
    glassReflections(1.0f, 150.0f, 0.40f, [] {
        rawTube(0.28f, 0.285f, 0.08f, 0.0f);
        rawDisk(0.28f, 0.0f);
        rawTube(0.285f, 0.32f, 0.85f, 0.08f);
        rawTube(0.24f, 0.275f, 0.85f, 0.08f);
        glPushMatrix();
        glTranslatef(0, 0.93f, 0);
        glRotatef(90, 1, 0, 0);
        glutSolidTorus(0.02, 0.295, 12, SLICES);
        glPopMatrix();
    });

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

    glPopMatrix();
}

// Clear Glass Water Carafe / Pitcher: bulbous bottom, flared neck, clear handle,
// with clear water and fresh lemon slice inside.
void drawClearGlassPitcher(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Lemon slice inside carafe
    glPushMatrix();
    glTranslatef(0.0f, 0.40f, 0.0f);
    glRotatef(45.0f, 1, 0, 1);
    setColor({ 0.95f, 0.88f, 0.15f });   // lemon yellow
    rawDisk(0.20f, 0.0f);
    setColor(WHITE);
    rawDisk(0.05f, 0.001f);
    glPopMatrix();

    // Water level inside
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    setMaterialGloss(0.85f, 0.95f, 1.0f, 90.0f);
    glColor4f(GLASS_WATER.r, GLASS_WATER.g, GLASS_WATER.b, 0.30f);
    rawTube(0.32f, 0.22f, 0.65f, 0.05f);
    rawDisk(0.32f, 0.05f);
    rawDisk(0.22f, 0.70f);

    // Clear glass exterior
    setMaterialDielectric(120.0f);
    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.18f);

    // Thick glass base
    rawDisk(0.35f, 0.0f);
    rawTube(0.35f, 0.36f, 0.06f, 0.0f);

    // Lower bulbous clear glass body
    rawTube(0.36f, 0.24f, 0.70f, 0.06f);

    // Tapered neck & flared pour spout
    rawTube(0.24f, 0.20f, 0.35f, 0.76f);
    rawTube(0.20f, 0.28f, 0.20f, 1.11f);
    rawCircleOutline(0.28f, 1.31f, CLEAR_GLASS);

    // Clear glass curved handle
    glPushMatrix();
    glTranslatef(0.28f, 0.75f, 0.0f);
    glRotatef(90.0f, 0, 0, 1);
    glutSolidTorus(0.035, 0.28, 12, SLICES);
    glPopMatrix();

    glassReflections(1.0f, 150.0f, 0.40f, [] {
        rawDisk(0.35f, 0.0f);
        rawTube(0.35f, 0.36f, 0.06f, 0.0f);
        rawTube(0.36f, 0.24f, 0.70f, 0.06f);
        rawTube(0.24f, 0.20f, 0.35f, 0.76f);
        rawTube(0.20f, 0.28f, 0.20f, 1.11f);
        rawDisk(0.22f, 0.70f);                 // water surface
        glPushMatrix();
        glTranslatef(0.28f, 0.75f, 0.0f);
        glRotatef(90.0f, 0, 0, 1);
        glutSolidTorus(0.035, 0.28, 12, SLICES);
        glPopMatrix();
    });

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

    glPopMatrix();
}

// Clear Glass Condiment / Spice Jar: transparent glass body showing colorful spices,
// topped with a brushed stainless steel or wooden shaker cap.
void drawClearGlassJar(Vec3 pos, Vec3 rot, Vec3 scale, Color contentColor)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Internal spice contents
    setColor(contentColor);
    rawTube(0.18f, 0.18f, 0.38f, 0.04f);
    rawDisk(0.18f, 0.04f);
    rawDisk(0.18f, 0.42f);

    // Clear Glass Outer Shell
    setMaterialDielectric(120.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.18f);

    rawDisk(0.22f, 0.0f);
    rawTube(0.22f, 0.22f, 0.50f, 0.0f);
    rawCircleOutline(0.22f, 0.50f, CLEAR_GLASS);

    glassReflections(1.0f, 150.0f, 0.40f, [] {
        rawDisk(0.22f, 0.0f);
        rawTube(0.22f, 0.22f, 0.50f, 0.0f);
    });

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

    // Brushed metal shaker cap
    setMaterialConductive(STEEL, 80.0f);
    beginSphereReflect();
    rawTube(0.23f, 0.21f, 0.12f, 0.50f);
    rawDisk(0.21f, 0.62f);
    endSphereReflect();
    resetMaterialGloss();

    // Shaker holes on top
    setLighting(false);
    setColor(BLACK);
    for (int i = 0; i < 5; i++) {
        float a = i * 2.0f * PI / 5.0f;
        rawDisk(0.02f, 0.622f);
    }
    setLighting(true);

    glPopMatrix();
}

// Clear Glass Counter Partition (Sneeze Guard):
// Tempered clear glass divider running along the counter, supported by polished chrome clamps.
void drawClearGlassPartition(Vec3 pos, Vec3 rot, Vec3 scale, float width, float height)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float halfW = width * 0.5f;
    float glassThick = 0.018f;

    // 1. Polished Chrome Standoff Clamps
    int numClamps = (int)ceilf(width / 1.4f) + 1;
    float step = width / (numClamps - 1);
    for (int i = 0; i < numClamps; i++) {
        float cx = -halfW + i * step;
        glPushMatrix();
        glTranslatef(cx, 0.0f, 0.0f);
        setMaterialConductive(CHROME, 95.0f);
        beginSphereReflect();
        // Mounting base block
        drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.08f, 0.10f, 0.06f }, CHROME);
        // Upright glass grip slot
        drawCuboid({ 0, 0.10f, -0.015f }, NO_ROT, { 0.06f, 0.08f, 0.012f }, CHROME);
        drawCuboid({ 0, 0.10f,  0.015f }, NO_ROT, { 0.06f, 0.08f, 0.012f }, CHROME);
        endSphereReflect();
        resetMaterialGloss();
        glPopMatrix();
    }

    // 2. Tempered Clear Glass Sheet
    setMaterialDielectric(128.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Front & Back large clear faces (ultra-transparent 14% opacity)
    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.14f);
    glBegin(GL_QUADS);
    // Front face (+Z)
    glNormal3f(0, 0, 1);
    glVertex3f(-halfW, 0.02f,  glassThick * 0.5f);
    glVertex3f( halfW, 0.02f,  glassThick * 0.5f);
    glVertex3f( halfW, height, glassThick * 0.5f);
    glVertex3f(-halfW, height, glassThick * 0.5f);
    // Back face (-Z)
    glNormal3f(0, 0, -1);
    glVertex3f( halfW, 0.02f, -glassThick * 0.5f);
    glVertex3f(-halfW, 0.02f, -glassThick * 0.5f);
    glVertex3f(-halfW, height, -glassThick * 0.5f);
    glVertex3f( halfW, height, -glassThick * 0.5f);
    glEnd();

    // Polished beveled edges (slightly higher opacity for realistic glass refraction look)
    glColor4f(0.75f, 0.90f, 0.88f, 0.35f);
    glBegin(GL_QUADS);
    // Top edge (+Y)
    glNormal3f(0, 1, 0);
    glVertex3f(-halfW, height, -glassThick * 0.5f);
    glVertex3f( halfW, height, -glassThick * 0.5f);
    glVertex3f( halfW, height,  glassThick * 0.5f);
    glVertex3f(-halfW, height,  glassThick * 0.5f);
    // Left edge (-X)
    glNormal3f(-1, 0, 0);
    glVertex3f(-halfW, 0.02f, -glassThick * 0.5f);
    glVertex3f(-halfW, 0.02f,  glassThick * 0.5f);
    glVertex3f(-halfW, height, glassThick * 0.5f);
    glVertex3f(-halfW, height, -glassThick * 0.5f);
    // Right edge (+X)
    glNormal3f(1, 0, 0);
    glVertex3f(halfW, 0.02f,  glassThick * 0.5f);
    glVertex3f(halfW, 0.02f, -glassThick * 0.5f);
    glVertex3f(halfW, height, -glassThick * 0.5f);
    glVertex3f(halfW, height,  glassThick * 0.5f);
    glEnd();

    // Reflections: per-light glints on both faces + soft light streaks
    glassReflections(0.9f, 140.0f, 0.10f, [&] {
        glassPane(-halfW, 0.02f, halfW, height,  glassThick * 0.5f,  1.0f, 24, 6);
        glassPane(-halfW, 0.02f, halfW, height, -glassThick * 0.5f, -1.0f, 24, 6);
    });
    glassStreaks(halfW, 0.02f, height, glassThick * 0.5f + 0.002f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

    glPopMatrix();
}

// Clear Glass Window / Sliding Door Pane:
// Realistic architectural glass with framing and clean specular response.
void drawClearGlassWindow(Vec3 pos, Vec3 rot, Vec3 scale, float width, float height)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float halfW = width * 0.5f;
    float halfH = height * 0.5f;

    // Clear glass sheet
    setMaterialDielectric(120.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.16f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-halfW, -halfH, 0);
    glVertex3f( halfW, -halfH, 0);
    glVertex3f( halfW,  halfH, 0);
    glVertex3f(-halfW,  halfH, 0);
    // Back face
    glNormal3f(0, 0, -1);
    glVertex3f( halfW, -halfH, 0);
    glVertex3f(-halfW, -halfH, 0);
    glVertex3f(-halfW,  halfH, 0);
    glVertex3f( halfW,  halfH, 0);
    glEnd();

    // Subtle glass bevel edge highlight
    glColor4f(0.80f, 0.95f, 1.0f, 0.32f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-halfW, -halfH, 0.002f);
    glVertex3f( halfW, -halfH, 0.002f);
    glVertex3f( halfW,  halfH, 0.002f);
    glVertex3f(-halfW,  halfH, 0.002f);
    glEnd();

    glassReflections(0.9f, 140.0f, 0.10f, [&] {
        glassPane(-halfW, -halfH, halfW, halfH, 0.0f,  1.0f, 16, 12);
        glassPane(-halfW, -halfH, halfW, halfH, 0.0f, -1.0f, 16, 12);
    });
    glassStreaks(halfW, -halfH, halfH, 0.004f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

    glPopMatrix();
}

// ─── Physical Light Fixtures ───────────────────────────────────────────────

// Ceiling Track Spotlight Fixture:
// Modern black aluminum track can with swivel mount, polished inner reflector cone,
// and glowing halogen/LED emitter.
void drawSpotlightFixture(Vec3 pos, Vec3 rot, Vec3 scale, bool isOn, bool whiteShade)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Thin hanging cord up to the ceiling
    drawCylinder({ 0, 0.0f, 0 }, NO_ROT, { 0.006f, 0.04f, 0.006f }, whiteShade ? Color{ 0.85f, 0.85f, 0.85f } : Color{ 0.04f, 0.04f, 0.04f });

    // Small wooden cap on top of the bell
    setMaterialPBR(Materials::WoodPolished, LIGHT_WOOD);
    drawCylinderCustom({ 0, -0.07f, 0 }, NO_ROT, ONE, LIGHT_WOOD, 0.060f, 0.045f, 0.075f);
    resetMaterialGloss();

    // Bell shade: lathe profile (neck, rounded shoulder, flared skirt, rolled lip)
    static const float prof[][2] = {      // { radius, y }
        { 0.062f, -0.105f }, { 0.062f, -0.175f }, { 0.072f, -0.205f }, { 0.098f, -0.235f },
        { 0.135f, -0.275f }, { 0.172f, -0.330f }, { 0.200f, -0.395f }, { 0.216f, -0.450f },
        { 0.220f, -0.485f }, { 0.215f, -0.500f }
    };
    const int N = (int)(sizeof(prof) / sizeof(prof[0])), SEG = 28;
    const Color shade = whiteShade ? Color{ 0.88f, 0.90f, 0.92f } : Color{ 0.12f, 0.12f, 0.13f };
    setMaterialDielectric(whiteShade ? 70.0f : 110.0f);      // glossy paint: tight white highlight
    glColor3f(shade.r, shade.g, shade.b);
    for (int pass = 0; pass < 2; pass++) {                    // outside, then a plain inside lining
        for (int i = 0; i + 1 < N; i++) {                     // one strip between profile rings i and i+1
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= SEG; j++) {
                float a = j * 2.0f * PI / SEG, ca = cosf(a), sa = sinf(a);
                for (int k = 0; k < 2; k++) {
                    int m = i + k, lo = m > 0 ? m - 1 : m, hi = m + 1 < N ? m + 1 : m;
                    float dr = prof[hi][0] - prof[lo][0], dy = prof[hi][1] - prof[lo][1];
                    float nl = sqrtf(dr * dr + dy * dy), nr = dy / nl, ny = -dr / nl;   // outward profile normal
                    float rr = prof[m][0] - (pass ? 0.006f : 0.0f);
                    if (pass) { nr = -nr; ny = -ny; }
                    glNormal3f(nr * ca, ny, nr * sa);
                    glVertex3f(rr * ca, prof[m][1], rr * sa);
                }
            }
            glEnd();
        }
    }
    resetMaterialGloss();

    // Warm bulb glowing inside the shade
    if (isOn) {
        setEmission(1.00f, 0.94f, 0.80f);
        drawSphere({ 0, -0.40f, 0 }, NO_ROT, { 0.05f, 0.05f, 0.05f }, WHITE);
        clearEmission();
    } else {
        drawSphere({ 0, -0.40f, 0 }, NO_ROT, { 0.05f, 0.05f, 0.05f }, GRAY);
    }

    glPopMatrix();
}

// Additive Volumetric Light Beam for Spotlight:
// Produces a soft, glowing conical light beam descending from the fixture to the counter.
void drawSpotlightBeam(Vec3 pos, float height, float topRadius, float bottomRadius)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // Additive light blending
    glDepthMask(GL_FALSE);
    setLighting(false);

    const int SEGS = 24;
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= SEGS; i++) {
        float a = (float)i * 2.0f * PI / (float)SEGS;
        float ca = cosf(a), sa = sinf(a);

        // Apex at spotlight lens (bright warm white, higher alpha)
        glColor4f(1.0f, 0.94f, 0.78f, 0.18f);
        glVertex3f(topRadius * ca, 0.0f, topRadius * sa);

        // Base on the counter/table (softly dissipates to 0 alpha)
        glColor4f(1.0f, 0.88f, 0.65f, 0.00f);
        glVertex3f(bottomRadius * ca, -height, bottomRadius * sa);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    setLighting(true);
    glDisable(GL_BLEND);

    glPopMatrix();
}

// Overhead Area Light Luminaire (Ceiling Softbox Panel):
// Sleek rectangular fixture with brushed aluminum casing, suspension wires,
// and glowing frosted diffuser panel providing broad, soft area illumination.
void drawAreaLightFixture(Vec3 pos, Vec3 rot, Vec3 scale, bool isOn)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    float panelL = 3.2f;
    float panelW = 0.85f;
    float panelH = 0.07f;

    // 4 Steel Suspension Wires to ceiling
    glColor3f(0.5f, 0.5f, 0.52f);
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            drawCylinder({ sx * 1.45f, panelH, sz * 0.36f }, NO_ROT,
                         { 0.008f, 0.012f, 0.008f }, STEEL);   // up to the ceiling (y 3.30), not through it
        }
    }

    // Brushed Aluminum Fixture Housing
    setMaterialConductive(STEEL, 70.0f);
    beginSphereReflect();
    // Top housing
    drawCuboid({ 0, 0, 0 }, NO_ROT, { panelL, panelH, panelW }, DARK_GRAY);
    // Outer border trim
    drawCuboid({ 0, -0.01f,  panelW * 0.48f }, NO_ROT, { panelL + 0.04f, 0.04f, 0.05f }, STEEL);
    drawCuboid({ 0, -0.01f, -panelW * 0.48f }, NO_ROT, { panelL + 0.04f, 0.04f, 0.05f }, STEEL);
    drawCuboid({  panelL * 0.49f, -0.01f, 0 }, NO_ROT, { 0.05f, 0.04f, panelW }, STEEL);
    drawCuboid({ -panelL * 0.49f, -0.01f, 0 }, NO_ROT, { 0.05f, 0.04f, panelW }, STEEL);
    endSphereReflect();
    resetMaterialGloss();

    // Emissive Frosted Diffuser Panel (facing downward)
    if (isOn) {
        setEmission(0.92f, 0.95f, 1.0f);   // 4500K crisp white area light
        setColor(WHITE);
    } else {
        clearEmission();
        setColor({ 0.65f, 0.67f, 0.70f });
    }

    glBegin(GL_QUADS);
    glNormal3f(0, -1, 0);
    glVertex3f(-panelL * 0.47f, -0.005f, -panelW * 0.45f);
    glVertex3f( panelL * 0.47f, -0.005f, -panelW * 0.45f);
    glVertex3f( panelL * 0.47f, -0.005f,  panelW * 0.45f);
    glVertex3f(-panelL * 0.47f, -0.005f,  panelW * 0.45f);
    glEnd();

    clearEmission();

    glPopMatrix();
}

// Clear Glass Pendant Lamp:
// Clear glass bell/dome shade hanging over the dining counter with visible
// glowing filament Edison light bulb inside the transparent glass!
void drawPendantGlassLamp(Vec3 pos, Vec3 rot, Vec3 scale, bool isOn)
{
    // Industrial pendant: riveted dark-steel dome shade, brass collar stack and bracket,
    // brass cage rings with steel bands around an exposed glowing bulb.
    const Color STEEL_D = { 0.30f, 0.34f, 0.38f }, BRASS = { 0.78f, 0.60f, 0.24f };
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // cable
    drawCylinder({ 0, 0.0f, 0 }, NO_ROT, { 0.014f, 0.45f, 0.014f }, BLACK);   // cord ends at the ceiling (y 3.30)

    // steel U-bracket with a small hook ring
    setMaterialConductive(STEEL_D, 70.0f);
    drawCuboid({ -0.07f, 0.17f, 0 }, NO_ROT, { 0.025f, 0.30f, 0.09f }, STEEL_D);
    drawCuboid({  0.07f, 0.17f, 0 }, NO_ROT, { 0.025f, 0.30f, 0.09f }, STEEL_D);
    drawCuboid({ 0, 0.46f, 0 }, NO_ROT, { 0.17f, 0.025f, 0.09f }, STEEL_D);
    setMaterialConductive(BRASS, 80.0f);
    drawTorus({ 0, 0.50f, 0 }, { 0, 90, 0 }, ONE, BRASS, 0.007f, 0.025f);

    // brass collar stack above the shade
    for (int i = 0; i < 5; i++) {
        float y = 0.13f + i * 0.045f;
        drawCylinder({ 0, y, 0 }, NO_ROT, { 0.085f - 0.004f * i, 0.030f, 0.085f - 0.004f * i }, BRASS);
        drawTorus({ 0, y + 0.03f, 0 }, NO_ROT, ONE, BRASS, 0.007f, 0.045f - 0.002f * i);
    }

    // dome shade (surface of revolution)
    setMaterialConductive(STEEL_D, 60.0f);
    setColor(STEEL_D);
    const int SL = 28, ST = 8;
    for (int i = 0; i < ST; i++) {
        float p0 = i * 1.5708f / ST, p1 = (i + 1) * 1.5708f / ST;
        float r0 = 0.09f + 0.25f * sinf(p0), y0 = 0.13f - 0.16f * (1.0f - cosf(p0));
        float r1 = 0.09f + 0.25f * sinf(p1), y1 = 0.13f - 0.16f * (1.0f - cosf(p1));
        float n0y = 0.25f + 0.75f * cosf(p0), n1y = 0.25f + 0.75f * cosf(p1);
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= SL; j++) {
            float a2 = j * 6.2832f / SL, ca = cosf(a2), sa = sinf(a2);
            glNormal3f(ca * sinf(p0), n0y, sa * sinf(p0)); glVertex3f(ca * r0, y0, sa * r0);
            glNormal3f(ca * sinf(p1), n1y, sa * sinf(p1)); glVertex3f(ca * r1, y1, sa * r1);
        }
        glEnd();
    }
    // rolled brass rim and a ring of rivets
    setMaterialConductive(BRASS, 80.0f);
    drawTorus({ 0, -0.03f, 0 }, NO_ROT, ONE, BRASS, 0.012f, 0.345f);
    for (int i = 0; i < 16; i++) {
        float a2 = i * 6.2832f / 16.0f;
        drawSphere({ cosf(a2) * 0.325f, -0.012f, sinf(a2) * 0.325f }, NO_ROT, { 0.026f, 0.026f, 0.026f }, BRASS);
    }
    // brass cage rings under the shade and four steel bands over the bulb
    drawTorus({ 0, -0.075f, 0 }, NO_ROT, ONE, BRASS, 0.008f, 0.30f);
    drawTorus({ 0, -0.125f, 0 }, NO_ROT, ONE, BRASS, 0.008f, 0.245f);
    drawTorus({ 0, -0.175f, 0 }, NO_ROT, ONE, BRASS, 0.008f, 0.17f);
    setMaterialConductive(STEEL_D, 60.0f);
    for (int k = 0; k < 4; k++) {
        float a2 = k * 1.5708f + 0.4f;
        glBegin(GL_QUAD_STRIP);
        glNormal3f(cosf(a2), 0.2f, sinf(a2));
        for (int i = 0; i <= 8; i++) {
            float t = i / 8.0f, r = 0.34f * cosf(t * 1.45f), y = -0.03f - 0.22f * sinf(t * 1.45f);
            float w = 0.012f;
            glVertex3f(cosf(a2) * r - sinf(a2) * w, y, sinf(a2) * r + cosf(a2) * w);
            glVertex3f(cosf(a2) * r + sinf(a2) * w, y, sinf(a2) * r - cosf(a2) * w);
        }
        glEnd();
    }
    resetMaterialGloss();

    // exposed bulb
    if (isOn) {
        setEmission(1.00f, 0.88f, 0.55f);
        drawSphere({ 0, -0.06f, 0 }, NO_ROT, { 0.15f, 0.20f, 0.15f }, { 1.0f, 0.93f, 0.65f });
        clearEmission();
    } else {
        drawSphere({ 0, -0.06f, 0 }, NO_ROT, { 0.15f, 0.20f, 0.15f }, { 0.75f, 0.75f, 0.72f });
    }

    glPopMatrix();
}

// ─── Japanese Floor Lantern Tower ─────────────────────────────────────────────

// Tall stacked andon-style floor lamp: natural bamboo/wood lattice frame with
// glowing washi-paper panels in each section, placed in a room corner.
void drawJapaneseFloorLanternTower(Vec3 pos, Vec3 rot, Vec3 scale, bool isOn)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    const float hw  = 0.155f;  // half-width of each section
    const float sh  = 0.40f;   // height of each section
    const int   NS  = 4;       // number of stacked sections
    const Color BAMBOO = { 0.72f, 0.56f, 0.30f };  // natural bamboo

    float flick = 1.0f + 0.05f * sinf(animTime * 3.2f)
                       + 0.03f * cosf(animTime * 8.9f);

    // ── Base disc ────────────────────────────────────────────────────────
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCylinder({ 0, 0.015f, 0 }, NO_ROT, { hw + 0.06f, 0.03f, hw + 0.06f }, DARK_WOOD);
    resetMaterialGloss();

    float baseY = 0.045f;   // top of the base disc

    for (int s = 0; s < NS; s++) {
        float y0 = baseY + s * sh;      // bottom of this section
        float y1 = y0 + sh;             // top of this section
        float yc = (y0 + y1) * 0.5f;

        // 4 corner posts
        setMaterialPBR(Materials::WoodMatte, BAMBOO);
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                drawCylinder({ sx * hw, yc, sz * hw }, NO_ROT,
                             { 0.016f, sh, 0.016f }, BAMBOO);

        // Bottom rail (shared with previous section's top rail except for s==0)
        drawCuboid({ 0, y0 + 0.009f, 0 }, NO_ROT,
                   { hw*2 + 0.01f, 0.018f, hw*2 + 0.01f }, BAMBOO);
        // Top rail
        drawCuboid({ 0, y1 - 0.009f, 0 }, NO_ROT,
                   { hw*2 + 0.01f, 0.018f, hw*2 + 0.01f }, BAMBOO);
        resetMaterialGloss();

        // ── Washi-paper panels on all 4 faces ────────────────────────────
        float pp0 = y0 + 0.020f;   // paper starts just above bottom rail
        float pp1 = y1 - 0.018f;   // paper ends just below top rail

        // Glow gradient like a real paper lamp: bright in the middle of each section, plain darker cream
        // toward the rails and posts.  Flat cream when switched off.
        {
            const int NC = 4, NR = 5;
            const float pw = hw * 2.0f, ph = pp1 - pp0;
            auto panel = [&](float ox, float oz, float ux, float uz, float nx, float nz) {
                setLighting(false);
                glBegin(GL_QUADS);
                glNormal3f(nx, 0, nz);
                for (int i = 0; i < NC; i++)
                    for (int j = 0; j < NR; j++) {
                        float u0 = i / (float)NC, u1 = (i + 1) / (float)NC, v0 = j / (float)NR, v1 = (j + 1) / (float)NR;
                        float uu[4] = { u0, u1, u1, u0 }, vv[4] = { v0, v0, v1, v1 };
                        for (int k = 0; k < 4; k++) {
                            float cu = uu[k] * 2.0f - 1.0f, dv = (vv[k] - 0.5f) / 0.55f;
                            float g = expf(-(cu * cu * 1.4f + dv * dv * 1.5f)) * (1.0f - 0.35f * powf(fabsf(cu), 4.0f));
                            if (isOn) { float f = g * flick; glColor3f(0.56f + 0.40f * f, 0.42f + 0.44f * f, 0.24f + 0.26f * f); }
                            else glColor3f(0.87f, 0.82f, 0.68f);
                            glVertex3f(ox + ux * uu[k], pp1 - ph * vv[k], oz + uz * uu[k]);
                        }
                    }
                glEnd();
                setLighting(true);
            };
            panel(-hw,  hw,  pw, 0,   0,  1);     // +Z front
            panel( hw, -hw, -pw, 0,   0, -1);     // -Z back
            panel(-hw, -hw,  0, pw,  -1,  0);     // -X left
            panel( hw,  hw,  0, -pw,  1,  0);     // +X right
        }

        clearEmission();
    }

    // ── Top finial ───────────────────────────────────────────────────────
    float topY = baseY + NS * sh;
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCuboid({ 0, topY + 0.015f, 0 }, NO_ROT,
               { hw*2 + 0.04f, 0.030f, hw*2 + 0.04f }, DARK_WOOD);
    drawCylinder({ 0, topY + 0.045f, 0 }, NO_ROT, { 0.032f, 0.08f, 0.032f }, DARK_WOOD);
    drawSphere  ({ 0, topY + 0.125f, 0 }, NO_ROT, { 0.038f, 0.038f, 0.038f }, DARK_WOOD);
    resetMaterialGloss();

    glPopMatrix();
}

// ─── Japanese Box Lantern Cluster ─────────────────────────────────────────────

// Internal helper: draws one andon-style box lantern.
// Called inside drawJapaneseBoxLanternCluster with the cluster centre already
// translated to the origin.  rx/rz are horizontal offsets from that origin;
// cordLen is how far the lantern hangs below the ceiling mount; phase gives
// each lantern an independent sway / flicker offset.
static void drawOneBoxLantern(float rx, float rz, float cordLen, bool isOn, float phase)
{
    glPushMatrix();
    glTranslatef(rx, 0.0f, rz);

    // Subtle independent sway
    float sway = sinf(animTime * 1.4f + phase) * 1.6f;
    glRotatef(sway, 0, 0, 1);

    // Thin dark hanging cord
    drawCylinder({ 0, -cordLen, 0 }, NO_ROT,
                 { 0.007f, cordLen, 0.007f }, DARK_WOOD);      // from the lantern up to the mount only

    glTranslatef(0, -cordLen, 0);   // move to top-centre of the lantern box

    const float hw = 0.115f;  // half-width  (X)
    const float hh = 0.148f;  // half-height (Y)
    const float hd = 0.115f;  // half-depth  (Z)

    // Per-lantern flicker  (incandescent character)
    float flick = 1.0f + 0.07f * sinf(animTime * 4.8f + phase * 2.1f)
                       + 0.03f * cosf(animTime * 11.3f + phase);

    // ── Dark-wood frame ──────────────────────────────────────────────────
    setMaterialPBR(Materials::WoodMatte, DARK_WOOD);

    drawCuboid({ 0,  hh + 0.014f, 0 }, NO_ROT,
               { hw*2+0.022f, 0.020f, hd*2+0.022f }, DARK_WOOD);   // top cap
    drawCuboid({ 0, -hh - 0.014f, 0 }, NO_ROT,
               { hw*2+0.022f, 0.020f, hd*2+0.022f }, DARK_WOOD);   // bottom cap

    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCylinder({ sx*hw*0.92f, 0, sz*hd*0.92f }, NO_ROT,
                         { 0.011f, hh*2+0.02f, 0.011f }, DARK_WOOD);  // corner posts

    resetMaterialGloss();

    // ── Washi-paper panels with a glow gradient ──────────────────────────
    // Like a real paper lamp (see reference): the paper is brightest in front of the bulb and fades to
    // plain, darker cream toward the top and bottom edges and near the frame posts.  When the lantern
    // is off the paper is a flat cream colour.
    {
        const int NC = 6, NR = 8;
        auto glowAt = [&](float u, float v) {                          // u: -1..1 across, v: 0 top .. 1 bottom
            float dv = (v - 0.46f) / 0.52f;
            float g = expf(-(u * u * 1.5f + dv * dv * 1.4f));          // soft hot spot at the bulb
            float post = 1.0f - 0.35f * powf(fabsf(u), 4.0f);          // darker toward the corner posts
            return g * post;
        };
        auto panel = [&](float ox, float oy, float oz, float ux, float uy, float uz, float vx, float vy, float vz,
                         float nx, float ny, float nz) {
            glBegin(GL_QUADS);
            glNormal3f(nx, ny, nz);
            for (int i = 0; i < NC; i++)
                for (int j = 0; j < NR; j++) {
                    float u0 = i / (float)NC, u1 = (i + 1) / (float)NC, v0 = j / (float)NR, v1 = (j + 1) / (float)NR;
                    float uu[4] = { u0, u1, u1, u0 }, vv[4] = { v0, v0, v1, v1 };
                    for (int k = 0; k < 4; k++) {
                        float g = isOn ? glowAt(uu[k] * 2.0f - 1.0f, vv[k]) : 0.0f;
                        if (isOn) {
                            float f = g * flick;
                            glColor3f(0.56f + 0.40f * f, 0.42f + 0.44f * f, 0.24f + 0.26f * f);   // cream -> soft warm glow
                        } else glColor3f(0.86f, 0.80f, 0.65f);
                        glVertex3f(ox + ux * uu[k] + vx * vv[k], oy + uy * uu[k] + vy * vv[k], oz + uz * uu[k] + vz * vv[k]);
                    }
                }
            glEnd();
        };
        const float W2 = 2.0f * hw, D2 = 2.0f * hd, H2 = 2.0f * hh;
        setLighting(false);                       // the glow is the lantern's own light, not lit by anything else
        panel(-hw,  hh,  hd,   W2, 0, 0,   0, -H2, 0,   0, 0,  1);      // +Z front
        panel( hw,  hh, -hd,  -W2, 0, 0,   0, -H2, 0,   0, 0, -1);      // -Z back
        panel(-hw,  hh, -hd,   0, 0, D2,   0, -H2, 0,  -1, 0,  0);      // -X left
        panel( hw,  hh,  hd,   0, 0, -D2,  0, -H2, 0,   1, 0,  0);      // +X right
        setLighting(true);
    }

    // soft yellow glow halo around the lit lantern
    if (isOn) {
        glPushMatrix();
        glTranslatef(0, 0.0f, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        setLighting(false);
        glColor4f(1.0f, 0.86f, 0.5f, 0.05f * flick);
        gluSphere(quad, 0.30f, 12, 12);
        glColor4f(1.0f, 0.88f, 0.55f, 0.04f * flick);
        gluSphere(quad, 0.19f, 12, 12);
        glDepthMask(GL_TRUE);
        setLighting(true);
        glDisable(GL_BLEND);
        glPopMatrix();
    }

    clearEmission();
    glPopMatrix();
}

// Japanese Box Lantern Cluster:
// Five rectangular washi-paper andon lanterns suspended at staggered heights
// from a single ceiling mount — warm amber glow, dark-wood frame, gentle sway.
void drawJapaneseBoxLanternCluster(Vec3 pos, Vec3 rot, Vec3 scale, bool isOn)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Ceiling mount canopy disc
    setMaterialPBRMetallic(Materials::BrushedMetal, DARK_GRAY);
    drawCylinder({ 0, -0.01f, 0 }, NO_ROT, { 0.055f, 0.028f, 0.055f }, DARK_GRAY);
    resetMaterialGloss();

    // (xOff, zOff, cordLen, animPhase)  — 5 lanterns at varying heights
    const float L[5][4] = {
        {  0.00f,  0.00f, 0.50f, 0.0f },   // centre — highest
        { -0.23f, -0.12f, 0.78f, 1.3f },   // left-back — lower
        {  0.21f,  0.08f, 0.65f, 2.5f },   // right-front
        { -0.07f,  0.24f, 0.93f, 0.8f },   // front-centre — lowest
        {  0.16f, -0.22f, 0.60f, 3.1f },   // right-back
    };
    for (int i = 0; i < 5; i++)
        drawOneBoxLantern(L[i][0], L[i][1], L[i][2], isOn, L[i][3]);

    glPopMatrix();
}

// Draw a box with its BOTTOM at pos.y, centred on pos.x/z.
// Each face is UV-mapped so one texture repeat spans uvScale world units.
// Falls back to drawCuboid when texID == 0.
void drawTexturedBox(Vec3 pos, Vec3 rot, Vec3 size, GLuint texID, Color tint, float uvScale)
{
    if (!texID) { drawCuboid(pos, rot, size, tint); return; }

    glPushMatrix();
    applyTransform(pos, rot, ONE);

    float hx = size.x * 0.5f;
    float hy = size.y;           // full height (bottom at y=0, top at y=hy)
    float hz = size.z * 0.5f;
    float uX = size.x / uvScale;
    float uY = size.y / uvScale;
    float uZ = size.z / uvScale;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    shaderSetTexture(true);
    setColor(tint);

    glBegin(GL_QUADS);
    // Top (+Y) — grain / tile runs along X (U direction).
    // Wound counter-clockwise seen from +Y; the old order was clockwise, which made
    // two-sided fixed-function lighting treat the top as a back face (flipped normal
    // -> black counter tops at night in Gouraud mode).
    glNormal3f( 0, 1, 0);
    glTexCoord2f(0,  uZ); glVertex3f(-hx, hy,  hz);
    glTexCoord2f(uX, uZ); glVertex3f( hx, hy,  hz);
    glTexCoord2f(uX, 0 ); glVertex3f( hx, hy, -hz);
    glTexCoord2f(0,  0 ); glVertex3f(-hx, hy, -hz);
    // Bottom (-Y) — counter-clockwise seen from below
    glNormal3f( 0,-1, 0);
    glTexCoord2f(0,  0 ); glVertex3f(-hx, 0, -hz);
    glTexCoord2f(uX, 0 ); glVertex3f( hx, 0, -hz);
    glTexCoord2f(uX, uZ); glVertex3f( hx, 0,  hz);
    glTexCoord2f(0,  uZ); glVertex3f(-hx, 0,  hz);
    // Front (+Z)
    glNormal3f( 0, 0, 1);
    glTexCoord2f(0,  0 ); glVertex3f(-hx, 0,  hz);
    glTexCoord2f(uX, 0 ); glVertex3f( hx, 0,  hz);
    glTexCoord2f(uX, uY); glVertex3f( hx, hy, hz);
    glTexCoord2f(0,  uY); glVertex3f(-hx, hy, hz);
    // Back (-Z)
    glNormal3f( 0, 0,-1);
    glTexCoord2f(0,  0 ); glVertex3f( hx, 0, -hz);
    glTexCoord2f(uX, 0 ); glVertex3f(-hx, 0, -hz);
    glTexCoord2f(uX, uY); glVertex3f(-hx, hy, -hz);
    glTexCoord2f(0,  uY); glVertex3f( hx, hy, -hz);
    // Right (+X)
    glNormal3f( 1, 0, 0);
    glTexCoord2f(0,  0 ); glVertex3f(hx, 0,  hz);
    glTexCoord2f(uZ, 0 ); glVertex3f(hx, 0, -hz);
    glTexCoord2f(uZ, uY); glVertex3f(hx, hy, -hz);
    glTexCoord2f(0,  uY); glVertex3f(hx, hy,  hz);
    // Left (-X)
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0,  0 ); glVertex3f(-hx, 0, -hz);
    glTexCoord2f(uZ, 0 ); glVertex3f(-hx, 0,  hz);
    glTexCoord2f(uZ, uY); glVertex3f(-hx, hy,  hz);
    glTexCoord2f(0,  uY); glVertex3f(-hx, hy, -hz);
    glEnd();

    shaderSetTexture(false);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// Textured wedge (gable roof shape) with UV-mapped sloped faces.
// Falls back to plain drawWedge when texID == 0.
// The wedge has its base at y=0, ridge at y=1, slopes along Z.
void drawTexturedWedge(Vec3 pos, Vec3 rot, Vec3 scale, GLuint texID, Color tint, float uvScale)
{
    if (!texID) { drawWedge(pos, rot, scale, tint); return; }

    glPushMatrix();
    applyTransform(pos, rot, scale);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    shaderSetTexture(true);
    setColor(tint);

    // Subdivide sloped faces for smooth per-pixel lighting + texturing
    const int subX = 16;  // subdivisions along X (ridge direction)
    const int subS = 12;  // subdivisions along the slope

    // South slope: from (x, 0, 0.5) to (x, 1, 0)
    {
        float nx = 0, ny = 1.0f, nz = 2.0f;
        float len = sqrtf(ny*ny + nz*nz);
        ny /= len; nz /= len;

        float slopeLen = sqrtf(0.5f * 0.5f + 1.0f * 1.0f);
        float uScale = 1.0f / uvScale;
        float vScale = slopeLen / uvScale;

        glBegin(GL_QUADS);
        for (int i = 0; i < subX; i++) {
            float x0 = -0.5f + (float)i / subX;
            float x1 = -0.5f + (float)(i+1) / subX;
            float u0 = (x0 + 0.5f) * uScale;
            float u1 = (x1 + 0.5f) * uScale;

            for (int j = 0; j < subS; j++) {
                float t0 = (float)j / subS;
                float t1 = (float)(j+1) / subS;
                float y0s = t0 * 1.0f;
                float z0s = 0.5f - t0 * 0.5f;
                float y1s = t1 * 1.0f;
                float z1s = 0.5f - t1 * 0.5f;
                float v0 = t0 * vScale;
                float v1 = t1 * vScale;

                glNormal3f(nx, ny, nz);
                glTexCoord2f(u0, v0); glVertex3f(x0, y0s, z0s);
                glTexCoord2f(u1, v0); glVertex3f(x1, y0s, z0s);
                glTexCoord2f(u1, v1); glVertex3f(x1, y1s, z1s);
                glTexCoord2f(u0, v1); glVertex3f(x0, y1s, z1s);
            }
        }
        glEnd();
    }

    // North slope: from (x, 0, -0.5) to (x, 1, 0)
    {
        float nx = 0, ny = 1.0f, nz = -2.0f;
        float len = sqrtf(ny*ny + nz*nz);
        ny /= len; nz /= len;

        float slopeLen = sqrtf(0.5f * 0.5f + 1.0f * 1.0f);
        float uScale = 1.0f / uvScale;
        float vScale = slopeLen / uvScale;

        glBegin(GL_QUADS);
        for (int i = 0; i < subX; i++) {
            float x0 = -0.5f + (float)i / subX;
            float x1 = -0.5f + (float)(i+1) / subX;
            float u0 = (x0 + 0.5f) * uScale;
            float u1 = (x1 + 0.5f) * uScale;

            for (int j = 0; j < subS; j++) {
                float t0 = (float)j / subS;
                float t1 = (float)(j+1) / subS;
                float y0s = t0 * 1.0f;
                float z0s = -0.5f + t0 * 0.5f;
                float y1s = t1 * 1.0f;
                float z1s = -0.5f + t1 * 0.5f;
                float v0 = t0 * vScale;
                float v1 = t1 * vScale;

                glNormal3f(nx, ny, nz);
                glTexCoord2f(u0, v0); glVertex3f(x0, y0s, z0s);
                glTexCoord2f(u0, v1); glVertex3f(x0, y1s, z1s);
                glTexCoord2f(u1, v1); glVertex3f(x1, y1s, z1s);
                glTexCoord2f(u1, v0); glVertex3f(x1, y0s, z0s);
            }
        }
        glEnd();
    }

    // Bottom face
    glBegin(GL_QUADS);
    glNormal3f(0, -1, 0);
    glTexCoord2f(0, 0); glVertex3f(-0.5f, 0, -0.5f);
    glTexCoord2f(1, 0); glVertex3f( 0.5f, 0, -0.5f);
    glTexCoord2f(1, 1); glVertex3f( 0.5f, 0,  0.5f);
    glTexCoord2f(0, 1); glVertex3f(-0.5f, 0,  0.5f);
    glEnd();

    // Triangular end caps
    glBegin(GL_TRIANGLES);
    glNormal3f(-1, 0, 0);
    glTexCoord2f(0, 0);    glVertex3f(-0.5f, 0, -0.5f);
    glTexCoord2f(1, 0);    glVertex3f(-0.5f, 0,  0.5f);
    glTexCoord2f(0.5f, 1); glVertex3f(-0.5f, 1,  0);
    glNormal3f( 1, 0, 0);
    glTexCoord2f(0, 0);    glVertex3f( 0.5f, 0, -0.5f);
    glTexCoord2f(1, 0);    glVertex3f( 0.5f, 0,  0.5f);
    glTexCoord2f(0.5f, 1); glVertex3f( 0.5f, 1,  0);
    glEnd();

    shaderSetTexture(false);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// Horizontal textured plane (Y=0), subdivided for smooth per-pixel lighting.
void drawTexturedPlane(Vec3 pos, Vec3 rot, Vec3 scale, GLuint texID, Color tint, float uvScale)
{
    if (!texID) { drawSubdividedPlane(pos, rot, scale, tint, 16, 16); return; }

    glPushMatrix();
    applyTransform(pos, rot, ONE);

    float hx = scale.x * 0.5f, hz = scale.z * 0.5f;
    int   gx = (int)ceilf(scale.x / 2.0f);
    int   gz = (int)ceilf(scale.z / 2.0f);
    float sx = scale.x / gx, sz = scale.z / gz;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    shaderSetTexture(true);
    setColor(tint);

    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    for (int i = 0; i < gx; i++) {
        for (int j = 0; j < gz; j++) {
            float x0 = -hx + i * sx,       x1 = x0 + sx;
            float z0 = -hz + j * sz,        z1 = z0 + sz;
            float u0 = (x0 + hx) / uvScale, u1 = (x1 + hx) / uvScale;
            float v0 = (z0 + hz) / uvScale, v1 = (z1 + hz) / uvScale;
            glTexCoord2f(u0, v0); glVertex3f(x0, 0, z0);
            glTexCoord2f(u1, v0); glVertex3f(x1, 0, z0);
            glTexCoord2f(u1, v1); glVertex3f(x1, 0, z1);
            glTexCoord2f(u0, v1); glVertex3f(x0, 0, z1);
        }
    }
    glEnd();

    shaderSetTexture(false);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}
