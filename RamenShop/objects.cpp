#include "objects.h"

SceneObject sceneObjects[OBJ_COUNT] = {
    { "Ramen Bowl",    {0,0,0}, {0,0,0}, 1.0f },
    { "Metal Kettle",  {0,0,0}, {0,0,0}, 1.0f },
    { "Left Lantern",  {0,0,0}, {0,0,0}, 1.0f },
    { "Right Lantern", {0,0,0}, {0,0,0}, 1.0f },
    { "Noren Curtain", {0,0,0}, {0,0,0}, 1.0f },
    { "Counter Stool", {0,0,0}, {0,0,0}, 1.0f },
};

int selectedObj = OBJ_NONE;

void applyObjDelta(int objIdx)
{
    if (objIdx < 0 || objIdx >= OBJ_COUNT) return;
    const SceneObject& o = sceneObjects[objIdx];
    glTranslatef(o.dPos.x, o.dPos.y, o.dPos.z);
    glRotatef(o.dRot.y, 0, 1, 0);
    glRotatef(o.dRot.x, 1, 0, 0);
    if (o.dScale != 1.0f) glScalef(o.dScale, o.dScale, o.dScale);
}

void selectNextObj()
{
    if (selectedObj == OBJ_NONE)
        selectedObj = 0;
    else if (selectedObj + 1 < OBJ_COUNT)
        selectedObj++;
    else
        selectedObj = OBJ_NONE;
}

void objTranslate(float dx, float dy, float dz)
{
    if (selectedObj < 0) return;
    sceneObjects[selectedObj].dPos.x += dx;
    sceneObjects[selectedObj].dPos.y += dy;
    sceneObjects[selectedObj].dPos.z += dz;
}

void objRotateY(float deg)
{
    if (selectedObj < 0) return;
    sceneObjects[selectedObj].dRot.y += deg;
}

void objRotateX(float deg)
{
    if (selectedObj < 0) return;
    sceneObjects[selectedObj].dRot.x += deg;
}

void objScaleMul(float factor)
{
    if (selectedObj < 0) return;
    float& s = sceneObjects[selectedObj].dScale;
    s *= factor;
    if (s < 0.1f)  s = 0.1f;
    if (s > 10.0f) s = 10.0f;
}
