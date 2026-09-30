#include "texture.h"
#include <cmath>
#include <cstring>

static GLuint texIDs[TEX_COUNT];

static unsigned char clampByte(float v) {
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}

// Simple integer hash → [0,1]
static float hashf(int x, int y) {
    unsigned int h = (unsigned int)(x * 1619u) ^ (unsigned int)(y * 31337u);
    h ^= h >> 16;
    h *= 0x45d9f3bu;
    h ^= h >> 16;
    return (float)(h & 0xFFFFu) / 65535.0f;
}

// Wood grain: horizontal bands that run along U (world X on top/front faces).
// r0,g0,b0 = base colour in [0,1].
static void genWood(unsigned char* px, int W, int H, float r0, float g0, float b0)
{
    const float PI2 = 6.28318f;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float fx = (float)x / W;
            float fy = (float)y / H;

            // Slight horizontal warp based on x position
            float warp = sinf(fx * 7.3f) * 0.04f + hashf(x * 2, 0) * 0.03f - 0.015f;
            float gy   = fy + warp;

            // Two-frequency grain bands
            float g1    = sinf(gy *  8.0f * PI2);
            float g2    = sinf(gy * 22.0f * PI2) * 0.25f;
            float grain = ((g1 + g2) / 1.25f) * 0.5f + 0.5f;   // [0,1]

            // Micro surface noise
            float n = (hashf(x, y) - 0.5f) * 0.05f;

            px[(y * W + x) * 3 + 0] = clampByte((r0 + (grain - 0.5f) * 0.20f + n) * 255);
            px[(y * W + x) * 3 + 1] = clampByte((g0 + (grain - 0.5f) * 0.14f + n) * 255);
            px[(y * W + x) * 3 + 2] = clampByte((b0 + (grain - 0.5f) * 0.07f + n) * 255);
        }
    }
}

// 4×4 ceramic tile grid with grout lines
static void genTile(unsigned char* px, int W, int H)
{
    int tW = W / 4, tH = H / 4;
    const int grout = 2;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int tx = x % tW, ty = y % tH;
            int ti = x / tW, tj = y / tH;
            bool isGrout = (tx < grout || tx >= tW - grout ||
                            ty < grout || ty >= tH - grout);

            float base, var;
            if (isGrout) {
                base = 0.30f;
                var  = (hashf(x, y) - 0.5f) * 0.02f;
            } else {
                float tileVar = (hashf(ti, tj) - 0.5f) * 0.04f;
                base = 0.60f + tileVar;
                var  = (hashf(x, y) - 0.5f) * 0.03f;
            }
            float v = base + var;
            px[(y * W + x) * 3 + 0] = clampByte((v + 0.01f) * 255);
            px[(y * W + x) * 3 + 1] = clampByte( v           * 255);
            px[(y * W + x) * 3 + 2] = clampByte((v - 0.01f) * 255);
        }
    }
}

// Cream plaster with multi-octave noise
static void genWall(unsigned char* px, int W, int H)
{
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float n = hashf(x,     y)     * 0.500f
                    + hashf(x * 2, y * 2) * 0.250f
                    + hashf(x * 4, y * 4) * 0.125f
                    + hashf(x * 8, y * 8) * 0.0625f;
            n = n / 0.9375f;         // normalize → ~[0,1]
            n = (n - 0.5f) * 0.06f; // small variation

            px[(y * W + x) * 3 + 0] = clampByte((0.93f + n)        * 255);
            px[(y * W + x) * 3 + 1] = clampByte((0.88f + n)        * 255);
            px[(y * W + x) * 3 + 2] = clampByte((0.76f + n * 0.5f) * 255);
        }
    }
}

// Dark asphalt with aggregate speckle
static void genAsphalt(unsigned char* px, int W, int H)
{
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float n = (hashf(x,     y)     * 0.50f
                     + hashf(x * 3, y * 3) * 0.30f
                     + hashf(x * 8, y * 8) * 0.20f - 0.5f) * 0.10f;
            float b = 0.22f + n;
            px[(y * W + x) * 3 + 0] = clampByte((b + 0.01f) * 255);
            px[(y * W + x) * 3 + 1] = clampByte((b + 0.01f) * 255);
            px[(y * W + x) * 3 + 2] = clampByte((b + 0.03f) * 255);
        }
    }
}

// Light concrete with expansion-joint lines every half texture width
static void genConcrete(unsigned char* px, int W, int H)
{
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int sx      = x % (W / 2);
            bool isJoint = (sx < 2 || sx >= W / 2 - 2);

            float n = (hashf(x,     y)     * 0.50f
                     + hashf(x * 2, y * 2) * 0.30f
                     + hashf(x * 6, y * 6) * 0.20f - 0.5f) * 0.07f;
            float base = (isJoint ? 0.52f : 0.70f) + n;

            px[(y * W + x) * 3 + 0] = clampByte( base          * 255);
            px[(y * W + x) * 3 + 1] = clampByte( base          * 255);
            px[(y * W + x) * 3 + 2] = clampByte((base - 0.02f) * 255);
        }
    }
}

// Warm interior environment map for sphere-map metal reflections.
// Simulates lantern glows reflected off polished steel surfaces.
static void genEnvMap(unsigned char* px, int W, int H)
{
    const float PI = 3.14159265f;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float u = (float)x / W;
            float v = (float)y / H;

            // Bright central overhead light (gaussian blob at center)
            float cx = u - 0.5f, cy = v - 0.5f;
            float d  = sqrtf(cx * cx + cy * cy);
            float center = expf(-d * 5.0f);

            // Warm lantern streaks (narrow horizontal bands mid-height)
            float s1 = expf(-fabsf(v - 0.45f) * 14.0f);
            float s2 = expf(-fabsf(v - 0.30f) * 22.0f) * 0.55f;
            float warm = (s1 + s2) * (sinf(u * PI * 3.2f) * 0.28f + 0.72f);

            // Fade at top (ceiling) and bottom (floor)
            float vf = sinf(v * PI);

            // Keep values dim so GL_ADD doesn't blow out base metal colour
            float r = (center * 0.12f + warm * 0.08f + 0.005f) * vf;
            float g = (center * 0.07f + warm * 0.04f + 0.002f) * vf;
            float b = (center * 0.03f + warm * 0.01f + 0.001f) * vf;

            px[(y * W + x) * 3 + 0] = clampByte(r * 255);
            px[(y * W + x) * 3 + 1] = clampByte(g * 255);
            px[(y * W + x) * 3 + 2] = clampByte(b * 255);
        }
    }
}

static GLuint makeGLTex(unsigned char* px, int W, int H)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, W, H, GL_RGB, GL_UNSIGNED_BYTE, px);
    return id;
}

void initTextures()
{
    const int W = 256, H = 256;
    unsigned char px[256 * 256 * 3];
    memset(texIDs, 0, sizeof(texIDs));

    genWood(px, W, H, 0.82f, 0.64f, 0.42f);   texIDs[TEX_WOOD]       = makeGLTex(px, W, H);
    genWood(px, W, H, 0.35f, 0.20f, 0.10f);   texIDs[TEX_DARK_WOOD]  = makeGLTex(px, W, H);
    genTile(px, W, H);                          texIDs[TEX_TILE_FLOOR] = makeGLTex(px, W, H);
    genWall(px, W, H);                          texIDs[TEX_WALL]       = makeGLTex(px, W, H);
    genAsphalt(px, W, H);                       texIDs[TEX_ASPHALT]    = makeGLTex(px, W, H);
    genConcrete(px, W, H);                      texIDs[TEX_CONCRETE]   = makeGLTex(px, W, H);
    genEnvMap(px, W, H);                        texIDs[TEX_ENV_MAP]    = makeGLTex(px, W, H);
}

GLuint getTexID(TexID id)
{
    if (id <= TEX_NONE || id >= TEX_COUNT) return 0;
    return texIDs[id];
}
