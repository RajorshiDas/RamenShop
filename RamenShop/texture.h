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
    TEX_GRASS,      // gradient grass with blade detail
    TEX_WATER,      // wave pattern for lake surface
    TEX_COUNT
};

void   initTextures();
GLuint getTexID(TexID id);
