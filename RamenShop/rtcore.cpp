#include "rtcore.h"
#include <cmath>
#include <algorithm>
#include <functional>

namespace rt {

// ─── Vector helpers ──────────────────────────────────────────────────────────
static inline V3 operator+(V3 a, V3 b)   { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline V3 operator-(V3 a, V3 b)   { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline V3 operator-(V3 a)         { return { -a.x, -a.y, -a.z }; }
static inline V3 operator*(V3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
static inline V3 operator*(V3 a, V3 b)   { return { a.x * b.x, a.y * b.y, a.z * b.z }; }
static inline float dot(V3 a, V3 b)      { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float length(V3 a)         { return sqrtf(dot(a, a)); }
static inline V3 normalize(V3 a)         { float l = length(a); return l > 0.0f ? a * (1.0f / l) : a; }
static inline float lum(V3 c)            { return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z; }
static inline V3 reflectDir(V3 d, V3 n)  { return d - n * (2.0f * dot(d, n)); }
static inline V3 clamp01(V3 c)           { return { fminf(fmaxf(c.x, 0.0f), 1.0f), fminf(fmaxf(c.y, 0.0f), 1.0f), fminf(fmaxf(c.z, 0.0f), 1.0f) }; }
static inline V3 vmin(V3 a, V3 b)        { return { fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z) }; }
static inline V3 vmax(V3 a, V3 b)        { return { fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z) }; }

static const float EPS = 1e-4f;

V3 Texture::sample(float u, float v) const
{
    if (w <= 0 || h <= 0) return { 1, 1, 1 };
    float x = u * w - 0.5f, y = v * h - 0.5f;
    float fx = floorf(x), fy = floorf(y);
    float ax = x - fx, ay = y - fy;
    int x0 = ((int)fx % w + w) % w, y0 = ((int)fy % h + h) % h;
    int x1 = (x0 + 1) % w, y1 = (y0 + 1) % h;
    auto px = [&](int xx, int yy) {
        const uint8_t* p = &rgb[((size_t)yy * w + xx) * 3];
        return V3{ p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f };
    };
    V3 top = px(x0, y0) * (1 - ax) + px(x1, y0) * ax;
    V3 bot = px(x0, y1) * (1 - ax) + px(x1, y1) * ax;
    return top * (1 - ay) + bot * ay;
}

// ─── Shape intersections (world space; each keeps the closer hit in h) ──────
static void hitBox(const Shape& s, int id, V3 o, V3 d, V3 invD, float tmin, Hit& h)
{
    float tNear = -1e30f, tFar = 1e30f;
    int axN = 0, axF = 0;
    const float* O = &o.x; const float* D = &d.x; const float* I = &invD.x; const float* MN = &s.a.x; const float* MX = &s.b.x;
    for (int i = 0; i < 3; i++) {
        float t0 = (MN[i] - O[i]) * I[i], t1 = (MX[i] - O[i]) * I[i];
        if (t0 > t1) std::swap(t0, t1);
        if (t0 > tNear) { tNear = t0; axN = i; }
        if (t1 < tFar)  { tFar = t1; axF = i; }
        if (tNear > tFar) return;
    }
    float t; int ax; float sign;
    if (tNear > tmin)     { t = tNear; ax = axN; sign = D[axN] > 0 ? -1.0f : 1.0f; }   // entering
    else if (tFar > tmin) { t = tFar;  ax = axF; sign = D[axF] > 0 ? 1.0f : -1.0f; }   // leaving (origin inside)
    else return;
    if (t >= h.t) return;
    h.t = t; h.p = o + d * t; h.shape = id;
    h.n = { 0, 0, 0 };
    (&h.n.x)[ax] = sign;
}

static void hitCone(const Shape& s, int id, V3 o, V3 d, float tmin, Hit& h)
{
    V3 lo = { o.x - s.a.x, o.y - s.a.y, o.z - s.a.z };   // relative to the base centre
    float k  = (s.r1 - s.r0) / s.h;                      // radius r(y) = r0 + k y
    float ro = s.r0 + k * lo.y;
    float A  = d.x * d.x + d.z * d.z - k * k * d.y * d.y;
    float B  = 2.0f * (lo.x * d.x + lo.z * d.z - k * d.y * ro);
    float C  = lo.x * lo.x + lo.z * lo.z - ro * ro;
    float ts[2];
    int n = 0;
    if (fabsf(A) < 1e-12f) {
        if (fabsf(B) > 1e-12f) ts[n++] = -C / B;
    } else {
        float disc = B * B - 4.0f * A * C;
        if (disc < 0.0f) return;
        float sq = sqrtf(disc);
        float q0 = (-B - sq) / (2.0f * A), q1 = (-B + sq) / (2.0f * A);
        ts[n++] = fminf(q0, q1);
        ts[n++] = fmaxf(q0, q1);
    }
    for (int i = 0; i < n; i++) {
        float t = ts[i];
        if (t <= tmin || t >= h.t) continue;
        float y = lo.y + t * d.y;
        if (y < 0.0f || y > s.h) continue;
        float px = lo.x + t * d.x, pz = lo.z + t * d.z;
        h.t = t; h.p = o + d * t; h.shape = id;
        h.n = normalize(V3{ px, -k * (s.r0 + k * y), pz });   // outward
        return;                                            // roots are sorted: the first valid one is closest
    }
}

static void hitDisk(const Shape& s, int id, V3 o, V3 d, float tmin, Hit& h)
{
    if (fabsf(d.y) < 1e-12f) return;
    float t = (s.a.y - o.y) / d.y;
    if (t <= tmin || t >= h.t) return;
    float px = o.x + t * d.x - s.a.x, pz = o.z + t * d.z - s.a.z, r2 = px * px + pz * pz;
    if (r2 < s.r0 * s.r0 || r2 > s.r1 * s.r1) return;
    h.t = t; h.p = o + d * t; h.shape = id;
    h.n = { 0, s.h, 0 };
}

static void hitSphere(const Shape& s, int id, V3 o, V3 d, float tmin, Hit& h)
{
    V3 oc = o - s.a;
    float b = dot(oc, d), c = dot(oc, oc) - s.r0 * s.r0;
    float disc = b * b - c;
    if (disc < 0.0f) return;
    float sq = sqrtf(disc);
    float t = -b - sq;
    if (t <= tmin) t = -b + sq;
    if (t <= tmin || t >= h.t) return;
    h.t = t; h.p = o + d * t; h.shape = id;
    h.n = (h.p - s.a) * (1.0f / s.r0);
}

static void hitShape(const Scene& sc, int id, V3 o, V3 d, V3 invD, float tmin, Hit& h)
{
    const Shape& s = sc.shapes[id];
    if ((s.facing.x != 0.0f || s.facing.y != 0.0f || s.facing.z != 0.0f) && dot(d, s.facing) >= 0.0f) return;
    switch (s.type) {
    case SH_BOX:    hitBox(s, id, o, d, invD, tmin, h); break;
    case SH_CONE:   hitCone(s, id, o, d, tmin, h); break;
    case SH_DISK:   hitDisk(s, id, o, d, tmin, h); break;
    case SH_SPHERE: hitSphere(s, id, o, d, tmin, h); break;
    }
    if (s.flip && h.shape == id) h.n = -h.n;
}

// ─── BVH ─────────────────────────────────────────────────────────────────────
void Scene::build()
{
    for (Shape& s : shapes) {
        switch (s.type) {
        case SH_BOX:    s.bmin = s.a; s.bmax = s.b; break;
        case SH_CONE: { float r = fmaxf(s.r0, s.r1);
                        s.bmin = { s.a.x - r, s.a.y, s.a.z - r }; s.bmax = { s.a.x + r, s.a.y + s.h, s.a.z + r }; break; }
        case SH_DISK:   s.bmin = { s.a.x - s.r1, s.a.y - 1e-4f, s.a.z - s.r1 }; s.bmax = { s.a.x + s.r1, s.a.y + 1e-4f, s.a.z + s.r1 }; break;
        case SH_SPHERE: s.bmin = s.a - V3{ s.r0, s.r0, s.r0 }; s.bmax = s.a + V3{ s.r0, s.r0, s.r0 }; break;
        }
    }
    order.clear();
    large.clear();
    featured.clear();
    objectShapes.clear();
    for (size_t i = 0; i < shapes.size(); i++) {
        if (shapes[i].object >= 0) {
            featured.push_back((int)i);
            if ((int)objectShapes.size() <= shapes[i].object) objectShapes.resize(shapes[i].object + 1);
            objectShapes[shapes[i].object].push_back((int)i);
        }
        V3 e = shapes[i].bmax - shapes[i].bmin;
        int big = (e.x > 3.0f) + (e.y > 3.0f) + (e.z > 3.0f);
        if (big >= 2) large.push_back((int)i); else order.push_back((int)i);
    }
    nodes.clear();
    if (order.empty()) return;

    // top-down: split the shapes at the median of their centres along the longest axis
    std::function<int(int, int)> buildNode = [&](int first, int count) -> int {
        Node nd;
        nd.bmin = { 1e30f, 1e30f, 1e30f }; nd.bmax = { -1e30f, -1e30f, -1e30f };
        for (int i = first; i < first + count; i++) { nd.bmin = vmin(nd.bmin, shapes[order[i]].bmin); nd.bmax = vmax(nd.bmax, shapes[order[i]].bmax); }
        int idx = (int)nodes.size();
        nodes.push_back(nd);
        if (count <= 2) { nodes[idx].left = nodes[idx].right = -1; nodes[idx].first = first; nodes[idx].count = count; return idx; }
        V3 ext = nd.bmax - nd.bmin;
        int axis = (ext.x > ext.y && ext.x > ext.z) ? 0 : (ext.y > ext.z ? 1 : 2);
        auto centre = [&](int i) { const Shape& s = shapes[i]; return (&s.bmin.x)[axis] + (&s.bmax.x)[axis]; };
        int mid = first + count / 2;
        std::nth_element(order.begin() + first, order.begin() + mid, order.begin() + first + count,
                         [&](int x, int y) { return centre(x) < centre(y); });
        int l = buildNode(first, mid - first);
        int r = buildNode(mid, first + count - mid);
        nodes[idx].left = l; nodes[idx].right = r; nodes[idx].first = 0; nodes[idx].count = 0;
        return idx;
    };
    buildNode(0, (int)order.size());
}

static inline bool boxHit(V3 mn, V3 mx, V3 o, V3 inv, float tmin, float tmax, float& tn)
{
    float t0x = (mn.x - o.x) * inv.x, t1x = (mx.x - o.x) * inv.x;
    float t0y = (mn.y - o.y) * inv.y, t1y = (mx.y - o.y) * inv.y;
    float t0z = (mn.z - o.z) * inv.z, t1z = (mx.z - o.z) * inv.z;
    tn = fmaxf(fmaxf(fminf(t0x, t1x), fminf(t0y, t1y)), fmaxf(fminf(t0z, t1z), tmin));
    float tf = fminf(fminf(fmaxf(t0x, t1x), fmaxf(t0y, t1y)), fminf(fmaxf(t0z, t1z), tmax));
    return tn <= tf;
}

bool intersect(const Scene& s, V3 o, V3 d, float tmin, float tmax, Hit& h)
{
    h.t = tmax;
    h.shape = -1;
    auto safeInv = [](float v) { return 1.0f / (fabsf(v) < 1e-12f ? (v < 0 ? -1e-12f : 1e-12f) : v); };
    V3 inv = { safeInv(d.x), safeInv(d.y), safeInv(d.z) };
    for (int id : s.large) hitShape(s, id, o, d, inv, tmin, h);  // floors, walls, ceilings
    if (s.nodes.empty()) return h.shape >= 0;
    int stack[64];
    int sp = 0;
    float tn;
    if (boxHit(s.nodes[0].bmin, s.nodes[0].bmax, o, inv, tmin, h.t, tn)) stack[sp++] = 0;
    while (sp > 0) {
        const Scene::Node& nd = s.nodes[stack[--sp]];
        if (nd.left < 0) {
            for (int i = nd.first; i < nd.first + nd.count; i++) hitShape(s, s.order[i], o, d, inv, tmin, h);
            continue;
        }
        float tl, tr;                                            // nearer child first
        bool hl = boxHit(s.nodes[nd.left].bmin, s.nodes[nd.left].bmax, o, inv, tmin, h.t, tl);
        bool hr = boxHit(s.nodes[nd.right].bmin, s.nodes[nd.right].bmax, o, inv, tmin, h.t, tr);
        if (sp > 60) continue;
        if (hl && hr) {
            if (tl <= tr) { stack[sp++] = nd.right; stack[sp++] = nd.left; }
            else          { stack[sp++] = nd.left;  stack[sp++] = nd.right; }
        } else if (hl) stack[sp++] = nd.left;
        else if (hr)   stack[sp++] = nd.right;
    }
    return h.shape >= 0;
}

bool refractDir(V3 d, V3 n, float eta, V3& t)
{
    float cosi = -dot(d, n);
    float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
    if (k < 0.0f) return false;                  // total internal reflection
    t = normalize(d * eta + n * (eta * cosi - sqrtf(k)));
    return true;
}

// ─── Shading ─────────────────────────────────────────────────────────────────
static V3 baseColor(const Material& m, V3 p)
{
    if (!m.tex || m.map == MAP_NONE) return m.color;
    float u = 0, v = 0;
    switch (m.map) {
    case MAP_XZ: u = (p.x - m.u0) / m.uvScale; v = (p.z - m.v0) / m.uvScale; break;
    case MAP_XY: u = (p.x - m.u0) / m.uvScale; v = (p.y - m.v0) / m.uvScale; break;
    case MAP_ZY: u = (p.z - m.u0) / m.uvScale; v = (p.y - m.v0) / m.uvScale; break;
    case MAP_TATAMI: {                           // mats 1.90 x 0.95 in a running bond from (u0, v0), cloth border
        const float ML = 1.90f, MW = 0.95f;
        float zr = p.z - m.v0;
        int row = (int)floorf(zr / MW);
        float inRow = zr - row * MW;
        if (inRow < 0.04f || inRow > MW - 0.04f) return { 0.10f, 0.14f, 0.10f };   // heri
        float start = m.u0 - ((row & 1) ? ML * 0.5f : 0.0f);
        float k = floorf((p.x - start) / ML);
        float matX0 = fmaxf(m.u0, start + k * ML);
        u = (p.x - matX0) / ML; v = inRow / ML;
        break;
    }
    default: break;
    }
    return m.color * m.tex->sample(u, v);
}

static float lightAt(const Light& L, V3 p, V3& dir)
{
    if (L.directional) { dir = normalize(L.pos); return 1.0f; }
    V3 tl = L.pos - p;
    float dist = length(tl);
    dir = tl * (1.0f / fmaxf(dist, 1e-6f));
    float att = 1.0f / (L.kc + L.kl * dist + L.kq * dist * dist);
    if (L.spotCos > -1.5f) {
        float sc = dot(-dir, normalize(L.spotDir));
        if (sc < L.spotCos) return 0.0f;
        att *= powf(fmaxf(sc, 0.0f), L.spotExp);
    }
    return att;
}

// Blinn-Phong with the shop's lights (the model of the OpenGL Phong shader; no shadows)
static V3 phong(const Scene& s, const Material& m, V3 p, V3 N, V3 V)
{
    V3 base = baseColor(m, p);
    V3 col = s.globalAmb * base;
    for (const Light& L : s.lights) {
        if (!L.on) continue;
        V3 dir;
        float att = lightAt(L, p, dir);
        if (att <= 0.0f) continue;
        col = col + L.amb * base * att;
        float ndl = dot(N, dir);
        if (ndl <= 0.0f) continue;
        V3 H = normalize(dir + V);
        col = col + (L.dif * base * ndl + L.spec * m.spec * powf(fmaxf(dot(N, H), 0.0f), m.shininess)) * att;
    }
    return clamp01(col);
}

static inline V3 schlick(V3 f0, float c)
{
    float k = powf(1.0f - fminf(fmaxf(c, 0.0f), 1.0f), 5.0f);
    return { f0.x + (1 - f0.x) * k, f0.y + (1 - f0.y) * k, f0.z + (1 - f0.z) * k };
}

static V3 radiance(const Scene& s, V3 o, V3 d, int depth);

// Closest hit with the shapes of one object only (a ray inside a glass can reach nothing else first)
static bool intersectObject(const Scene& s, int obj, V3 o, V3 d, Hit& h)
{
    h.t = 1e30f;
    h.shape = -1;
    if (obj < 0 || obj >= (int)s.objectShapes.size()) return false;
    auto safeInv = [](float v) { return 1.0f / (fabsf(v) < 1e-12f ? (v < 0 ? -1e-12f : 1e-12f) : v); };
    V3 inv = { safeInv(d.x), safeInv(d.y), safeInv(d.z) };
    for (int id : s.objectShapes[obj]) hitShape(s, id, o, d, inv, 0.0f, h);
    return h.shape >= 0;
}

// Glass and water: Fresnel reflection at the first surface, Snell refraction through every
// surface (each one treated as an air / medium boundary; hollow glasses have an inner wall)
static V3 glassRadiance(const Scene& s, Hit cur, V3 d, int depth)
{
    V3 col = { 0, 0, 0 }, thr = { 1, 1, 1 };
    for (int i = 0; i < 10; i++) {
        const Material& gm = s.materials[s.shapes[cur.shape].material];
        const float IOR = gm.ior;
        const float R0 = ((1.0f - IOR) / (1.0f + IOR)) * ((1.0f - IOR) / (1.0f + IOR));
        bool entering = dot(d, cur.n) < 0.0f;
        V3 n = entering ? cur.n : -cur.n;        // facing against the ray
        float eta = entering ? 1.0f / IOR : IOR;
        V3 t;
        bool tir = !refractDir(d, n, eta, t);
        float c = entering ? -dot(d, n) : (tir ? 0.0f : -dot(t, n));    // cosine on the air side
        float F = tir ? 1.0f : R0 + (1.0f - R0) * powf(1.0f - c, 5.0f);
        if (i == 0 && depth < s.maxDepth) col = col + radiance(s, cur.p + n * EPS, reflectDir(d, n), depth + 1) * F;
        if (tir) { d = reflectDir(d, n); cur.p = cur.p + n * EPS; }
        else     { thr = thr * (1.0f - F); d = t; cur.p = cur.p - n * EPS; }
        thr = thr * gm.color;                    // absorption of the medium
        Hit nh;
        // next surface of the same glass object (fast); once the ray has left it, the whole scene
        if (!intersectObject(s, s.shapes[cur.shape].object, cur.p, d, nh) &&
            !intersect(s, cur.p, d, 0.0f, 1e30f, nh)) return col + thr * s.sky;
        if (s.materials[s.shapes[nh.shape].material].type != MAT_GLASS)
            return col + thr * radiance(s, cur.p, d, depth + 1);
        cur = nh;
    }
    return col;
}

static V3 shadeHit(const Scene& s, const Hit& h, V3 d, int depth)
{
    const Material& m = s.materials[s.shapes[h.shape].material];
    V3 V = -d;
    V3 N = dot(h.n, V) >= 0.0f ? h.n : -h.n;
    switch (m.type) {
    case MAT_EMISSIVE: return m.emission;
    case MAT_DIFFUSE:  return phong(s, m, h.p, N, V);
    case MAT_GLASS:    return depth < s.maxDepth ? glassRadiance(s, h, d, depth) : m.color * 0.5f;
    default: {                                   // metal, glaze, liquid, mirror: lit surface + mirror image
        V3 base = phong(s, m, h.p, N, V);
        if (depth >= s.maxDepth) return base;
        V3 F = schlick(m.f0, dot(N, V));
        V3 refl = radiance(s, h.p + N * EPS, reflectDir(d, N), depth + 1);
        return base * (V3{ 1, 1, 1 } - F) + refl * F;
    }
    }
}

static V3 radiance(const Scene& s, V3 o, V3 d, int depth)
{
    Hit h;
    if (!intersect(s, o, d, EPS, 1e30f, h)) return s.sky;
    return shadeHit(s, h, d, depth);
}

static float windowDepth(const Camera& cam, V3 p)
{
    const float* m = cam.viewProj;
    float cz = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
    float cw = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
    return cw > 1e-6f ? cz / cw * 0.5f + 0.5f : 1.0f;
}

// ─── One eye ray ─────────────────────────────────────────────────────────────
void tracePixel(const Scene& s, const Camera& cam, V3 dir, PixelOut& out)
{
    out.add[0] = out.add[1] = out.add[2] = 0.0f;
    out.weight = 1.0f;
    out.depth = 1.0f;
    out.mode = 0.0f;

    // Eye ray: only the ray-traced objects are tested.  Whether something else is in front of
    // them is decided by the compositing shader with the OpenGL depth buffer (same surface test).
    Hit h;
    h.t = 1e30f;
    h.shape = -1;
    auto safeInv = [](float v) { return 1.0f / (fabsf(v) < 1e-12f ? (v < 0 ? -1e-12f : 1e-12f) : v); };
    V3 inv = { safeInv(dir.x), safeInv(dir.y), safeInv(dir.z) };
    for (int id : s.featured) hitShape(s, id, cam.pos, dir, inv, 0.0f, h);
    if (h.shape < 0) return;                     // no ray-traced object here: keep the raster pixel
    const Shape& sh = s.shapes[h.shape];
    const Material& m = s.materials[sh.material];
    V3 V = -dir;
    V3 N = dot(h.n, V) >= 0.0f ? h.n : -h.n;
    V3 add;
    float weight;
    switch (m.type) {
    case MAT_METAL: case MAT_GLAZE: case MAT_LIQUID: {
        // Real reflection on top of the rasterized (lit) surface:  raster * (1 - F) + reflection * F
        V3 F = schlick(m.f0, dot(N, V));
        add = radiance(s, h.p + N * EPS, reflectDir(dir, N), 1) * F;
        weight = 1.0f - lum(F);
        break;
    }
    case MAT_MIRROR: {
        V3 F = schlick(m.f0, dot(N, V));
        add = phong(s, m, h.p, N, V) * (V3{ 1, 1, 1 } - F) + radiance(s, h.p + N * EPS, reflectDir(dir, N), 1) * F;
        weight = 0.0f;
        break;
    }
    case MAT_GLASS:
        add = glassRadiance(s, h, dir, 0);
        weight = 0.0f;
        break;
    default:
        return;
    }
    out.add[0] = add.x; out.add[1] = add.y; out.add[2] = add.z;
    out.weight = weight;
    out.depth = windowDepth(cam, h.p);
    out.mode = sh.rasterDepth ? 1.0f : 2.0f;
}

}   // namespace rt
