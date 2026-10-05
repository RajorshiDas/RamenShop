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

// Japanese hon-kawara roof tiles — corrugated horizontal ridges with rounded profile
static void genRoofTile(unsigned char* px, int W, int H)
{
    const float PI = 3.14159265f;
    // Warm brown clay base
    const float baseR = 0.32f, baseG = 0.22f, baseB = 0.16f;

    // Many fine corrugated ridge rows per texture repeat
    const int numRidges = 16;
    const float ridgeH  = (float)H / numRidges;

    // Individual tile segments per row
    const int tilesPerRow = 10;
    const int tileW = W / tilesPerRow;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float rowF = (float)y / ridgeH;
            int   row  = (int)rowF % numRidges;
            float t    = rowF - (int)rowF; // 0..1 within this ridge row

            // ── Rounded ridge profile ──
            // The top ~60% is the convex raised part, bottom ~40% is the
            // concave channel / shadow underneath the next row's overhang.
            float profile;
            if (t < 0.15f) {
                // Deep shadow under the overhang of the row above
                profile = -0.18f + t * 0.6f;
            } else if (t < 0.85f) {
                // Convex rounded tile surface (sin curve: dark→bright→dark)
                float s = (t - 0.15f) / 0.70f; // 0..1 across the raised part
                profile = sinf(s * PI) * 0.16f;
            } else {
                // Bottom lip edge before next shadow
                float s = (t - 0.85f) / 0.15f;
                profile = 0.02f - s * 0.12f;
            }

            // ── Tile segment divisions (subtle vertical gaps) ──
            int xShift = (row & 1) ? tileW / 2 : 0;
            int lx = ((x + xShift) % tileW + tileW) % tileW;
            int tileIdx = ((x + xShift) / tileW);

            float vertGap = 0.0f;
            if (lx < 1 || lx >= tileW - 1) {
                vertGap = -0.06f;
            }

            // ── Per-tile color variation ──
            float tileVar = (hashf(tileIdx * 3 + 17, row * 7 + 5) - 0.5f) * 0.05f;

            // ── Surface noise (fine grain) ──
            float noise = (hashf(x, y) - 0.5f) * 0.025f;
            float grain = (hashf(x / 2, y / 2) - 0.5f) * 0.02f;

            float mod = profile + vertGap + tileVar + noise + grain;

            float r = baseR + mod;
            float g = baseG + mod * 0.85f;
            float b = baseB + mod * 0.65f;

            px[(y * W + x) * 3 + 0] = clampByte(r * 255);
            px[(y * W + x) * 3 + 1] = clampByte(g * 255);
            px[(y * W + x) * 3 + 2] = clampByte(b * 255);
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

// Grass texture — smooth blurred gradient from dark green to light green
static void genGrass(unsigned char* px, int W, int H)
{
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            // Radial gradient: dark at edges, light in center
            float u = (float)x / W - 0.5f;
            float v = (float)y / H - 0.5f;
            float dist = sqrtf(u * u + v * v) * 2.0f;  // 0 center, ~1.4 corners
            if (dist > 1.0f) dist = 1.0f;
            // Smooth ease (cubic) for soft blur transition
            float t = 1.0f - dist;
            t = t * t * (3.0f - 2.0f * t);  // smoothstep

            // Dark edge → bright center
            float darkR = 0.14f, darkG = 0.26f, darkB = 0.09f;
            float litR  = 0.44f, litG  = 0.66f, litB  = 0.32f;
            float r = darkR + (litR - darkR) * t;
            float g = darkG + (litG - darkG) * t;
            float b = darkB + (litB - darkB) * t;

            // Very subtle low-frequency variation (soft patches, no sharp detail)
            float soft = (hashf(x / 16, y / 16) - 0.5f) * 0.03f;
            r += soft * 0.5f;
            g += soft;
            b += soft * 0.4f;

            px[(y * W + x) * 3 + 0] = clampByte(r * 255);
            px[(y * W + x) * 3 + 1] = clampByte(g * 255);
            px[(y * W + x) * 3 + 2] = clampByte(b * 255);
        }
    }
}

// Water texture — smooth blurred gradient from deep blue center to light edges
// with soft subtle wave modulation
static void genWater(unsigned char* px, int W, int H)
{
    const float PI = 3.14159265f;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float u = (float)x / W - 0.5f;
            float v = (float)y / H - 0.5f;
            float dist = sqrtf(u * u + v * v) * 2.0f;
            if (dist > 1.0f) dist = 1.0f;

            // Smooth ease for soft blur transition
            float t = dist;
            t = t * t * (3.0f - 2.0f * t);  // smoothstep

            // Deep blue center → lighter turquoise at edges
            float deepR = 0.08f, deepG = 0.20f, deepB = 0.45f;
            float edgeR = 0.28f, edgeG = 0.55f, edgeB = 0.70f;
            float r = deepR + (edgeR - deepR) * t;
            float g = deepG + (edgeG - deepG) * t;
            float b = deepB + (edgeB - deepB) * t;

            // Very soft, broad wave modulation (no sharp ripples)
            float wave = sinf(dist * 8.0f) * 0.025f * (1.0f - dist);
            float swell = sinf(u * 4.0f + v * 3.0f) * 0.015f;

            r += (wave + swell) * 0.5f;
            g += (wave + swell) * 0.7f;
            b += wave + swell;

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
    genRoofTile(px, W, H);                      texIDs[TEX_ROOF_TILE]  = makeGLTex(px, W, H);
    genGrass(px, W, H);                         texIDs[TEX_GRASS]      = makeGLTex(px, W, H);
    genWater(px, W, H);                         texIDs[TEX_WATER]      = makeGLTex(px, W, H);
}

GLuint getTexID(TexID id)
{
    if (id <= TEX_NONE || id >= TEX_COUNT) return 0;
    return texIDs[id];
}
