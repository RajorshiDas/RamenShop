#include "shader.h"
#include "scene.h"
#include "lighting.h"
#include <cstdio>
#include <cstdlib>

bool usePhongShading = false;

// ─── Shader handles ────────────────────────────────────────────────────────
static GLuint phongProgram = 0;
static bool   shaderReady  = false;
static bool   shaderActive = false;   // true while glUseProgram(phongProgram) is live

// Uniform locations (cached after linking)
static GLint uLightingOn = -1;
static GLint uTexEnabled = -1;
static GLint uTex0       = -1;
static GLint uFogOn      = -1;
static GLint uFogDensity = -1;
static GLint uFogColor   = -1;
static GLint uLightOn    = -1;   // float[8]

// ─── GLSL source ───────────────────────────────────────────────────────────

static const char* vertSrc = R"GLSL(
#version 120

varying vec3  vNormal;
varying vec3  vPos;
varying vec4  vColor;
varying vec2  vTexCoord;
varying float vFogDist;

void main()
{
    vec4 eyePos = gl_ModelViewMatrix * gl_Vertex;
    vPos        = eyePos.xyz;
    vNormal     = normalize(gl_NormalMatrix * gl_Normal);
    vColor      = gl_Color;
    vTexCoord   = (gl_TextureMatrix[0] * gl_MultiTexCoord0).st;
    vFogDist    = abs(eyePos.z);
    gl_Position = gl_ProjectionMatrix * eyePos;
}
)GLSL";

static const char* fragSrc = R"GLSL(
#version 120

varying vec3  vNormal;
varying vec3  vPos;
varying vec4  vColor;
varying vec2  vTexCoord;
varying float vFogDist;

uniform bool      lightingOn;
uniform bool      texEnabled;
uniform sampler2D tex0;
uniform bool      fogOn;
uniform float     fogDensity;
uniform vec4      fogColor;
uniform float     lightOn[8];   // 1.0 = enabled, 0.0 = disabled

void main()
{
    // Base surface colour (from vertex colour, optionally modulated by texture)
    vec4 baseColor = vColor;
    if (texEnabled) {
        baseColor *= texture2D(tex0, vTexCoord);
    }

    // ── Unlit pass (outlines, steam, text, etc.) ───────────────────────
    if (!lightingOn) {
        gl_FragColor = baseColor;
    } else {
        // ── Per-pixel Phong illumination ───────────────────────────────
        vec3 N = normalize(vNormal);
        vec3 V = normalize(-vPos);          // view direction (eye at origin)

        // Global ambient
        vec4 result = gl_LightModel.ambient * baseColor;

        // Material emission
        result += gl_FrontMaterial.emission;

        // Accumulate contribution from each light source
        for (int i = 0; i < 8; i++) {
            if (lightOn[i] < 0.5) continue;   // skip disabled lights

            vec3  L;
            float atten = 1.0;

            if (gl_LightSource[i].position.w == 0.0) {
                // ── Directional light ──────────────────────────────────
                L = normalize(gl_LightSource[i].position.xyz);
            } else {
                // ── Point / Spot light ─────────────────────────────────
                vec3  toLight = gl_LightSource[i].position.xyz - vPos;
                float d       = length(toLight);
                L = toLight / d;
                atten = 1.0 / (gl_LightSource[i].constantAttenuation
                              + gl_LightSource[i].linearAttenuation    * d
                              + gl_LightSource[i].quadraticAttenuation * d * d);

                // Spotlight cone
                if (gl_LightSource[i].spotCutoff < 90.0) {
                    float spotCos   = dot(-L, normalize(gl_LightSource[i].spotDirection));
                    float cutoffCos = cos(radians(gl_LightSource[i].spotCutoff));
                    if (spotCos < cutoffCos) {
                        atten = 0.0;
                    } else {
                        atten *= pow(spotCos, gl_LightSource[i].spotExponent);
                    }
                }
            }

            // Ambient
            result += gl_LightSource[i].ambient * baseColor * atten;

            // Diffuse (Lambertian)
            float NdotL = max(dot(N, L), 0.0);
            result += gl_LightSource[i].diffuse * baseColor * NdotL * atten;

            // Specular (Phong reflection)
            if (NdotL > 0.0) {
                vec3  R         = reflect(-L, N);
                float RdotV     = max(dot(R, V), 0.0);
                float shininess = max(gl_FrontMaterial.shininess, 1.0);
                float spec      = pow(RdotV, shininess);
                result += gl_LightSource[i].specular
                        * gl_FrontMaterial.specular
                        * spec * atten;
            }
        }

        result.a     = baseColor.a;
        gl_FragColor = clamp(result, 0.0, 1.0);
    }

    // ── Fog (EXP2, matches fixed-function setup) ───────────────────────
    if (fogOn) {
        float f = exp(-fogDensity * fogDensity * vFogDist * vFogDist);
        f = clamp(f, 0.0, 1.0);
        gl_FragColor = mix(fogColor, gl_FragColor, f);
    }
}
)GLSL";

// ─── Helper: compile one shader stage ──────────────────────────────────────
static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);

    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "[Phong] %s shader compile error:\n%s\n",
                (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment"), log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

// ─── Public API ────────────────────────────────────────────────────────────

void initPhongShader()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!vs || !fs) { fprintf(stderr, "[Phong] Shader compilation failed.\n"); return; }

    phongProgram = glCreateProgram();
    glAttachShader(phongProgram, vs);
    glAttachShader(phongProgram, fs);
    glLinkProgram(phongProgram);

    GLint ok;
    glGetProgramiv(phongProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(phongProgram, sizeof(log), NULL, log);
        fprintf(stderr, "[Phong] Program link error:\n%s\n", log);
        glDeleteProgram(phongProgram);
        phongProgram = 0;
        return;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    // Cache uniform locations
    uLightingOn = glGetUniformLocation(phongProgram, "lightingOn");
    uTexEnabled = glGetUniformLocation(phongProgram, "texEnabled");
    uTex0       = glGetUniformLocation(phongProgram, "tex0");
    uFogOn      = glGetUniformLocation(phongProgram, "fogOn");
    uFogDensity = glGetUniformLocation(phongProgram, "fogDensity");
    uFogColor   = glGetUniformLocation(phongProgram, "fogColor");
    uLightOn    = glGetUniformLocation(phongProgram, "lightOn");

    shaderReady = true;
    fprintf(stderr, "[Phong] Shader compiled & linked OK.  Press G to toggle.\n");
}

void enablePhongShader()
{
    if (!shaderReady || !usePhongShading) return;
    glUseProgram(phongProgram);
    shaderActive = true;

    // Defaults
    glUniform1i(uLightingOn, 1);
    glUniform1i(uTexEnabled, 0);
    glUniform1i(uTex0, 0);        // texture unit 0
}

void disablePhongShader()
{
    if (!shaderActive) return;
    glUseProgram(0);
    shaderActive = false;
}

void updatePhongUniforms()
{
    if (!shaderActive) return;

    // Light enable mask — matches the glEnable/glDisable state in scene.cpp
    //   LIGHT0 = Moon/Sun (directional)
    //   LIGHT1 = Interior dining pendant (point)
    //   LIGHT2 = Left exterior chochin   (point)
    //   LIGHT3 = Right exterior chochin  (point)
    //   LIGHT4 = Kitchen spotlight       (spot)
    //   LIGHT5 = Street lamps            (area toggle)
    //   LIGHT6 = Hanging lanterns         (area toggle)
    //   LIGHT7 = Second floor ceiling     (area toggle)
    float mask[8];
    mask[0] = lightDirectional ? 1.0f : 0.0f;
    mask[1] = lightPoint       ? 1.0f : 0.0f;
    mask[2] = lightPoint       ? 1.0f : 0.0f;
    mask[3] = lightPoint       ? 1.0f : 0.0f;
    mask[4] = lightSpot        ? 1.0f : 0.0f;
    mask[5] = (lightArea && !isDayTime) ? 1.0f : 0.0f;
    mask[6] = lightArea        ? 1.0f : 0.0f;
    mask[7] = lightArea        ? 1.0f : 0.0f;
    glUniform1fv(uLightOn, 8, mask);

    // Fog state
    glUniform1i(uFogOn, showFog ? 1 : 0);
    glUniform1f(uFogDensity, isDayTime ? 0.012f : 0.022f);

    GLfloat fc[4];
    glGetFloatv(GL_FOG_COLOR, fc);
    glUniform4fv(uFogColor, 1, fc);
}

void shaderSetTexture(bool on)
{
    if (!shaderActive) return;
    glUniform1i(uTexEnabled, on ? 1 : 0);
}

void setLighting(bool on)
{
    if (on)  glEnable(GL_LIGHTING);
    else     glDisable(GL_LIGHTING);

    if (shaderActive) {
        glUniform1i(uLightingOn, on ? 1 : 0);
    }
}
