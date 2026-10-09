#pragma once
#include <GL/glew.h>
#include <GL/freeglut.h>

enum TexID {
    TEX_NONE = 0,
    TEX_WOOD,
    TEX_DARK_WOOD,
    TEX_TILE_FLOOR,
    TEX_WALL,
    TEX_ASPHALT,
    TEX_CONCRETE,
    TEX_ENV_MAP,    // procedural interior environment map (sphere-map reflections)
    TEX_ROOF_TILE,  // Japanese kawara clay roof tiles
    TEX_GRASS,      // realistic procedural lawn (greens, dry patches, blades, clover)
    TEX_WATER,      // wave pattern for lake surface
    TEX_CLOUD,      // soft cloud puff sprite (RGBA)
    TEX_GRASS_MACRO,// large-scale mottling (neutral grey ~0.5) that hides the lawn tiling
    TEX_TATAMI,     // woven igusa-rush tatami surface (rows run across U)
    TEX_COUNT
};

void   initTextures();
GLuint getTexID(TexID id);
