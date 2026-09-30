#include "furniture.h"
#include "objects.h"

void drawTable(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0.72f, 0 }, NO_ROT, { 1.2f, 0.06f, 0.8f }, LIGHT_WOOD);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * 0.52f, 0, sz * 0.32f }, NO_ROT, { 0.07f, 0.72f, 0.07f }, DARK_WOOD);
    glPopMatrix();
}

void drawChair(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0.42f, 0 }, NO_ROT, { 0.45f, 0.05f, 0.45f }, WOOD);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * 0.19f, 0, sz * 0.19f }, NO_ROT, { 0.05f, 0.42f, 0.05f }, DARK_WOOD);
    drawCuboid({ -0.19f, 0.47f, -0.2f }, NO_ROT, { 0.05f, 0.5f, 0.05f }, DARK_WOOD);
    drawCuboid({  0.19f, 0.47f, -0.2f }, NO_ROT, { 0.05f, 0.5f, 0.05f }, DARK_WOOD);
    drawCube({ 0, 0.85f, -0.2f }, NO_ROT, { 0.45f, 0.15f, 0.04f }, WOOD);
    glPopMatrix();
}

void drawStool(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Metal base: brushed aluminum with softer reflections
    setMaterialPBRMetallic(Materials::BrushedMetal, METAL);
    beginSphereReflect();
    drawCylinder({ 0, 0, 0 },     NO_ROT, { 0.40f, 0.05f, 0.40f }, METAL);
    drawCylinder({ 0, 0.05f, 0 }, NO_ROT, { 0.07f, 0.67f, 0.07f }, METAL);
    drawTorus({ 0, 0.30f, 0 },    NO_ROT, ONE, METAL, 0.02f, 0.18f);
    endSphereReflect();

    // Cushion top: fabric material with no specularity
    resetMaterialGloss();
    setMaterialPBR(Materials::Fabric, CUSHION);
    drawCylinder({ 0, 0.72f, 0 }, NO_ROT, { 0.38f, 0.06f, 0.38f }, CUSHION);
    glPopMatrix();
}

void drawBench(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ 0, 0.42f, 0 }, NO_ROT, { 1.8f, 0.06f, 0.4f }, WOOD);
    drawCuboid({ -0.75f, 0, 0 }, NO_ROT, { 0.06f, 0.42f, 0.36f }, DARK_WOOD);
    drawCuboid({  0.75f, 0, 0 }, NO_ROT, { 0.06f, 0.42f, 0.36f }, DARK_WOOD);
    drawCuboid({ 0, 0.15f, 0 }, NO_ROT, { 1.5f, 0.05f, 0.05f }, DARK_WOOD);
    glPopMatrix();
}

void drawCounter(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Main counter body: polished wood with satin finish
    setMaterialPBR(Materials::WoodPolished, WOOD);
    drawTexturedBox({ 0, 0, 0 }, NO_ROT, { 6, 1.0f, 0.6f },
                    getTexID(TEX_WOOD), WHITE, 1.0f);

    // Top surface: slightly more glossy, like finished lacquer
    setMaterialGloss(0.48f, 0.42f, 0.30f, 35.0f);
    drawTexturedBox({ 0, 1.0f, 0.05f }, NO_ROT, { 6.2f, 0.06f, 0.8f },
                    getTexID(TEX_WOOD), WHITE, 0.5f);

    // Support braces: dark matte wood
    resetMaterialGloss();
    setMaterialPBR(Materials::WoodMatte, DARK_WOOD);
    for (float x = -2.75f; x <= 2.76f; x += 0.5f)
        drawCuboid({ x, 0.05f, 0.31f }, NO_ROT, { 0.05f, 0.9f, 0.02f }, DARK_WOOD);

    glPopMatrix();
}

void drawShelf(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCuboid({ -1.0f, 0, 0 }, NO_ROT, { 0.04f, 0.68f, 0.3f }, DARK_WOOD);
    drawCuboid({  1.0f, 0, 0 }, NO_ROT, { 0.04f, 0.68f, 0.3f }, DARK_WOOD);
    for (int i = 0; i < 2; i++)
        drawCuboid({ 0, i * 0.6f, 0 }, NO_ROT, { 2.04f, 0.04f, 0.3f }, WOOD);
    glPopMatrix();
}

void drawCabinet(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Cabinet body: wood
    setMaterialPBR(Materials::WoodMatte, WOOD);
    drawTexturedBox({ 0, 0, 0 }, NO_ROT, { 1.0f, 0.9f, 0.6f },
                    getTexID(TEX_WOOD), WHITE, 0.5f);

    // Top trim: brushed stainless steel
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    drawCuboid({ 0, 0.9f, 0 }, NO_ROT, { 1.02f, 0.04f, 0.62f }, STEEL);
    resetMaterialGloss();

    for (int side = -1; side <= 1; side += 2) {
        // Doors: natural wood
        setMaterialPBR(Materials::WoodMatte, WOOD);
        drawCube({ side * 0.245f, 0.45f, 0.305f }, NO_ROT, { 0.47f, 0.8f, 0.02f }, WOOD);

        // Handles: polished metal with bright reflection
        setMaterialPBRMetallic(Materials::Polished, METAL);
        setMaterialGloss(0.85f, 0.85f, 0.85f, 65.0f);
        drawCube({ side * 0.06f,  0.60f, 0.32f  }, NO_ROT, { 0.03f, 0.15f, 0.03f }, METAL);
        resetMaterialGloss();
    }
    glPopMatrix();
}

void drawSink(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setMaterialConductive(STEEL, 80.0f);
    beginSphereReflect();
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.7f, 0.02f, 0.5f }, STEEL);
    drawCuboid({ 0, 0,  0.24f }, NO_ROT, { 0.7f, 0.08f, 0.02f }, STEEL);
    drawCuboid({ 0, 0, -0.24f }, NO_ROT, { 0.7f, 0.08f, 0.02f }, STEEL);
    drawCuboid({ -0.34f, 0, 0 }, NO_ROT, { 0.02f, 0.08f, 0.5f }, STEEL);
    drawCuboid({  0.34f, 0, 0 }, NO_ROT, { 0.02f, 0.08f, 0.5f }, STEEL);
    drawCylinder({ 0, 0, -0.22f },     NO_ROT,     { 0.04f, 0.35f, 0.04f }, METAL);
    drawCylinder({ 0, 0.33f, -0.22f }, { 90, 0, 0 }, { 0.04f, 0.2f, 0.04f }, METAL);
    endSphereReflect();
    drawPlane({ 0, 0.021f, 0.02f }, NO_ROT, { 0.56f, 1, 0.36f }, DARK_GRAY);
    resetMaterialGloss();
    glPopMatrix();
}

void drawCookingPot(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Main stainless steel vessel - highly polished with sharp reflection
    setMaterialPBRMetallic(Materials::Polished, STEEL);
    beginSphereReflect();
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.5f, 0.45f, 0.5f }, STEEL);
    endSphereReflect();

    // Liquid broth inside - glossy reflective surface
    setMaterialPBR(Materials::Broth, BROTH);
    setMaterialGloss(0.80f, 0.65f, 0.40f, 65.0f);
    drawCylinder({ 0, 0.45f, 0 }, NO_ROT, { 0.46f, 0.005f, 0.46f }, BROTH);

    // Rim of pot - brushed stainless with softer highlight
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    beginSphereReflect();
    drawTorus({ 0, 0.45f, 0 }, NO_ROT, ONE, STEEL, 0.02f, 0.25f);
    endSphereReflect();

    // Handles - matte material
    resetMaterialGloss();
    drawTorus({ -0.28f, 0.36f, 0 }, { 90, 0, 0 }, ONE, DARK_GRAY, 0.015f, 0.06f);
    drawTorus({  0.28f, 0.36f, 0 }, { 90, 0, 0 }, ONE, DARK_GRAY, 0.015f, 0.06f);
    glPopMatrix();
}

void drawKitchen(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    for (int i = 0; i < 7; i++)
        drawCabinet({ -3.0f + i, 0, 0 });

    float top = 0.94f;

    // Stove with two burner rings and two pots
    drawCuboid({ -1.5f, top, 0 }, NO_ROT, { 1.8f, 0.1f, 0.55f }, DARK_GRAY);
    drawTorus({ -1.95f, top + 0.1f, 0 }, NO_ROT, ONE, BLACK, 0.02f, 0.15f);
    drawTorus({ -1.05f, top + 0.1f, 0 }, NO_ROT, ONE, BLACK, 0.02f, 0.15f);
    glPushMatrix();
    glTranslatef(-1.95f, top + 0.1f, 0);
    applyObjDelta(OBJ_KETTLE);
    drawCookingPot({ 0, 0, 0 });
    glPopMatrix();
    drawCookingPot({ -1.05f, top + 0.1f, 0 }, NO_ROT, { 0.8f, 0.8f, 0.8f });

    // Cutting board, sink and a stack of plates
    drawBoard({ 0.2f, top + 0.0125f, 0 }, { 0, 10, 0 }, { 0.5f, 0.5f, 0.35f }, LIGHT_WOOD);
    drawSink({ 1.5f, top, 0 });
    for (int i = 0; i < 4; i++)
        drawPlate({ 2.7f, top + i * 0.025f, 0 }, NO_ROT, { 0.3f, 0.3f, 0.3f }, PLATE_WHITE);

    glPopMatrix();
}

void drawCashRegister(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // ── Body ──────────────────────────────────────────────────────────────
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.38f, 0.30f, 0.28f }, DARK_GRAY);

    // ── Screen (tilted back 25°) ───────────────────────────────────────────
    drawCuboid({ 0, 0.30f, -0.04f }, { -25, 0, 0 }, { 0.32f, 0.20f, 0.02f }, BLACK);
    setEmission(0.05f, 0.18f, 0.28f);
    drawCuboid({ 0, 0.30f, -0.04f }, { -25, 0, 0 }, { 0.26f, 0.14f, 0.004f }, { 0.10f, 0.55f, 0.80f });
    clearEmission();

    // ── Key panel ─────────────────────────────────────────────────────────
    drawCuboid({ 0, 0.31f, 0.04f }, { -8, 0, 0 }, { 0.34f, 0.14f, 0.20f }, GRAY);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 5; col++) {
            float kx = -0.13f + col * 0.065f;
            float ky  = 0.325f + row * 0.032f;
            float kz  = 0.10f  - row * 0.022f;
            Color kc  = (row == 3 && col == 4) ? RED :
                        (row == 3 && col == 3) ? Color{ 0.25f, 0.70f, 0.25f } :
                                                 Color{ 0.55f, 0.55f, 0.58f };
            drawCube({ kx, ky, kz }, { -8, 0, 0 }, { 0.040f, 0.016f, 0.036f }, kc);
        }
    }

    // ── Cash drawer ────────────────────────────────────────────────────────
    setMaterialConductive(METAL, 50.0f);
    beginSphereReflect();
    drawCuboid({ 0, 0.07f, 0.142f }, NO_ROT, { 0.34f, 0.09f, 0.008f }, METAL);
    endSphereReflect();
    resetMaterialGloss();

    glPopMatrix();
}

void drawLadle(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    setMaterialConductive(STEEL, 75.0f);
    beginSphereReflect();
    drawCylinderCustom({ 0, 0, 0 }, NO_ROT, ONE, STEEL, 0.018f, 0.012f, 0.65f);
    drawTorus({ 0, 0.65f, 0 }, { 90, 0, 0 }, ONE, STEEL, 0.012f, 0.028f);
    drawBowl({ 0, -0.02f, 0 }, NO_ROT, { 0.18f, 0.10f, 0.18f }, STEEL);
    endSphereReflect();
    resetMaterialGloss();

    glPopMatrix();
}
