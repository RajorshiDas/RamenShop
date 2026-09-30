#include "decorations.h"
#include "shader.h"

void drawLantern(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Subtle breeze sway
    float swayZ = sin(animTime * 2.2f + pos.x * 0.6f) * 2.8f;
    float swayX = cos(animTime * 1.7f + pos.z * 0.6f) * 1.8f;
    glRotatef(swayX, 1, 0, 0);
    glRotatef(swayZ, 0, 0, 1);

    // Glowing body (emissive warm red light) with subtle flicker
    float flick = 1.0f + 0.05f * sin(animTime * 5.0f + pos.x);
    setEmission(0.85f * flick, 0.25f * flick, 0.08f * flick);

    // Lantern paper body: slight translucency, diffuse material
    setMaterialPBR(Materials::GlazedMatte, RED);
    drawSphere({ 0, 0, 0 },          NO_ROT, { 0.5f, 0.65f, 0.5f },  RED);       // body

    // Wooden ribs: matte finish
    setMaterialPBR(Materials::WoodMatte, DARK_RED);
    resetMaterialGloss();
    drawTorus({ 0, 0, 0 },           NO_ROT, ONE, DARK_RED, 0.01f, 0.25f);      // rib
    drawTorus({ 0, 0.15f, 0 },       NO_ROT, ONE, DARK_RED, 0.01f, 0.22f);
    drawTorus({ 0, -0.15f, 0 },      NO_ROT, ONE, DARK_RED, 0.01f, 0.22f);
    clearEmission();

    // Metal caps and hardware
    setMaterialPBRMetallic(Materials::BrushedMetal, BLACK);
    drawCylinder({ 0,  0.28f, 0 },   NO_ROT, { 0.25f, 0.06f, 0.25f }, BLACK);   // top cap
    drawCylinder({ 0, -0.34f, 0 },   NO_ROT, { 0.25f, 0.06f, 0.25f }, BLACK);   // bottom cap
    drawCylinder({ 0,  0.34f, 0 },   NO_ROT, { 0.02f, 0.5f, 0.02f },  BLACK);   // string

    // Gold tassel: metallic with slight reflection
    setMaterialPBRMetallic(Materials::Gold, GOLD);
    drawCone({ 0, -0.55f, 0 },       NO_ROT, { 0.08f, 0.21f, 0.08f }, GOLD);    // tassel

    resetMaterialGloss();
    glPopMatrix();
}

void drawCylinderLantern(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Gentle interior air current sway
    float sway = sin(animTime * 1.8f + pos.x * 0.7f) * 2.2f;
    glRotatef(sway, 0, 0, 1);

    float flick = 1.0f + 0.06f * sin(animTime * 4.6f + pos.x);
    setEmission(0.90f * flick, 0.50f * flick, 0.10f * flick);

    // Orange paper lantern: semi-translucent with warm glow
    setMaterialPBR(Materials::GlazedMatte, LANTERN_ORANGE);
    drawCylinderCustom({ 0, -0.4f, 0 }, NO_ROT, ONE, LANTERN_ORANGE, 0.22f, 0.22f, 0.8f);
    clearEmission();

    // Wooden bands and hardware: natural grain
    setMaterialPBR(Materials::WoodMatte, DARK_WOOD);
    drawTorus({ 0, -0.40f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.02f, 0.23f);
    drawTorus({ 0, -0.13f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.015f, 0.23f);
    drawTorus({ 0,  0.13f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.015f, 0.23f);
    drawTorus({ 0,  0.40f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.02f, 0.23f);

    // String support: dark material
    setMaterialPBR(Materials::SeaweedNori, BLACK);
    drawCylinder({ 0, 0.40f, 0 }, NO_ROT, { 0.02f, 0.7f, 0.02f }, BLACK);

    resetMaterialGloss();
    glPopMatrix();
}

void drawNoren(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Rod: natural wood with matte finish
    setMaterialPBR(Materials::WoodMatte, DARK_WOOD);
    drawCylinder({ -3.5f, 0, 0 }, { 0, 0, -90 }, { 0.04f, 7.0f, 0.04f }, DARK_WOOD);

    // Hanging cloth panels with gentle breeze ripple
    for (int i = -4; i <= 4; i++) {
        float sway = sin(animTime * 2.2f + (float)i * 0.5f) * 3.5f;
        glPushMatrix();
        glTranslatef(i * 0.72f, 0, 0.02f);
        glRotatef(sway, 1, 0, 0);

        // Traditional navy cotton fabric: very matte, diffuse
        setMaterialPBR(Materials::Fabric, NAVY);
        drawBoard({ 0, -0.65f, 0 }, { 90, 0, 0 }, { 0.68f, 0.6f, 1.3f }, NAVY);

        glPopMatrix();
    }

    resetMaterialGloss();
    glPopMatrix();
}

void drawMenuBoard(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCube({ 0, 0, 0 }, NO_ROT, { 1.2f, 0.9f, 0.05f }, DARK_WOOD);
    for (int i = 0; i < 4; i++) {
        float x = -0.42f + i * 0.28f;
        drawCube({ x, 0, 0.03f }, NO_ROT, { 0.2f, 0.75f, 0.01f }, PAPER);
        for (int j = 0; j < 3; j++)
            drawCube({ x, 0.22f - j * 0.2f, 0.036f }, NO_ROT, { 0.04f, 0.12f, 0.005f }, BLACK);
    }
    glPopMatrix();
}

void drawWallDecoration(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCube({ 0, 0, 0 },     NO_ROT, { 1.0f, 0.70f, 0.04f }, DARK_WOOD);      // frame
    drawCube({ 0, 0, 0.02f }, NO_ROT, { 0.88f, 0.58f, 0.01f }, PAPER);         // canvas
    drawCylinder({ 0.22f, 0.12f, 0.026f }, { 90, 0, 0 }, { 0.18f, 0.005f, 0.18f }, RED);   // sun
    // Mountain = wedge turned to face us; snow cap = smaller white wedge
    drawWedge({ -0.1f, -0.25f, 0.027f }, { 0, 90, 0 }, { 0.005f, 0.40f, 0.60f }, MOUNTAIN);
    drawWedge({ -0.1f,  0.02f, 0.029f }, { 0, 90, 0 }, { 0.005f, 0.13f, 0.195f }, WHITE);
    glPopMatrix();
}

void drawWallClock(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // ── Outer frame ring (torus in X-Y plane, 90° X-tilt) ──────────────
    setMaterialDielectric(20.0f);
    drawTorus({ 0, 0, 0 }, { 90, 0, 0 }, ONE, DARK_WOOD, 0.045f, 0.48f);
    resetMaterialGloss();

    // ── Clock face disc ──────────────────────────────────────────────────
    drawCylinder({ 0, 0, 0 }, { 90, 0, 0 }, { 0.88f, 0.025f, 0.88f }, PAPER);

    // ── 12 hour markers ──────────────────────────────────────────────────
    for (int i = 0; i < 12; i++) {
        float ang = (float)i * 30.0f * (PI / 180.0f);
        float mx  = sinf(ang) * 0.36f;
        float my  = cosf(ang) * 0.36f;
        drawCube({ mx, my, 0.026f }, NO_ROT, { 0.028f, 0.075f, 0.012f }, DARK_GRAY);
    }

    // ── Animated hands ───────────────────────────────────────────────────
    // animTime increments ~0.016/frame; 6 deg/unit ≈ 1 rev per real minute
    float minAng = fmod(animTime * 6.0f, 360.0f);
    float hrAng  = minAng / 12.0f;

    // Minute hand (long, thin, black)
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.036f);
    glRotatef(-minAng, 0.0f, 0.0f, 1.0f);
    drawCuboid({ 0, 0.16f, 0 }, NO_ROT, { 0.020f, 0.32f, 0.010f }, BLACK);
    glPopMatrix();

    // Hour hand (shorter, wider, dark gray)
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.042f);
    glRotatef(-hrAng, 0.0f, 0.0f, 1.0f);
    drawCuboid({ 0, 0.10f, 0 }, NO_ROT, { 0.026f, 0.20f, 0.010f }, DARK_GRAY);
    glPopMatrix();

    // ── Center pin ───────────────────────────────────────────────────────
    drawSphere({ 0, 0, 0.048f }, NO_ROT, { 0.040f, 0.040f, 0.040f }, RED);

    glPopMatrix();
}

void drawSignBoard(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    drawCube({ 0, 0, -0.02f }, NO_ROT, { 2.8f, 0.85f, 0.10f }, DARK_WOOD);   // frame
    setEmission(0.15f, 0.14f, 0.12f);   // slight self-illumination
    drawCube({ 0, 0,  0.03f }, NO_ROT, { 2.6f, 0.70f, 0.06f }, WHITE);       // white board
    clearEmission();

    // Letters
    const char* text = "RAMEN";
    float textWidth = (float)glutStrokeLength(GLUT_STROKE_ROMAN, (const unsigned char*)text);
    float s = 1.8f / textWidth;
    setLighting(false);
    setColor(BLACK);
    glLineWidth(3);
    glPushMatrix();
    glTranslatef(-0.9f, -50 * s, 0.07f);
    glScalef(s, s, s);
    for (const char* p = text; *p; p++)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    glPopMatrix();
    glLineWidth(1.5f);
    setLighting(true);

    glPopMatrix();
}
