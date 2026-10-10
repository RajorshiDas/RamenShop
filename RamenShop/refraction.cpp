#include "refraction.h"
#include "scene.h"
#include "shader.h"
#include "lighting.h"
#include "rtpass.h"
#include <cstdio>
#include <cmath>
#include <vector>

bool useGlassRefraction = true;

// On the left customer table (drawTable at (-2.4, FLOOR_Y, 2.8), y-scale 0.9: top at FLOOR_Y + 0.675)
const Vec3  TABLE_TUMBLER_POS   = { -2.15f, FLOOR_Y + 0.677f, 2.72f };
const float TABLE_TUMBLER_SCALE = 0.22f;      // as the tumblers on the counter

static GLuint g_prog = 0;
static GLint  u_background = -1, u_viewport = -1, u_copyRect = -1, u_strength = -1, u_tint = -1, u_lightOn = -1;
static GLuint g_bgTex = 0;
static int    g_bgW = 0, g_bgH = 0;

// ─── Shaders (GLSL 1.20, like the Phong shader) ─────────────────────────────
static const char* refrVertSrc = R"GLSL(
#version 120
varying vec3 vNormal;
varying vec3 vPos;
void main()
{
    vec4 eyePos = gl_ModelViewMatrix * gl_Vertex;
    vPos        = eyePos.xyz;
    vNormal     = gl_NormalMatrix * gl_Normal;
    gl_Position = gl_ProjectionMatrix * eyePos;
}
)GLSL";

static const char* refrFragSrc = R"GLSL(
#version 120
uniform sampler2D background;   // the frame behind the glass, copied just before this draw
uniform vec2  viewport;         // window size in pixels
uniform vec4  copyRect;         // copied region in texture coordinates (x0, y0, x1, y1)
uniform float strength;         // how far the glass shifts what is seen through it
uniform vec3  tint;             // colour of thick glass
uniform float lightOn[8];
varying vec3  vNormal;
varying vec3  vPos;

void main()
{
    vec3 I = normalize(vPos);                       // eye -> surface
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;                    // far wall: its inside faces the camera
    float cosI = clamp(dot(N, -I), 0.0, 1.0);

    // Refraction (screen-space approximation): shift the lookup by how much refract() bends
    // the view ray; closer glass shifts more pixels than distant glass.
    vec3 T    = refract(I, N, 1.0 / 1.5);
    vec2 offs = (T.xy - I.xy) * strength / max(-vPos.z, 0.25);
    vec2 uv   = clamp(gl_FragCoord.xy / viewport + offs, copyRect.xy, copyRect.zw);
    vec3 seen = texture2D(background, uv).rgb;

    // Glass is thicker along the line of sight near the silhouette: slightly tinted there
    vec3 body = seen * mix(vec3(1.0), tint, 0.25 + 0.75 * (1.0 - cosI));

    // Fresnel reflection of the warm room, strongest at grazing angles
    float F = 0.04 + 0.96 * pow(1.0 - cosI, 5.0);
    vec3 col = mix(body, vec3(0.55, 0.48, 0.38), F);

    // Sharp highlights of the shop's lamps (Blinn-Phong, same light setup as the Phong shader)
    for (int i = 0; i < 8; i++) {
        if (lightOn[i] < 0.5) continue;
        vec3  L;
        float atten = 1.0;
        if (gl_LightSource[i].position.w == 0.0) {
            L = normalize(gl_LightSource[i].position.xyz);
        } else {
            vec3  toLight = gl_LightSource[i].position.xyz - vPos;
            float d       = length(toLight);
            L = toLight / d;
            atten = 1.0 / (gl_LightSource[i].constantAttenuation
                         + gl_LightSource[i].linearAttenuation    * d
                         + gl_LightSource[i].quadraticAttenuation * d * d);
            if (gl_LightSource[i].spotCutoff < 90.0) {
                float spotCos = dot(-L, normalize(gl_LightSource[i].spotDirection));
                atten = (spotCos < cos(radians(gl_LightSource[i].spotCutoff)))
                      ? 0.0 : atten * pow(spotCos, gl_LightSource[i].spotExponent);
            }
        }
        vec3 H = normalize(L - I);
        col += gl_LightSource[i].specular.rgb * pow(max(dot(N, H), 0.0), 140.0) * atten;
    }
    gl_FragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
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
        fprintf(stderr, "[Glass] %s shader compile error:\n%s\n", type == GL_VERTEX_SHADER ? "Vertex" : "Fragment", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

void initGlassRefraction()
{
    GLuint vs = compileStage(GL_VERTEX_SHADER, refrVertSrc);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, refrFragSrc);
    if (!vs || !fs) {
        fprintf(stderr, "[Glass] Refraction shader unavailable: the ordinary glass tumbler is drawn instead.\n");
        return;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), NULL, log);
        fprintf(stderr, "[Glass] Refraction program link error:\n%s\n", log);
        glDeleteProgram(p);
        return;
    }
    g_prog = p;
    u_background = glGetUniformLocation(p, "background");
    u_viewport   = glGetUniformLocation(p, "viewport");
    u_copyRect   = glGetUniformLocation(p, "copyRect");
    u_strength   = glGetUniformLocation(p, "strength");
    u_tint       = glGetUniformLocation(p, "tint");
    u_lightOn    = glGetUniformLocation(p, "lightOn");
    glGenTextures(1, &g_bgTex);
    fprintf(stderr, "[Glass] Screen-space refraction ready (tumbler on the left table).  Press Z to toggle.\n");
}

bool glassRefractionActive() { return useGlassRefraction && g_prog != 0; }

// ─── Tumbler mesh (outer shell of drawClearGlassTumbler: base, tapered wall, bottom, rim) ──
struct GlassVert { float p[3], n[3]; };
static std::vector<GlassVert> g_mesh;

static void buildMesh()
{
    const int   SEG = 40;
    const float prof[3][2] = { { 0.280f, 0.00f }, { 0.285f, 0.08f }, { 0.320f, 0.93f } };   // (radius, height)
    const float rimIn = 0.275f, rimY = 0.93f;
    auto put = [](float x, float y, float z, float nx, float ny, float nz) {
        float l = sqrtf(nx * nx + ny * ny + nz * nz);
        g_mesh.push_back({ { x, y, z }, { nx / l, ny / l, nz / l } });
    };
    for (int j = 0; j < SEG; j++) {
        float a0 = 6.2831853f * j / SEG, a1 = 6.2831853f * (j + 1) / SEG;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        for (int b = 0; b < 2; b++) {                                  // side bands, outward, counter-clockwise
            float r0 = prof[b][0], y0 = prof[b][1], r1 = prof[b + 1][0], y1 = prof[b + 1][1];
            float nr = y1 - y0, ny = -(r1 - r0);                       // profile normal
            put(r0 * c0, y0, r0 * s0, nr * c0, ny, nr * s0);  put(r1 * c1, y1, r1 * s1, nr * c1, ny, nr * s1);  put(r0 * c1, y0, r0 * s1, nr * c1, ny, nr * s1);
            put(r0 * c0, y0, r0 * s0, nr * c0, ny, nr * s0);  put(r1 * c0, y1, r1 * s0, nr * c0, ny, nr * s0);  put(r1 * c1, y1, r1 * s1, nr * c1, ny, nr * s1);
        }
        float rb = prof[0][0];                                         // bottom disk, facing down
        put(0, 0, 0, 0, -1, 0);  put(rb * c0, 0, rb * s0, 0, -1, 0);  put(rb * c1, 0, rb * s1, 0, -1, 0);
        float ro = prof[2][0];                                         // rim ring, facing up
        put(rimIn * c0, rimY, rimIn * s0, 0, 1, 0);  put(rimIn * c1, rimY, rimIn * s1, 0, 1, 0);  put(ro * c1, rimY, ro * s1, 0, 1, 0);
        put(rimIn * c0, rimY, rimIn * s0, 0, 1, 0);  put(ro * c1, rimY, ro * s1, 0, 1, 0);        put(ro * c0, rimY, ro * s0, 0, 1, 0);
    }
}

static void drawMesh()
{
    if (g_mesh.empty()) buildMesh();
    glBegin(GL_TRIANGLES);
    for (const GlassVert& v : g_mesh) { glNormal3fv(v.n); glVertex3fv(v.p); }
    glEnd();
}

// Screen rectangle of the tumbler (+ margin) in window pixels; false when it is not in view
static bool screenRect(const GLint vp[4], int r[4])
{
    float MV[16], P[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, MV);
    glGetFloatv(GL_PROJECTION_MATRIX, P);
    const float s = TABLE_TUMBLER_SCALE, R = 0.34f * s, Hh = 0.96f * s;
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    int inFront = 0, behind = 0;
    for (int k = 0; k < 8; k++) {
        float w[4] = { TABLE_TUMBLER_POS.x + ((k & 1) ? R : -R), TABLE_TUMBLER_POS.y + ((k & 2) ? Hh : 0.0f),
                       TABLE_TUMBLER_POS.z + ((k & 4) ? R : -R), 1.0f };
        float e[4], c[4];
        for (int i = 0; i < 4; i++) e[i] = MV[i] * w[0] + MV[4 + i] * w[1] + MV[8 + i] * w[2] + MV[12 + i] * w[3];
        for (int i = 0; i < 4; i++) c[i] = P[i] * e[0] + P[4 + i] * e[1] + P[8 + i] * e[2] + P[12 + i] * e[3];
        if (c[3] <= 0.05f) { behind++; continue; }
        inFront++;
        float px = vp[0] + (c[0] / c[3] * 0.5f + 0.5f) * vp[2], py = vp[1] + (c[1] / c[3] * 0.5f + 0.5f) * vp[3];
        x0 = fminf(x0, px); x1 = fmaxf(x1, px); y0 = fminf(y0, py); y1 = fmaxf(y1, py);
    }
    if (inFront == 0) return false;
    if (behind > 0) { x0 = (float)vp[0]; y0 = (float)vp[1]; x1 = (float)(vp[0] + vp[2]); y1 = (float)(vp[1] + vp[3]); }
    const float margin = 24.0f;                                         // room for the refraction shift
    r[0] = (int)fmaxf(x0 - margin, (float)vp[0]);
    r[1] = (int)fmaxf(y0 - margin, (float)vp[1]);
    r[2] = (int)fminf(x1 + margin, (float)(vp[0] + vp[2]));
    r[3] = (int)fminf(y1 + margin, (float)(vp[1] + vp[3]));
    return r[2] > r[0] && r[3] > r[1];
}

static void copyBackground(const int r[4])
{
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, r[0], r[1], r[0], r[1], r[2] - r[0], r[3] - r[1]);
}

void drawRefractiveTumbler()
{
    if (!glassRefractionActive() || rayTracingActive()) return;   // R: the ray tracer draws this glass

    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int rect[4];
    if (!screenRect(vp, rect)) return;                                 // not in view: no cost

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glActiveTexture(GL_TEXTURE2);                                      // units 0 / 1 belong to the scene and the shadow map
    glBindTexture(GL_TEXTURE_2D, g_bgTex);
    if (g_bgW != vp[2] || g_bgH != vp[3]) {                            // window-sized background texture
        g_bgW = vp[2]; g_bgH = vp[3];
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, g_bgW, g_bgH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    setInteriorLightScope(true);                                       // the tumbler stands inside the shop
    float lightOn[8];
    for (int i = 0; i < 8; i++) lightOn[i] = glIsEnabled(GL_LIGHT0 + i) ? 1.0f : 0.0f;

    glUseProgram(g_prog);
    glUniform1i(u_background, 2);
    glUniform2f(u_viewport, (float)vp[2], (float)vp[3]);
    glUniform4f(u_copyRect, (rect[0] + 0.5f) / vp[2], (rect[1] + 0.5f) / vp[3], (rect[2] - 0.5f) / vp[2], (rect[3] - 0.5f) / vp[3]);
    glUniform1f(u_strength, 0.05f);
    glUniform3f(u_tint, 0.80f, 0.90f, 0.86f);
    glUniform1fv(u_lightOn, 8, lightOn);

    // Behind nearer objects (depth test) but never hiding anything (no depth writes, drawn last)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_FOG);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);                                               // the mesh is wound counter-clockwise outward

    glPushMatrix();
    glTranslatef(TABLE_TUMBLER_POS.x, TABLE_TUMBLER_POS.y, TABLE_TUMBLER_POS.z);
    glScalef(TABLE_TUMBLER_SCALE, TABLE_TUMBLER_SCALE, TABLE_TUMBLER_SCALE);
    copyBackground(rect);                                              // 1) far wall refracts the scene behind it
    glCullFace(GL_FRONT);
    drawMesh();
    copyBackground(rect);                                              // 2) near wall refracts scene + far wall
    glCullFace(GL_BACK);
    drawMesh();
    glPopMatrix();

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    setInteriorLightScope(false);
    glPopAttrib();
}

void toggleGlassRefraction()
{
    useGlassRefraction = !useGlassRefraction;
    if (useGlassRefraction && !g_prog)
        fprintf(stderr, "[Glass] Refraction shader is not available; the ordinary glass tumbler stays.\n");
}

const char* getGlassStatus()
{
    if (!g_prog)             return "n/a";
    return useGlassRefraction ? "refract" : "off";
}
