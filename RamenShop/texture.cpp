#include "texture.h"
#include <cmath>
#include <cstring>
#include <vector>

static GLuint texIDs[TEX_COUNT];

static float fbmTile(float u, float v, int baseFreq, int octaves);
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
    // Wooden planks: 6 boards across the texture, long grain streaks along U, a dark seam
    // between boards, a slightly different tone per board, and a few knots / end joints.
    const int PLANKS = 6;
    for (int y = 0; y < H; y++) {
        int plank = y * PLANKS / H;
        float inPlank = (float)(y * PLANKS % H) / H;                       // 0..1 across one board
        float tone = 0.86f + 0.28f * hashf(plank * 17 + 3, 5);             // board colour variation
        float seam = (inPlank < 0.035f || inPlank > 0.975f) ? 0.45f : 1.0f; // dark gap between boards
        float shift = hashf(plank * 31 + 7, 9);                             // each board's grain is offset
        for (int x = 0; x < W; x++) {
            float fx = (float)x / W;
            // long stretched grain: slow along x, quick across the board
            float wave = sinf((fx * 3.0f + shift * 6.0f) * 6.28318f) * 0.02f;
            float across = inPlank + wave + shift;
            float g1 = sinf(across * 34.0f) * 0.5f + 0.5f;
            float g2 = sinf(across * 91.0f + hashf(x / 24, plank) * 6.0f) * 0.5f + 0.5f;
            float grain = g1 * 0.6f + g2 * 0.4f;
            // end joint: a short dark line at a per-board position
            float jx = hashf(plank * 13 + 1, 2);
            float joint = (fabsf(fx - jx) < 0.006f) ? 0.55f : 1.0f;
            // knot: darker rounded spot
            float kx = hashf(plank * 7 + 4, 8), ky = 0.3f + 0.4f * hashf(plank * 5 + 2, 6);
            float dx = (fx - kx) * 6.0f, dy = (inPlank - ky) * 2.2f;
            float kd = sqrtf(dx * dx + dy * dy);
            float knot = (hashf(plank, 77) > 0.55f && kd < 0.5f) ? (0.62f + 0.38f * (kd / 0.5f)) : 1.0f;
            float n = (hashf(x, y) - 0.5f) * 0.04f;
            float k = tone * seam * joint * knot * (0.80f + 0.30f * grain) + n;
            px[(y * W + x) * 3 + 0] = clampByte(r0 * k * 255);
            px[(y * W + x) * 3 + 1] = clampByte(g0 * k * 255);
            px[(y * W + x) * 3 + 2] = clampByte(b0 * k * 255);
        }
    }
}

// 4×4 ceramic tile grid with grout lines
static void genTile(unsigned char* px, int W, int H)
{
    // Warm glazed ceramic floor tiles: per-tile tone and tint, soft bevelled edges (bright
    // top-left lip, darker bottom-right), fine speckle, and dark narrow grout lines.
    int tW = W / 4, tH = H / 4;
    const int grout = 2;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int tx = x % tW, ty = y % tH;
            int ti = x / tW, tj = y / tH;
            bool isGrout = (tx < grout || tx >= tW - grout || ty < grout || ty >= tH - grout);
            float r, g, b;
            if (isGrout) {
                float v = 0.26f + (hashf(x, y) - 0.5f) * 0.03f;
                r = v + 0.01f; g = v; b = v - 0.01f;
            } else {
                float tone = 0.60f + (hashf(ti * 5 + 1, tj * 3 + 2) - 0.5f) * 0.10f;      // each tile a bit different
                float warm = (hashf(ti * 7 + 3, tj * 11 + 5) - 0.5f) * 0.05f;
                float edge = 0.0f;                                                       // bevel
                int e = 4;
                if (tx < grout + e) edge += 0.07f * (1.0f - (tx - grout) / (float)e);
                if (ty < grout + e) edge += 0.07f * (1.0f - (ty - grout) / (float)e);
                if (tx >= tW - grout - e) edge -= 0.08f * (1.0f - (tW - grout - 1 - tx) / (float)e);
                if (ty >= tH - grout - e) edge -= 0.08f * (1.0f - (tH - grout - 1 - ty) / (float)e);
                float speck = (hashf(x * 3, y * 5) > 0.985f) ? -0.10f : 0.0f;              // tiny dark flecks
                float n = (hashf(x, y) - 0.5f) * 0.025f;
                float v = tone + edge + speck + n;
                r = v + 0.05f + warm; g = v + 0.01f; b = v - 0.07f - warm;
            }
            px[(y * W + x) * 3 + 0] = clampByte(r * 255);
            px[(y * W + x) * 3 + 1] = clampByte(g * 255);
            px[(y * W + x) * 3 + 2] = clampByte(b * 255);
        }
    }
}

// Cream plaster with multi-octave noise
static void genWall(unsigned char* px, int W, int H)
{
    // Warm lime-plaster wall: soft large-scale mottling plus a fine trowel grain.
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float u = (float)x / W, v = (float)y / H;
            float big = fbmTile(u, v, 3, 4) - 0.5f;                       // soft clouds
            float fine = (hashf(x, y) - 0.5f) * 0.035f;
            float stroke = (fbmTile(u * 1.0f + 0.2f, v * 0.2f, 12, 2) - 0.5f) * 0.05f;   // faint horizontal trowel marks
            float n = big * 0.10f + fine + stroke;
            px[(y * W + x) * 3 + 0] = clampByte((0.93f + n)        * 255);
            px[(y * W + x) * 3 + 1] = clampByte((0.87f + n)        * 255);
            px[(y * W + x) * 3 + 2] = clampByte((0.74f + n * 0.6f) * 255);
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

// ─── Realistic lawn ─────────────────────────────────────────────────────────
// Tileable value noise (wraps with the given lattice period)
static float vnoise(float x, float y, int per)
{
    int x0 = (int)floorf(x), y0 = (int)floorf(y);
    float fx = x - x0, fy = y - y0;
    auto h = [&](int ix, int iy) {
        ix = ((ix % per) + per) % per;
        iy = ((iy % per) + per) % per;
        return hashf(ix * 7 + 3, iy * 13 + 5);
    };
    float a = h(x0, y0), b = h(x0 + 1, y0), cc = h(x0, y0 + 1), d = h(x0 + 1, y0 + 1);
    float sx = fx * fx * (3.0f - 2.0f * fx), sy = fy * fy * (3.0f - 2.0f * fy);
    return a + (b - a) * sx + (cc - a) * sy + (a - b - cc + d) * sx * sy;
}

// Fractal noise in [0,1], tileable over u,v in [0,1)
static float fbmTile(float u, float v, int baseFreq, int octaves)
{
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    int f = baseFreq;
    for (int i = 0; i < octaves; i++) {
        sum += amp * vnoise(u * f, v * f, f);
        norm += amp; amp *= 0.5f; f *= 2;
    }
    return sum / norm;
}

static float sstep(float e0, float e1, float x)
{
    float t = (x - e0) / (e1 - e0);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

static unsigned int g_grassRng = 12345u;
static float rnd01()
{
    g_grassRng = g_grassRng * 1664525u + 1013904223u;
    return ((g_grassRng >> 8) & 0xFFFFu) / 65535.0f;
}

struct Rgb { float r, g, b; };
static Rgb mixRgb(Rgb a, Rgb b, float t) { return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t }; }

// Realistic grass: several greens, soft dry/brown patches, thousands of fine blade
// strokes, clover / leaf specks and a few tiny pale flowers.  Wraps at the edges.
static void genGrass(unsigned char* px, int W, int H)
{
    std::vector<float> f((size_t)W * H * 3);
    const Rgb G0 = { 0.12f, 0.24f, 0.08f }, G1 = { 0.22f, 0.36f, 0.11f }, G2 = { 0.33f, 0.46f, 0.15f },
              G3 = { 0.45f, 0.53f, 0.21f }, BR = { 0.36f, 0.29f, 0.14f };

    // 1. base colour field: big soft patches of different greens + dry brown areas
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float u = (float)x / W, v = (float)y / H;
            float n1 = fbmTile(u, v, 3, 5);
            float n2 = fbmTile(u + 0.31f, v + 0.77f, 7, 4);
            float n3 = fbmTile(u + 0.12f, v + 0.45f, 22, 3);
            float dry = fbmTile(u + 0.55f, v + 0.20f, 4, 4);
            Rgb c = mixRgb(G1, G2, sstep(0.30f, 0.70f, n2) * 0.55f);
            c = mixRgb(c, G2, sstep(0.45f, 0.75f, n1) * 0.35f);
            c = mixRgb(c, G3, sstep(0.60f, 0.85f, n3) * 0.12f);
            c = mixRgb(c, BR, sstep(0.52f, 0.68f, dry) * 0.0f);                     // no dry patches: plain green field
            float shade = 0.94f + 0.12f * fbmTile(u + 0.9f, v + 0.1f, 40, 2);       // fine mottling
            float* o = &f[((size_t)y * W + x) * 3];
            o[0] = c.r * shade; o[1] = c.g * shade; o[2] = c.b * shade;
        }
    }

    auto plot = [&](int x, int y, Rgb col, float a) {
        x = ((x % W) + W) % W; y = ((y % H) + H) % H;
        float* o = &f[((size_t)y * W + x) * 3];
        o[0] += (col.r - o[0]) * a; o[1] += (col.g - o[1]) * a; o[2] += (col.b - o[2]) * a;
    };

    // 2. fine blades: thousands of short strokes in many directions
    g_grassRng = 987654u;
    const int blades = W * H / 40;
    for (int i = 0; i < blades; i++) {
        float x0 = rnd01() * W, y0 = rnd01() * H;
        float ang = (rnd01() < 0.80f) ? (1.5708f + (rnd01() - 0.5f) * 1.2f) : (rnd01() * 6.2832f);
        float len = 3.5f + rnd01() * 6.5f;
        float dx = cosf(ang), dy = sinf(ang);
        float k = rnd01();
        Rgb col;
        if (k < 0.45f)      col = mixRgb(G0, G1, rnd01());
        else if (k < 0.75f) col = mixRgb(G1, G2, rnd01());
        else if (k < 0.88f) col = mixRgb(G2, G3, rnd01());
        else if (k < 0.95f) col = mixRgb(G3, { 0.62f, 0.64f, 0.28f }, rnd01());      // sun-bleached
        else                col = mixRgb(BR, { 0.28f, 0.22f, 0.11f }, rnd01());      // dead blade
        for (float t = 0.0f; t < len; t += 0.7f) {
            float taper = 1.0f - 0.5f * (t / len);                                  // tip fades
            int ix = (int)(x0 + dx * t), iy = (int)(y0 + dy * t);
            plot(ix, iy, col, 0.22f * taper);
            plot(ix + 1, iy, col, 0.08f * taper);
        }
    }

    // 3. clover / leaf specks (small clumps of three tiny round leaves)
    const int clumps = 0;
    for (int i = 0; i < clumps; i++) {
        float cx = rnd01() * W, cy = rnd01() * H;
        Rgb col = { 0.40f + 0.18f * rnd01(), 0.58f + 0.14f * rnd01(), 0.16f + 0.10f * rnd01() };
        for (int l = 0; l < 3; l++) {
            float ox = cx + (rnd01() - 0.5f) * 5.0f, oy = cy + (rnd01() - 0.5f) * 5.0f;
            float r = 0.7f + rnd01() * 0.8f;
            for (int yy = (int)(-r - 1); yy <= (int)(r + 1); yy++)
                for (int xx = (int)(-r - 1); xx <= (int)(r + 1); xx++) {
                    float d = sqrtf((float)(xx * xx + yy * yy));
                    if (d < r) plot((int)ox + xx, (int)oy + yy, col, 0.60f * (1.0f - 0.4f * d / r));
                }
        }
    }
    // a few tiny pale flowers
    const int flowers = 0;
    for (int i = 0; i < flowers; i++) {
        int fx = (int)(rnd01() * W), fy = (int)(rnd01() * H);
        Rgb col = (rnd01() < 0.5f) ? Rgb{ 0.92f, 0.90f, 0.55f } : Rgb{ 0.95f, 0.95f, 0.92f };
        plot(fx, fy, col, 0.9f); plot(fx + 1, fy, col, 0.5f); plot(fx, fy + 1, col, 0.5f);
    }

    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float n = 0.97f + 0.06f * hashf(x * 3 + 11, y * 5 + 7);            // +-3% grain
            for (int ch = 0; ch < 3; ch++) {
                size_t i = ((size_t)y * W + x) * 3 + ch;
                px[i] = clampByte(f[i] * n * 255.0f);
            }
        }
}

// Large-scale mottling, neutral at 0.5: used with blend (dst*src*2) to break up tiling
static void genGrassMacro(unsigned char* px, int W, int H)
{
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float u = (float)x / W, v = (float)y / H;
            float a = fbmTile(u, v, 3, 4);
            float b = fbmTile(u + 0.4f, v + 0.2f, 5, 3);
            float val = 0.5f + (a - 0.5f) * 0.18f;                    // very gentle large-scale variation
            float warm = (b - 0.5f) * 0.12f;                           // slight warm / cool shift
            px[(y * W + x) * 3 + 0] = clampByte((val + warm) * 255.0f);
            px[(y * W + x) * 3 + 1] = clampByte(val * 255.0f);
            px[(y * W + x) * 3 + 2] = clampByte((val - warm) * 255.0f);
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

// Soft cloud puff sprite: round, fluffy edge broken up with noise, brighter on top and
// slightly grey underneath.  RGBA so clouds can be blended as billboards.
static GLuint makeCloudTexture()
{
    const int W = 128, H = 128;
    std::vector<unsigned char> px((size_t)W * H * 4);
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float u = (x + 0.5f) / W, v = (y + 0.5f) / H;
            float dx = (u - 0.5f) * 2.0f, dy = (v - 0.5f) * 2.0f;
            float r = sqrtf(dx * dx + dy * dy);
            float fall = 1.0f - r;
            fall = fall < 0.0f ? 0.0f : fall;
            fall = fall * fall * (3.0f - 2.0f * fall);                       // smooth round falloff
            float n = fbmTile(u, v, 4, 4);                                   // 0..1 fluffy breakup
            float a = fall * (0.35f + 1.25f * n);
            a = a > 1.0f ? 1.0f : a;
            float top = 1.0f - v;                                            // y up in the texture = lighter
            float shade = 0.80f + 0.20f * top;
            size_t i = ((size_t)y * W + x) * 4;
            px[i + 0] = clampByte(255.0f * shade);
            px[i + 1] = clampByte(255.0f * (shade + 0.01f));
            px[i + 2] = clampByte(255.0f * (shade + 0.04f));
            px[i + 3] = clampByte(255.0f * a);
        }
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    return id;
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
    if (GLEW_EXT_texture_filter_anisotropic) {
        GLfloat mx = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &mx);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, mx > 8.0f ? 8.0f : mx);
    }
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, W, H, GL_RGB, GL_UNSIGNED_BYTE, px);
    return id;
}

// Tatami: woven igusa rush.  Rows of rounded reeds run across U (one mat length = one
// texture repeat), each row slightly different in tone, with fine fibres along the rows
// and a soft large-scale mottling from wear and sun.
static void genTatami(unsigned char* px, int W, int H)
{
    const float ROWS = 80.0f;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            float u = (float)x / W, v = (float)y / H;
            float r = u * ROWS, f = r - floorf(r);
            float reed = 0.78f + 0.22f * sinf(3.14159f * f);                 // rounded reed, darker groove between rows
            int   row = (int)r;
            float rowTone = (hashf(row, 7) - 0.5f) * 0.08f;                   // each row a little different
            float fibre = (hashf(row * 13 + (int)(v * 96.0f), y / 3) - 0.5f) * 0.06f;
            float mott = (fbmTile(u, v, 3, 3) - 0.5f) * 0.10f;
            float stitch = (fmodf(v * 8.0f, 1.0f) < 0.02f) ? -0.10f : 0.0f;  // warp threads holding the rushes
            float k = reed + rowTone + fibre + mott + stitch;
            px[(y * W + x) * 3 + 0] = clampByte(0.80f * k * 255);
            px[(y * W + x) * 3 + 1] = clampByte(0.76f * k * 255);
            px[(y * W + x) * 3 + 2] = clampByte(0.50f * k * 255);
        }
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
    {   // lawn is generated at 512x512 for fine detail, plus a small macro-variation map
        std::vector<unsigned char> big(512 * 512 * 3);
        genGrass(big.data(), 512, 512);
        texIDs[TEX_GRASS] = makeGLTex(big.data(), 512, 512);
        genGrassMacro(px, W, H);
        texIDs[TEX_GRASS_MACRO] = makeGLTex(px, W, H);
    }
    genWater(px, W, H);                         texIDs[TEX_WATER]      = makeGLTex(px, W, H);
    genTatami(px, W, H);                        texIDs[TEX_TATAMI]     = makeGLTex(px, W, H);
    texIDs[TEX_CLOUD] = makeCloudTexture();
}

GLuint getTexID(TexID id)
{
    if (id <= TEX_NONE || id >= TEX_COUNT) return 0;
    return texIDs[id];
}
