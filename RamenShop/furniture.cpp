#include "furniture.h"
#include "objects.h"

// Glossy wood: white-ish specular strength + shininess.  Wood needs a strong,
// fairly tight highlight (0.4-0.8, shininess 60-110) to read as lacquered under
// the pendant lamps; the PBR "dielectric" presets give only ~0.02 which is
// effectively invisible.
static void woodGloss(float strength, float shininess)
{
    setMaterialGloss(strength, strength * 0.96f, strength * 0.88f, shininess);
}

void drawTable(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    woodGloss(1.00f, 55.0f);   // lacquered tabletop
    drawTexturedBox({ 0, 0.69f, 0 }, NO_ROT, { 1.2f, 0.06f, 0.8f }, getTexID(TEX_WOOD), WHITE, 1.0f);
    woodGloss(0.80f, 50.0f);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * 0.52f, 0, sz * 0.32f }, NO_ROT, { 0.07f, 0.69f, 0.07f }, DARK_WOOD);
    resetMaterialGloss();
    glPopMatrix();
}

void drawChair(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    woodGloss(0.95f, 50.0f);   // seat + backrest
    drawTexturedBox({ 0, 0.42f, 0 }, NO_ROT, { 0.45f, 0.05f, 0.45f }, getTexID(TEX_WOOD), WHITE, 1.8f);
    drawTexturedBox({ 0, 0.85f, -0.2f }, NO_ROT, { 0.45f, 0.15f, 0.04f }, getTexID(TEX_WOOD), WHITE, 1.8f);
    woodGloss(0.80f, 50.0f);   // legs + back posts
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCuboid({ sx * 0.19f, 0, sz * 0.19f }, NO_ROT, { 0.05f, 0.42f, 0.05f }, DARK_WOOD);
    drawCuboid({ -0.19f, 0.47f, -0.2f }, NO_ROT, { 0.05f, 0.5f, 0.05f }, DARK_WOOD);
    drawCuboid({  0.19f, 0.47f, -0.2f }, NO_ROT, { 0.05f, 0.5f, 0.05f }, DARK_WOOD);
    resetMaterialGloss();
    glPopMatrix();
}

void drawStool(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Wide base disc
    woodGloss(0.75f, 50.0f);
    drawCylinder({ 0, 0.02f, 0 }, NO_ROT, { 0.36f, 0.04f, 0.36f }, DARK_WOOD);

    // Thick central wooden pedestal post
    woodGloss(0.70f, 45.0f);
    drawCylinderCustom({ 0, 0.06f, 0 }, NO_ROT, ONE, WOOD, 0.065f, 0.055f, 0.64f);

    // Decorative ring at mid-height
    setMaterialPBR(Materials::WoodMatte, DARK_WOOD);
    drawTorus({ 0, 0.36f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.018f, 0.13f);

    // Round wooden seat
    woodGloss(1.00f, 55.0f);   // glossy lacquered seat top catches the pendant light
    drawCylinder({ 0, 0.70f, 0 }, NO_ROT, { 0.38f, 0.055f, 0.38f }, LIGHT_WOOD);

    resetMaterialGloss();
    glPopMatrix();
}

void drawBench(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    woodGloss(0.55f, 80.0f);
    drawCuboid({ 0, 0.42f, 0 }, NO_ROT, { 1.8f, 0.06f, 0.4f }, WOOD);
    woodGloss(0.25f, 45.0f);
    drawCuboid({ -0.75f, 0, 0 }, NO_ROT, { 0.06f, 0.42f, 0.36f }, DARK_WOOD);
    drawCuboid({  0.75f, 0, 0 }, NO_ROT, { 0.06f, 0.42f, 0.36f }, DARK_WOOD);
    drawCuboid({ 0, 0.15f, 0 }, NO_ROT, { 1.5f, 0.05f, 0.05f }, DARK_WOOD);
    resetMaterialGloss();
    glPopMatrix();
}

void drawCounter(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Main counter body: polished wood with satin finish
    woodGloss(0.65f, 40.0f);
    drawTexturedBox({ 0, 0, 0 }, NO_ROT, { 6, 1.0f, 0.6f },
                    getTexID(TEX_WOOD), WHITE, 1.0f);

    // Top surface: slightly more glossy, like finished lacquer
    woodGloss(1.00f, 60.0f);   // lacquered counter top: bright, clearly visible highlight
    drawTexturedBox({ 0, 1.0f, 0.05f }, NO_ROT, { 6.2f, 0.06f, 0.8f },
                    getTexID(TEX_WOOD), WHITE, 0.5f);

    // Support braces: dark matte wood
    resetMaterialGloss();
    setMaterialPBR(Materials::WoodMatte, DARK_WOOD);
    // (vertical dark braces on the counter front removed)

    glPopMatrix();
}

void drawShelf(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    woodGloss(0.35f, 55.0f);   // satin varnished shelf
    drawCuboid({ -1.0f, 0, 0 }, NO_ROT, { 0.04f, 0.68f, 0.3f }, DARK_WOOD);
    drawCuboid({  1.0f, 0, 0 }, NO_ROT, { 0.04f, 0.68f, 0.3f }, DARK_WOOD);
    for (int i = 0; i < 2; i++)
        drawCuboid({ 0, i * 0.6f, 0 }, NO_ROT, { 2.04f, 0.04f, 0.3f }, WOOD);
    resetMaterialGloss();
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

    float top = 0.94f;   // cabinet top surface height

    // ═══════════════════════════════════════════════════════════════════════
    //  BASE CABINETS — full width, refrigerator replaces far-right cabinet
    // ═══════════════════════════════════════════════════════════════════════
    // Prep zone (LEFT)
    drawCabinet({ -3.0f, 0, 0 });
    drawCabinet({ -2.0f, 0, 0 });
    // Cooking zone (CENTER)
    drawCabinet({ -1.0f, 0, 0 });
    drawCabinet({  0.0f, 0, 0 });
    drawCabinet({  1.0f, 0, 0 });
    // Sink / storage zone (RIGHT)
    drawCabinet({  2.0f, 0, 0 });
    drawCabinet({  3.0f, 0, 0 });

    // ═══════════════════════════════════════════════════════════════════════
    //  TILE BACKSPLASH — ceramic tiles behind cooking area
    // ═══════════════════════════════════════════════════════════════════════
    setMaterialPBR(Materials::Ceramic, PLATE_WHITE);
    drawCuboid({ 0, top + 0.30f, -0.45f }, NO_ROT, { 3.0f, 0.60f, 0.03f }, PLATE_WHITE);
    // Horizontal grout lines
    setMaterialPBR(Materials::WoodMatte, GRAY);
    for (int i = 0; i < 5; i++)
        drawCube({ 0, top + 0.04f + i * 0.14f, -0.435f }, NO_ROT,
                 { 3.0f, 0.003f, 0.001f }, GRAY);
    // Vertical grout lines
    for (int i = 0; i < 22; i++)
        drawCube({ -1.5f + i * 0.14f, top + 0.30f, -0.435f }, NO_ROT,
                 { 0.003f, 0.60f, 0.001f }, GRAY);
    resetMaterialGloss();

    // Under-cabinet warm LED strip
    setEmission(0.25f, 0.18f, 0.08f);
    drawCuboid({ 0, top + 0.58f, -0.38f }, NO_ROT,
               { 2.8f, 0.012f, 0.02f }, { 1.0f, 0.85f, 0.55f });
    clearEmission();

    // ═══════════════════════════════════════════════════════════════════════
    //  ZONE 1: PREP STATION  (LEFT, x ≈ -3.5 to -1.5)
    // ═══════════════════════════════════════════════════════════════════════

    // Stainless steel prep surface strip
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    beginSphereReflect();
    drawCuboid({ -2.5f, top, 0 }, NO_ROT, { 2.0f, 0.04f, 0.55f }, STEEL);
    endSphereReflect();
    resetMaterialGloss();

    // ── Cutting board with chef's knife ──
    setMaterialPBR(Materials::WoodPolished, LIGHT_WOOD);
    drawBoard({ -3.0f, top + 0.055f, 0.02f }, { 0, 8, 0 },
              { 0.55f, 0.6f, 0.38f }, LIGHT_WOOD);
    resetMaterialGloss();
    drawKitchenKnife({ -3.0f, top + 0.07f, 0.14f }, { 0, -15, 0 });

    // ── Ingredient containers (mise en place) ──
    drawIngredientContainer({ -2.4f, top + 0.04f,  0.12f }, NO_ROT, ONE, ONION_GREEN);
    drawIngredientContainer({ -2.2f, top + 0.04f,  0.12f }, NO_ROT, ONE, NORI);
    drawIngredientContainer({ -2.4f, top + 0.04f, -0.12f }, NO_ROT, ONE, MEAT);
    drawIngredientContainer({ -2.2f, top + 0.04f, -0.12f }, NO_ROT, ONE, NOODLE);
    drawIngredientContainer({ -2.0f, top + 0.04f,  0.00f }, NO_ROT, ONE,
                            { 0.78f, 0.68f, 0.38f });   // menma

    // ── Stacked serving bowls (ready for plating) ──
    drawStackedBowls({ -1.7f, top + 0.04f, -0.12f }, NO_ROT, ONE, 4);
    drawStackedBowls({ -1.7f, top + 0.04f,  0.12f }, NO_ROT, ONE, 3);

    // ── Wok near cooking zone border ──
    drawWok({ -1.5f, top + 0.02f, 0.05f }, { 0, 15, 0 }, { 0.6f, 0.6f, 0.6f });

    // ── Spoon rest ──
    setMaterialConductive(STEEL, 60.0f);
    drawPlate({ -1.85f, top + 0.04f, 0.0f }, NO_ROT, { 0.15f, 0.15f, 0.15f }, STEEL);
    resetMaterialGloss();

    // ═══════════════════════════════════════════════════════════════════════
    //  ZONE 2: COOKING STATION  (CENTER, x ≈ -1.3 to +1.3)
    // ═══════════════════════════════════════════════════════════════════════

    // Commercial gas stove top — centered as focal point
    drawStoveTop({ 0, top, 0 });

    // ── Two large ramen pots, one per burner (left one selectable as OBJ_KETTLE) ──
    glPushMatrix();
    glTranslatef(-0.55f, top + 0.07f, 0);
    applyObjDelta(OBJ_KETTLE);
    drawLargeRamenPot({ 0, 0, 0 });
    glPopMatrix();

    drawLargeRamenPot({ 0.55f, top + 0.07f, 0 });

    // ═══════════════════════════════════════════════════════════════════════
    //  ZONE 3: SINK STATION  (RIGHT, x ≈ +1.8 to +3.0)
    // ═══════════════════════════════════════════════════════════════════════

    drawSink({ 2.3f, top, 0 });

    // ── Dish drying rack next to sink ──
    drawDishRack({ 2.85f, top + 0.01f, 0 });

    // ── Soap dispenser ──
    setMaterialPBR(Materials::Plastic, WHITE);
    drawCylinder({ 1.95f, top + 0.01f, 0.18f }, NO_ROT, { 0.04f, 0.10f, 0.04f }, WHITE);
    drawSphere({ 1.95f, top + 0.115f, 0.18f }, NO_ROT, { 0.025f, 0.015f, 0.025f }, STEEL);
    resetMaterialGloss();

    // ── Stacked plates ──
    setMaterialPBR(Materials::Porcelain, PLATE_WHITE);
    for (int i = 0; i < 5; i++)
        drawPlate({ 3.2f, top + i * 0.022f, 0 }, NO_ROT,
                  { 0.30f, 0.30f, 0.30f }, PLATE_WHITE);
    resetMaterialGloss();

    // ═══════════════════════════════════════════════════════════════════════
    //  ZONE 4: REFRIGERATOR  (FAR RIGHT)
    // ═══════════════════════════════════════════════════════════════════════
    drawRefrigerator({ 3.8f, 0, 0 });

    // ═══════════════════════════════════════════════════════════════════════
    //  WALL-MOUNTED EQUIPMENT  (above counter height)
    // ═══════════════════════════════════════════════════════════════════════

    // ── Exhaust hood centered over cooking station ──
    // Hood must clear the tops of the pots (rim ~ y 1.58): bottom lip at 1.95-0.08
    drawKitchenHood({ 0, 1.95f, 0 });

    // ── Wall shelves (flanking the hood) ──
    float shelfY = 1.65f;
    drawShelf({ -2.5f, shelfY, -0.43f });
    drawShelf({  2.5f, shelfY, -0.43f });

    // Items on left shelf: red bowls (lower), plates (upper)
    for (int i = 0; i < 3; i++) {
        drawBowl({ -3.1f + i * 0.45f, shelfY + 0.04f, -0.43f },
                 NO_ROT, { 0.25f, 0.25f, 0.25f }, BOWL_RED);
        drawPlate({ -3.1f + i * 0.45f, shelfY + 0.64f, -0.43f },
                  NO_ROT, { 0.30f, 0.30f, 0.30f }, PLATE_WHITE);
    }
    // Items on right shelf: cups (lower), bottles (upper)
    for (int i = 0; i < 3; i++) {
        drawCup({ 1.9f + i * 0.45f, shelfY + 0.04f, -0.43f },
                NO_ROT, { 0.12f, 0.12f, 0.12f }, STEEL);
        drawBottle({ 1.9f + i * 0.45f, shelfY + 0.64f, -0.43f },
                   NO_ROT, { 0.30f, 0.30f, 0.30f },
                   (i % 2 == 0) ? SOY : CUP_GREEN);
    }
    // Clear glass spice jars
    drawClearGlassJar({ -1.3f, shelfY + 0.04f, -0.43f }, NO_ROT,
                      { 0.20f, 0.20f, 0.20f }, { 0.85f, 0.25f, 0.10f });
    drawClearGlassJar({ -1.05f, shelfY + 0.04f, -0.43f }, NO_ROT,
                      { 0.20f, 0.20f, 0.20f }, { 0.15f, 0.45f, 0.15f });

    // ── Utensil hanging rail ──
    float railY = shelfY + 0.55f;
    drawCylinder({ -0.5f, railY, -0.52f }, { 0, 0, -90 },
                 { 0.02f, 2.0f, 0.02f }, DARK_WOOD);
    // Hanging utensils
    drawLadle({    -1.3f, railY - 0.04f, -0.48f }, NO_ROT, { 0.7f, 0.7f, 0.7f });
    drawSpatula({  -0.8f, railY - 0.04f, -0.48f }, NO_ROT, { 0.7f, 0.7f, 0.7f });
    drawTongs({    -0.3f, railY - 0.04f, -0.48f }, { 0, 15, 0 }, { 0.7f, 0.7f, 0.7f });
    drawStrainer({  0.2f, railY - 0.04f, -0.48f }, NO_ROT, { 0.6f, 0.6f, 0.6f });
    drawLadle({     0.7f, railY - 0.04f, -0.48f }, { 0, -10, 0 }, { 0.6f, 0.6f, 0.6f });

    // ── Sauce bottles on wall-mounted shelf (left side) ──
    setMaterialPBR(Materials::WoodMatte, DARK_WOOD);
    drawCuboid({ -2.5f, 1.45f, -0.48f }, NO_ROT, { 1.0f, 0.03f, 0.15f }, DARK_WOOD);
    resetMaterialGloss();
    drawBottle({ -2.85f, 1.48f, -0.48f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, SOY);
    drawBottle({ -2.65f, 1.48f, -0.48f }, NO_ROT, { 0.22f, 0.22f, 0.22f }, RED, GOLD);
    drawBottle({ -2.45f, 1.48f, -0.48f }, NO_ROT, { 0.22f, 0.22f, 0.22f },
               { 0.30f, 0.20f, 0.10f });   // sesame oil
    drawBottle({ -2.25f, 1.48f, -0.48f }, NO_ROT, { 0.22f, 0.22f, 0.22f },
               { 0.85f, 0.78f, 0.55f });   // mirin

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

    // Bamboo/wood handle
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCylinderCustom({ 0, 0, 0 }, NO_ROT, ONE, DARK_WOOD, 0.018f, 0.012f, 0.65f);
    drawTorus({ 0, 0.65f, 0 }, { 90, 0, 0 }, ONE, DARK_WOOD, 0.012f, 0.028f);
    resetMaterialGloss();

    // Steel bowl head
    setMaterialConductive(STEEL, 75.0f);
    beginSphereReflect();
    drawBowl({ 0, -0.02f, 0 }, NO_ROT, { 0.18f, 0.10f, 0.18f }, STEEL);
    endSphereReflect();
    resetMaterialGloss();

    glPopMatrix();
}

void drawSpatula(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Wood handle
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCylinderCustom({ 0, 0, 0 }, NO_ROT, ONE, DARK_WOOD, 0.014f, 0.010f, 0.50f);
    drawTorus({ 0, 0.50f, 0 }, { 90, 0, 0 }, ONE, DARK_WOOD, 0.008f, 0.022f);
    resetMaterialGloss();

    // Steel blade
    setMaterialConductive(STEEL, 75.0f);
    beginSphereReflect();
    drawCube({ 0, -0.06f, 0 }, NO_ROT, { 0.08f, 0.12f, 0.012f }, STEEL);
    endSphereReflect();
    resetMaterialGloss();
    glPopMatrix();
}

void drawStrainer(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Wood handle
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCylinderCustom({ 0, 0.12f, 0 }, NO_ROT, ONE, DARK_WOOD, 0.014f, 0.010f, 0.45f);
    drawTorus({ 0, 0.57f, 0 }, { 90, 0, 0 }, ONE, DARK_WOOD, 0.008f, 0.022f);
    resetMaterialGloss();

    // Steel strainer basket
    setMaterialConductive(STEEL, 75.0f);
    beginSphereReflect();
    drawCylinder({ 0, 0, 0 }, NO_ROT, { 0.20f, 0.02f, 0.20f }, STEEL);
    drawTorus({ 0, 0.02f, 0 }, NO_ROT, ONE, STEEL, 0.008f, 0.10f);
    endSphereReflect();
    resetMaterialGloss();
    glPopMatrix();
}

void drawTongs(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // All-wood tongs (traditional bamboo)
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCylinderCustom({ -0.02f, 0, 0 }, { 0, 0, -3 }, ONE, DARK_WOOD, 0.010f, 0.008f, 0.50f);
    drawCylinderCustom({  0.02f, 0, 0 }, { 0, 0,  3 }, ONE, DARK_WOOD, 0.010f, 0.008f, 0.50f);
    drawTorus({ 0, 0.48f, 0 }, { 90, 0, 0 }, ONE, DARK_WOOD, 0.006f, 0.018f);
    drawCube({ -0.035f, -0.02f, 0 }, NO_ROT, { 0.02f, 0.05f, 0.025f }, DARK_WOOD);
    drawCube({  0.035f, -0.02f, 0 }, NO_ROT, { 0.02f, 0.05f, 0.025f }, DARK_WOOD);
    resetMaterialGloss();
    glPopMatrix();
}

void drawWok(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);
    setMaterialConductive(DARK_GRAY, 60.0f);
    beginSphereReflect();
    drawBowl({ 0, 0, 0 }, NO_ROT, { 0.50f, 0.22f, 0.50f }, DARK_GRAY);
    drawCube({ -0.26f, 0.12f, 0 }, { 0, 0, -15 }, { 0.08f, 0.03f, 0.03f }, DARK_WOOD);
    drawCube({  0.26f, 0.12f, 0 }, { 0, 0,  15 }, { 0.08f, 0.03f, 0.03f }, DARK_WOOD);
    endSphereReflect();
    resetMaterialGloss();
    glPopMatrix();
}

void drawKitchenHood(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Main hood body — brushed stainless steel
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    beginSphereReflect();
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 2.4f, 0.08f, 0.60f }, STEEL);
    // Flared capture lip
    drawCuboid({ 0, -0.08f, 0 }, NO_ROT, { 2.6f, 0.04f, 0.70f }, STEEL);
    // Angled side panels
    drawCube({ -1.25f, -0.04f, 0 }, { 0, 0,  8 }, { 0.03f, 0.15f, 0.65f }, STEEL);
    drawCube({  1.25f, -0.04f, 0 }, { 0, 0, -8 }, { 0.03f, 0.15f, 0.65f }, STEEL);
    endSphereReflect();

    // Exhaust duct — OUTSIDE sphere reflect to avoid golden reflection artifact
    resetMaterialGloss();
    setMaterialPBR(Materials::WoodMatte, DARK_GRAY);
    drawCuboid({ 0, 0.08f, 0 }, NO_ROT, { 0.35f, 0.45f, 0.30f }, DARK_GRAY);

    // Angled grease filter baffles (aluminum)
    setMaterialPBRMetallic(Materials::BrushedMetal, METAL);
    for (float gx = -0.9f; gx <= 0.91f; gx += 0.20f)
        drawCube({ gx, -0.11f, 0 }, { 12, 0, 0 }, { 0.18f, 0.006f, 0.45f }, METAL);

    // Warm LED task light strip under hood
    setEmission(0.30f, 0.22f, 0.10f);
    drawCuboid({ 0, -0.12f, 0.25f }, NO_ROT,
               { 1.8f, 0.01f, 0.02f }, { 1.0f, 0.90f, 0.65f });
    clearEmission();

    resetMaterialGloss();
    glPopMatrix();
}

// ═══════════════════════════════════════════════════════════════════════════════
//   MODULAR KITCHEN EQUIPMENT
// ═══════════════════════════════════════════════════════════════════════════════

// ── Commercial Gas Stove Top ────────────────────────────────────────────────
// 2-burner gas range with cast-iron grates, gas flame glow, control knobs and
// a plain stainless back panel.
void drawStoveTop(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Stainless steel cooking surface
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    beginSphereReflect();
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 1.9f, 0.06f, 0.60f }, STEEL);
    endSphereReflect();

    // 2 detailed gas burners (modular)
    for (int i = 0; i < 2; i++) {
        float bx = -0.55f + i * 1.10f;
        drawGasBurner({ bx, 0, 0 });
    }

    // Control knobs on front face with chrome mounting rings
    for (int i = 0; i < 2; i++) {
        float kx = -0.55f + i * 1.10f;
        // Chrome mounting ring
        setMaterialConductive(CHROME, 90.0f);
        drawTorus({ kx, 0.03f, 0.28f }, { 90, 0, 0 }, ONE, CHROME, 0.004f, 0.042f);
        // Black plastic knob
        setMaterialPBR(Materials::Plastic, BLACK);
        drawCylinder({ kx, 0.03f, 0.30f }, { 90, 0, 0 },
                     { 0.035f, 0.025f, 0.035f }, BLACK);
        // Indicator dot
        drawSphere({ kx, 0.035f, 0.325f }, NO_ROT,
                   { 0.008f, 0.008f, 0.004f }, WHITE);
    }

    // Plain back panel
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    drawCuboid({ 0, 0.06f, -0.30f }, NO_ROT, { 1.9f, 0.14f, 0.03f }, STEEL);

    resetMaterialGloss();
    glPopMatrix();
}

// ── Large Professional Ramen Pot ────────────────────────────────────────────
// Oversized commercial pot for simmering tonkotsu/miso broth — polished
// stainless steel with thick rim, two welded handles, and glossy broth inside.
void drawLargeRamenPot(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Main stainless steel vessel — real metal: dark diffuse body, strong bright specular
    // (metals reflect most light as specular, very little as diffuse)
    const Color POT_STEEL = { 0.42f, 0.43f, 0.46f };
    setMaterialGloss(1.0f, 1.0f, 1.0f, 90.0f);
    beginSphereReflect();
    drawCylinderCustom({ 0, 0, 0 }, NO_ROT, ONE, POT_STEEL, 0.30f, 0.32f, 0.55f);
    endSphereReflect();

    // Broth liquid inside — glossy warm surface
    setMaterialPBR(Materials::Broth, BROTH);
    setMaterialGloss(0.85f, 0.70f, 0.45f, 70.0f);
    drawCylinder({ 0, 0.53f, 0 }, NO_ROT, { 0.58f, 0.008f, 0.58f }, BROTH);

    // Fat/foam ring around broth edge
    setMaterialPBR(Materials::Noodle, { 0.95f, 0.90f, 0.75f });
    drawTorus({ 0, 0.535f, 0 }, NO_ROT, ONE,
              { 0.95f, 0.90f, 0.75f }, 0.012f, 0.27f);

    // Thick rolled rim — polished stainless
    setMaterialGloss(1.0f, 1.0f, 1.0f, 120.0f);
    beginSphereReflect();
    drawTorus({ 0, 0.55f, 0 }, NO_ROT, ONE, { 0.50f, 0.51f, 0.54f }, 0.025f, 0.32f);
    endSphereReflect();

    // Two welded handles with riveted mounting plates
    resetMaterialGloss();
    setMaterialConductive(DARK_GRAY, 55.0f);
    drawTorus({ -0.35f, 0.42f, 0 }, { 90, 0, 0 }, ONE, DARK_GRAY, 0.018f, 0.07f);
    drawTorus({  0.35f, 0.42f, 0 }, { 90, 0, 0 }, ONE, DARK_GRAY, 0.018f, 0.07f);
    // Mounting plates
    drawCube({ -0.34f, 0.42f, 0 }, NO_ROT, { 0.05f, 0.07f, 0.02f }, DARK_GRAY);
    drawCube({  0.34f, 0.42f, 0 }, NO_ROT, { 0.05f, 0.07f, 0.02f }, DARK_GRAY);
    // Rivets
    setMaterialConductive(STEEL, 80.0f);
    drawSphere({ -0.34f, 0.44f, 0.011f }, NO_ROT, { 0.006f, 0.006f, 0.006f }, STEEL);
    drawSphere({ -0.34f, 0.40f, 0.011f }, NO_ROT, { 0.006f, 0.006f, 0.006f }, STEEL);
    drawSphere({  0.34f, 0.44f, 0.011f }, NO_ROT, { 0.006f, 0.006f, 0.006f }, STEEL);
    drawSphere({  0.34f, 0.40f, 0.011f }, NO_ROT, { 0.006f, 0.006f, 0.006f }, STEEL);

    resetMaterialGloss();
    glPopMatrix();
}

// ── Mise en Place Ingredient Container ──────────────────────────────────────
// Small stainless steel rectangular bain-marie insert filled with a topping.
void drawIngredientContainer(Vec3 pos, Vec3 rot, Vec3 scale, Color fill)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Stainless steel individual walls (more depth than a single cuboid)
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.16f, 0.01f, 0.12f }, STEEL);       // bottom
    drawCube({ 0, 0.04f,  0.055f }, NO_ROT, { 0.16f, 0.08f, 0.008f }, STEEL); // front
    drawCube({ 0, 0.04f, -0.055f }, NO_ROT, { 0.16f, 0.08f, 0.008f }, STEEL); // back
    drawCube({ -0.075f, 0.04f, 0 }, NO_ROT, { 0.008f, 0.08f, 0.12f }, STEEL); // left
    drawCube({  0.075f, 0.04f, 0 }, NO_ROT, { 0.008f, 0.08f, 0.12f }, STEEL); // right
    // Rolled rim
    drawCuboid({ 0, 0.08f, 0 }, NO_ROT, { 0.17f, 0.006f, 0.13f }, STEEL);
    resetMaterialGloss();

    // Food contents — flat fill + mounded top
    setMaterialPBR(Materials::SeaweedNori, fill);
    drawCuboid({ 0, 0.02f, 0 }, NO_ROT, { 0.14f, 0.05f, 0.10f }, fill);
    drawSphere({ 0, 0.07f, 0 }, NO_ROT, { 0.08f, 0.03f, 0.06f }, fill);
    resetMaterialGloss();

    glPopMatrix();
}

// ── Stacked Serving Bowls ───────────────────────────────────────────────────
// A neat stack of ceramic bowls ready for service, count configurable.
void drawStackedBowls(Vec3 pos, Vec3 rot, Vec3 scale, int count)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Traditional red ramen bowls with matte glaze
    setMaterialPBR(Materials::GlazedMatte, BOWL_RED);
    for (int i = 0; i < count; i++) {
        drawBowl({ 0, i * 0.045f, 0 }, NO_ROT,
                 { 0.28f, 0.28f, 0.28f }, BOWL_RED);
    }
    resetMaterialGloss();

    glPopMatrix();
}

// ── Commercial Upright Refrigerator ─────────────────────────────────────────
// Tall stainless steel commercial fridge with single door, vertical handle,
// rubber gasket, and manufacturer badge.
void drawRefrigerator(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Main body — brushed stainless steel
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    beginSphereReflect();
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.70f, 2.05f, 0.60f }, STEEL);
    endSphereReflect();

    // Top cap
    drawCuboid({ 0, 2.05f, 0 }, NO_ROT, { 0.72f, 0.04f, 0.62f }, DARK_GRAY);

    // Door panel — slightly raised face
    setMaterialPBRMetallic(Materials::Polished, STEEL);
    beginSphereReflect();
    drawCube({ 0, 1.05f, 0.305f }, NO_ROT, { 0.62f, 1.90f, 0.015f }, STEEL);
    endSphereReflect();

    // Vertical door handle — polished chrome bar
    setMaterialConductive(CHROME, 95.0f);
    beginSphereReflect();
    drawCylinder({ -0.24f, 0.55f, 0.32f }, NO_ROT, { 0.015f, 1.0f, 0.015f }, CHROME);
    // Handle mounting brackets
    drawCube({ -0.24f, 1.50f, 0.33f }, NO_ROT, { 0.04f, 0.03f, 0.025f }, CHROME);
    drawCube({ -0.24f, 0.60f, 0.33f }, NO_ROT, { 0.04f, 0.03f, 0.025f }, CHROME);
    endSphereReflect();

    // Rubber door gasket (thin dark line around door edge)
    resetMaterialGloss();
    drawCube({ 0, 1.05f, 0.298f }, NO_ROT, { 0.66f, 1.96f, 0.004f }, BLACK);

    // Manufacturer badge — small metallic disc at top center
    setMaterialConductive(CHROME, 80.0f);
    drawCylinder({ 0, 1.85f, 0.315f }, { 90, 0, 0 }, { 0.04f, 0.004f, 0.04f }, CHROME);

    // Green LED temperature display
    setEmission(0.05f, 0.35f, 0.05f);
    drawCube({ 0.15f, 1.75f, 0.32f }, NO_ROT,
             { 0.08f, 0.03f, 0.005f }, { 0.10f, 0.85f, 0.10f });
    clearEmission();

    // Ventilation grille at bottom
    for (int i = 0; i < 4; i++)
        drawCube({ -0.15f + i * 0.10f, 0.06f, 0.31f }, NO_ROT,
                 { 0.06f, 0.005f, 0.01f }, DARK_GRAY);

    // 4 chrome leveling feet
    setMaterialConductive(CHROME, 70.0f);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            drawCylinder({ sx * 0.28f, -0.02f, sz * 0.22f }, NO_ROT,
                         { 0.025f, 0.02f, 0.025f }, CHROME);

    resetMaterialGloss();
    glPopMatrix();
}

// ── Dish Drying Rack ────────────────────────────────────────────────────────
// Stainless steel wire rack with a few plates and a bowl leaning to dry.
void drawDishRack(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    setMaterialConductive(STEEL, 65.0f);

    // Drip tray with raised rim on all 4 sides
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.40f, 0.012f, 0.28f }, STEEL);
    drawCuboid({ 0, 0.012f, -0.135f }, NO_ROT, { 0.40f, 0.015f, 0.01f }, STEEL);
    drawCuboid({ 0, 0.012f,  0.135f }, NO_ROT, { 0.40f, 0.015f, 0.01f }, STEEL);
    drawCuboid({ -0.195f, 0.012f, 0 }, NO_ROT, { 0.01f, 0.015f, 0.28f }, STEEL);
    drawCuboid({  0.195f, 0.012f, 0 }, NO_ROT, { 0.01f, 0.015f, 0.28f }, STEEL);

    // 8 wire rack slots
    for (int i = 0; i < 8; i++) {
        float wx = -0.16f + i * 0.045f;
        drawCylinder({ wx, 0.015f, 0 }, NO_ROT, { 0.004f, 0.18f, 0.004f }, STEEL);
    }
    // Connecting horizontal bars
    drawCylinder({ 0, 0.19f, 0 }, { 0, 0, -90 }, { 0.004f, 0.32f, 0.004f }, STEEL);
    drawCylinder({ 0, 0.10f, 0 }, { 0, 0, -90 }, { 0.004f, 0.32f, 0.004f }, STEEL);

    resetMaterialGloss();

    // A few plates leaning in the rack
    setMaterialPBR(Materials::Porcelain, PLATE_WHITE);
    drawPlate({ -0.10f, 0.04f, 0 }, { 0, 0, 8 }, { 0.18f, 0.18f, 0.18f }, PLATE_WHITE);
    drawPlate({ -0.02f, 0.04f, 0 }, { 0, 0, 6 }, { 0.18f, 0.18f, 0.18f }, PLATE_WHITE);
    drawPlate({  0.06f, 0.04f, 0 }, { 0, 0, 10 }, { 0.16f, 0.16f, 0.16f }, PLATE_WHITE);
    resetMaterialGloss();

    // A small bowl at the end
    drawBowl({ 0.14f, 0.02f, 0 }, { 0, 0, 5 }, { 0.14f, 0.14f, 0.14f }, PLATE_WHITE);

    glPopMatrix();
}

// ── Chef's Knife ────────────────────────────────────────────────────────────
// Japanese gyuto-style chef's knife with dark wood handle and polished blade.
void drawKitchenKnife(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Dark wood handle
    setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.12f, 0.022f, 0.024f }, DARK_WOOD);
    resetMaterialGloss();

    // Kashira (end cap) — metal butt
    setMaterialConductive(DARK_GRAY, 60.0f);
    drawCube({ -0.065f, 0.013f, 0 }, NO_ROT, { 0.012f, 0.025f, 0.026f }, DARK_GRAY);

    // Blade bolster — metal collar
    setMaterialConductive(STEEL, 90.0f);
    drawCube({ 0.065f, 0.013f, 0 }, NO_ROT, { 0.015f, 0.025f, 0.025f }, STEEL);

    // Steel blade — long, tapered
    setMaterialPBRMetallic(Materials::Polished, STEEL);
    beginSphereReflect();
    drawCube({ 0.20f, 0.015f, 0 }, NO_ROT, { 0.26f, 0.05f, 0.004f }, STEEL);
    endSphereReflect();

    // Blade tip
    drawCube({ 0.34f, 0.022f, 0 }, NO_ROT, { 0.04f, 0.030f, 0.003f }, STEEL);

    // Cutting edge detail (thin dark line along bottom)
    drawCube({ 0.20f, -0.008f, 0 }, NO_ROT, { 0.26f, 0.002f, 0.005f }, DARK_GRAY);

    resetMaterialGloss();
    glPopMatrix();
}

// ── Order Ticket Rail ───────────────────────────────────────────────────────
// Horizontal stainless bar with spring clips holding paper order tickets,
// mounted above the pass-through window between kitchen and counter.
void drawTicketRail(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Horizontal rail bar
    setMaterialConductive(STEEL, 70.0f);
    drawCylinder({ 0, 0, 0 }, { 0, 0, -90 }, { 0.012f, 1.6f, 0.012f }, STEEL);
    // Chrome end brackets
    setMaterialConductive(CHROME, 85.0f);
    drawCube({ -0.80f, 0, 0 }, NO_ROT, { 0.04f, 0.04f, 0.03f }, CHROME);
    drawCube({  0.80f, 0, 0 }, NO_ROT, { 0.04f, 0.04f, 0.03f }, CHROME);
    resetMaterialGloss();

    // Spring clips with paper tickets (5 tickets)
    for (int i = 0; i < 5; i++) {
        float tx = -0.55f + i * 0.28f;
        // Clip
        drawCube({ tx, -0.02f, 0 }, NO_ROT, { 0.03f, 0.025f, 0.015f }, METAL);
        // Paper ticket hanging below
        setEmission(0.04f, 0.03f, 0.02f);
        drawBoard({ tx, -0.10f, 0.005f }, NO_ROT, { 0.10f, 0.12f, 0.12f }, PAPER);
        clearEmission();
        // Red text marks on ticket
        drawCube({ tx - 0.02f, -0.08f, 0.006f }, NO_ROT,
                 { 0.04f, 0.003f, 0.001f }, RED);
        drawCube({ tx + 0.01f, -0.11f, 0.006f }, NO_ROT,
                 { 0.035f, 0.003f, 0.001f }, RED);
    }

    glPopMatrix();
}

// ═══════════════════════════════════════════════════════════════════════════════
//   NEW MODULAR KITCHEN EQUIPMENT
// ═══════════════════════════════════════════════════════════════════════════════

// ── Individual Gas Burner ───────────────────────────────────────────────────
// Detailed single burner: cast-iron grate with 6 radial bars, copper burner
// head with gas port holes, blue flame ring, and orange flame tips.
void drawGasBurner(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Cast-iron grate — outer ring
    setMaterialPBR(Materials::WoodMatte, BLACK);
    drawTorus({ 0, 0.065f, 0 }, NO_ROT, ONE, BLACK, 0.016f, 0.20f);
    // Inner support ring
    drawTorus({ 0, 0.065f, 0 }, NO_ROT, ONE, BLACK, 0.010f, 0.10f);
    // 6 radial grate bars
    for (int r = 0; r < 6; r++) {
        float a = r * 30.0f;
        drawCube({ 0, 0.07f, 0 }, { 0, a, 0 },
                 { 0.38f, 0.012f, 0.014f }, BLACK);
    }

    // Copper/brass burner head (center disc)
    setMaterialPBRMetallic(Materials::Copper, { 0.72f, 0.45f, 0.20f });
    drawCylinder({ 0, 0.03f, 0 }, NO_ROT,
                 { 0.10f, 0.02f, 0.10f }, { 0.72f, 0.45f, 0.20f });
    // Gas port holes around burner head
    for (int h = 0; h < 12; h++) {
        float a = h * 30.0f * PI / 180.0f;
        float hx = cosf(a) * 0.08f;
        float hz = sinf(a) * 0.08f;
        drawCylinder({ hx, 0.035f, hz }, NO_ROT,
                     { 0.008f, 0.008f, 0.008f }, BLACK);
    }

    // Blue flame ring
    setEmission(0.10f, 0.18f, 0.50f);
    drawTorus({ 0, 0.045f, 0 }, NO_ROT, ONE,
              { 0.12f, 0.22f, 0.65f }, 0.006f, 0.13f);
    clearEmission();

    // Orange flame tips (subtle, above blue ring)
    setEmission(0.35f, 0.15f, 0.02f);
    for (int f = 0; f < 8; f++) {
        float a = f * 45.0f * PI / 180.0f;
        float fx = cosf(a) * 0.13f;
        float fz = sinf(a) * 0.13f;
        drawSphere({ fx, 0.055f, fz }, NO_ROT,
                   { 0.012f, 0.018f, 0.012f }, { 1.0f, 0.55f, 0.10f });
    }
    clearEmission();

    resetMaterialGloss();
    glPopMatrix();
}

// ── Noodle Boiling Station ──────────────────────────────────────────────────
// Rectangular stainless steel boiling tank with semi-transparent water,
// 3 mesh noodle baskets with wood handles, temperature dial, and gas pipe.
void drawNoodleStation(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Rectangular stainless steel boiling tank
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    beginSphereReflect();
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 0.55f, 0.28f, 0.45f }, STEEL);
    // Rim
    drawCuboid({ 0, 0.28f, 0 }, NO_ROT, { 0.58f, 0.02f, 0.48f }, STEEL);
    endSphereReflect();

    // Semi-transparent water surface
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.75f, 0.82f, 0.88f, 0.45f);
    drawPlane({ 0, 0.26f, 0 }, NO_ROT, { 0.50f, 1.0f, 0.40f }, GLASS_WATER);
    glDisable(GL_BLEND);

    // 3 mesh noodle baskets
    for (int b = 0; b < 3; b++) {
        float bx = -0.16f + b * 0.16f;
        // Wire mesh basket cylinder
        setMaterialConductive(STEEL, 70.0f);
        drawCylinderCustom({ bx, 0.10f, 0 }, NO_ROT, ONE,
                           STEEL, 0.06f, 0.07f, 0.22f);
        // Mesh ring at top
        drawTorus({ bx, 0.32f, 0 }, NO_ROT, ONE, STEEL, 0.005f, 0.07f);
        // Mesh ring at middle
        drawTorus({ bx, 0.22f, 0 }, NO_ROT, ONE, STEEL, 0.003f, 0.065f);
        // Dark wood handle sticking up
        setMaterialPBR(Materials::WoodPolished, DARK_WOOD);
        drawCylinderCustom({ bx, 0.32f, 0 }, NO_ROT, ONE,
                           DARK_WOOD, 0.012f, 0.010f, 0.20f);
        resetMaterialGloss();
    }

    // Temperature control dial on front
    setMaterialPBR(Materials::Plastic, BLACK);
    drawCylinder({ -0.20f, 0.14f, 0.24f }, { 90, 0, 0 },
                 { 0.03f, 0.02f, 0.03f }, BLACK);
    // Chrome bezel
    setMaterialConductive(CHROME, 85.0f);
    drawTorus({ -0.20f, 0.14f, 0.23f }, { 90, 0, 0 }, ONE,
              CHROME, 0.003f, 0.035f);

    // Gas supply pipe on back
    setMaterialPBR(Materials::WoodMatte, DARK_GRAY);
    drawCylinder({ 0, 0.08f, -0.25f }, { 0, 0, -90 },
                 { 0.02f, 0.40f, 0.02f }, DARK_GRAY);

    resetMaterialGloss();
    glPopMatrix();
}

// ── Serving Counter (Pass-Through Ledge) ────────────────────────────────────
// Stainless steel pass-through ledge with drip guard, L-bracket supports,
// and overhead heat lamp with chrome reflector.
void drawServingCounter(Vec3 pos, Vec3 rot, Vec3 scale)
{
    glPushMatrix();
    applyTransform(pos, rot, scale);

    // Stainless steel pass-through ledge
    setMaterialPBRMetallic(Materials::BrushedMetal, STEEL);
    beginSphereReflect();
    drawCuboid({ 0, 0, 0 }, NO_ROT, { 3.0f, 0.03f, 0.25f }, STEEL);
    // Drip guard lip at front edge
    drawCuboid({ 0, 0.03f, 0.12f }, NO_ROT, { 3.0f, 0.025f, 0.01f }, STEEL);
    endSphereReflect();

    // L-bracket supports underneath
    setMaterialConductive(STEEL, 60.0f);
    for (float bx = -1.2f; bx <= 1.21f; bx += 0.8f) {
        drawCube({ bx, -0.10f, 0.08f }, NO_ROT,
                 { 0.03f, 0.10f, 0.015f }, STEEL);
        drawCube({ bx, -0.10f, 0.02f }, NO_ROT,
                 { 0.03f, 0.015f, 0.12f }, STEEL);
    }

    // Overhead heat lamp (centered)
    // Aluminum housing
    setMaterialPBRMetallic(Materials::BrushedMetal, METAL);
    drawCylinderCustom({ 0, 0.30f, 0 }, NO_ROT, ONE,
                       METAL, 0.08f, 0.06f, 0.12f);
    // Chrome reflector cone
    setMaterialConductive(CHROME, 90.0f);
    beginSphereReflect();
    drawCylinderCustom({ 0, 0.28f, 0 }, NO_ROT, ONE,
                       CHROME, 0.10f, 0.04f, 0.06f);
    endSphereReflect();
    // Warm orange emissive bulb
    setEmission(0.50f, 0.30f, 0.08f);
    drawSphere({ 0, 0.30f, 0 }, NO_ROT,
               { 0.03f, 0.04f, 0.03f }, { 1.0f, 0.85f, 0.50f });
    clearEmission();
    // Support arm
    setMaterialPBR(Materials::WoodMatte, DARK_GRAY);
    drawCylinder({ 0, 0.42f, 0 }, NO_ROT,
                 { 0.015f, 0.18f, 0.015f }, DARK_GRAY);

    resetMaterialGloss();
    glPopMatrix();
}
