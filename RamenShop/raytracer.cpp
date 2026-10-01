#include "raytracer.h"
#include "camera.h"
#include "scene.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

// ─── Ray Tracing State ─────────────────────────────────────────────────────
bool useRayTracing       = false; // Press 'R' to toggle
int  rayTraceBounces      = 4;     // Max bounces: 1 to 5 (Press 'B' to cycle)
bool rayTraceSoftShadows = true;  // Soft vs hard shadows (Press 'Y' to toggle)
bool rayTraceAO          = true;  // Ambient occlusion

// ─── Shader & GL Handles ───────────────────────────────────────────────────
static GLuint rayTraceProgram = 0;
static bool   rayTracerReady  = false;

// Uniform locations
static GLint uCamPosLoc        = -1;
static GLint uCamForwardLoc    = -1;
static GLint uCamRightLoc      = -1;
static GLint uCamUpLoc         = -1;
static GLint uTanHalfFovLoc    = -1;
static GLint uAspectLoc        = -1;
static GLint uResolutionLoc    = -1;
static GLint uTimeLoc          = -1;
static GLint uDayTimeLoc       = -1;
static GLint uShowRoofLoc      = -1;
static GLint uShowSteamLoc     = -1;
static GLint uMaxBouncesLoc    = -1;
static GLint uSoftShadowsLoc   = -1;
static GLint uAOLoc            = -1;
static GLint uLightAmbientLoc  = -1;
static GLint uLightDiffuseLoc  = -1;
static GLint uLightSpecularLoc = -1;
static GLint uLightDirLoc      = -1;
static GLint uLightPointLoc    = -1;
static GLint uLightSpotLoc     = -1;
static GLint uLightAreaLoc     = -1;

// ─── Vertex Shader Source ──────────────────────────────────────────────────
static const char* rtVertSrc = R"GLSL(
#version 120

varying vec2 vTexCoord;

void main()
{
    vTexCoord   = gl_Vertex.xy * 0.5 + 0.5;
    gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);
}
)GLSL";

// ─── Fragment Shader Source ────────────────────────────────────────────────
static const char* rtFragSrc = R"GLSL(
#version 120

uniform vec3  uCamPos;
uniform vec3  uCamForward;
uniform vec3  uCamRight;
uniform vec3  uCamUp;
uniform float uTanHalfFov;
uniform float uAspect;
uniform vec2  uResolution;
uniform float uTime;
uniform bool  uDayTime;
uniform bool  uShowRoof;
uniform bool  uShowSteam;
uniform int   uMaxBounces;
uniform bool  uSoftShadows;
uniform bool  uAO;
uniform bool  uLightAmbient;
uniform bool  uLightDiffuse;
uniform bool  uLightSpecular;
uniform bool  uLightDir;
uniform bool  uLightPoint;
uniform bool  uLightSpot;
uniform bool  uLightArea;

// ── Material Structure ─────────────────────────────────────────────────────
struct Material {
    vec3  albedo;
    float roughness;
    float metallic;
    float transmission; // 0.0 = opaque, 1.0 = dielectric glass
    float ior;          // index of refraction
    vec3  emission;
};

struct Hit {
    float    t;
    vec3     pos;
    vec3     normal;
    Material mat;
};

// ── Primitive Intersections ────────────────────────────────────────────────
bool intersectAABB(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax, out float tHit, out vec3 normal) {
    vec3 invD = 1.0 / rd;
    vec3 t0s = (bmin - ro) * invD;
    vec3 t1s = (bmax - ro) * invD;
    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger  = max(t0s, t1s);
    float tmin = max(tsmaller.x, max(tsmaller.y, tsmaller.z));
    float tmax = min(tbigger.x, min(tbigger.y, tbigger.z));
    if (tmin > tmax || tmax < 0.002) return false;
    tHit = (tmin > 0.002) ? tmin : tmax;
    vec3 hitP = ro + rd * tHit;
    vec3 c = (bmin + bmax) * 0.5;
    vec3 halfSize = (bmax - bmin) * 0.5;
    vec3 d = abs(hitP - c) / halfSize;
    if (d.x > d.y && d.x > d.z) normal = vec3(sign(hitP.x - c.x), 0.0, 0.0);
    else if (d.y > d.z)          normal = vec3(0.0, sign(hitP.y - c.y), 0.0);
    else                         normal = vec3(0.0, 0.0, sign(hitP.z - c.z));
    return true;
}

bool intersectCylinder(vec3 ro, vec3 rd, vec2 centerXZ, float r, float yMin, float yMax, out float tHit, out vec3 normal) {
    float roX = ro.x - centerXZ.x;
    float roZ = ro.z - centerXZ.y;
    float a = rd.x * rd.x + rd.z * rd.z;
    float b = 2.0 * (roX * rd.x + roZ * rd.z);
    float c = roX * roX + roZ * roZ - r * r;
    
    float tBest = 1e20;
    vec3 normBest = vec3(0.0);
    bool hit = false;
    
    float discr = b * b - 4.0 * a * c;
    if (discr >= 0.0 && a > 1e-6) {
        float sqrtD = sqrt(discr);
        float t1 = (-b - sqrtD) / (2.0 * a);
        float t2 = (-b + sqrtD) / (2.0 * a);
        if (t1 > 0.002) {
            float y = ro.y + rd.y * t1;
            if (y >= yMin && y <= yMax) {
                tBest = t1;
                normBest = normalize(vec3(roX + rd.x * t1, 0.0, roZ + rd.z * t1));
                hit = true;
            }
        }
        if (!hit && t2 > 0.002) {
            float y = ro.y + rd.y * t2;
            if (y >= yMin && y <= yMax) {
                tBest = t2;
                normBest = normalize(vec3(roX + rd.x * t2, 0.0, roZ + rd.z * t2));
                hit = true;
            }
        }
    }
    
    // Top and bottom circular caps
    if (abs(rd.y) > 1e-6) {
        float tTop = (yMax - ro.y) / rd.y;
        if (tTop > 0.002 && tTop < tBest) {
            vec2 pXZ = ro.xz + rd.xz * tTop;
            if (distance(pXZ, centerXZ) <= r) {
                tBest = tTop;
                normBest = vec3(0.0, 1.0, 0.0);
                hit = true;
            }
        }
        float tBot = (yMin - ro.y) / rd.y;
        if (tBot > 0.002 && tBot < tBest) {
            vec2 pXZ = ro.xz + rd.xz * tBot;
            if (distance(pXZ, centerXZ) <= r) {
                tBest = tBot;
                normBest = vec3(0.0, -1.0, 0.0);
                hit = true;
            }
        }
    }
    
    if (hit) {
        tHit = tBest;
        normal = normBest;
    }
    return hit;
}

bool intersectSphere(vec3 ro, vec3 rd, vec3 center, float r, out float tHit, out vec3 normal) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - r * r;
    float discr = b * b - c;
    if (discr < 0.0) return false;
    float sqrtD = sqrt(discr);
    float t = -b - sqrtD;
    if (t < 0.002) t = -b + sqrtD;
    if (t < 0.002) return false;
    tHit = t;
    normal = normalize((ro + rd * t) - center);
    return true;
}

bool intersectDisc(vec3 ro, vec3 rd, vec3 center, float r, vec3 n, out float tHit, out vec3 normal) {
    float denom = dot(rd, n);
    if (abs(denom) < 1e-6) return false;
    float t = dot(center - ro, n) / denom;
    if (t < 0.002) return false;
    vec3 p = ro + rd * t;
    if (length(p - center) <= r) {
        tHit = t;
        normal = (denom < 0.0) ? n : -n;
        return true;
    }
    return false;
}

// ── Full Scene Intersection ────────────────────────────────────────────────
bool intersectScene(vec3 ro, vec3 rd, out Hit hit) {
    hit.t = 1e20;
    float tTemp;
    vec3  nTemp;

    // ── 1. GROUND & FLOOR TILES ────────────────────────────────────────────
    // Interior ceramic floor slab
    if (intersectAABB(ro, rd, vec3(-4.9, -0.05, -3.9), vec3(4.9, 0.001, 3.9), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            // Floor tile pattern: glossy tiles with subtle grout
            vec2 tileCoord = fract(hit.pos.xz * 1.5);
            bool isGrout = (tileCoord.x < 0.04 || tileCoord.y < 0.04);
            vec3 tileColor = isGrout ? vec3(0.20, 0.20, 0.22) : vec3(0.38, 0.38, 0.42);
            hit.mat = Material(tileColor, 0.14, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // Sidewalk outside
    if (intersectAABB(ro, rd, vec3(-12.0, -0.08, 3.9), vec3(12.0, -0.01, 6.8), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.48, 0.46, 0.44), 0.75, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // Asphalt street
    if (intersectAABB(ro, rd, vec3(-20.0, -0.15, 6.8), vec3(20.0, -0.06, 20.0), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            vec3 aspColor = vec3(0.18, 0.18, 0.20);
            if (abs(hit.pos.z - 11.0) < 0.12 && fract(hit.pos.x * 0.3) > 0.4) {
                aspColor = vec3(0.85, 0.85, 0.85); // Road divider line
            }
            hit.mat = Material(aspColor, 0.85, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // ── 2. WALLS & CEILING ─────────────────────────────────────────────────
    // Back wall
    if (intersectAABB(ro, rd, vec3(-4.9, 0.0, -3.95), vec3(4.9, 3.3, -3.75), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            vec3 wallCol = (hit.pos.y < 1.2) ? vec3(0.22, 0.14, 0.08) : vec3(0.82, 0.78, 0.72);
            hit.mat = Material(wallCol, 0.70, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // Left wall with paper shoji window
    if (intersectAABB(ro, rd, vec3(-4.95, 0.0, -3.9), vec3(-4.75, 3.3, 3.9), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            vec3 wallCol = vec3(0.80, 0.76, 0.70);
            vec3 emis = vec3(0.0);
            if (abs(hit.pos.z) < 1.8 && hit.pos.y > 1.2 && hit.pos.y < 2.5) {
                wallCol = vec3(0.95, 0.90, 0.82); // Shoji paper window
                emis = uDayTime ? vec3(0.35, 0.30, 0.20) : vec3(0.15, 0.10, 0.05);
            }
            hit.mat = Material(wallCol, 0.65, 0.0, 0.0, 1.5, emis);
        }
    }

    // Right wall with paper shoji window
    if (intersectAABB(ro, rd, vec3(4.75, 0.0, -3.9), vec3(4.95, 3.3, 3.9), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            vec3 wallCol = vec3(0.80, 0.76, 0.70);
            vec3 emis = vec3(0.0);
            if (abs(hit.pos.z) < 1.8 && hit.pos.y > 1.2 && hit.pos.y < 2.5) {
                wallCol = vec3(0.95, 0.90, 0.82);
                emis = uDayTime ? vec3(0.35, 0.30, 0.20) : vec3(0.15, 0.10, 0.05);
            }
            hit.mat = Material(wallCol, 0.65, 0.0, 0.0, 1.5, emis);
        }
    }

    // Timber corner posts & front posts
    if (intersectAABB(ro, rd, vec3(-4.95, 0.0, 3.75), vec3(-4.65, 3.3, 4.05), tTemp, nTemp) ||
        intersectAABB(ro, rd, vec3( 4.65, 0.0, 3.75), vec3( 4.95, 3.3, 4.05), tTemp, nTemp) ||
        intersectAABB(ro, rd, vec3(-1.90, 0.0, 3.80), vec3(-1.70, 3.3, 4.00), tTemp, nTemp) ||
        intersectAABB(ro, rd, vec3( 1.70, 0.0, 3.80), vec3( 1.90, 3.3, 4.00), tTemp, nTemp) ||
        intersectAABB(ro, rd, vec3(-4.95, 3.15, 3.80), vec3(4.95, 3.32, 4.02), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.18, 0.10, 0.06), 0.55, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // Ceiling slab
    if (uShowRoof && intersectAABB(ro, rd, vec3(-5.0, 3.30, -4.0), vec3(5.0, 3.42, 4.0), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.32, 0.22, 0.16), 0.60, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // Exterior roof awning (indigo blue fabric)
    if (intersectAABB(ro, rd, vec3(-5.3, 2.95, 3.90), vec3(5.3, 3.25, 5.20), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            vec3 awnColor = (fract(hit.pos.x * 1.8) > 0.4) ? vec3(0.12, 0.22, 0.42) : vec3(0.85, 0.88, 0.92);
            hit.mat = Material(awnColor, 0.80, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // ── 3. SERVICE COUNTER (MIRROR LACQUER WOOD TOP) ───────────────────────
    // Counter body
    if (intersectAABB(ro, rd, vec3(-3.0, 0.0, -0.25), vec3(3.0, 1.00, 0.35), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.38, 0.20, 0.10), 0.55, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // Counter Top Slab: High-gloss polished wood lacquer (Sharp ray-traced reflections!)
    if (intersectAABB(ro, rd, vec3(-3.15, 1.00, -0.38), vec3(3.15, 1.06, 0.46), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            // Rich amber finished wood with ultra-low roughness = mirror reflection of ramen & lights!
            hit.mat = Material(vec3(0.48, 0.24, 0.10), 0.05, 0.0, 0.0, 1.54, vec3(0.0));
        }
    }

    // ── 4. CUSTOMER STOOLS (POLISHED CHROME + FABRIC CUSHIONS) ─────────────
    float stoolXs[6];
    stoolXs[0] = -2.4; stoolXs[1] = -1.4; stoolXs[2] = -0.4;
    stoolXs[3] =  0.6; stoolXs[4] =  1.6; stoolXs[5] =  2.6;

    for (int s = 0; s < 6; s++) {
        vec2 stPos = vec2(stoolXs[s], 0.90);
        // Chrome Base Disc
        if (intersectCylinder(ro, rd, stPos, 0.20, 0.0, 0.04, tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                hit.mat = Material(vec3(0.88, 0.90, 0.92), 0.06, 0.95, 0.0, 1.5, vec3(0.0));
            }
        }
        // Chrome Column Post
        if (intersectCylinder(ro, rd, stPos, 0.035, 0.04, 0.70, tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                hit.mat = Material(vec3(0.90, 0.92, 0.95), 0.04, 0.98, 0.0, 1.5, vec3(0.0));
            }
        }
        // Crimson Fabric Cushion Top
        if (intersectCylinder(ro, rd, stPos, 0.19, 0.70, 0.76, tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                hit.mat = Material(vec3(0.72, 0.15, 0.15), 0.80, 0.0, 0.0, 1.5, vec3(0.0));
            }
        }
    }

    // ── 5. KITCHEN PREP STATION & COOKING POTS ────────────────────────────
    // Stainless steel prep table
    if (intersectAABB(ro, rd, vec3(-3.5, 0.0, -3.6), vec3(3.5, 0.94, -2.8), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.82, 0.84, 0.86), 0.18, 0.85, 0.0, 1.5, vec3(0.0));
        }
    }

    // Kitchen Shelves
    if (intersectAABB(ro, rd, vec3(-3.0, 1.70, -3.75), vec3(-0.6, 1.74, -3.45), tTemp, nTemp) ||
        intersectAABB(ro, rd, vec3( 0.6, 1.70, -3.75), vec3( 3.0, 1.74, -3.45), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.40, 0.22, 0.12), 0.60, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // Stainless Stockpot 1 on kitchen stove: Polished chrome mirror
    if (intersectCylinder(ro, rd, vec2(-1.95, -3.20), 0.28, 0.94, 1.38, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.92, 0.94, 0.96), 0.04, 0.95, 0.0, 1.5, vec3(0.0));
        }
    }
    // Simmering broth inside pot
    if (intersectDisc(ro, rd, vec3(-1.95, 1.34, -3.20), 0.26, vec3(0, 1, 0), tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.65, 0.38, 0.14), 0.02, 0.10, 0.0, 1.34, vec3(0.0));
        }
    }

    // Stainless Stockpot 2 on shelf
    if (intersectCylinder(ro, rd, vec2(2.4, -3.60), 0.24, 1.74, 2.15, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.90, 0.92, 0.94), 0.05, 0.95, 0.0, 1.5, vec3(0.0));
        }
    }

    // ── 6. DETAILED RAMEN BOWLS ON THE COUNTER ────────────────────────────
    float bowlXs[3];
    bowlXs[0] = -2.2; bowlXs[1] = -0.8; bowlXs[2] = 0.6;

    for (int b = 0; b < 3; b++) {
        vec3 bPos = vec3(bowlXs[b], 1.06, -0.05);

        // Outer glazed ceramic bowl (Dark ceramic with glossy glaze)
        if (intersectCylinder(ro, rd, bPos.xz, 0.24, 1.06, 1.22, tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                vec3 ceramicCol = (hit.pos.y > 1.20) ? vec3(0.85, 0.15, 0.10) : vec3(0.12, 0.12, 0.15); // Red rim
                hit.mat = Material(ceramicCol, 0.12, 0.0, 0.0, 1.5, vec3(0.0));
            }
        }

        // Glossy Ramen Broth (Tonkotsu amber broth with mirror reflection!)
        if (intersectDisc(ro, rd, vec3(bPos.x, 1.18, bPos.z), 0.22, vec3(0, 1, 0), tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                // High specularity, ultra-low roughness mirror surface
                hit.mat = Material(vec3(0.68, 0.40, 0.12), 0.02, 0.08, 0.0, 1.34, vec3(0.0));
            }
        }

        // Boiled Egg (Ajitsuke Tamago) Halves:
        // Egg White
        if (intersectSphere(ro, rd, vec3(bPos.x - 0.07, 1.20, bPos.z + 0.05), 0.055, tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                hit.mat = Material(vec3(0.96, 0.95, 0.90), 0.35, 0.0, 0.0, 1.5, vec3(0.0));
            }
        }
        // Egg Creamy Golden Yolk
        if (intersectSphere(ro, rd, vec3(bPos.x - 0.07, 1.215, bPos.z + 0.05), 0.032, tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                hit.mat = Material(vec3(0.98, 0.60, 0.08), 0.30, 0.0, 0.0, 1.5, vec3(0.0));
            }
        }

        // Chashu braised pork belly
        if (intersectDisc(ro, rd, vec3(bPos.x + 0.06, 1.19, bPos.z - 0.04), 0.065, vec3(0, 1, 0), tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                hit.mat = Material(vec3(0.62, 0.34, 0.22), 0.45, 0.0, 0.0, 1.5, vec3(0.0));
            }
        }

        // Nori Seaweed Sheet (dark crisp sheet)
        if (intersectAABB(ro, rd, vec3(bPos.x - 0.03, 1.18, bPos.z - 0.18), vec3(bPos.x + 0.07, 1.30, bPos.z - 0.16), tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                hit.mat = Material(vec3(0.08, 0.12, 0.08), 0.85, 0.0, 0.0, 1.5, vec3(0.0));
            }
        }

        // Chopsticks resting beside bowl
        if (intersectAABB(ro, rd, vec3(bPos.x + 0.28, 1.06, bPos.z - 0.18), vec3(bPos.x + 0.30, 1.08, bPos.z + 0.18), tTemp, nTemp)) {
            if (tTemp < hit.t) {
                hit.t = tTemp;
                hit.pos = ro + rd * tTemp;
                hit.normal = nTemp;
                hit.mat = Material(vec3(0.70, 0.52, 0.32), 0.40, 0.0, 0.0, 1.5, vec3(0.0));
            }
        }
    }

    // ── 7. CLEAR GLASS WATER PITCHER / CARAFE (TRUE RAY-TRACED REFRACTION) ─
    vec3 pitcherPos = vec3(0.35, 1.06, -0.15);
    // Outer glass body
    if (intersectCylinder(ro, rd, pitcherPos.xz, 0.12, 1.06, 1.38, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            // Dielectric optical glass with Snell's law refraction
            hit.mat = Material(vec3(0.95, 0.98, 1.00), 0.01, 0.0, 0.94, 1.52, vec3(0.0));
        }
    }
    // Lemon slice floating inside carafe
    if (intersectSphere(ro, rd, vec3(pitcherPos.x, 1.20, pitcherPos.z), 0.045, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.96, 0.88, 0.12), 0.30, 0.0, 0.0, 1.35, vec3(0.0));
        }
    }

    // ── 8. CLEAR GLASS SPICE JARS (RED CHILI, PICKLED GARLIC, SESAME) ──────
    vec3 jarXs = vec3(1.65, 1.92, 2.18);
    // Jar 1: Shichimi Chili
    if (intersectCylinder(ro, rd, vec2(jarXs.x, -0.15), 0.075, 1.06, 1.24, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.92, 0.20, 0.12), 0.02, 0.0, 0.70, 1.50, vec3(0.0));
        }
    }
    // Jar 2: Garlic
    if (intersectCylinder(ro, rd, vec2(jarXs.y, -0.15), 0.075, 1.06, 1.24, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.95, 0.92, 0.78), 0.02, 0.0, 0.70, 1.50, vec3(0.0));
        }
    }
    // Jar 3: Toasted Sesame
    if (intersectCylinder(ro, rd, vec2(jarXs.z, -0.15), 0.075, 1.06, 1.24, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.24, 0.20, 0.16), 0.02, 0.0, 0.70, 1.50, vec3(0.0));
        }
    }

    // ── 9. CONDIMENT BOTTLES & CERAMIC TEA CUP ─────────────────────────────
    // Soy sauce bottle (dark amber liquid)
    if (intersectCylinder(ro, rd, vec2(2.70, -0.15), 0.08, 1.06, 1.32, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.22, 0.12, 0.08), 0.08, 0.0, 0.35, 1.48, vec3(0.0));
        }
    }
    // Green Ceramic Matcha Cup
    if (intersectCylinder(ro, rd, vec2(-2.70, -0.15), 0.08, 1.06, 1.22, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            hit.mat = Material(vec3(0.28, 0.48, 0.32), 0.15, 0.0, 0.0, 1.5, vec3(0.0));
        }
    }

    // ── 10. GLOWING HANGING LANTERNS & LIGHT FIXTURES ──────────────────────
    // Warm Interior Pendant Light (Center of shop above counter)
    if (intersectSphere(ro, rd, vec3(0.0, 2.75, -0.2), 0.16, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            vec3 emis = (uLightPoint) ? vec3(3.2, 2.3, 1.1) : vec3(0.8, 0.7, 0.6);
            hit.mat = Material(vec3(1.0, 0.9, 0.7), 0.2, 0.0, 0.0, 1.0, emis);
        }
    }

    // Interior Hanging Paper Lanterns
    if (intersectCylinder(ro, rd, vec2(-1.8, 0.0), 0.22, 2.65, 3.10, tTemp, nTemp) ||
        intersectCylinder(ro, rd, vec2( 1.8, 0.0), 0.22, 2.65, 3.10, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            vec3 emis = (uLightPoint) ? vec3(2.4, 1.6, 0.7) : vec3(0.4, 0.3, 0.2);
            hit.mat = Material(vec3(0.98, 0.78, 0.48), 0.50, 0.0, 0.0, 1.0, emis);
        }
    }

    // Exterior Street Lanterns (Left & Right)
    if (intersectCylinder(ro, rd, vec2(-3.8, 4.5), 0.26, 2.35, 2.85, tTemp, nTemp) ||
        intersectCylinder(ro, rd, vec2( 3.8, 4.5), 0.26, 2.35, 2.85, tTemp, nTemp)) {
        if (tTemp < hit.t) {
            hit.t = tTemp;
            hit.pos = ro + rd * tTemp;
            hit.normal = nTemp;
            vec3 emis = (uLightPoint) ? vec3(2.6, 1.5, 0.5) : vec3(0.5, 0.3, 0.2);
            hit.mat = Material(vec3(0.95, 0.65, 0.35), 0.40, 0.0, 0.0, 1.0, emis);
        }
    }

    return (hit.t < 1e19);
}

// ── Ray-Traced Shadow Occlusion Test ───────────────────────────────────────
bool isOccluded(vec3 ro, vec3 rd, float maxDist) {
    float tTemp;
    vec3  nTemp;

    // Fast tests against opaque blocking geometry
    if (intersectAABB(ro, rd, vec3(-3.0, 0.0, -0.25), vec3(3.0, 1.06, 0.46), tTemp, nTemp) && tTemp < maxDist) return true;
    if (intersectAABB(ro, rd, vec3(-3.5, 0.0, -3.6), vec3(3.5, 0.94, -2.8), tTemp, nTemp) && tTemp < maxDist) return true;
    if (intersectAABB(ro, rd, vec3(-4.9, 0.0, -3.95), vec3(4.9, 3.3, -3.75), tTemp, nTemp) && tTemp < maxDist) return true;
    if (uShowRoof && intersectAABB(ro, rd, vec3(-5.0, 3.30, -4.0), vec3(5.0, 3.42, 4.0), tTemp, nTemp) && tTemp < maxDist) return true;
    if (intersectAABB(ro, rd, vec3(-5.3, 2.95, 3.90), vec3(5.3, 3.25, 5.20), tTemp, nTemp) && tTemp < maxDist) return true;

    // Test stool cushions & posts
    float stoolXs[6];
    stoolXs[0] = -2.4; stoolXs[1] = -1.4; stoolXs[2] = -0.4;
    stoolXs[3] =  0.6; stoolXs[4] =  1.6; stoolXs[5] =  2.6;
    for (int s = 0; s < 6; s++) {
        if (intersectCylinder(ro, rd, vec2(stoolXs[s], 0.90), 0.19, 0.04, 0.76, tTemp, nTemp) && tTemp < maxDist) return true;
    }

    return false;
}

// ── Soft Shadow Penumbra Evaluator ─────────────────────────────────────────
float calcShadowFactor(vec3 P, vec3 lightPos, float lightRadius, float dist) {
    vec3 L0 = normalize(lightPos - P);
    if (!uSoftShadows) {
        return isOccluded(P, L0, dist - 0.04) ? 0.0 : 1.0;
    }
    // Stratified soft shadow sampling
    float unoccludedCount = 0.0;
    if (!isOccluded(P, L0, dist - 0.04)) unoccludedCount += 1.0;

    // Small orthogonal jitter offsets
    vec3 right = normalize(cross(L0, vec3(0.0, 1.0, 0.0) + 1e-4));
    vec3 up    = cross(right, L0);

    vec3 sample1 = normalize(lightPos + right * lightRadius - P);
    if (!isOccluded(P, sample1, dist - 0.04)) unoccludedCount += 1.0;

    vec3 sample2 = normalize(lightPos - right * lightRadius - P);
    if (!isOccluded(P, sample2, dist - 0.04)) unoccludedCount += 1.0;

    vec3 sample3 = normalize(lightPos + up * lightRadius - P);
    if (!isOccluded(P, sample3, dist - 0.04)) unoccludedCount += 1.0;

    return unoccludedCount * 0.25;
}

// ── Sky / Environment Color ────────────────────────────────────────────────
vec3 getSkyColor(vec3 rd) {
    if (uDayTime) {
        float t = max(rd.y, 0.0);
        vec3 horizon = vec3(0.65, 0.78, 0.95);
        vec3 zenith  = vec3(0.20, 0.45, 0.85);
        vec3 sky = mix(horizon, zenith, t);
        // Sun disc
        vec3 sunDir = normalize(vec3(0.5, 0.65, -0.5));
        float sunSpec = max(dot(rd, sunDir), 0.0);
        sky += vec3(1.0, 0.9, 0.7) * pow(sunSpec, 64.0) * 1.5;
        return sky;
    } else {
        float t = max(rd.y, 0.0);
        vec3 horizon = vec3(0.08, 0.08, 0.14);
        vec3 zenith  = vec3(0.02, 0.02, 0.06);
        vec3 sky = mix(horizon, zenith, t);
        // Moon disc
        vec3 moonDir = normalize(vec3(0.5, 0.35, -0.7));
        float moonSpec = max(dot(rd, moonDir), 0.0);
        sky += vec3(0.85, 0.92, 1.0) * pow(moonSpec, 128.0) * 1.8;
        return sky;
    }
}

// ── Direct Illumination Evaluation with Shadow Rays ────────────────────────
vec3 evaluateDirectLighting(Hit hit, vec3 viewDir) {
    vec3 totalLight = vec3(0.0);
    vec3 N = hit.normal;
    vec3 P = hit.pos + N * 0.003; // Shadow ray bias

    // 1. Ambient Light
    if (uLightAmbient) {
        vec3 ambCol = uDayTime ? vec3(0.25, 0.28, 0.32) : vec3(0.08, 0.08, 0.12);
        // Ray-traced Contact Ambient Occlusion
        float aoFactor = 1.0;
        if (uAO) {
            // Darken corners and crevices near floors and walls
            if (hit.pos.y < 0.12) aoFactor *= smoothstep(0.0, 0.12, hit.pos.y) * 0.5 + 0.5;
            if (hit.pos.y > 0.98 && hit.pos.y < 1.08 && abs(hit.pos.z - 0.05) < 0.4) aoFactor *= 0.85;
        }
        totalLight += ambCol * hit.mat.albedo * aoFactor;
    }

    // 2. Directional Sun / Moon Light
    if (uLightDir) {
        vec3 L = uDayTime ? normalize(vec3(0.5, 0.65, -0.5)) : normalize(vec3(0.5, 0.35, -0.7));
        vec3 lightCol = uDayTime ? vec3(1.1, 1.05, 0.92) : vec3(0.35, 0.45, 0.65);
        
        // Shadow ray
        if (!isOccluded(P, L, 50.0)) {
            float NdotL = max(dot(N, L), 0.0);
            if (uLightDiffuse) totalLight += lightCol * hit.mat.albedo * NdotL;
            if (uLightSpecular && NdotL > 0.0) {
                vec3 H = normalize(L + viewDir);
                float spec = pow(max(dot(N, H), 0.0), mix(128.0, 8.0, hit.mat.roughness));
                totalLight += lightCol * spec * mix(0.04, 1.0, hit.mat.metallic);
            }
        }
    }

    // 3. Central Warm Interior Pendant Light
    if (uLightPoint) {
        vec3 lightPos = vec3(0.0, 2.75, -0.2);
        vec3 toLight = lightPos - hit.pos;
        float dist = length(toLight);
        vec3 L = toLight / dist;
        float atten = 1.0 / (1.0 + 0.04 * dist + 0.015 * dist * dist);
        vec3 lightCol = vec3(1.15, 0.88, 0.52); // Warm 2700K tungsten glow

        // Soft shadow factor
        float shadow = calcShadowFactor(P, lightPos, 0.15, dist);
        if (shadow > 0.0) {
            float NdotL = max(dot(N, L), 0.0);
            if (uLightDiffuse) totalLight += lightCol * hit.mat.albedo * NdotL * atten * shadow;
            if (uLightSpecular && NdotL > 0.0) {
                vec3 H = normalize(L + viewDir);
                float spec = pow(max(dot(N, H), 0.0), mix(128.0, 8.0, hit.mat.roughness));
                totalLight += lightCol * spec * mix(0.04, 1.0, hit.mat.metallic) * atten * shadow;
            }
        }
    }

    // 4. Exterior Lantern Lights
    if (uLightPoint) {
        vec3 lanternPos[2];
        lanternPos[0] = vec3(-3.8, 2.60, 4.5);
        lanternPos[1] = vec3( 3.8, 2.60, 4.5);
        vec3 lanternCol = vec3(1.05, 0.65, 0.30); // Cozy lantern flame
        for (int i = 0; i < 2; i++) {
            vec3 toL = lanternPos[i] - hit.pos;
            float dist = length(toL);
            vec3 L = toL / dist;
            float atten = 1.0 / (1.0 + 0.08 * dist + 0.02 * dist * dist);
            float shadow = calcShadowFactor(P, lanternPos[i], 0.18, dist);
            if (shadow > 0.0) {
                float NdotL = max(dot(N, L), 0.0);
                if (uLightDiffuse) totalLight += lanternCol * hit.mat.albedo * NdotL * atten * shadow;
            }
        }
    }

    // 5. Chef Counter Spotlight
    if (uLightSpot) {
        vec3 spotPos = vec3(-1.5, 3.25, -0.3);
        vec3 spotDir = vec3(0.0, -1.0, 0.0);
        vec3 toLight = spotPos - hit.pos;
        float dist = length(toLight);
        vec3 L = toLight / dist;
        float spotCos = dot(-L, spotDir);
        float cutoffCos = cos(radians(28.0));
        if (spotCos > cutoffCos) {
            float spotFactor = pow(spotCos, 24.0);
            float atten = spotFactor / (1.0 + 0.05 * dist + 0.02 * dist * dist);
            vec3 spotCol = vec3(1.1, 0.98, 0.85);
            float shadow = calcShadowFactor(P, spotPos, 0.12, dist);
            if (shadow > 0.0) {
                float NdotL = max(dot(N, L), 0.0);
                if (uLightDiffuse) totalLight += spotCol * hit.mat.albedo * NdotL * atten * shadow;
                if (uLightSpecular && NdotL > 0.0) {
                    vec3 H = normalize(L + viewDir);
                    float spec = pow(max(dot(N, H), 0.0), mix(128.0, 8.0, hit.mat.roughness));
                    totalLight += spotCol * spec * atten * shadow;
                }
            }
        }
    }

    // 6. Overhead Diffused Area Lights
    if (uLightArea) {
        vec3 areaPos[2];
        areaPos[0] = vec3(-1.4, 3.22, -1.2);
        areaPos[1] = vec3( 1.4, 3.22, -1.2);
        vec3 areaCol = vec3(0.95, 0.90, 0.82) * 0.7;
        for (int i = 0; i < 2; i++) {
            vec3 toL = areaPos[i] - hit.pos;
            float dist = length(toL);
            vec3 L = toL / dist;
            float atten = 1.0 / (1.0 + 0.03 * dist + 0.008 * dist * dist);
            float NdotL = max(dot(N, L), 0.0);
            if (uLightDiffuse) totalLight += areaCol * hit.mat.albedo * NdotL * atten;
        }
    }

    return totalLight;
}

// ── Recursive Ray Tracing Engine ───────────────────────────────────────────
vec3 traceRay(vec3 startOrigin, vec3 startDir) {
    vec3 accumulatedColor = vec3(0.0);
    vec3 throughput = vec3(1.0);
    vec3 rayOrig = startOrigin;
    vec3 rayDir  = startDir;

    for (int bounce = 0; bounce < 5; bounce++) {
        if (bounce >= uMaxBounces) break;

        Hit hit;
        if (!intersectScene(rayOrig, rayDir, hit)) {
            // Hit sky background
            accumulatedColor += throughput * getSkyColor(rayDir);
            break;
        }

        // Emissive light surfaces (lanterns, lamps)
        if (length(hit.mat.emission) > 0.01) {
            accumulatedColor += throughput * hit.mat.emission;
            break;
        }

        vec3 V = -rayDir;

        // Direct illumination via Ray-Traced Shadow Rays
        vec3 directLighting = evaluateDirectLighting(hit, V);
        accumulatedColor += throughput * directLighting;

        // Fresnel equations for dielectric / conductor reflection
        float cosi = clamp(dot(V, hit.normal), 0.0, 1.0);
        float F0 = (hit.mat.metallic > 0.5) ? 0.95 : 0.04;
        float fresnel = F0 + (1.0 - F0) * pow(1.0 - cosi, 5.0);

        // Glass Refraction (Snell's Law)
        if (hit.mat.transmission > 0.5) {
            bool entering = (dot(rayDir, hit.normal) < 0.0);
            float eta = entering ? (1.0 / hit.mat.ior) : hit.mat.ior;
            vec3 n = entering ? hit.normal : -hit.normal;
            vec3 refrDir = refract(rayDir, n, eta);

            // Mix Fresnel reflection and transmission
            if (length(refrDir) > 0.01 && (1.0 - fresnel) > 0.15) {
                // Ray transmits through transparent glass
                rayOrig = hit.pos + refrDir * 0.004;
                rayDir  = refrDir;
                throughput *= mix(hit.mat.albedo, vec3(1.0), 0.6) * (1.0 - fresnel * 0.5);
            } else {
                // Total internal reflection or high-angle Fresnel reflection
                vec3 reflDir = reflect(rayDir, hit.normal);
                rayOrig = hit.pos + reflDir * 0.004;
                rayDir  = reflDir;
                throughput *= hit.mat.albedo;
            }
        }
        // Specular Mirror Reflection (Polished Lacquer Counter, Chrome Metals, Glossy Broth, Tiles)
        else if (hit.mat.roughness < 0.28 || hit.mat.metallic > 0.5) {
            vec3 reflDir = reflect(rayDir, hit.normal);
            rayOrig = hit.pos + reflDir * 0.004;
            rayDir  = reflDir;

            vec3 reflTint = mix(vec3(1.0), hit.mat.albedo, hit.mat.metallic);
            throughput *= reflTint * mix(fresnel, 1.0, hit.mat.metallic);
        }
        // Diffuse surface: direct lighting was gathered, ray terminates
        else {
            break;
        }

        // Early out if throughput is negligible
        if (max(max(throughput.r, throughput.g), throughput.b) < 0.02) break;
    }

    return accumulatedColor;
}

// ── Main Fragment Entry ────────────────────────────────────────────────────
void main()
{
    // Compute normalized screen coordinates [-1, 1]
    vec2 uv = (gl_FragCoord.xy / uResolution) * 2.0 - 1.0;
    uv.x *= uAspect;

    // Generate primary camera ray matching gluPerspective(45.0) exactly
    vec3 rayDir = normalize(uCamForward + uv.x * uTanHalfFov * uCamRight + uv.y * uTanHalfFov * uCamUp);

    // Trace ray through the 3D Ramen Shop scene
    vec3 color = traceRay(uCamPos, rayDir);

    // ACES-style Tone Mapping for rich highlights and deep contrast
    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);

    // Subtle atmospheric vignette
    vec2 screenUV = gl_FragCoord.xy / uResolution;
    float vignette = 1.0 - 0.25 * length(screenUV - 0.5);
    color *= vignette;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
)GLSL";

// ─── Helper: compile shader stage ──────────────────────────────────────────
static GLuint compileShaderStage(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);

    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(s, sizeof(log), NULL, log);
        fprintf(stderr, "[RayTracer] %s compile error:\n%s\n",
                (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment"), log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

// ─── Public API Implementation ─────────────────────────────────────────────

void initRayTracer() {
    GLuint vs = compileShaderStage(GL_VERTEX_SHADER,   rtVertSrc);
    GLuint fs = compileShaderStage(GL_FRAGMENT_SHADER, rtFragSrc);
    if (!vs || !fs) {
        fprintf(stderr, "[RayTracer] Failed to compile shaders.\n");
        return;
    }

    rayTraceProgram = glCreateProgram();
    glAttachShader(rayTraceProgram, vs);
    glAttachShader(rayTraceProgram, fs);
    glLinkProgram(rayTraceProgram);

    GLint ok;
    glGetProgramiv(rayTraceProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(rayTraceProgram, sizeof(log), NULL, log);
        fprintf(stderr, "[RayTracer] Program link error:\n%s\n", log);
        glDeleteProgram(rayTraceProgram);
        rayTraceProgram = 0;
        return;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    // Cache uniform locations
    uCamPosLoc        = glGetUniformLocation(rayTraceProgram, "uCamPos");
    uCamForwardLoc    = glGetUniformLocation(rayTraceProgram, "uCamForward");
    uCamRightLoc      = glGetUniformLocation(rayTraceProgram, "uCamRight");
    uCamUpLoc         = glGetUniformLocation(rayTraceProgram, "uCamUp");
    uTanHalfFovLoc    = glGetUniformLocation(rayTraceProgram, "uTanHalfFov");
    uAspectLoc        = glGetUniformLocation(rayTraceProgram, "uAspect");
    uResolutionLoc    = glGetUniformLocation(rayTraceProgram, "uResolution");
    uTimeLoc          = glGetUniformLocation(rayTraceProgram, "uTime");
    uDayTimeLoc       = glGetUniformLocation(rayTraceProgram, "uDayTime");
    uShowRoofLoc      = glGetUniformLocation(rayTraceProgram, "uShowRoof");
    uShowSteamLoc     = glGetUniformLocation(rayTraceProgram, "uShowSteam");
    uMaxBouncesLoc    = glGetUniformLocation(rayTraceProgram, "uMaxBounces");
    uSoftShadowsLoc   = glGetUniformLocation(rayTraceProgram, "uSoftShadows");
    uAOLoc            = glGetUniformLocation(rayTraceProgram, "uAO");
    uLightAmbientLoc  = glGetUniformLocation(rayTraceProgram, "uLightAmbient");
    uLightDiffuseLoc  = glGetUniformLocation(rayTraceProgram, "uLightDiffuse");
    uLightSpecularLoc = glGetUniformLocation(rayTraceProgram, "uLightSpecular");
    uLightDirLoc      = glGetUniformLocation(rayTraceProgram, "uLightDir");
    uLightPointLoc    = glGetUniformLocation(rayTraceProgram, "uLightPoint");
    uLightSpotLoc     = glGetUniformLocation(rayTraceProgram, "uLightSpot");
    uLightAreaLoc     = glGetUniformLocation(rayTraceProgram, "uLightArea");

    rayTracerReady = true;
    fprintf(stderr, "[RayTracer] Real-time GPU Ray Tracer initialized OK! Press 'R' to toggle.\n");
}

void toggleRayTracing() {
    useRayTracing = !useRayTracing;
    printf("[RayTracer] Ray Tracing is now %s\n", useRayTracing ? "ENABLED (Press 'R' to return to raster)" : "DISABLED");
}

void cycleRayBounces() {
    rayTraceBounces = (rayTraceBounces % 5) + 1;
    printf("[RayTracer] Ray Bounces set to %d\n", rayTraceBounces);
}

void toggleRayShadows() {
    rayTraceSoftShadows = !rayTraceSoftShadows;
    printf("[RayTracer] Soft Shadows: %s\n", rayTraceSoftShadows ? "ON" : "OFF");
}

void toggleRayAO() {
    rayTraceAO = !rayTraceAO;
    printf("[RayTracer] Ambient Occlusion: %s\n", rayTraceAO ? "ON" : "OFF");
}

const char* getRayTracingStatusString() {
    static char buf[128];
    if (useRayTracing) {
        sprintf_s(buf, sizeof(buf), "RAY TRACE [R]: ON (Bnc:%d [B])", rayTraceBounces);
    } else {
        sprintf_s(buf, sizeof(buf), "RAY TRACE [R]: OFF");
    }
    return buf;
}

// Compute exact camera basis vectors matching current view
static void computeCameraBasis(Vec3& outEye, Vec3& outFwd, Vec3& outRight, Vec3& outUp) {
    if (currentCamMode == CAM_ORBIT) {
        float a = camAngle * 3.14159265f / 180.0f;
        outEye = { camDistance * sin(a), camHeight, camDistance * cos(a) };
        Vec3 target = { 0.0f, 1.5f, 1.0f };
        Vec3 diff = { target.x - outEye.x, target.y - outEye.y, target.z - outEye.z };
        float dLen = sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        outFwd = { diff.x / dLen, diff.y / dLen, diff.z / dLen };

        // Right = normalize(Fwd x WorldUp)
        Vec3 r = { outFwd.z, 0.0f, -outFwd.x };
        float rLen = sqrt(r.x * r.x + r.z * r.z);
        outRight = { r.x / rLen, 0.0f, r.z / rLen };

        // Up = Right x Fwd
        outUp = {
            outRight.y * outFwd.z - outRight.z * outFwd.y,
            outRight.z * outFwd.x - outRight.x * outFwd.z,
            outRight.x * outFwd.y - outRight.y * outFwd.x
        };
    }
    else if (currentCamMode == CAM_FPS) {
        outEye = fpsPos;
        float radYaw   = fpsYaw   * 3.14159265f / 180.0f;
        float radPitch = fpsPitch * 3.14159265f / 180.0f;
        outFwd = {
            cos(radPitch) * sin(radYaw),
            sin(radPitch),
            cos(radPitch) * cos(radYaw)
        };
        float fLen = sqrt(outFwd.x * outFwd.x + outFwd.y * outFwd.y + outFwd.z * outFwd.z);
        outFwd = { outFwd.x / fLen, outFwd.y / fLen, outFwd.z / fLen };

        Vec3 r = { outFwd.z, 0.0f, -outFwd.x };
        float rLen = sqrt(r.x * r.x + r.z * r.z);
        if (rLen < 1e-4f) { r = { 1.0f, 0.0f, 0.0f }; rLen = 1.0f; }
        outRight = { r.x / rLen, 0.0f, r.z / rLen };

        outUp = {
            outRight.y * outFwd.z - outRight.z * outFwd.y,
            outRight.z * outFwd.x - outRight.x * outFwd.z,
            outRight.x * outFwd.y - outRight.y * outFwd.x
        };
    }
    else { // CAM_FOCUSED: Close-up of the ramen counter
        outEye = { -2.2f, 2.2f, 2.5f };
        Vec3 target = { -2.2f, 1.2f, -0.3f };
        Vec3 diff = { target.x - outEye.x, target.y - outEye.y, target.z - outEye.z };
        float dLen = sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        outFwd = { diff.x / dLen, diff.y / dLen, diff.z / dLen };

        Vec3 r = { outFwd.z, 0.0f, -outFwd.x };
        float rLen = sqrt(r.x * r.x + r.z * r.z);
        outRight = { r.x / rLen, 0.0f, r.z / rLen };

        outUp = {
            outRight.y * outFwd.z - outRight.z * outFwd.y,
            outRight.z * outFwd.x - outRight.x * outFwd.z,
            outRight.x * outFwd.y - outRight.y * outFwd.x
        };
    }
}

static void drawRayTracingHUD(int width, int height) {
    glUseProgram(0);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Dark semi-transparent card in top-left
    glColor4f(0.04f, 0.05f, 0.08f, 0.78f);
    glBegin(GL_QUADS);
        glVertex2i(16, height - 16);
        glVertex2i(380, height - 16);
        glVertex2i(380, height - 76);
        glVertex2i(16, height - 76);
    glEnd();

    // Gold accent outline
    glColor4f(0.96f, 0.68f, 0.18f, 0.90f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2i(16, height - 16);
        glVertex2i(380, height - 16);
        glVertex2i(380, height - 76);
        glVertex2i(16, height - 76);
    glEnd();

    // Line 1: Header
    glColor3f(1.0f, 0.88f, 0.38f);
    glRasterPos2i(28, height - 38);
    const char* l1 = "RAY TRACING: ACTIVE [R to toggle]";
    for (const char* c = l1; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // Line 2: Details & Hotkeys
    char l2[128];
    sprintf_s(l2, sizeof(l2), "Bounces: %d [B] | Shadows: %s [Y]",
              rayTraceBounces, rayTraceSoftShadows ? "SOFT" : "HARD");
    glColor3f(0.85f, 0.90f, 0.95f);
    glRasterPos2i(28, height - 60);
    for (const char* c = l2; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void renderRayTracedFrame(int width, int height) {
    if (!rayTracerReady || !rayTraceProgram) return;

    glUseProgram(rayTraceProgram);

    // Compute camera vectors
    Vec3 eye, fwd, right, up;
    computeCameraBasis(eye, fwd, right, up);

    float fovY = 45.0f * 3.14159265f / 180.0f;
    float tanHalfFov = tan(fovY * 0.5f);
    float aspect = (float)width / (float)(height > 0 ? height : 1);

    // Upload camera uniforms
    glUniform3f(uCamPosLoc, eye.x, eye.y, eye.z);
    glUniform3f(uCamForwardLoc, fwd.x, fwd.y, fwd.z);
    glUniform3f(uCamRightLoc, right.x, right.y, right.z);
    glUniform3f(uCamUpLoc, up.x, up.y, up.z);
    glUniform1f(uTanHalfFovLoc, tanHalfFov);
    glUniform1f(uAspectLoc, aspect);
    glUniform2f(uResolutionLoc, (float)width, (float)height);
    glUniform1f(uTimeLoc, animTime);

    // Upload scene & lighting uniforms
    glUniform1i(uDayTimeLoc,   isDayTime ? 1 : 0);
    glUniform1i(uShowRoofLoc,  showRoof ? 1 : 0);
    glUniform1i(uShowSteamLoc, showSteam ? 1 : 0);
    glUniform1i(uMaxBouncesLoc, rayTraceBounces);
    glUniform1i(uSoftShadowsLoc, rayTraceSoftShadows ? 1 : 0);
    glUniform1i(uAOLoc, rayTraceAO ? 1 : 0);

    glUniform1i(uLightAmbientLoc,  lightAmbient     ? 1 : 0);
    glUniform1i(uLightDiffuseLoc,  lightDiffuse     ? 1 : 0);
    glUniform1i(uLightSpecularLoc, lightSpecular    ? 1 : 0);
    glUniform1i(uLightDirLoc,      lightDirectional ? 1 : 0);
    glUniform1i(uLightPointLoc,    lightPoint       ? 1 : 0);
    glUniform1i(uLightSpotLoc,     lightSpot        ? 1 : 0);
    glUniform1i(uLightAreaLoc,     lightArea        ? 1 : 0);

    // Render fullscreen quad in Normalized Device Coordinates
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
        glVertex2f(-1.0f, -1.0f);
        glVertex2f( 1.0f, -1.0f);
        glVertex2f( 1.0f,  1.0f);
        glVertex2f(-1.0f,  1.0f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
    glUseProgram(0);

    // Render elegant Ray Tracing HUD badge
    drawRayTracingHUD(width, height);
}
