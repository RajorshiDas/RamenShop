#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>
#include "texture.h"

const float PI = 3.14159265f;
const int SLICES = 32;
const int STACKS = 24;
const float FLOOR_Y = 0.1f;

// 3D vector for position, rotation, or scale
struct Vec3 { float x, y, z; };

// RGB Color with range 0.0 - 1.0
struct Color { float r, g, b; };

const Vec3 NO_ROT = { 0, 0, 0 };
const Vec3 ONE    = { 1, 1, 1 };

// Global colors
extern const Color SKY;
extern const Color GRASS;
extern const Color ASPHALT;
extern const Color SIDEWALK;
extern const Color GRAY;
extern const Color DARK_GRAY;
extern const Color WHITE;
extern const Color BLACK;
extern const Color WALL;
extern const Color DARK_WOOD;
extern const Color WOOD;
extern const Color LIGHT_WOOD;
extern const Color FLOOR_WOOD;
extern const Color ROOF_TILE;
extern const Color RED;
extern const Color DARK_RED;
extern const Color NAVY;
extern const Color GOLD;
extern const Color PAPER;
extern const Color METAL;
extern const Color STEEL;
extern const Color CUSHION;
extern const Color CLAY_POT;
extern const Color SOIL;
extern const Color LEAF;
extern const Color LEAF_LIGHT;
extern const Color BOWL_RED;
extern const Color PLATE_WHITE;
extern const Color CUP_GREEN;
extern const Color SOY;
extern const Color BROTH;
extern const Color NOODLE;
extern const Color EGG_WHITE;
extern const Color YOLK;
extern const Color MEAT;
extern const Color MEAT_EDGE;
extern const Color NORI;
extern const Color ONION_GREEN;
extern const Color MOUNTAIN;
extern const Color TILE_FLOOR;
extern const Color UPPER_WALL;
extern const Color LANTERN_CREAM;
extern const Color LANTERN_ORANGE;
extern const Color FENCE_WOOD;
extern const Color TRUNK;
extern const Color AWNING_BLUE;
extern const Color CHROME;
extern const Color CLEAR_GLASS;
extern const Color GLASS_WATER;

// Shared global state
extern GLUquadric* quad;
extern bool showOutlines;
extern float animTime;
extern bool drawingShadow;   // true during shadow-projection pass (skips sphere map)

// Transform & material helpers
void applyTransform(Vec3 pos, Vec3 rot, Vec3 scale);
void setColor(Color c);
Color darker(Color c);
void setEmission(float r, float g, float b);
void clearEmission();
void setMaterialGloss(float r, float g, float b, float shininess);
void resetMaterialGloss();

// Phong material presets
// Dielectric (non-metal): specular highlight is WHITE regardless of surface colour.
void setMaterialDielectric(float shininess);
// Conductive (metal): specular highlight is tinted by the surface colour.
void setMaterialConductive(Color surfaceColor, float shininess);

// ========== PBR-INSPIRED MATERIAL SYSTEM ==========
// Material properties following physically-based rendering principles
struct MaterialPBR {
    float metallic;      // 0.0 = dielectric (non-metal), 1.0 = metal
    float roughness;     // 0.0 = mirror-smooth, 1.0 = full matte (affects shininess)
    float ior;           // Index of Refraction (non-metals: 1.3-1.6, metals: higher)
    Color baseColor;     // Color before lighting
};

// Common material presets (energy-conserving PBR values)
namespace Materials {
    // Food materials
    constexpr MaterialPBR Ceramic      = { 0.0f, 0.3f, 1.45f, {1.0f, 1.0f, 1.0f} };  // pottery bowl
    constexpr MaterialPBR Porcelain    = { 0.0f, 0.15f, 1.52f, {1.0f, 1.0f, 1.0f} }; // fine china
    constexpr MaterialPBR GlazedMatte  = { 0.0f, 0.45f, 1.48f, {1.0f, 1.0f, 1.0f} }; // matte glaze

    // Food texture materials
    constexpr MaterialPBR Noodle       = { 0.0f, 0.5f, 1.35f, {1.0f, 1.0f, 1.0f} };  // starchy surface
    constexpr MaterialPBR Broth        = { 0.0f, 0.8f, 1.33f, {1.0f, 1.0f, 1.0f} };  // smooth liquid
    constexpr MaterialPBR RolledMeat   = { 0.1f, 0.4f, 1.4f,  {1.0f, 1.0f, 1.0f} };  // fatty chashu
    constexpr MaterialPBR EggYolk      = { 0.15f, 0.35f, 1.38f, {1.0f, 1.0f, 1.0f} }; // slightly glossy
    constexpr MaterialPBR SeaweedNori  = { 0.0f, 0.95f, 1.32f, {1.0f, 1.0f, 1.0f} }; // very matte

    // Furniture materials
    constexpr MaterialPBR WoodPolished = { 0.0f, 0.25f, 1.5f,  {1.0f, 1.0f, 1.0f} };  // satin wood
    constexpr MaterialPBR WoodMatte    = { 0.0f, 0.6f,  1.49f, {1.0f, 1.0f, 1.0f} };  // raw wood
    constexpr MaterialPBR Fabric       = { 0.0f, 0.85f, 1.3f,  {1.0f, 1.0f, 1.0f} };  // cushion cloth
    constexpr MaterialPBR Plastic      = { 0.05f, 0.4f, 1.49f, {1.0f, 1.0f, 1.0f} };  // modern plastic

    // Metal materials
    constexpr MaterialPBR Polished     = { 0.85f, 0.1f,  2.8f, {1.0f, 1.0f, 1.0f} };  // chrome/stainless
    constexpr MaterialPBR BrushedMetal = { 0.8f, 0.35f, 2.7f,  {1.0f, 1.0f, 1.0f} };  // brushed steel
    constexpr MaterialPBR Copper       = { 0.95f, 0.25f, 2.4f, {1.0f, 1.0f, 1.0f} };  // shiny copper
    constexpr MaterialPBR Gold         = { 0.95f, 0.2f,  2.6f, {1.0f, 1.0f, 1.0f} };  // polished gold

    // Glass and transparent materials
    constexpr MaterialPBR ClearGlass   = { 0.0f, 0.05f, 1.52f, {1.0f, 1.0f, 1.0f} };  // window glass
    constexpr MaterialPBR FrostedGlass = { 0.0f, 0.6f,  1.52f, {1.0f, 1.0f, 1.0f} };  // frosted pane
    constexpr MaterialPBR Glass        = { 0.0f, 0.08f, 1.52f, {1.0f, 1.0f, 1.0f} };  // drinking glass
}

// Apply PBR material: converts metallic/roughness to OpenGL Blinn-Phong parameters
void setMaterialPBR(const MaterialPBR& mat, const Color& surfaceColor);
void setMaterialPBRMetallic(const MaterialPBR& mat, const Color& metalColor);

// Sphere-map environment reflection (ray-tracing approximation on metals).
// Call beginSphereReflect() before drawing metal geometry, endSphereReflect() after.
// Uses GL_ADD blending: adds a warm interior reflection on top of the lit surface.
void beginSphereReflect();
void endSphereReflect();

// Particle vapor effect
void drawSteam(Vec3 pos, float scale, float timeOffset);

// Basic shape drawing primitives
void drawCube(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawCuboid(Vec3 pos, Vec3 rot = NO_ROT, Vec3 size = ONE, Color c = { 1, 1, 1 });
void drawCylinderCustom(Vec3 pos, Vec3 rot, Vec3 scale, Color c, float bottomRadius, float topRadius, float height);
void drawCylinder(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawCone(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawSphere(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawTorus(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 }, float tubeRadius = 0.1f, float ringRadius = 0.4f);
void drawPlane(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawSubdividedPlane(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 }, int gridX = 24, int gridZ = 24);
void drawBoard(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawWedge(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawBowl(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawPlate(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawCup(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });
void drawBottle(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 }, Color capColor = { 0.80f, 0.10f, 0.10f });
void drawGlass(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color c = { 1, 1, 1 });

// Realistic Clear Glass components
void drawClearGlassTumbler(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, bool hasWater = true, bool hasIce = true);
void drawClearGlassPitcher(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawClearGlassJar(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color contentColor = { 0.85f, 0.20f, 0.10f });
void drawClearGlassPartition(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, float width = 5.5f, float height = 0.35f);
void drawClearGlassWindow(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, float width = 2.0f, float height = 1.5f);

// Physical light fixtures (Spotlight can, Area light luminaire panel, Pendant glass lamp)
void drawSpotlightFixture(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, bool isOn = true);
void drawSpotlightBeam(Vec3 pos, float height = 2.2f, float topRadius = 0.08f, float bottomRadius = 0.75f);
void drawAreaLightFixture(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, bool isOn = true);
void drawPendantGlassLamp(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, bool isOn = true);
// Five andon-style box lanterns hung at staggered heights from a single ceiling mount
void drawJapaneseBoxLanternCluster(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, bool isOn = true);
// Tall stacked andon floor lamp tower for a room corner
void drawJapaneseFloorLanternTower(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, bool isOn = true);

// Textured geometry (falls back to solid color when texID == 0)
void drawTexturedBox  (Vec3 pos, Vec3 rot, Vec3 size, GLuint texID, Color tint = WHITE, float uvScale = 1.0f);
void drawTexturedPlane(Vec3 pos, Vec3 rot, Vec3 scale, GLuint texID, Color tint = WHITE, float uvScale = 1.0f);
void drawTexturedWedge(Vec3 pos, Vec3 rot, Vec3 scale, GLuint texID, Color tint = WHITE, float uvScale = 1.0f);
