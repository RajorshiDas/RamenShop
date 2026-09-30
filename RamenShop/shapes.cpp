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
bool showOutlines = true;
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

void setEmission(float r, float g, float b)
{
    GLfloat em[] = { r, g, b, 1.0f };
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
    GLfloat noSpec[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, noSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
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
// Convert roughness (0=smooth, 1=matte) to OpenGL shininess exponent (1-128)
// Using inverse relationship: shininess = 2 / (roughness^4 + 0.0001)
static float roughnessToShininess(float roughness)
{
    constexpr float maxShininess = 128.0f;
    constexpr float minShininess = 2.0f;
    float r2 = roughness * roughness;
    float r4 = r2 * r2;
    float t = r4 * 0.95f + 0.05f;  // interpolation factor
    return maxShininess * (1.0f - t) + minShininess * t;
}

// Calculate specular intensity based on metallic value (energy conservation)
static float metallicToSpecularIntensity(float metallic)
{
    // Metals have 4-5% base reflectivity + metallic tint
    // Dielectrics have 2-4% base reflectivity (white)
    return 0.02f + metallic * 0.18f;
}

// Apply PBR material properties to currently set color
void setMaterialPBR(const MaterialPBR& mat, const Color& surfaceColor)
{
    // Non-metallic dielectric material (ceramic, plastic, fabric, etc.)
    float shininess = roughnessToShininess(mat.roughness);
    float specIntensity = metallicToSpecularIntensity(mat.metallic) * (1.0f - mat.roughness * 0.3f);

    // Dielectrics always have white specular (color-independent)
    setMaterialGloss(specIntensity, specIntensity, specIntensity, shininess);
}

// Apply metallic PBR material with color tinting
void setMaterialPBRMetallic(const MaterialPBR& mat, const Color& metalColor)
{
    // For metals: metallicness > 0.8
    float shininess = roughnessToShininess(mat.roughness);
    float specIntensity = metallicToSpecularIntensity(mat.metallic);

    // Metals have color-tinted specular highlights
    setMaterialGloss(
        metalColor.r * specIntensity,
        metalColor.g * specIntensity,
        metalColor.b * specIntensity,
        shininess
    );
}

// Sphere-map environment reflection — adds a warm interior glow on top of the
// Phong-lit metal surface, approximating ray-traced reflections.
void beginSphereReflect()
{
    if (drawingShadow) return;   // skip during shadow-projection pass
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
    if (drawingShadow) return;
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
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.4f, 0.55f, 0.4f }, c);
    drawCylinderCustom({ 0, 0.55f, 0 }, NO_ROT, ONE, c, 0.2f, 0.07f, 0.15f);
    drawCylinder({ 0, 0.70f, 0 }, NO_ROT, { 0.14f, 0.2f, 0.14f }, c);
    drawCylinder({ 0, 0.90f, 0 }, NO_ROT, { 0.17f, 0.1f, 0.17f }, capColor);
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

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

    glPopMatrix();
}

// ─── Physical Light Fixtures ───────────────────────────────────────────────

// Ceiling Track Spotlight Fixture:
// Modern black aluminum track can with swivel mount, polished inner reflector cone,
// and glowing halogen/LED emitter.
void drawSpotlightFixture(Vec3 pos, Vec3 rot, Vec3 scale, bool isOn)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Track rail clamp & bracket
    drawCuboid({ 0, 0.22f, 0 }, NO_ROT, { 0.12f, 0.08f, 0.08f }, DARK_GRAY);
    drawCylinder({ 0, 0.12f, 0 }, NO_ROT, { 0.03f, 0.10f, 0.03f }, METAL);

    // Swivel knuckle
    drawSphere({ 0, 0.10f, 0 }, NO_ROT, { 0.05f, 0.05f, 0.05f }, DARK_GRAY);

    // Spotlight cylindrical can body
    glPushMatrix();
    setMaterialConductive(DARK_GRAY, 45.0f);
    drawCylinderCustom({ 0, -0.16f, 0 }, NO_ROT, ONE, DARK_GRAY, 0.13f, 0.11f, 0.26f);
    resetMaterialGloss();

    // Inner parabolic reflector cone (polished specular metal)
    setMaterialConductive(STEEL, 100.0f);
    beginSphereReflect();
    drawCone({ 0, -0.15f, 0 }, { 180, 0, 0 }, { 0.18f, 0.14f, 0.18f }, STEEL);
    endSphereReflect();
    resetMaterialGloss();

    // High-intensity center emitter bulb / lens
    if (isOn) {
        setEmission(1.00f, 0.94f, 0.80f);
        drawSphere({ 0, -0.12f, 0 }, NO_ROT, { 0.065f, 0.065f, 0.065f }, WHITE);
        clearEmission();
    } else {
        drawSphere({ 0, -0.12f, 0 }, NO_ROT, { 0.065f, 0.065f, 0.065f }, GRAY);
    }

    // Outer lens rim ring
    drawTorus({ 0, -0.16f, 0 }, { 90, 0, 0 }, ONE, DARK_GRAY, 0.015f, 0.13f);
    glPopMatrix();

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
                         { 0.008f, 0.80f, 0.008f }, STEEL);
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
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Hanging black cord & brass fitting
    drawCylinder({ 0, 0.20f, 0 }, NO_ROT, { 0.015f, 1.20f, 0.015f }, BLACK);
    setMaterialConductive(GOLD, 75.0f);
    drawCylinder({ 0, 0.12f, 0 }, NO_ROT, { 0.06f, 0.12f, 0.06f }, GOLD);
    resetMaterialGloss();

    // Incandescent / Edison Bulb inside the shade
    if (isOn) {
        setEmission(1.00f, 0.85f, 0.45f);
        drawSphere({ 0, -0.05f, 0 }, NO_ROT, { 0.12f, 0.14f, 0.12f }, { 1.0f, 0.92f, 0.60f });
        // Glowing inner filament
        setEmission(1.00f, 0.95f, 0.70f);
        drawTorus({ 0, -0.05f, 0 }, { 90, 0, 0 }, ONE, GOLD, 0.015f, 0.04f);
        clearEmission();
    } else {
        drawSphere({ 0, -0.05f, 0 }, NO_ROT, { 0.12f, 0.14f, 0.12f }, GRAY);
    }

    // Clear Glass Bell Shade (drawn transparently around the bulb)
    setMaterialDielectric(125.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.20f);
    // Tapered glass bell neck and flared dome
    rawTube(0.08f, 0.18f, 0.15f, 0.04f);
    rawTube(0.18f, 0.32f, 0.22f, -0.18f);

    // Rounded lip ring at bottom
    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.35f);
    glPushMatrix();
    glTranslatef(0, -0.18f, 0);
    glRotatef(90, 1, 0, 0);
    glutSolidTorus(0.02, 0.32, 12, SLICES);
    glPopMatrix();

    rawCircleOutline(0.32f, -0.18f, CLEAR_GLASS);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    resetMaterialGloss();

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
    // Top (+Y) — grain / tile runs along X (U direction)
    glNormal3f( 0, 1, 0);
    glTexCoord2f(0,  0 ); glVertex3f(-hx, hy, -hz);
    glTexCoord2f(uX, 0 ); glVertex3f( hx, hy, -hz);
    glTexCoord2f(uX, uZ); glVertex3f( hx, hy,  hz);
    glTexCoord2f(0,  uZ); glVertex3f(-hx, hy,  hz);
    // Bottom (-Y)
    glNormal3f( 0,-1, 0);
    glTexCoord2f(0,  0 ); glVertex3f(-hx, 0,  hz);
    glTexCoord2f(uX, 0 ); glVertex3f( hx, 0,  hz);
    glTexCoord2f(uX, uZ); glVertex3f( hx, 0, -hz);
    glTexCoord2f(0,  uZ); glVertex3f(-hx, 0, -hz);
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
