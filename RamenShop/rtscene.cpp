#include "rtscene.h"
#include "scene.h"
#include "objects.h"
#include "lighting.h"
#include "shader.h"
#include "refraction.h"
#include <cmath>

using rt::V3;

// ─── Places of the new objects (shared by OpenGL and the ray tracer) ────────
static const float COUNTER_TOP = FLOOR_Y + 1.06f;                 // drawCounter(): top slab at 1.0 .. 1.06
static const V3    BALL_STAND  = { -1.30f, COUNTER_TOP, -0.50f }; // on the counter, between bowl 0 and 1
static const float STAND_R = 0.035f, STAND_H = 0.018f, BALL_R = 0.075f;
static const V3    BALL_C = { BALL_STAND.x, COUNTER_TOP + STAND_H + 0.0663f, BALL_STAND.z };   // rests on the stand ring

// Upper tatami room (drawSecondFloor): floor TY, interior x RX0..RX1, z RZ0..RZ1, ceiling CY
static const float TY = 3.3f + 0.12f + 0.06f, RX0 = -4.8f, RX1 = 3.45f, RZ0 = -3.8f, RZ1 = 3.8f, CY = 3.3f + 2.0f - 0.03f;
// Mirror on the back wall, left of the fusuma doors (between the posts at x -4.74 and -2.35)
static const float MIR_X0 = -3.90f, MIR_X1 = -3.10f, MIR_Y0 = TY + 0.30f, MIR_Y1 = TY + 1.60f;
static const float MIR_Z  = RZ0 + 0.035f;                         // glass surface (wall plaster ends at RZ0 + 0.01)
static const float FRAME  = 0.05f;
static const Color TIMBER = { 0.24f, 0.15f, 0.09f };

// ─── CPU copies of the scene textures ───────────────────────────────────────
static rt::Texture g_wood, g_darkWood, g_tile, g_wall, g_tatami;

static void readTexture(GLuint id, rt::Texture& t)
{
    if (!id) return;
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &t.w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &t.h);
    t.rgb.resize((size_t)t.w * t.h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, t.rgb.data());
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void rtLoadTextures()
{
    readTexture(getTexID(TEX_WOOD), g_wood);
    readTexture(getTexID(TEX_DARK_WOOD), g_darkWood);
    readTexture(getTexID(TEX_TILE_FLOOR), g_tile);
    readTexture(getTexID(TEX_WALL), g_wall);
    readTexture(getTexID(TEX_TATAMI), g_tatami);
}

// ─── Scene building helpers ─────────────────────────────────────────────────
static V3 C(Color c) { return { c.r, c.g, c.b }; }
static V3 scaled(V3 v, float k) { return { v.x * k, v.y * k, v.z * k }; }

struct Build {
    rt::Scene& s;
    rt::Shape& add(int type, int mat, int obj) { s.shapes.push_back(rt::Shape()); rt::Shape& sh = s.shapes.back(); sh.type = type; sh.material = mat; sh.object = obj; return sh; }
    void box(V3 mn, V3 mx, int mat, int obj = -1)                     { rt::Shape& b = add(rt::SH_BOX, mat, obj); b.a = mn; b.b = mx; }
    void cone(V3 base, float r0, float r1, float h, int mat, int obj = -1) { rt::Shape& c = add(rt::SH_CONE, mat, obj); c.a = base; c.r0 = r0; c.r1 = r1; c.h = h; }
    void disk(V3 c, float rIn, float rOut, float ny, int mat, int obj = -1) { rt::Shape& d = add(rt::SH_DISK, mat, obj); d.a = c; d.r0 = rIn; d.r1 = rOut; d.h = ny; }
    void sphere(V3 c, float r, int mat, int obj = -1)                  { rt::Shape& p = add(rt::SH_SPHERE, mat, obj); p.a = c; p.r0 = r; }
    // centred box helper: x / z centre, y from bottom, full sizes
    void cuboid(float cx, float y0, float cz, float sx, float sy, float sz, int mat) { box({ cx - sx * 0.5f, y0, cz - sz * 0.5f }, { cx + sx * 0.5f, y0 + sy, cz + sz * 0.5f }, mat); }
};

static int diffuse(rt::Scene& s, V3 color, V3 spec = { 0, 0, 0 }, float sh = 1.0f,
                   const rt::Texture* tex = nullptr, int map = rt::MAP_NONE, float scale = 1.0f, float u0 = 0.0f, float v0 = 0.0f)
{
    rt::Material m;
    m.type = rt::MAT_DIFFUSE; m.color = color; m.spec = spec; m.shininess = sh;
    m.tex = (tex && tex->w > 0) ? tex : nullptr; m.map = map; m.uvScale = scale; m.u0 = u0; m.v0 = v0;
    return s.addMaterial(m);
}
static int reflective(rt::Scene& s, int type, V3 color, V3 f0, V3 spec, float sh)
{
    rt::Material m;
    m.type = type; m.color = color; m.f0 = f0; m.spec = spec; m.shininess = sh;
    return s.addMaterial(m);
}
static int emissive(rt::Scene& s, V3 e)
{
    rt::Material m;
    m.type = rt::MAT_EMISSIVE; m.emission = e;
    return s.addMaterial(m);
}

// ─── Clear glassware (hollow: outer and inner surfaces, water as its own medium) ─────
// drawClearGlassTumbler() model at p x sc: heavy solid base, 10 mm wall (at scale 0.22), open top
static void addTumbler(Build& b, V3 p, float sc, bool water, int mGlass, int mWater, int obj, RtObjectBounds& bd)
{
    rt::Scene& s = b.s;
    size_t first = s.shapes.size();
    b.cone({ p.x, p.y + 0.08f * sc, p.z }, 0.285f * sc, 0.320f * sc, 0.85f * sc, mGlass, obj);   // outer wall
    b.cone(p, 0.280f * sc, 0.285f * sc, 0.08f * sc, mGlass, obj);                                // side of the base
    b.cone({ p.x, p.y + 0.08f * sc, p.z }, 0.240f * sc, 0.275f * sc, 0.85f * sc, mGlass, obj);   // cavity wall
    s.shapes.back().flip = true;                                                                 // (the glass is outside it)
    b.disk(p, 0.0f, 0.280f * sc, -1.0f, mGlass, obj);                                            // bottom
    b.disk({ p.x, p.y + 0.08f * sc, p.z }, 0.0f, 0.240f * sc, 1.0f, mGlass, obj);                // cavity floor
    b.disk({ p.x, p.y + 0.93f * sc, p.z }, 0.275f * sc, 0.320f * sc, 1.0f, mGlass, obj);         // rim
    if (water) {                                                                                 // water column (thin air gap to the glass)
        b.cone({ p.x, p.y + 0.083f * sc, p.z }, 0.236f * sc, 0.256f * sc, 0.647f * sc, mWater, obj);
        b.disk({ p.x, p.y + 0.083f * sc, p.z }, 0.0f, 0.236f * sc, -1.0f, mWater, obj);
        b.disk({ p.x, p.y + 0.73f * sc, p.z }, 0.0f, 0.256f * sc, 1.0f, mWater, obj);
    }
    for (size_t i = first; i < s.shapes.size(); i++) s.shapes[i].rasterDepth = false;   // OpenGL glass writes no depth
    bd = { true, { p.x - 0.33f * sc, p.y, p.z - 0.33f * sc }, { p.x + 0.33f * sc, p.y + 0.95f * sc, p.z + 0.33f * sc } };
}

// drawClearGlassPitcher() model at p x sc (the OpenGL one is a single shell; here the glass
// gets a 0.012-unit wall) with its water; the handle and the lemon slice are left out
static void addJug(Build& b, V3 p, float sc, int mGlass, int mWater, int obj, RtObjectBounds& bd)
{
    rt::Scene& s = b.s;
    size_t first = s.shapes.size();
    const float T = 0.012f;
    const float prof[5][2] = { { 0.35f, 0.00f }, { 0.36f, 0.06f }, { 0.24f, 0.76f }, { 0.20f, 1.11f }, { 0.28f, 1.31f } };
    b.disk(p, 0.0f, 0.35f * sc, -1.0f, mGlass, obj);                                             // base
    for (int k = 0; k < 4; k++) {
        float r0 = prof[k][0], y0 = prof[k][1], r1 = prof[k + 1][0], y1 = prof[k + 1][1];
        b.cone({ p.x, p.y + y0 * sc, p.z }, r0 * sc, r1 * sc, (y1 - y0) * sc, mGlass, obj);      // outside
        if (k > 0) {                                                                             // inside (above the solid base)
            b.cone({ p.x, p.y + y0 * sc, p.z }, (r0 - T) * sc, (r1 - T) * sc, (y1 - y0) * sc, mGlass, obj);
            s.shapes.back().flip = true;
        }
    }
    b.disk({ p.x, p.y + 0.06f * sc, p.z }, 0.0f, (0.36f - T) * sc, 1.0f, mGlass, obj);          // inner floor
    b.disk({ p.x, p.y + 1.31f * sc, p.z }, (0.28f - T) * sc, 0.28f * sc, 1.0f, mGlass, obj);     // lip
    b.cone({ p.x, p.y + 0.07f * sc, p.z }, 0.32f * sc, 0.22f * sc, 0.63f * sc, mWater, obj);      // water
    b.disk({ p.x, p.y + 0.07f * sc, p.z }, 0.0f, 0.32f * sc, -1.0f, mWater, obj);
    b.disk({ p.x, p.y + 0.70f * sc, p.z }, 0.0f, 0.22f * sc, 1.0f, mWater, obj);
    for (size_t i = first; i < s.shapes.size(); i++) s.shapes[i].rasterDepth = false;
    bd = { true, { p.x - 0.37f * sc, p.y, p.z - 0.37f * sc }, { p.x + 0.37f * sc, p.y + 1.32f * sc, p.z + 0.37f * sc } };
}

// ─── The scene ───────────────────────────────────────────────────────────────
void rtBuildShopScene(rt::Scene& s, RtObjectBounds bounds[RTO_COUNT])
{
    s.shapes.clear();
    s.materials.clear();
    Build b{ s };
    const float FY = FLOOR_Y;
    const float day = isDayTime ? 0.6f : 1.0f;

    // ── Materials ──
    const V3 W1 = { 1, 1, 1 };
    int mTile    = diffuse(s, W1, { 0.75f, 0.73f, 0.66f }, 55.0f, &g_tile, rt::MAP_XZ, 2.0f, -5.0f, -4.0f);
    int mCeil    = diffuse(s, { 0.62f, 0.48f, 0.30f }, { 0.05f, 0.05f, 0.05f }, 10.0f, &g_darkWood, rt::MAP_XZ, 1.5f, -4.9f, -3.9f);
    int mWallZ   = diffuse(s, W1, { 0.03f, 0.03f, 0.03f }, 8.0f, &g_wall, rt::MAP_XY, 2.0f, -5.0f, 0.0f);
    int mWallX   = diffuse(s, W1, { 0.03f, 0.03f, 0.03f }, 8.0f, &g_wall, rt::MAP_ZY, 2.0f, -4.0f, 0.0f);
    int mWainsct = diffuse(s, W1, { 0.10f, 0.10f, 0.10f }, 20.0f, &g_darkWood, rt::MAP_ZY, 1.0f, -4.0f, 0.0f);
    int mShoji   = diffuse(s, C(PAPER), { 0.02f, 0.02f, 0.02f }, 8.0f);
    int mCounter = diffuse(s, W1, { 0.65f, 0.62f, 0.57f }, 40.0f, &g_wood, rt::MAP_XY, 1.0f, -3.0f, FY);
    int mCtrTop  = diffuse(s, W1, { 1.0f, 0.96f, 0.88f }, 60.0f, &g_wood, rt::MAP_XZ, 0.5f, -3.1f, -0.85f);
    int mDarkW   = diffuse(s, C(DARK_WOOD), { 0.1f, 0.1f, 0.1f }, 20.0f);
    int mWood    = diffuse(s, C(WOOD), { 0.2f, 0.2f, 0.2f }, 24.0f);
    int mSeat    = diffuse(s, C(LIGHT_WOOD), { 0.3f, 0.3f, 0.3f }, 30.0f);
    int mTable   = diffuse(s, W1, { 1.0f, 0.96f, 0.88f }, 55.0f, &g_wood, rt::MAP_XZ, 0.8f, -2.88f, 2.48f);
    int mCabinet = diffuse(s, W1, { 0.1f, 0.1f, 0.1f }, 20.0f, &g_wood, rt::MAP_XY, 1.0f, -3.5f, FY);
    int mSteel   = reflective(s, rt::MAT_METAL, C(STEEL), { 0.45f, 0.45f, 0.47f }, { 0.8f, 0.8f, 0.8f }, 60.0f);
    int mPot     = reflective(s, rt::MAT_METAL, { 0.42f, 0.43f, 0.46f }, { 0.62f, 0.63f, 0.64f }, { 1, 1, 1 }, 90.0f);
    int mGlaze   = reflective(s, rt::MAT_GLAZE, C(BOWL_RED), { 0.05f, 0.05f, 0.05f }, { 0.9f, 0.9f, 0.9f }, 110.0f);
    int mSoup    = reflective(s, rt::MAT_LIQUID, { 0.92f, 0.74f, 0.40f }, { 0.03f, 0.03f, 0.03f }, { 0.9f, 0.75f, 0.5f }, 85.0f);
    int mGlass   = reflective(s, rt::MAT_GLASS, { 0.97f, 0.99f, 0.98f }, { 0.04f, 0.04f, 0.04f }, { 1, 1, 1 }, 140.0f);
    int mMirror  = reflective(s, rt::MAT_MIRROR, { 0.0f, 0.0f, 0.0f }, { 0.96f, 0.96f, 0.96f }, { 1, 1, 1 }, 200.0f);
    int mHoodDuct= diffuse(s, C(DARK_GRAY));
    int mNoren   = diffuse(s, { 0.78f, 0.13f, 0.11f }, { 0.05f, 0.05f, 0.05f }, 8.0f);

    const bool pendantOn = lightPoint && fixtureOn[FX_PENDANTS];
    const bool hangingOn = lightPoint && fixtureOn[FX_HANGING];
    const bool boxOn     = lightPoint && fixtureOn[FX_BOX];
    const bool panelOn   = lightArea && fixtureOn[FX_PANEL];
    const bool domeOn    = lightArea && fixtureOn[FX_DOME];
    int mBulb    = pendantOn ? emissive(s, scaled(V3{ 3.0f, 2.5f, 1.7f }, day)) : diffuse(s, { 0.45f, 0.35f, 0.15f });
    int mChochin = emissive(s, scaled(V3{ 1.6f, 1.1f, 0.5f }, (hangingOn ? day : 0.12f)));
    int mAndon   = boxOn ? emissive(s, scaled(V3{ 1.4f, 1.1f, 0.7f }, day)) : diffuse(s, C(PAPER));
    int mPanel   = panelOn ? emissive(s, scaled(V3{ 2.0f, 1.9f, 1.7f }, day)) : diffuse(s, C(DARK_GRAY));

    // ══ Ground floor room ══
    b.box({ -4.8f, 0.0f, -3.8f }, { 4.8f, FY, 3.8f }, mTile);                      // tiled floor (top FLOOR_Y)
    b.box({ -4.9f, 3.30f, -3.9f }, { 4.9f, 3.42f, 3.9f }, mCeil);                  // ceiling slab
    b.box({ -4.9f, 0.0f, -4.0f }, { 4.9f, 3.30f, -3.8f }, mWallZ);                 // back wall (behind the kitchen)
    b.box({ -4.9f, 0.0f, 3.8f }, { 4.9f, 3.30f, 4.0f }, mShoji);                   // shoji front: seen from inside only
    s.shapes.back().facing = { 0, 0, -1 };                                         // (the counter camera stands outside it)
    for (int sx = -1; sx <= 1; sx += 2) {                                          // side walls: dark wainscot + plaster
        float x0 = sx < 0 ? -5.0f : 4.8f, x1 = sx < 0 ? -4.8f : 5.0f;
        b.box({ x0, 0.0f, -3.9f }, { x1, 1.2f, 3.9f }, mWainsct);
        b.box({ x0, 1.2f, -3.9f }, { x1, 3.30f, 3.9f }, mWallX);
    }

    // Dining counter (drawCounter at (0, FY, -0.5))
    b.box({ -3.0f, FY, -0.80f }, { 3.0f, FY + 1.0f, -0.20f }, mCounter);
    b.box({ -3.1f, FY + 1.0f, -0.85f }, { 3.1f, COUNTER_TOP, -0.05f }, mCtrTop);

    // Stools (drawStool), first one follows its object delta
    for (int i = 0; i < 6; i++) {
        V3 p = { -2.8f + i * 1.12f, FY, 0.8f };
        float sc = 1.0f;
        if (i == 0) {
            const SceneObject& o = sceneObjects[OBJ_STOOL];
            if (fabsf(o.dRot.x) > 0.01f) continue;
            p = { p.x + o.dPos.x, p.y + o.dPos.y, p.z + o.dPos.z }; sc = o.dScale;
        }
        b.cone({ p.x, p.y + 0.02f * sc, p.z }, 0.18f * sc, 0.18f * sc, 0.04f * sc, mDarkW);
        b.disk({ p.x, p.y + 0.06f * sc, p.z }, 0.0f, 0.18f * sc, 1.0f, mDarkW);
        b.cone({ p.x, p.y + 0.06f * sc, p.z }, 0.065f * sc, 0.055f * sc, 0.64f * sc, mWood);
        b.cone({ p.x, p.y + 0.70f * sc, p.z }, 0.19f * sc, 0.19f * sc, 0.055f * sc, mSeat);
        b.disk({ p.x, p.y + 0.755f * sc, p.z }, 0.0f, 0.19f * sc, 1.0f, mSeat);
    }

    // Customer tables (drawTable at (+-2.4, FY, 2.8), scale 0.8 / 0.9 / 0.8)
    for (int t = 0; t < 2; t++) {
        float tx = (t == 0) ? -2.4f : 2.4f;
        rt::Material tm = s.materials[mTable];
        tm.u0 = tx - 0.48f;
        int mT = s.addMaterial(tm);
        b.box({ tx - 0.48f, FY + 0.621f, 2.48f }, { tx + 0.48f, FY + 0.675f, 3.12f }, mT);
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                b.cuboid(tx + sx * 0.416f, FY, 2.8f + sz * 0.256f, 0.056f, 0.621f, 0.056f, mDarkW);
        // two chairs (drawChair x 0.85, facing the table): seat, backrest, legs, back posts
        for (int c = 0; c < 2; c++) {
            const float cz = (c == 0) ? 2.25f : 3.35f, back = (c == 0) ? -1.0f : 1.0f, k = 0.85f;
            b.cuboid(tx, FY + 0.42f * k, cz, 0.45f * k, 0.05f * k, 0.45f * k, mWood);
            b.cuboid(tx, FY + 0.85f * k, cz + back * 0.2f * k, 0.45f * k, 0.15f * k, 0.04f * k, mWood);
            for (int sx = -1; sx <= 1; sx += 2) {
                for (int sz = -1; sz <= 1; sz += 2)
                    b.cuboid(tx + sx * 0.19f * k, FY, cz + sz * 0.19f * k, 0.05f * k, 0.42f * k, 0.05f * k, mDarkW);
                b.cuboid(tx + sx * 0.19f * k, FY + 0.47f * k, cz + back * 0.2f * k, 0.05f * k, 0.5f * k, 0.05f * k, mDarkW);
            }
        }
    }

    // Kitchen (drawKitchen at (0, FY, -3.2)): cabinets, steel worktop, stove, hood
    b.box({ -3.5f, FY, -3.5f }, { 3.5f, FY + 0.90f, -2.9f }, mCabinet);
    b.box({ -3.51f, FY + 0.90f, -3.51f }, { 3.51f, FY + 0.94f, -2.89f }, mSteel);
    b.box({ -0.95f, FY + 0.94f, -3.50f }, { 0.95f, FY + 1.00f, -2.90f }, mSteel);
    b.box({ -1.30f, FY + 1.87f, -3.55f }, { 1.30f, FY + 1.91f, -2.85f }, mSteel);   // hood skirt
    b.box({ -1.20f, FY + 1.95f, -3.50f }, { 1.20f, FY + 2.03f, -2.90f }, mSteel);   // hood body
    b.box({ -0.175f, FY + 2.03f, -3.35f }, { 0.175f, FY + 2.48f, -3.05f }, mHoodDuct);

    // Noren curtain (drawNoren at (0, 3.0, -1.3)), follows its object delta
    {
        const SceneObject& o = sceneObjects[OBJ_NOREN];
        if (fabsf(o.dRot.x) < 0.01f && fabsf(o.dRot.y) < 0.01f) {
            V3 p = { o.dPos.x, 3.0f + o.dPos.y, -1.3f + o.dPos.z };
            float sc = o.dScale;
            b.box({ p.x - 3.4f * sc, p.y - 0.65f * sc, p.z - 0.02f }, { p.x + 3.4f * sc, p.y - 0.05f * sc, p.z + 0.02f }, mNoren);
        }
    }

    // Lamps as light-giving shapes (they show up in reflections)
    for (int sx = -1; sx <= 1; sx += 2) {
        b.sphere({ sx * 1.2f, 2.93f, -0.2f }, 0.06f, mBulb);                  // pendant glass lamps
        b.sphere({ sx * 1.8f, 2.65f, 0.0f }, 0.14f, mChochin);                // hanging paper lanterns
    }
    b.box({ 0.25f, 2.75f, 0.30f }, { 0.75f, 3.15f, 0.80f }, mAndon);           // box lantern cluster
    b.box({ -1.6f, 3.22f, -1.625f }, { 1.6f, 3.29f, -0.775f }, mPanel);         // ceiling area light

    // ── Featured: the two stock pots (drawLargeRamenPot), left one follows OBJ_KETTLE ──
    for (int k = 0; k < 2; k++) {
        int obj = (k == 0) ? RTO_POT_L : RTO_POT_R;
        V3 p = { (k == 0) ? -0.55f : 0.55f, FY + 0.94f + 0.07f, -3.2f };
        float sc = 1.0f;
        bounds[obj].traced = true;
        if (k == 0) {
            const SceneObject& o = sceneObjects[OBJ_KETTLE];
            if (fabsf(o.dRot.x) > 0.01f) { bounds[obj].traced = false; continue; }
            p = { p.x + o.dPos.x, p.y + o.dPos.y, p.z + o.dPos.z }; sc = o.dScale;
        }
        b.cone(p, 0.30f * sc, 0.32f * sc, 0.55f * sc, mPot, obj);
        b.disk({ p.x, p.y + 0.55f * sc, p.z }, 0.0f, 0.32f * sc, 1.0f, mPot, obj);
        bounds[obj].bmin = { p.x - 0.36f * sc, p.y, p.z - 0.36f * sc };
        bounds[obj].bmax = { p.x + 0.36f * sc, p.y + 0.58f * sc, p.z + 0.36f * sc };
    }

    // ── Featured: four ramen bowls (drawBowl x 0.24, broth disk), first follows OBJ_BOWL ──
    for (int i = 0; i < 4; i++) {
        int obj = RTO_BOWL_0 + i;
        V3 p = { -2.2f + i * 1.5f, COUNTER_TOP, -0.3f };
        float sc = 0.24f;
        bounds[obj].traced = true;
        if (i == 0) {
            const SceneObject& o = sceneObjects[OBJ_BOWL];
            if (fabsf(o.dRot.x) > 0.01f) { bounds[obj].traced = false; continue; }
            p = { p.x + o.dPos.x, p.y + o.dPos.y, p.z + o.dPos.z }; sc *= o.dScale;
        }
        b.cone(p, 0.22f * sc, 0.22f * sc, 0.06f * sc, mGlaze, obj);                         // foot ring
        b.cone({ p.x, p.y + 0.06f * sc, p.z }, 0.25f * sc, 0.50f * sc, 0.34f * sc, mGlaze, obj);   // body
        b.disk({ p.x, p.y + 0.31f * sc, p.z }, 0.0f, 0.415f * sc, 1.0f, mSoup, obj);         // soup surface
        bounds[obj].bmin = { p.x - 0.52f * sc, p.y, p.z - 0.52f * sc };
        bounds[obj].bmax = { p.x + 0.52f * sc, p.y + 0.42f * sc, p.z + 0.52f * sc };
    }

    // ── Featured: glass ball on its stand ──
    b.cone(BALL_STAND, STAND_R, STAND_R, STAND_H, mDarkW);
    b.disk({ BALL_STAND.x, BALL_STAND.y + STAND_H, BALL_STAND.z }, 0.0f, STAND_R, 1.0f, mDarkW);
    b.sphere(BALL_C, BALL_R, mGlass, RTO_GLASS_BALL);
    bounds[RTO_GLASS_BALL] = { true, { BALL_C.x - BALL_R, BALL_C.y - BALL_R, BALL_C.z - BALL_R },
                                     { BALL_C.x + BALL_R, BALL_C.y + BALL_R, BALL_C.z + BALL_R } };

    // ── Featured: clear glassware (glass n = 1.5, water n = 1.33) ──
    int mClear = reflective(s, rt::MAT_GLASS, { 0.985f, 0.995f, 0.99f }, { 0.04f, 0.04f, 0.04f }, { 1, 1, 1 }, 140.0f);
    int mWater = reflective(s, rt::MAT_GLASS, { 0.96f, 0.985f, 0.995f }, { 0.02f, 0.02f, 0.02f }, { 1, 1, 1 }, 120.0f);
    s.materials[mWater].ior = 1.33f;
    addTumbler(b, { TABLE_TUMBLER_POS.x, TABLE_TUMBLER_POS.y, TABLE_TUMBLER_POS.z }, TABLE_TUMBLER_SCALE, false,
               mClear, mWater, RTO_TABLE_GLASS, bounds[RTO_TABLE_GLASS]);                   // empty glass on the left table
    const float glassX[4] = { -1.85f, -0.35f, 1.15f, 2.65f };                                  // as in drawTransparentPass()
    for (int i = 0; i < 4; i++)
        addTumbler(b, { glassX[i], COUNTER_TOP + 0.004f, -0.22f }, 0.22f, true, mClear, mWater,
                   RTO_COUNTER_GLASS_0 + i, bounds[RTO_COUNTER_GLASS_0 + i]);                 // water glasses on the counter
    addJug(b, { 0.35f, COUNTER_TOP + 0.004f, -0.36f }, 0.30f, mClear, mWater, RTO_JUG, bounds[RTO_JUG]);

    // ══ Upper tatami room (drawSecondFloor) ══
    int mTatami  = diffuse(s, W1, { 0.10f, 0.10f, 0.08f }, 12.0f, &g_tatami, rt::MAP_TATAMI, 1.0f, RX0, RZ0);
    const V3 PLASTER = { 0.84f, 0.80f, 0.72f };
    int mPlastZ  = diffuse(s, PLASTER, { 0.03f, 0.03f, 0.03f }, 8.0f, &g_wall, rt::MAP_XY, 1.5f, -4.8f, TY);
    int mPlastX  = diffuse(s, PLASTER, { 0.03f, 0.03f, 0.03f }, 8.0f, &g_wall, rt::MAP_ZY, 1.5f, -3.8f, TY);
    int mBoards  = diffuse(s, { 0.78f, 0.66f, 0.50f }, { 0.15f, 0.14f, 0.12f }, 30.0f, &g_wood, rt::MAP_XZ, 1.6f, -4.8f, -3.8f);
    int mTimber  = diffuse(s, C(TIMBER), { 0.2f, 0.2f, 0.2f }, 30.0f);
    int mPaper   = diffuse(s, C(PAPER), { 0.02f, 0.02f, 0.02f }, 8.0f);
    int mChabu   = diffuse(s, { 0.92f, 0.72f, 0.50f }, { 0.5f, 0.45f, 0.4f }, 40.0f, &g_wood, rt::MAP_XZ, 1.2f, -1.15f, 0.36f);
    int mLegs    = diffuse(s, { 0.55f, 0.36f, 0.20f });
    int mTansu   = diffuse(s, { 1.0f, 0.95f, 0.9f }, { 0.2f, 0.2f, 0.2f }, 30.0f, &g_darkWood, rt::MAP_ZY, 1.0f, -0.725f, TY);
    int mDome    = domeOn ? emissive(s, scaled(V3{ 0.85f, 0.70f, 0.42f }, (2.2f * (isDayTime ? 0.25f : 1.0f)))) : mPaper;

    b.box({ RX0, TY - 0.06f, RZ0 }, { RX1, TY, RZ1 }, mTatami);
    b.box({ RX0, TY, RZ0 - 0.10f }, { RX1, CY, RZ0 + 0.01f }, mPlastZ);           // back wall
    b.box({ RX0 - 0.10f, TY, RZ0 }, { RX0 + 0.01f, CY, RZ1 }, mPlastX);           // left wall
    b.box({ RX1 - 0.01f, TY, RZ0 }, { RX1 + 0.10f, CY, RZ1 }, mPlastX);           // staircase partition
    b.box({ RX0, TY, RZ1 - 0.01f }, { RX1, CY, RZ1 + 0.10f }, mPlastZ);           // front wall
    b.box({ -4.2f, 3.65f, RZ1 - 0.02f }, { -2.2f, 4.95f, RZ1 - 0.01f }, mPaper);  // shoji windows of the facade
    b.box({ 2.2f, 3.65f, RZ1 - 0.02f }, { RX1 - 0.01f, 4.95f, RZ1 - 0.01f }, mPaper);
    b.box({ RX0, CY, RZ0 }, { RX1, CY + 0.05f, RZ1 }, mBoards);                   // board ceiling
    const float postX[4] = { RX0 + 0.06f, -2.35f, 1.05f, RX1 - 0.06f };
    for (float px : postX) b.cuboid(px, TY, RZ0 + 0.06f, 0.11f, 1.82f, 0.11f, mTimber);
    const float postZ[3] = { -1.3f, 1.3f, RZ1 - 0.06f };
    for (float pz : postZ) b.cuboid(RX0 + 0.06f, TY, pz, 0.11f, 1.82f, 0.11f, mTimber);
    b.box({ -0.29f, 3.3f + 2.0f - 0.36f, -0.29f }, { 0.29f, 3.3f + 2.0f - 0.22f, 0.29f }, mDome);   // washi ceiling lamp

    // Chabudai tea table with the tea set
    {
        const float tx = -0.6f, tz = 0.7f, top = TY + 0.32f;
        b.box({ tx - 0.55f, TY + 0.28f, tz - 0.34f }, { tx + 0.55f, top, tz + 0.34f }, mChabu);
        for (int sx = -1; sx <= 1; sx += 2)
            for (int sz = -1; sz <= 1; sz += 2)
                b.cuboid(tx + sx * 0.46f, TY, tz + sz * 0.25f, 0.05f, 0.28f, 0.05f, mLegs);
        b.disk({ tx - 0.18f, top + 0.012f, tz + 0.02f }, 0.0f, 0.17f, 1.0f, diffuse(s, { 0.45f, 0.10f, 0.08f }, { 0.4f, 0.4f, 0.4f }, 40.0f));
        b.sphere({ tx - 0.22f, top + 0.065f, tz + 0.02f }, 0.06f, diffuse(s, { 0.36f, 0.16f, 0.10f }, { 0.5f, 0.5f, 0.5f }, 60.0f));
        int mCup1 = diffuse(s, { 0.30f, 0.45f, 0.32f }, { 0.5f, 0.5f, 0.5f }, 60.0f), mCup2 = diffuse(s, { 0.82f, 0.80f, 0.72f }, { 0.5f, 0.5f, 0.5f }, 60.0f);
        b.cone({ tx + 0.10f, top, tz - 0.10f }, 0.032f, 0.036f, 0.085f, mCup1);
        b.cone({ tx + 0.24f, top, tz + 0.12f }, 0.032f, 0.036f, 0.085f, mCup2);
        int mVase = diffuse(s, { 0.93f, 0.90f, 0.88f }, { 0.5f, 0.5f, 0.5f }, 60.0f);
        b.cone({ tx + 0.36f, top, tz - 0.08f }, 0.075f, 0.070f, 0.24f, mVase);
        b.disk({ tx + 0.36f, top + 0.24f, tz - 0.08f }, 0.0f, 0.070f, 1.0f, diffuse(s, { 0.20f, 0.18f, 0.18f }));
    }
    // Zabuton cushions
    {
        int mZab = diffuse(s, { 0.50f, 0.56f, 0.44f }), mZab2 = diffuse(s, { 0.58f, 0.22f, 0.18f });
        b.cuboid(-0.6f, TY, 1.45f, 0.56f, 0.075f, 0.56f, mZab);
        b.cuboid(-0.6f, TY, -0.05f, 0.56f, 0.075f, 0.56f, mZab);
        b.cuboid(-1.55f, TY, 0.7f, 0.56f, 0.075f, 0.56f, mZab2);
    }
    // Tansu chest and the folded futons on the left wall
    b.box({ RX0 + 0.035f, TY, -0.725f }, { RX0 + 0.485f, TY + 1.45f, 0.325f }, mTansu);
    {
        const float fx = RX0 + 0.55f, fz = 1.55f;
        int mF1 = diffuse(s, { 0.92f, 0.90f, 0.84f }), mF2 = diffuse(s, { 0.68f, 0.20f, 0.22f }), mF3 = diffuse(s, { 0.82f, 0.82f, 0.86f });
        b.cuboid(fx, TY, fz, 0.95f, 0.13f, 0.70f, mF3);
        b.cuboid(fx, TY + 0.13f, fz, 0.93f, 0.12f, 0.69f, mF1);
        b.cuboid(fx, TY + 0.25f, fz, 0.94f, 0.13f, 0.70f, mF2);
        b.cuboid(fx, TY + 0.38f, fz, 0.90f, 0.10f, 0.66f, mF1);
        b.cuboid(fx + 0.10f, TY + 0.48f, fz + 0.05f, 0.42f, 0.10f, 0.26f, diffuse(s, { 0.94f, 0.92f, 0.86f }));
    }
    // Futon bed and the paper floor lamp beside it
    {
        const float bx = 2.15f;
        int mSheet = diffuse(s, { 0.96f, 0.95f, 0.91f }), mQuilt = diffuse(s, { 0.74f, 0.27f, 0.28f });
        b.box({ bx - 0.525f, TY, 1.55f }, { bx + 0.525f, TY + 0.07f, 3.50f }, mSheet);
        b.box({ bx - 0.575f, TY + 0.07f, 1.51f }, { bx + 0.575f, TY + 0.18f, 2.95f }, mQuilt);
        b.box({ bx - 0.23f, TY + 0.07f, 3.14f }, { bx + 0.23f, TY + 0.20f, 3.38f }, mSheet);
        b.cuboid(bx + 0.95f, TY + 0.10f, 3.25f, 0.22f, 0.40f, 0.22f, mPaper);
    }

    // ── Featured: the mirror (glass with silver backing) in its timber frame ──
    b.box({ MIR_X0, MIR_Y0, RZ0 + 0.01f }, { MIR_X1, MIR_Y1, MIR_Z }, mMirror, RTO_MIRROR);
    b.box({ MIR_X0 - FRAME, MIR_Y0 - FRAME, RZ0 + 0.01f }, { MIR_X0, MIR_Y1 + FRAME, MIR_Z + 0.025f }, mTimber);
    b.box({ MIR_X1, MIR_Y0 - FRAME, RZ0 + 0.01f }, { MIR_X1 + FRAME, MIR_Y1 + FRAME, MIR_Z + 0.025f }, mTimber);
    b.box({ MIR_X0, MIR_Y1, RZ0 + 0.01f }, { MIR_X1, MIR_Y1 + FRAME, MIR_Z + 0.025f }, mTimber);
    b.box({ MIR_X0, MIR_Y0 - FRAME, RZ0 + 0.01f }, { MIR_X1, MIR_Y0, MIR_Z + 0.025f }, mTimber);
    bounds[RTO_MIRROR] = { true, { MIR_X0, MIR_Y0, RZ0 }, { MIR_X1, MIR_Y1, MIR_Z } };

    s.build();
}

// ─── OpenGL versions of the new objects ─────────────────────────────────────
void drawCounterGlassBall()
{
    // small turned-wood stand
    drawCylinderCustom({ BALL_STAND.x, BALL_STAND.y, BALL_STAND.z }, NO_ROT, ONE, DARK_WOOD, STAND_R, STAND_R, STAND_H);

    // clear glass ball: blended, but it writes depth so the ray-traced version can find it
    glPushMatrix();
    glTranslatef(BALL_C.x, BALL_C.y, BALL_C.z);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    setMaterialGloss(1.0f, 1.0f, 1.0f, 140.0f);
    glColor4f(CLEAR_GLASS.r, CLEAR_GLASS.g, CLEAR_GLASS.b, 0.22f);
    gluSphere(quad, BALL_R, SLICES, STACKS);
    resetMaterialGloss();
    glDisable(GL_BLEND);
    glPopMatrix();
}

void drawRoomMirror()
{
    // timber frame
    drawCuboid({ MIR_X0 - FRAME * 0.5f, MIR_Y0 - FRAME, (RZ0 + 0.01f + MIR_Z + 0.025f) * 0.5f }, NO_ROT,
               { FRAME, MIR_Y1 - MIR_Y0 + 2 * FRAME, MIR_Z + 0.015f - RZ0 }, TIMBER);
    drawCuboid({ MIR_X1 + FRAME * 0.5f, MIR_Y0 - FRAME, (RZ0 + 0.01f + MIR_Z + 0.025f) * 0.5f }, NO_ROT,
               { FRAME, MIR_Y1 - MIR_Y0 + 2 * FRAME, MIR_Z + 0.015f - RZ0 }, TIMBER);
    drawCuboid({ (MIR_X0 + MIR_X1) * 0.5f, MIR_Y1, (RZ0 + 0.01f + MIR_Z + 0.025f) * 0.5f }, NO_ROT,
               { MIR_X1 - MIR_X0, FRAME, MIR_Z + 0.015f - RZ0 }, TIMBER);
    drawCuboid({ (MIR_X0 + MIR_X1) * 0.5f, MIR_Y0 - FRAME, (RZ0 + 0.01f + MIR_Z + 0.025f) * 0.5f }, NO_ROT,
               { MIR_X1 - MIR_X0, FRAME, MIR_Z + 0.015f - RZ0 }, TIMBER);

    // silvered glass: without ray tracing just a bright, glossy grey surface
    setMaterialGloss(1.0f, 1.0f, 1.0f, 120.0f);
    glColor3f(0.70f, 0.74f, 0.78f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(MIR_X0, MIR_Y0, MIR_Z); glVertex3f(MIR_X1, MIR_Y0, MIR_Z);
    glVertex3f(MIR_X1, MIR_Y1, MIR_Z); glVertex3f(MIR_X0, MIR_Y1, MIR_Z);
    glEnd();
    resetMaterialGloss();
}
