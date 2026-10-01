#include "camera.h"
#include "scene.h"
#include "objects.h"
#include "shader.h"
#include "raytracer.h"
#include <cstdio>

CameraMode currentCamMode = CAM_ORBIT;

// Orbit Camera
float camAngle    = 30.0f;
float camDistance = 20.0f;
float camHeight   = 8.0f;

// FPS Walkthrough Camera
Vec3  fpsPos   = { 0.0f, 1.65f, 9.0f };
float fpsYaw   = 180.0f;
float fpsPitch = 0.0f;

// Key states
bool keyStates[256]        = { false };
bool specialKeyStates[256] = { false };

static int  lastMouseX     = -1, lastMouseY = -1;
static bool isMouseDragging = false;

// ─── Window title ──────────────────────────────────────────────────────────
void updateWindowTitle()
{
    char buf[512];
    const char* camName =
        (currentCamMode == CAM_ORBIT)   ? "Orbit" :
        (currentCamMode == CAM_FPS)     ? "Walkthrough" : "Counter View";

    const char* objName =
        (selectedObj == OBJ_NONE) ? "none" : sceneObjects[selectedObj].name;

    sprintf_s(buf, sizeof(buf),
        "3D Ramen Shop | %s | %s[T] | %s[G] | Cam:%s[C] | Amb:%s[1] Dif:%s[2] Spec:%s[3] | Dir:%s[4] Pt:%s[5] Spot:%s[6] Area:%s[7] | Preset:%s[P]",
        getRayTracingStatusString(),
        getDayNightModeName(),
        usePhongShading ? "Phong" : "Gouraud",
        camName,
        lightAmbient     ? "ON" : "OFF",
        lightDiffuse     ? "ON" : "OFF",
        lightSpecular    ? "ON" : "OFF",
        lightDirectional ? "ON" : "OFF",
        lightPoint       ? "ON" : "OFF",
        lightSpot        ? "ON" : "OFF",
        lightArea        ? "ON" : "OFF",
        getCurrentPresetName());
    glutSetWindowTitle(buf);
}

// ─── Apply camera view ─────────────────────────────────────────────────────
void applyCameraView()
{
    if (currentCamMode == CAM_ORBIT) {
        float a = camAngle * PI / 180.0f;
        gluLookAt(camDistance * sin(a), camHeight, camDistance * cos(a),
                  0, 1.5f, 1,
                  0, 1, 0);
    }
    else if (currentCamMode == CAM_FPS) {
        float radYaw   = fpsYaw   * PI / 180.0f;
        float radPitch = fpsPitch * PI / 180.0f;
        float fx = cos(radPitch) * sin(radYaw);
        float fy = sin(radPitch);
        float fz = cos(radPitch) * cos(radYaw);
        gluLookAt(fpsPos.x, fpsPos.y, fpsPos.z,
                  fpsPos.x + fx, fpsPos.y + fy, fpsPos.z + fz,
                  0, 1, 0);
    }
    else {   // CAM_FOCUSED — close-up of the counter / first bowl
        gluLookAt(-2.2f, 2.2f, 2.5f,   // eye: above and in front of first bowl
                  -2.2f, 1.2f, -0.3f,  // look-at: first bowl position
                  0, 1, 0);
    }
}

// ─── Continuous FPS movement ───────────────────────────────────────────────
void updateCameraMovement()
{
    if (currentCamMode != CAM_FPS) return;

    float radYaw  = fpsYaw * PI / 180.0f;
    float sinY    = sin(radYaw), cosY = cos(radYaw);
    Vec3 fwd   = { sinY, 0, cosY };
    Vec3 right = { -fwd.z, 0, fwd.x };
    float spd  = 0.085f;

    if (keyStates['w'] || keyStates['W']) { fpsPos.x += fwd.x*spd; fpsPos.z += fwd.z*spd; }
    if (keyStates['s'] || keyStates['S']) { fpsPos.x -= fwd.x*spd; fpsPos.z -= fwd.z*spd; }
    if (keyStates['d'] || keyStates['D']) { fpsPos.x += right.x*spd; fpsPos.z += right.z*spd; }
    if (keyStates['a'] || keyStates['A']) { fpsPos.x -= right.x*spd; fpsPos.z -= right.z*spd; }
    if (keyStates['e'] || keyStates['E']) fpsPos.y += spd * 0.8f;
    if (keyStates['q'] || keyStates['Q']) fpsPos.y -= spd * 0.8f;
    if (fpsPos.y < 0.6f) fpsPos.y = 0.6f;
}

// ─── Keyboard handlers ─────────────────────────────────────────────────────
void handleKeyboardDown(unsigned char key, int, int)
{
    keyStates[key] = true;

    // ── Camera mode cycle (Orbit → FPS → Focused → Orbit) ──
    if (key == 'c' || key == 'C') {
        if      (currentCamMode == CAM_ORBIT)   currentCamMode = CAM_FPS;
        else if (currentCamMode == CAM_FPS)     currentCamMode = CAM_FOCUSED;
        else                                    currentCamMode = CAM_ORBIT;
        updateWindowTitle();
    }

    // ── Scene toggles ──
    if (key == 'h' || key == 'H') showRoof = !showRoof;
    if (key == 'o' || key == 'O') showOutlines = !showOutlines;
    if (key == 'f' || key == 'F') {
        showFog = !showFog;
        if (showFog) glEnable(GL_FOG); else glDisable(GL_FOG);
    }
    if (key == 'x' || key == 'X') showSteam = !showSteam;  // X = steam (S freed)
    if (key == ' ') animPaused = !animPaused;
    if (key == 27)  exit(0);

    // ── Object selection (TAB = ASCII 9) ──
    if (key == 9) {
        selectNextObj();
        updateWindowTitle();
    }

    // ── Object manipulation (only when something is selected) ──
    if (selectedObj != OBJ_NONE) {
        const float dt = 0.15f;   // translate step (world units)
        const float dr = 10.0f;   // rotate step (degrees)
        const float ds = 1.12f;   // scale step factor

        if (key == 'i' || key == 'I') objTranslate( 0,  0, -dt);  // forward
        if (key == 'k' || key == 'K') objTranslate( 0,  0,  dt);  // backward
        if (key == 'j' || key == 'J') objTranslate(-dt, 0,  0);   // left
        if (key == 'l' || key == 'L') objTranslate( dt, 0,  0);   // right
        if (key == 'u' || key == 'U') objTranslate( 0,  dt, 0);   // up
        if (key == 'n' || key == 'N') objTranslate( 0, -dt, 0);   // down

        if (key == '[')  objRotateY(-dr);   // rotate CW
        if (key == ']')  objRotateY( dr);   // rotate CCW
        if (key == ';')  objRotateX(-dr);   // tilt forward
        if (key == '\'') objRotateX( dr);   // tilt backward

        if (key == ',')  objScaleMul(1.0f / ds);   // shrink
        if (key == '.')  objScaleMul(ds);           // grow
    }

    // ── Ray Tracing Toggles ──
    if (key == 'r' || key == 'R') { toggleRayTracing(); updateWindowTitle(); }
    if (key == 'b' || key == 'B') { cycleRayBounces();  updateWindowTitle(); }
    if (key == 'y' || key == 'Y') { toggleRayShadows(); updateWindowTitle(); }

    // ── Light component & source toggles ──
    if (key == '1') { toggleAmbient();     updateWindowTitle(); }
    if (key == '2') { toggleDiffuse();     updateWindowTitle(); }
    if (key == '3') { toggleSpecular();    updateWindowTitle(); }
    if (key == '4') { toggleDirectional(); updateWindowTitle(); }
    if (key == '5') { togglePointLights(); updateWindowTitle(); }
    if (key == '6') { toggleSpotLight();   updateWindowTitle(); }
    if (key == '7') { toggleAreaLight();   updateWindowTitle(); }
    if (key == 'p' || key == 'P') { cycleLightingPreset(); updateWindowTitle(); }
    if (key == 't' || key == 'T') { toggleDayNight(); updateWindowTitle(); }
    if (key == 'g' || key == 'G') { usePhongShading = !usePhongShading; updateWindowTitle(); }

    glutPostRedisplay();
}

void handleKeyboardUp(unsigned char key, int, int)
{
    keyStates[key] = false;
}

void handleSpecialDown(int key, int, int)
{
    specialKeyStates[key] = true;
    if (currentCamMode == CAM_ORBIT) {
        if (key == GLUT_KEY_LEFT)                 camAngle   -= 5;
        if (key == GLUT_KEY_RIGHT)                camAngle   += 5;
        if (key == GLUT_KEY_UP && camDistance > 3) camDistance -= 1;
        if (key == GLUT_KEY_DOWN)                 camDistance += 1;
        if (key == GLUT_KEY_PAGE_UP)              camHeight  += 0.5f;
        if (key == GLUT_KEY_PAGE_DOWN)            camHeight  -= 0.5f;
    } else if (currentCamMode == CAM_FPS) {
        if (key == GLUT_KEY_PAGE_UP)              fpsPos.y += 0.3f;
        if (key == GLUT_KEY_PAGE_DOWN && fpsPos.y > 0.8f) fpsPos.y -= 0.3f;
    }
    glutPostRedisplay();
}

void handleSpecialUp(int key, int, int)
{
    specialKeyStates[key] = false;
}

void handleMouseClick(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Ray-cast to detect click on entrance door
        GLdouble mv[16], proj[16];
        GLint vp[4];
        glGetDoublev(GL_MODELVIEW_MATRIX, mv);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, vp);

        GLdouble nx, ny, nz, fx, fy, fz;
        double wy = (double)(vp[3] - y);
        gluUnProject(x, wy, 0.0, mv, proj, vp, &nx, &ny, &nz);
        gluUnProject(x, wy, 1.0, mv, proj, vp, &fx, &fy, &fz);

        double rdx = fx - nx, rdy = fy - ny, rdz = fz - nz;

        // Intersect ray with entrance door plane z = 3.92
        if (fabs(rdz) > 0.0001) {
            double t = (3.92 - nz) / rdz;
            if (t > 0) {
                double hitX = nx + t * rdx;
                double hitY = ny + t * rdy;
                // Door bounds: x in [-1.75, 1.75], y in [0.10, 3.15]
                if (hitX > -1.75 && hitX < 1.75 && hitY > 0.10 && hitY < 3.15) {
                    doorOpen = !doorOpen;
                }
            }
        }

        // Intersect ray with left wall plane x = -4.9 (sliding shoji door)
        if (fabs(rdx) > 0.0001) {
            double t = (-4.9 - nx) / rdx;
            if (t > 0) {
                double hitY = ny + t * rdy;
                double hitZ = nz + t * rdz;
                // Sliding door centered at z=2.8, y=1.5, size 2.4×2.8
                // Z bounds: 2.8 ± 1.2 = [1.6, 4.0]
                // Y bounds: 1.5 ± 1.4 = [0.1, 2.9]
                if (hitZ > 1.6 && hitZ < 4.0 && hitY > 0.1 && hitY < 2.9) {
                    slideDoorOpen = !slideDoorOpen;
                }
            }
        }
    }
    if (button == GLUT_LEFT_BUTTON) {
        isMouseDragging = (state == GLUT_DOWN);
        lastMouseX = x; lastMouseY = y;
    }
    if (button == 3 && state == GLUT_DOWN) {   // wheel up
        if (currentCamMode == CAM_ORBIT && camDistance > 3) camDistance -= 0.8f;
        else if (currentCamMode == CAM_FPS)  fpsPos.y += 0.2f;
        glutPostRedisplay();
    }
    if (button == 4 && state == GLUT_DOWN) {   // wheel down
        if (currentCamMode == CAM_ORBIT)        camDistance += 0.8f;
        else if (currentCamMode == CAM_FPS && fpsPos.y > 0.8f) fpsPos.y -= 0.2f;
        glutPostRedisplay();
    }
}

void handleMouseMotion(int x, int y)
{
    if (!isMouseDragging) return;
    int dx = x - lastMouseX, dy = y - lastMouseY;

    if (currentCamMode == CAM_FPS) {
        fpsYaw   -= dx * 0.25f;
        fpsPitch -= dy * 0.25f;
        if (fpsPitch >  85) fpsPitch =  85;
        if (fpsPitch < -85) fpsPitch = -85;
    } else if (currentCamMode == CAM_ORBIT) {
        camAngle  += dx * 0.35f;
        camHeight += dy * 0.04f;
        if (camHeight < 0.5f) camHeight = 0.5f;
    }
    lastMouseX = x; lastMouseY = y;
    glutPostRedisplay();
}
