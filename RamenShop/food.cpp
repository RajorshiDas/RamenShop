#include "food.h"

void drawRamenNoodles(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    for (int i = 0; i < 6; i++) {
        float x = 0.04f * (i % 3) - 0.04f;
        float z = 0.03f * (i % 2) - 0.015f;
        drawTorus({ x, 0.01f * (i % 2), z }, NO_ROT, ONE, NOODLE, 0.025f, 0.08f + 0.04f * i);
    }
    for (int i = 0; i < 3; i++) {
        drawCylinderCustom({ -0.05f, 0.03f, -0.1f + 0.1f * i }, { 0, 40.0f * i, -90 }, ONE,
                           NOODLE, 0.02f, 0.02f, 0.3f);
    }
    glPopMatrix();
}

void drawEgg(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawSphere({ 0, 0, 0 },     NO_ROT, { 0.16f, 0.10f, 0.20f }, EGG_WHITE);
    drawSphere({ 0, 0.03f, 0 }, NO_ROT, { 0.09f, 0.05f, 0.11f }, YOLK);
    glPopMatrix();
}

void drawMeat(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.26f, 0.03f, 0.26f }, MEAT);
    drawTorus({ 0, 0.015f, 0 }, NO_ROT, ONE, MEAT_EDGE, 0.018f, 0.125f);
    glPopMatrix();
}

void drawSeaweed(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    drawBoard({ 0, 0, 0 }, { 90, 0, 0 }, { 0.25f, 0.3f, 0.3f }, NORI);
    glPopMatrix();
}

void drawGreenOnion(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    float spots[6][2] = { { 0, 0 }, { 0.06f, 0.03f }, { -0.05f, 0.05f },
                          { 0.03f, -0.06f }, { -0.07f, -0.02f }, { 0.09f, -0.03f } };
    for (int i = 0; i < 6; i++)
        drawTorus({ spots[i][0], 0, spots[i][1] }, NO_ROT, ONE, ONION_GREEN, 0.008f, 0.018f);
    glPopMatrix();
}

void drawRamenBowl(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Ceramic bowl with a high-gloss glaze
    setMaterialGloss(0.90f, 0.90f, 0.90f, 110.0f);
    drawBowl({ 0, 0, 0 }, NO_ROT, ONE, BOWL_RED);

    // Broth: glossy liquid surface (high specular, lower roughness)
    setMaterialPBR(Materials::Broth, BROTH);
    setMaterialGloss(0.90f, 0.75f, 0.50f, 85.0f);  // Blinn-Phong glossiness
    drawCylinder({ 0, 0.29f, 0 }, NO_ROT, { 0.83f, 0.02f, 0.83f }, BROTH);
    resetMaterialGloss();

    // Noodles: slightly rough starchy surface
    setMaterialPBR(Materials::Noodle, NOODLE);
    drawRamenNoodles({ 0, 0.31f, 0 });

    // Chashu pork: fatty, slightly reflective
    setMaterialPBR(Materials::RolledMeat, MEAT);
    drawMeat({ -0.17f, 0.33f, 0.12f }, { 10, 0, 0 });

    // Egg: smooth surface with soft highlight
    setMaterialPBR(Materials::EggYolk, EGG_WHITE);
    setMaterialGloss(0.45f, 0.45f, 0.40f, 42.0f);  // soft, diffused highlight
    drawEgg({ 0.18f, 0.32f, 0.10f }, { 0, 30, 0 });
    resetMaterialGloss();

    // Seaweed: very matte, no specular
    setMaterialPBR(Materials::SeaweedNori, NORI);
    drawSeaweed({ 0, 0.38f, -0.28f }, { -20, 0, 0 });

    // Green onion: matte vegetation
    setMaterialPBR(Materials::SeaweedNori, ONION_GREEN);
    drawGreenOnion({ 0.05f, 0.33f, -0.08f });

    resetMaterialGloss();
    glPopMatrix();
}

void drawChopsticks(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setMaterialGloss(0.70f, 0.66f, 0.60f, 90.0f);   // lacquered chopsticks
    drawCylinderCustom({ -0.5f, 0, -0.03f }, { 0, 0, -90 }, ONE, DARK_WOOD, 0.03f, 0.018f, 1.0f);
    drawCylinderCustom({ -0.5f, 0,  0.03f }, { 0, 0, -90 }, ONE, DARK_WOOD, 0.03f, 0.018f, 1.0f);
    resetMaterialGloss();
    glPopMatrix();
}

void drawSpoon(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setMaterialDielectric(100.0f);   // glazed ceramic spoon: white highlight
    drawSphere({ 0, 0.03f, 0 },    NO_ROT,         { 0.35f, 0.12f, 0.45f }, WHITE);
    drawCube({ 0, 0.07f, 0.33f }, { -15, 0, 0 },  { 0.10f, 0.04f, 0.35f }, WHITE);
    resetMaterialGloss();
    glPopMatrix();
}
