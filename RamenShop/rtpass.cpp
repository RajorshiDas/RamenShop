#include "rtpass.h"
#include "rtcore.h"
#include "rtscene.h"
#include "shader.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <array>

bool useRayTracing = false;

static int  g_rayBudget = 150000;             // eye rays per frame; adapts to keep the CPU time near the target
static const double TARGET_MS = 25.0;

static bool   g_ready = false;
static GLuint g_prog = 0;
static GLint  u_add = -1, u_info = -1, u_color = -1, u_depth = -1, u_viewport = -1, u_grid = -1, u_cell = -1,
              u_near = -1, u_far = -1;
static GLuint g_addTex = 0, g_infoTex = 0, g_colorTex = 0, g_depthTex = 0;
static int    g_gridW = 0, g_gridH = 0, g_texW = 0, g_texH = 0, g_frameW = 0, g_frameH = 0;
static std::vector<float> g_addBuf, g_infoBuf;   // grid cells: rgb + weight;  depth + flag
static std::vector<float> g_accAddBuf;           // temporal accumulation of add (colour + weight)
static float g_prevVP[16] = { 0 };
static int   g_accFrames = 0;                    // frames accumulated (reset on camera change)
static int   g_frameNum = 0;                     // monotonic frame counter for jitter seed
static double g_msAvg = 0.0, g_lastReport = 0.0;

// Hash-based jitter: deterministic per (cell, frame, axis), in [-0.4, 0.4]
static inline float cellJitter(int cx, int cy, int frame, int axis) {
    unsigned h = (unsigned)(cx * 73856093u ^ cy * 19349663u ^ frame * 83492791u ^ axis * 39916801u);
    return ((float)(h & 0xFFFFu) / 65536.0f - 0.5f) * 0.8f;
}

static double nowMs()
{
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

// ─── Compositing shader:  colour = add + weight * raster, only where the raster frame
//     shows the same surface (depth buffer), so nearer objects keep hiding it ──────
static const char* compVertSrc = R"GLSL(
#version 120
void main() { gl_Position = ftransform(); }
)GLSL";

static const char* compFragSrc = R"GLSL(
#version 120
uniform sampler2D rtAdd;       // grid cells: rgb = ray-traced light, a = weight of the raster colour
uniform sampler2D rtInfo;      // grid cells: r = window depth of the traced surface, g = 1 / 2 where traced
uniform sampler2D frameColor;  // the rasterized frame
uniform sampler2D frameDepth;  // its depth buffer
uniform vec2  viewport;
uniform vec2  gridSize;        // texture size in cells
uniform float cell;            // pixels per cell
uniform float zNear;
uniform float zFar;

float linearDepth(float d)
{
    float z = d * 2.0 - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z * (zFar - zNear));
}

void main()
{
    vec2 c    = (floor(gl_FragCoord.xy / cell) + 0.5) / gridSize;
    vec4 info = texture2D(rtInfo, c);
    if (info.g < 0.5) discard;
    float traced = linearDepth(info.r);
    float raster = linearDepth(texture2D(frameDepth, gl_FragCoord.xy / viewport).r);
    float tol    = 0.002 + 0.0015 * traced;
    if (info.g < 1.5) { if (abs(raster - traced) > tol) discard; }  // 1: the raster shows this same surface
    else if (raster < traced - tol) discard;                         // 2: clear glass, nothing opaque in front
    vec2 sc   = gl_FragCoord.xy / (cell * gridSize);                 // continuous UV for bilinear color
    vec2 ts   = 1.0 / gridSize;                                      // one texel in UV space
    // 3x3 Gaussian-weighted blur (1-2-1 kernel) for smooth reflections
    vec4 a  = texture2D(rtAdd, sc) * 4.0;
    a += texture2D(rtAdd, sc + vec2(-ts.x, 0.0)) * 2.0;
    a += texture2D(rtAdd, sc + vec2( ts.x, 0.0)) * 2.0;
    a += texture2D(rtAdd, sc + vec2(0.0, -ts.y)) * 2.0;
    a += texture2D(rtAdd, sc + vec2(0.0,  ts.y)) * 2.0;
    a += texture2D(rtAdd, sc + vec2(-ts.x, -ts.y));
    a += texture2D(rtAdd, sc + vec2( ts.x, -ts.y));
    a += texture2D(rtAdd, sc + vec2(-ts.x,  ts.y));
    a += texture2D(rtAdd, sc + vec2( ts.x,  ts.y));
    a /= 16.0;
    gl_FragColor = vec4(a.rgb + a.a * texture2D(frameColor, gl_FragCoord.xy / viewport).rgb, 1.0);
}
)GLSL";

static GLuint compileStage(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "[RT] %s shader compile error:\n%s\n", type == GL_VERTEX_SHADER ? "Vertex" : "Fragment", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint makeTexture(GLenum filter = GL_NEAREST)
{
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

void initRayTracing()
{
    rtLoadTextures();

    GLuint vs = compileStage(GL_VERTEX_SHADER, compVertSrc);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, compFragSrc);
    if (!vs || !fs) { fprintf(stderr, "[RT] Ray tracing unavailable (compositing shader).\n"); return; }
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(g_prog, sizeof(log), NULL, log);
        fprintf(stderr, "[RT] Compositing program link error:\n%s\n", log);
        glDeleteProgram(g_prog);
        g_prog = 0;
        return;
    }
    u_add      = glGetUniformLocation(g_prog, "rtAdd");
    u_info     = glGetUniformLocation(g_prog, "rtInfo");
    u_color    = glGetUniformLocation(g_prog, "frameColor");
    u_depth    = glGetUniformLocation(g_prog, "frameDepth");
    u_viewport = glGetUniformLocation(g_prog, "viewport");
    u_grid     = glGetUniformLocation(g_prog, "gridSize");
    u_cell     = glGetUniformLocation(g_prog, "cell");
    u_near     = glGetUniformLocation(g_prog, "zNear");
    u_far      = glGetUniformLocation(g_prog, "zFar");
    g_addTex   = makeTexture(GL_LINEAR);   // bilinear for smooth color interpolation
    g_infoTex  = makeTexture();            // nearest for crisp depth/mode decisions
    g_colorTex = makeTexture();
    g_depthTex = makeTexture();
    g_ready = true;
    fprintf(stderr, "[RT] CPU ray tracer ready (%u threads): pots, bowls, glass ball, upper-room mirror.  Press R.\n",
            std::max(1u, std::thread::hardware_concurrency()));
}

bool rayTracingActive() { return useRayTracing && g_ready; }

// ─── Matrix helpers (column-major) ───────────────────────────────────────────
static void mul4(const float* A, const float* B, float* C)
{
    float R[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++) {
            float v = 0.0f;
            for (int k = 0; k < 4; k++) v += A[k * 4 + r] * B[c * 4 + k];
            R[c * 4 + r] = v;
        }
    memcpy(C, R, sizeof(R));
}

static bool invert4(const float* m, float* out)
{
    float inv[16];
    inv[0]  =  m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4]  = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8]  =  m[4] * m[9]  * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9]  * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1]  = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5]  =  m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9]  = -m[0] * m[9]  * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] =  m[0] * m[9]  * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2]  =  m[1] * m[6]  * m[15] - m[1] * m[7]  * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7]  - m[13] * m[3] * m[6];
    inv[6]  = -m[0] * m[6]  * m[15] + m[0] * m[7]  * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7]  + m[12] * m[3] * m[6];
    inv[10] =  m[0] * m[5]  * m[15] - m[0] * m[7]  * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7]  - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5]  * m[14] + m[0] * m[6]  * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6]  + m[12] * m[2] * m[5];
    inv[3]  = -m[1] * m[6]  * m[11] + m[1] * m[7]  * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9]  * m[2] * m[7]  + m[9]  * m[3] * m[6];
    inv[7]  =  m[0] * m[6]  * m[11] - m[0] * m[7]  * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8]  * m[2] * m[7]  - m[8]  * m[3] * m[6];
    inv[11] = -m[0] * m[5]  * m[11] + m[0] * m[7]  * m[9]  + m[4] * m[1] * m[11] - m[4] * m[3] * m[9]  - m[8]  * m[1] * m[7]  + m[8]  * m[3] * m[5];
    inv[15] =  m[0] * m[5]  * m[10] - m[0] * m[6]  * m[9]  - m[4] * m[1] * m[10] + m[4] * m[2] * m[9]  + m[8]  * m[1] * m[6]  - m[8]  * m[2] * m[5];
    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (fabsf(det) < 1e-20f) return false;
    for (int i = 0; i < 16; i++) out[i] = inv[i] / det;
    return true;
}

static rt::V3 xform(const float* M, float x, float y, float z, float w)
{
    return { M[0] * x + M[4] * y + M[8] * z + M[12] * w,
             M[1] * x + M[5] * y + M[9] * z + M[13] * w,
             M[2] * x + M[6] * y + M[10] * z + M[14] * w };
}

// The lights of the interior scope (pots, bowls and the upper room are all inside), in world space
static void readLights(const float* invView, rt::Scene& s)
{
    setInteriorLightScope(true);
    s.lights.clear();
    for (int i = 0; i < 8; i++) {
        GLenum id = GL_LIGHT0 + i;
        rt::Light L;
        L.on = glIsEnabled(id) != 0;
        if (!L.on) continue;
        float p[4], a[4], d[4], sp[4], sd[3], cut, ex;
        glGetLightfv(id, GL_POSITION, p);                         // eye space
        glGetLightfv(id, GL_AMBIENT, a);
        glGetLightfv(id, GL_DIFFUSE, d);
        glGetLightfv(id, GL_SPECULAR, sp);
        glGetLightfv(id, GL_CONSTANT_ATTENUATION, &L.kc);
        glGetLightfv(id, GL_LINEAR_ATTENUATION, &L.kl);
        glGetLightfv(id, GL_QUADRATIC_ATTENUATION, &L.kq);
        glGetLightfv(id, GL_SPOT_CUTOFF, &cut);
        glGetLightfv(id, GL_SPOT_EXPONENT, &ex);
        glGetLightfv(id, GL_SPOT_DIRECTION, sd);
        L.directional = (p[3] == 0.0f);
        if (L.directional) L.pos = xform(invView, p[0], p[1], p[2], 0.0f);
        else { rt::V3 w = xform(invView, p[0], p[1], p[2], p[3]); L.pos = { w.x / p[3], w.y / p[3], w.z / p[3] }; }
        L.amb  = { a[0], a[1], a[2] };
        L.dif  = { d[0], d[1], d[2] };
        L.spec = { sp[0], sp[1], sp[2] };
        L.spotDir = xform(invView, sd[0], sd[1], sd[2], 0.0f);
        L.spotCos = (cut <= 90.0f) ? cosf(cut * 3.14159265f / 180.0f) : -2.0f;
        L.spotExp = ex;
        s.lights.push_back(L);
    }
    float ga[4];
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT, ga);
    s.globalAmb = { ga[0], ga[1], ga[2] };
    setInteriorLightScope(false);
}

// Window rectangle of a world box; false when it is not in front of the camera / on screen
static bool screenRect(const float* VP, const GLint vp[4], rt::V3 mn, rt::V3 mx, int r[4])
{
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    int inFront = 0, behind = 0;
    for (int k = 0; k < 8; k++) {
        float wx = (k & 1) ? mx.x : mn.x, wy = (k & 2) ? mx.y : mn.y, wz = (k & 4) ? mx.z : mn.z;
        float c[4];
        for (int i = 0; i < 4; i++) c[i] = VP[i] * wx + VP[4 + i] * wy + VP[8 + i] * wz + VP[12 + i];
        if (c[3] <= 0.05f) { behind++; continue; }
        inFront++;
        float px = vp[0] + (c[0] / c[3] * 0.5f + 0.5f) * vp[2], py = vp[1] + (c[1] / c[3] * 0.5f + 0.5f) * vp[3];
        x0 = std::min(x0, px); x1 = std::max(x1, px); y0 = std::min(y0, py); y1 = std::max(y1, py);
    }
    if (inFront == 0) return false;
    if (behind > 0) { x0 = (float)vp[0]; y0 = (float)vp[1]; x1 = (float)(vp[0] + vp[2]); y1 = (float)(vp[1] + vp[3]); }
    r[0] = std::max((int)floorf(x0) - 1, (int)vp[0]);
    r[1] = std::max((int)floorf(y0) - 1, (int)vp[1]);
    r[2] = std::min((int)ceilf(x1) + 1, (int)(vp[0] + vp[2]));
    r[3] = std::min((int)ceilf(y1) + 1, (int)(vp[1] + vp[3]));
    return r[2] > r[0] && r[3] > r[1];
}

void renderRayTracing()
{
    if (!rayTracingActive()) return;
    double t0 = nowMs();

    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    float V[16], P[16], VP[16], invVP[16], invV[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, V);
    glGetFloatv(GL_PROJECTION_MATRIX, P);
    mul4(P, V, VP);
    if (!invert4(VP, invVP) || !invert4(V, invV)) return;

    // ── Scene of this frame and the screen rectangles of the featured objects ──
    static rt::Scene scene;
    RtObjectBounds bounds[RTO_COUNT];
    double tA = nowMs();
    rtBuildShopScene(scene, bounds);
    double tB = nowMs();
    std::vector<std::array<int, 4>> rects;
    long long area = 0;
    for (int o = 0; o < RTO_COUNT; o++) {
        int r[4];
        if (!bounds[o].traced || !screenRect(VP, vp, bounds[o].bmin, bounds[o].bmax, r)) continue;
        rects.push_back({ r[0], r[1], r[2], r[3] });
        area += (long long)(r[2] - r[0]) * (r[3] - r[1]);
    }
    if (rects.empty()) return;                                      // nothing ray traced in view: no cost

    readLights(invV, scene);
    float clear[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
    scene.sky = { clear[0], clear[1], clear[2] };
    scene.maxDepth = 3;

    rt::Camera cam;
    cam.pos = { invV[12], invV[13], invV[14] };
    memcpy(cam.viewProj, VP, sizeof(VP));

    // ── One eye ray per cell of `step` x `step` pixels, within the ray budget ──
    int step = 1;
    while (area / ((long long)step * step) > g_rayBudget) step++;
    const int gw = (vp[2] + step - 1) / step, gh = (vp[3] + step - 1) / step;
    if (gw != g_gridW || gh != g_gridH) {
        g_gridW = gw; g_gridH = gh;
        g_addBuf.resize((size_t)gw * gh * 4);
        g_infoBuf.resize((size_t)gw * gh * 2);
    }
    // Clear each frame: add=(0,0,0), weight=1.0 so bilinear edges pass through to raster
    memset(g_addBuf.data(), 0, g_addBuf.size() * sizeof(float));
    for (size_t i = 3; i < g_addBuf.size(); i += 4) g_addBuf[i] = 1.0f;
    memset(g_infoBuf.data(), 0, g_infoBuf.size() * sizeof(float));
    struct CellRect { int x0, y0, x1, y1; };
    std::vector<CellRect> cells;
    for (auto& r : rects)
        cells.push_back({ (r[0] - vp[0]) / step, (r[1] - vp[1]) / step,
                          std::min(gw, (r[2] - vp[0] + step - 1) / step), std::min(gh, (r[3] - vp[1] + step - 1) / step) });
    std::vector<std::pair<int, int>> rows;                          // (rectangle, cell row)
    for (int i = 0; i < (int)cells.size(); i++)
        for (int y = cells[i].y0; y < cells[i].y1; y++) rows.push_back({ i, y });

    int frameNum = g_frameNum;
    std::atomic<int> next{ 0 };
    auto worker = [&]() {
        for (;;) {
            int k = next.fetch_add(1);
            if (k >= (int)rows.size()) break;
            const CellRect& cr = cells[rows[k].first];
            int cy = rows[k].second;
            for (int cx = cr.x0; cx < cr.x1; cx++) {
                float jx = cellJitter(cx, cy, frameNum, 0);
                float jy = cellJitter(cx, cy, frameNum, 1);
                float px = vp[0] + (cx + 0.5f + jx) * step, py = vp[1] + (cy + 0.5f + jy) * step;
                float nx = (px - vp[0]) / vp[2] * 2.0f - 1.0f, ny = (py - vp[1]) / vp[3] * 2.0f - 1.0f;
                float fw = invVP[3] * nx + invVP[7] * ny + invVP[11] + invVP[15];
                rt::V3 f = xform(invVP, nx, ny, 1.0f, 1.0f);
                rt::V3 d = { f.x / fw - cam.pos.x, f.y / fw - cam.pos.y, f.z / fw - cam.pos.z };
                float l = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
                d = { d.x / l, d.y / l, d.z / l };
                rt::PixelOut o;
                rt::tracePixel(scene, cam, d, o);
                size_t idx = (size_t)cy * gw + cx;
                float* a = &g_addBuf[idx * 4];
                a[0] = o.add[0]; a[1] = o.add[1]; a[2] = o.add[2]; a[3] = o.weight;
                g_infoBuf[idx * 2] = o.depth; g_infoBuf[idx * 2 + 1] = o.mode;
            }
        }
    };
    double tC = nowMs();
    int nThreads = (int)std::max(1u, std::min(std::thread::hardware_concurrency(), 8u));
    std::vector<std::thread> pool;
    for (int t = 1; t < nThreads; t++) pool.emplace_back(worker);
    worker();
    for (auto& th : pool) th.join();
    double tD = nowMs();

    // ── Temporal accumulation: blend with previous frames when the camera is stationary ──
    bool camChanged = memcmp(g_prevVP, VP, sizeof(VP)) != 0;
    memcpy(g_prevVP, VP, sizeof(VP));
    g_frameNum++;

    if (camChanged || g_accAddBuf.size() != g_addBuf.size()) {
        g_accAddBuf = g_addBuf;                                         // first frame: use as-is
        g_accFrames = 1;
    } else {
        g_accFrames = std::min(g_accFrames + 1, 8);
        float alpha = 1.0f / g_accFrames;                               // diminishing new-frame weight
        float beta  = 1.0f - alpha;
        size_t n = g_addBuf.size() / 4;
        for (size_t i = 0; i < n; i++) {
            if (g_infoBuf[i * 2 + 1] > 0.5f) {                          // only blend traced cells
                g_accAddBuf[i * 4]     = alpha * g_addBuf[i * 4]     + beta * g_accAddBuf[i * 4];
                g_accAddBuf[i * 4 + 1] = alpha * g_addBuf[i * 4 + 1] + beta * g_accAddBuf[i * 4 + 1];
                g_accAddBuf[i * 4 + 2] = alpha * g_addBuf[i * 4 + 2] + beta * g_accAddBuf[i * 4 + 2];
                g_accAddBuf[i * 4 + 3] = g_addBuf[i * 4 + 3];           // weight: always latest
            } else {
                g_accAddBuf[i * 4 + 3] = g_addBuf[i * 4 + 3];           // pass-through weight for untraced
            }
        }
    }

    // ── Upload the accumulated cells and composite them over the frame ──
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, gw);
    glActiveTexture(GL_TEXTURE3);                                   // units 0-2 belong to the scene, shadow map, glass
    glBindTexture(GL_TEXTURE_2D, g_addTex);
    if (g_texW != gw || g_texH != gh)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, gw, gh, 0, GL_RGBA, GL_FLOAT, NULL);
    for (auto& c : cells) {
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, c.x0); glPixelStorei(GL_UNPACK_SKIP_ROWS, c.y0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, c.x0, c.y0, c.x1 - c.x0, c.y1 - c.y0, GL_RGBA, GL_FLOAT, g_accAddBuf.data());
    }
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, g_infoTex);
    if (g_texW != gw || g_texH != gh)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, gw, gh, 0, GL_RG, GL_FLOAT, NULL);
    for (auto& c : cells) {
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, c.x0); glPixelStorei(GL_UNPACK_SKIP_ROWS, c.y0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, c.x0, c.y0, c.x1 - c.x0, c.y1 - c.y0, GL_RG, GL_FLOAT, g_infoBuf.data());
    }
    g_texW = gw; g_texH = gh;
    glPopClientAttrib();

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, g_colorTex);
    if (g_frameW != vp[2] || g_frameH != vp[3])
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, vp[2], vp[3], 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp[0], vp[1], vp[2], vp[3]);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, g_depthTex);
    if (g_frameW != vp[2] || g_frameH != vp[3]) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, vp[2], vp[3], 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        g_frameW = vp[2]; g_frameH = vp[3];
    }
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp[0], vp[1], vp[2], vp[3]);

    glUseProgram(g_prog);
    glUniform1i(u_add, 3);
    glUniform1i(u_info, 4);
    glUniform1i(u_color, 5);
    glUniform1i(u_depth, 6);
    glUniform2f(u_viewport, (float)vp[2], (float)vp[3]);
    glUniform2f(u_grid, (float)gw, (float)gh);
    glUniform1f(u_cell, (float)step);
    glUniform1f(u_near, P[14] / (P[10] - 1.0f));
    glUniform1f(u_far,  P[14] / (P[10] + 1.0f));
    glDisable(GL_DEPTH_TEST);                                       // visibility is decided in the shader
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(vp[0], vp[0] + vp[2], vp[1], vp[1] + vp[3], -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glBegin(GL_QUADS);
    for (auto& c : cells) {
        float x0 = (float)(vp[0] + c.x0 * step), y0 = (float)(vp[1] + c.y0 * step);
        float x1 = (float)std::min(vp[0] + c.x1 * step, vp[0] + vp[2]), y1 = (float)std::min(vp[1] + c.y1 * step, vp[1] + vp[3]);
        glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1);
    }
    glEnd();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glUseProgram(0);
    for (int u = 6; u >= 3; u--) { glActiveTexture(GL_TEXTURE0 + u); glBindTexture(GL_TEXTURE_2D, 0); }
    glActiveTexture(GL_TEXTURE0);
    glPopAttrib();

    // ── Timing: adapt the ray budget, report every 3 s ──
    double ms = nowMs() - t0;
    g_msAvg = (g_msAvg == 0.0) ? ms : g_msAvg * 0.9 + ms * 0.1;
    if (ms > TARGET_MS * 1.3 && g_rayBudget > 15000)  g_rayBudget = std::max(15000, (int)(g_rayBudget * 0.85));
    if (ms < TARGET_MS * 0.6 && g_rayBudget < 400000) g_rayBudget = std::min(400000, (int)(g_rayBudget * 1.15));
    if (t0 - g_lastReport > 3000.0) {
        g_lastReport = t0;
        fprintf(stderr, "[RT-PROF] pre %.1f build %.1f setup %.1f trace %.1f rest %.1f\n", tA - t0, tB - tA, tC - tB, tD - tC, nowMs() - tD);
        fprintf(stderr, "[RT] %zu object(s) in view, %lld eye rays (1 per %dx%d px), %.1f ms per frame on %d threads\n",
                rects.size(), area / ((long long)step * step), step, step, g_msAvg, nThreads);
    }
}

void toggleRayTracing()
{
    useRayTracing = !useRayTracing;
    if (useRayTracing && !g_ready)
        fprintf(stderr, "[RT] Ray tracing is not available (see the start-up messages).\n");
    else
        fprintf(stderr, "[RT] Ray tracing %s\n", useRayTracing ? "ON (CPU ray tracer)" : "OFF (OpenGL only)");
}

const char* getRayTracingStatus()
{
    if (!g_ready) return "n/a";
    return useRayTracing ? "ON" : "off";
}
