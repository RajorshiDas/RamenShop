#pragma once
#include "shapes.h"

// ─── Selectable scene objects ────────────────────────────────────────────────
enum ObjID {
    OBJ_NONE      = -1,
    OBJ_BOWL      =  0,   // first ramen bowl + chopsticks
    OBJ_KETTLE    =  1,   // metal cooking pot / kettle
    OBJ_LANTERN_L =  2,   // left front lantern
    OBJ_LANTERN_R =  3,   // right front lantern
    OBJ_NOREN     =  4,   // noren curtain
    OBJ_STOOL     =  5,   // first counter stool
    OBJ_COUNT     =  6
};

struct SceneObject {
    const char* name;
    Vec3  dPos;     // accumulated delta translation
    Vec3  dRot;     // accumulated delta rotation  (degrees: x=pitch, y=yaw)
    float dScale;   // uniform scale multiplier  (1.0 = unchanged)
};

extern SceneObject sceneObjects[OBJ_COUNT];
extern int         selectedObj;   // OBJ_NONE or 0 .. OBJ_COUNT-1

// Call inside a glPushMatrix / glPopMatrix pair to prepend the stored delta.
void applyObjDelta(int objIdx);

// Cycle selection:  none → 0 → 1 → … → OBJ_COUNT-1 → none
void selectNextObj();

// Manipulation helpers (no-ops when selectedObj == OBJ_NONE)
void objTranslate(float dx, float dy, float dz);
void objRotateY  (float deg);
void objRotateX  (float deg);
void objScaleMul (float factor);
