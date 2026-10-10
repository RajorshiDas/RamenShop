#pragma once

// ════════════════════════════════════════════════════════════════════════════
//  CPU RAY TRACER - core (no OpenGL here, so it can be tested on its own)
//
//  A small scene of exact analytic shapes (boxes, truncated cones, disks, spheres) with a
//  bounding volume hierarchy (BVH) over them.  Rays are intersected with the shapes, then
//    - metal, glazed ceramic, liquid and mirror surfaces spawn reflection rays (Fresnel),
//    - glass bends rays with Snell's law (and total internal reflection),
//    - other surfaces are shaded with the same Blinn-Phong model as the OpenGL scene.
//  Only "featured" shapes (object id >= 0: the pots, bowls, glass ball and mirror) are
//  ray traced on screen; everything else exists so the rays have something to hit.
// ════════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <vector>

namespace rt {

struct V3 { float x, y, z; };

struct Texture {                        // RGB8, row 0 = v 0 (as uploaded to OpenGL), repeating
    int w = 0, h = 0;
    std::vector<uint8_t> rgb;
    V3 sample(float u, float v) const;  // bilinear
};

struct Light {                          // one OpenGL light, in world space
    bool  on = false, directional = false;
    V3    pos = { 0, 0, 0 };            // position, or direction toward the light when directional
    V3    amb = { 0, 0, 0 }, dif = { 0, 0, 0 }, spec = { 0, 0, 0 };
    float kc = 1, kl = 0, kq = 0;       // attenuation 1 / (kc + kl d + kq d^2)
    V3    spotDir = { 0, -1, 0 };
    float spotCos = -2, spotExp = 0;    // spotCos < -1: no cone
};

enum MatType { MAT_DIFFUSE, MAT_METAL, MAT_GLAZE, MAT_LIQUID, MAT_GLASS, MAT_MIRROR, MAT_EMISSIVE };
enum TexMap  { MAP_NONE, MAP_XZ, MAP_XY, MAP_ZY, MAP_TATAMI };   // planar texture projections

struct Material {
    int   type = MAT_DIFFUSE;
    V3    color = { 1, 1, 1 };          // base colour (x texture); glass / water: tint per surface crossed
    V3    spec = { 0, 0, 0 };           // Blinn-Phong specular colour and exponent
    float shininess = 1.0f;
    V3    emission = { 0, 0, 0 };       // MAT_EMISSIVE: light given off (may exceed 1: lamps)
    V3    f0 = { 0.04f, 0.04f, 0.04f }; // reflectance at normal incidence (metal, mirror, glaze, liquid)
    float ior = 1.5f;                   // MAT_GLASS: refractive index (glass 1.5, water 1.33)
    const Texture* tex = nullptr;
    int   map = MAP_NONE;
    float uvScale = 1.0f, u0 = 0.0f, v0 = 0.0f;   // u = (coord - u0) / uvScale
};

enum ShapeType { SH_BOX, SH_CONE, SH_DISK, SH_SPHERE };
struct Shape {
    int   type = SH_BOX;
    int   material = 0;
    int   object = -1;                  // >= 0: featured object, its pixels are ray traced on screen
    V3    a = { 0, 0, 0 }, b = { 0, 0, 0 };   // box: min / max corner;  cone, disk, sphere: centre (cone: base)
    float r0 = 0, r1 = 0, h = 0;        // cone: radius at a.y and at a.y + h;  disk: inner / outer radius,
                                        // h = +1 or -1 (normal up / down);  sphere: radius r0
    bool  flip = false;                 // normal points inward (inner wall of a hollow glass: the glass is outside)
    bool  rasterDepth = true;           // false: the OpenGL version writes no depth (clear glass) - composited
                                        // wherever nothing opaque is in front instead of on the same surface
    V3    facing = { 0, 0, 0 };         // non-zero: one-sided, only rays travelling against it hit
                                        // (a wall seen from inside the room, see-through from outside)
    V3    bmin = { 0, 0, 0 }, bmax = { 0, 0, 0 };   // bounds (Scene::build)
};

struct Scene {
    std::vector<Material> materials;
    std::vector<Shape>    shapes;
    std::vector<Light>    lights;
    V3  globalAmb = { 0, 0, 0 };
    V3  sky = { 0.10f, 0.10f, 0.16f };  // rays that leave everything
    int maxDepth = 3;                   // reflection / refraction bounces

    // BVH over the small shapes; the large ones (floors, walls, ceilings) are tested directly,
    // since they overlap everything and would make every BVH box large (Scene::build fills these)
    struct Node { V3 bmin, bmax; int left, right, first, count; };
    std::vector<Node> nodes;
    std::vector<int>  order;
    std::vector<int>  large;
    std::vector<int>  featured;         // shapes of the ray-traced objects (tested by the eye rays)
    std::vector<std::vector<int>> objectShapes;   // shapes of each ray-traced object (rays inside a glass)
    int  addMaterial(const Material& m) { materials.push_back(m); return (int)materials.size() - 1; }
    void build();
};

struct Hit { float t; V3 p; V3 n; int shape; };     // n: geometric outward normal

bool intersect(const Scene& s, V3 o, V3 d, float tmin, float tmax, Hit& h);

// Snell's law: refract unit direction d at a surface whose normal n faces against d.
// eta = n1 / n2.  Returns false on total internal reflection.
bool refractDir(V3 d, V3 n, float eta, V3& t);

struct Camera { V3 pos; float viewProj[16]; };      // column-major, as OpenGL

// What the compositing shader does with one pixel:  colour = add + weight * raster
struct PixelOut {
    float add[3];
    float weight;
    float depth;       // window depth of the ray-traced surface (it must be the one the raster shows)
    float mode;        // 0 keep the raster pixel, 1 write (same surface as the raster), 2 write (nothing in front)
};

// Trace one eye ray (unit direction): written only where its first hit is a featured shape
void tracePixel(const Scene& s, const Camera& cam, V3 dir, PixelOut& out);

}   // namespace rt
