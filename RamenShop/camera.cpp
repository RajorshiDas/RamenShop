#include "camera.h"
#include "scene.h"
#include "objects.h"
#include "shader.h"
#include "shadowmap.h"
#include "refraction.h"
#include "rtpass.h"
#include "game.h"
#include "lighting.h"
#include <cstdio>

CameraMode currentCamMode = CAM_ORBIT;

// Orbit Camera (exterior overview)
float camAngle    = 30.0f;
float camDistance  = 20.0f;
float camHeight   = 8.0f;
float camLookAtY  = 1.5f;

// FPS Walkthrough Camera
Vec3  fpsPos   = { 0.0f, 1.65f, 9.0f };
float fpsYaw   = 180.0f;
float fpsPitch = 0.0f;

// Upper Room orbit
float upperAngle    = 40.0f;
float upperDistance  = 3.6f;
float upperHeight   = 4.5f;
float upperLookAtY  = 3.5f;

// Pond View orbit (around the lake behind the shop at (0, -24), where the ducks swim)
static const float POND_X = 0.0f, POND_Z = -24.0f;
float pondAngle    = 70.0f;        // from the shop side, a little to the right
float pondDistance = 9.0f;
float pondHeight   = 3.8f;
float pondLookAtY  = 0.2f;

// Counter View orbit
float counterAngle    = 16.0f;     // slightly off-axis: shows the counter, kitchen and stools in depth
float counterDistance  = 4.6f;
float counterHeight   = 1.75f;     // roughly seated-guest eye height
float counterLookAtY  = 1.30f;

// Bowl close-up (key B): orbits the second ramen bowl on the counter, which stands between the
// glass ball, a water glass and the jug - a good spot to see the ray-traced reflections (R)
static const float BOWL_X = -0.70f, BOWL_Y = FLOOR_Y + 1.06f + 0.08f, BOWL_Z = -0.30f;
float bowlAngle    = 20.0f;        // degrees around the bowl, 0 = from the dining side
float bowlDistance = 0.70f;        // horizontal distance from the bowl
float bowlHeight   = 0.32f;        // eye height above the bowl
static CameraMode camBeforeBowl = CAM_ORBIT;

// Key states
bool keyStates[256]        = { false };
bool specialKeyStates[256] = { false };

static int  lastMouseX     = -1, lastMouseY = -1;
static bool isMouseDragging = false;

// ─── Light tour ─────────────────────────────────────────────────────────────
// Key 8: the camera glides to the next interior light source and that light is switched on.
// Key 9: toggles the light group of the source you are looking at.
struct LightStop { const char* name; Vec3 cam, look; int group; };   // group: 0 point, 1 spot, 2 area
static const LightStop LIGHT_STOPS[] = {
    { "Dining pendant lamps",       {  0.0f, 2.35f,  2.6f }, {  0.0f, 2.80f, -0.2f }, 0 },
    { "Table spotlights (x2)",       {  0.0f, 2.45f,  0.9f }, {  0.0f, 3.10f,  2.8f }, 1 },
    { "Ceiling area light",         {  0.0f, 2.45f,  1.8f }, {  0.0f, 3.20f, -1.2f }, 2 },
    { "Paper box lanterns",         {  1.4f, 2.45f,  2.6f }, {  0.5f, 3.10f,  0.6f }, 0 },
    { "Hanging lanterns",           {  0.0f, 2.40f,  2.2f }, { -1.8f, 3.00f,  0.0f }, 0 },
    { "Floor lantern tower",        { -2.4f, 1.60f,  4.4f }, { -4.1f, 1.40f,  3.2f }, 0 },
    { "Second-floor paper dome",    {  0.0f, 4.35f,  2.8f }, {  0.0f, 5.10f,  0.0f }, 2 },
};
static const int NUM_LIGHT_STOPS = sizeof(LIGHT_STOPS) / sizeof(LIGHT_STOPS[0]);
static int  lightStop = -1;
static Vec3 lightCamPos = { 0, 2.4f, 3.0f }, lightCamLook = { 0, 2.8f, 0 };

const char* getLightTourName() { return (lightStop >= 0) ? LIGHT_STOPS[lightStop].name : "-"; }

static bool groupOn(int g) { return g == 0 ? lightPoint : (g == 1 ? lightSpot : lightArea); }
static void toggleGroup(int g) { if (g == 0) togglePointLights(); else if (g == 1) toggleSpotLight(); else toggleAreaLight(); }

static void nextLightStop()
{
    if (currentCamMode != CAM_LIGHT) {                       // start the tour from where the camera is now
        lightCamPos  = { fpsPos.x, fpsPos.y, fpsPos.z };
        lightCamLook = { 0.0f, 2.5f, 0.0f };
    }
    lightStop = (lightStop + 1) % NUM_LIGHT_STOPS;
    currentCamMode = CAM_LIGHT;
    if (!groupOn(LIGHT_STOPS[lightStop].group)) toggleGroup(LIGHT_STOPS[lightStop].group);   // switch the group on
    fixtureOn[lightStop] = true;                                                             // and this fixture
    updateWindowTitle();
}

// ─── Window title ──────────────────────────────────────────────────────────
void updateWindowTitle()
{
    char buf[512];
    const char* camName =
        (currentCamMode == CAM_ORBIT)   ? "Orbit" :
        (currentCamMode == CAM_FPS)     ? "Walkthrough" :
        (currentCamMode == CAM_UPPER)   ? "Upper Room" :
        (currentCamMode == CAM_POND)    ? "Pond View" :
        (currentCamMode == CAM_LIGHT)   ? "Light Tour" :
        (currentCamMode == CAM_BOWL)    ? "Bowl Close-up[B]" : "Counter View";
    char tourBuf[96];
    if (currentCamMode == CAM_LIGHT) {
        sprintf_s(tourBuf, sizeof(tourBuf), "Light: %s %s [8 next, 9 toggle]", getLightTourName(), (lightStop >= 0 && fixtureOn[lightStop]) ? "ON" : "OFF");
        camName = tourBuf;
    }

    const char* objName =
        (selectedObj == OBJ_NONE) ? "none" : sceneObjects[selectedObj].name;

    sprintf_s(buf, sizeof(buf),
        "3D Ramen Shop | %s | Cam:%s[C] | Sun/Moon[0]:%s | Lights 1-7: %c %c %c %c %c %c %c | [-] Amb:%s [=] Dif:%s [Bksl] Spec:%s | [8] tour | Shadow[Y]:%s Glass[Z]:%s RT[R]:%s",
        getDayNightModeName(),
        camName,
        lightDirectional ? "ON" : "off",
        (lightPoint && fixtureOn[0]) ? '1' : '-', (lightSpot && fixtureOn[1]) ? '2' : '-', (lightArea && fixtureOn[2]) ? '3' : '-',
        (lightPoint && fixtureOn[3]) ? '4' : '-', (lightPoint && fixtureOn[4]) ? '5' : '-', (lightPoint && fixtureOn[5]) ? '6' : '-',
        (lightArea && fixtureOn[6]) ? '7' : '-',
        lightAmbient ? "ON" : "off", lightDiffuse ? "ON" : "off", lightSpecular ? "ON" : "off",
        getShadowStatus(), getGlassStatus(), getRayTracingStatus());
    glutSetWindowTitle(buf);
}

// ─── Apply camera view ─────────────────────────────────────────────────────
void applyCameraView()
{
    if (currentCamMode == CAM_ORBIT) {
        float a = camAngle * PI / 180.0f;
        gluLookAt(camDistance * sin(a), camHeight, camDistance * cos(a),
                  0, camLookAtY, 0,
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
    else if (currentCamMode == CAM_LIGHT) {
        gluLookAt(lightCamPos.x, lightCamPos.y, lightCamPos.z,
                  lightCamLook.x, lightCamLook.y, lightCamLook.z,
                  0, 1, 0);
    }
    else if (currentCamMode == CAM_POND) {
        // Orbit around the pond, looking at the water where the ducks swim
        float a = pondAngle * PI / 180.0f;
        gluLookAt(POND_X + pondDistance * sin(a), pondHeight, POND_Z + pondDistance * cos(a),
                  POND_X, pondLookAtY, POND_Z,
                  0, 1, 0);
    }
    else if (currentCamMode == CAM_UPPER) {
        // Orbit inside the second-floor tatami room
        float a = upperAngle * PI / 180.0f;
        gluLookAt(upperDistance * sin(a), upperHeight, upperDistance * cos(a),
                  0, upperLookAtY, 0,
                  0, 1, 0);
    }
    else if (currentCamMode == CAM_BOWL) {
        // Close look at a ramen bowl (and the glass ball, glass and jug beside it)
        float a = bowlAngle * PI / 180.0f;
        gluLookAt(BOWL_X + bowlDistance * sin(a), BOWL_Y + bowlHeight, BOWL_Z + bowlDistance * cos(a),
                  BOWL_X, BOWL_Y, BOWL_Z,
                  0, 1, 0);
    }
    else {   // CAM_FOCUSED — orbit around the counter area
        float a = counterAngle * PI / 180.0f;
        float lookX = 0.0f, lookZ = -0.3f;
        gluLookAt(lookX + counterDistance * sin(a), counterHeight, lookZ + counterDistance * cos(a),
                  lookX, counterLookAtY, lookZ,
                  0, 1, 0);
    }
}

// ─── Floor height under the camera (handles stairs + second floor) ──────────
// Returns the surface Y the camera should stand on at world (x, z).
// currentY lets us know whether we are already on the second floor.
static float getFloorY(float x, float z, float currentY)
{
    const float GRND   = FLOOR_Y;   // 0.10  ground floor surface
    const float FLOOR2 = 3.42f;     // GH(3.3) + slab(0.12) — second-floor surface
    const float EYE    = 1.65f;     // eye height above floor

    // ── Staircase parameters (must match scene.cpp drawInterior) ──
    const float SX  = 4.0f;   // centre X
    const float SW  = 1.0f;   // width
    const float SZ0 = 2.5f;   // Z of front face of step 0
    const float SH  = 0.332f; // riser height
    const float SD  = 0.30f;  // tread depth
    const int   NS  = 10;     // step count

    // Inside staircase x-column?
    if (x >= SX - SW * 0.5f - 0.2f && x <= SX + SW * 0.5f + 0.2f) {
        float dist = SZ0 - z;   // how far along the run (0 = front, NS*SD = back)

        // On a step tread
        if (dist >= 0.0f && dist < NS * SD) {
            int step = (int)(dist / SD);
            if (step >= NS) step = NS - 1;
            return GRND + (step + 1) * SH;   // tread top of step i
        }
        // Past the last step — already on the second-floor landing
        if (dist >= NS * SD && dist < NS * SD + 1.5f) {
            return FLOOR2;
        }
    }

    // Already walking on the second floor (anywhere inside the building)
    if (currentY >= FLOOR2 + EYE * 0.5f) {
        if (x >= -4.8f && x <= 4.8f && z >= -3.8f && z <= 3.8f)
            return FLOOR2;
    }

    return GRND;
}

// ─── Continuous FPS movement ───────────────────────────────────────────────
void updateCameraMovement()
{
    // Frame-rate independent movement: real elapsed time since the last call
    static int lastMoveMs = 0;
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt = (lastMoveMs == 0) ? 0.016f : (nowMs - lastMoveMs) * 0.001f;
    lastMoveMs = nowMs;
    if (dt > 0.1f) dt = 0.1f;
    if (dt <= 0.0f) dt = 0.001f;

    if (currentCamMode == CAM_LIGHT && lightStop >= 0) {      // glide toward the chosen light
        float k = 1.0f - expf(-4.0f * dt);
        const LightStop& t = LIGHT_STOPS[lightStop];
        lightCamPos.x  += (t.cam.x  - lightCamPos.x)  * k;  lightCamPos.y  += (t.cam.y  - lightCamPos.y)  * k;  lightCamPos.z  += (t.cam.z  - lightCamPos.z)  * k;
        lightCamLook.x += (t.look.x - lightCamLook.x) * k;  lightCamLook.y += (t.look.y - lightCamLook.y) * k;  lightCamLook.z += (t.look.z - lightCamLook.z) * k;
    }
    if (currentCamMode != CAM_FPS) return;

    // In the game the arrow keys turn the cook (keyboard alternative to mouse drag)
    if (gameActive) {
        if (specialKeyStates[GLUT_KEY_LEFT])  fpsYaw   += 140.0f * dt;
        if (specialKeyStates[GLUT_KEY_RIGHT]) fpsYaw   -= 140.0f * dt;
        if (specialKeyStates[GLUT_KEY_UP]   && fpsPitch <  60.0f) fpsPitch += 90.0f * dt;
        if (specialKeyStates[GLUT_KEY_DOWN] && fpsPitch > -60.0f) fpsPitch -= 90.0f * dt;
    }

    float radYaw  = fpsYaw * PI / 180.0f;
    float sinY    = sin(radYaw), cosY = cos(radYaw);
    Vec3 fwd   = { sinY, 0, cosY };
    Vec3 right = { -fwd.z, 0, fwd.x };
    float spd  = 5.3f * dt;     // 5.3 units/s (was 0.085 per frame at 60 fps)

    if (keyStates['w'] || keyStates['W']) { fpsPos.x += fwd.x*spd; fpsPos.z += fwd.z*spd; }
    if (keyStates['s'] || keyStates['S']) { fpsPos.x -= fwd.x*spd; fpsPos.z -= fwd.z*spd; }
    if (keyStates['d'] || keyStates['D']) { fpsPos.x += right.x*spd; fpsPos.z += right.z*spd; }
    if (keyStates['a'] || keyStates['A']) { fpsPos.x -= right.x*spd; fpsPos.z -= right.z*spd; }
    if (!gameActive) {   // in the game E / Q are interact / discard, so no flying
        if (keyStates['e'] || keyStates['E']) fpsPos.y += spd * 0.8f;
        if (keyStates['q'] || keyStates['Q']) fpsPos.y -= spd * 0.8f;
    }
    gameClampPlayer(fpsPos);

    // Ground/stair following: keep camera eye above the surface beneath it
    const float eyeH  = 1.65f;
    float groundY     = getFloorY(fpsPos.x, fpsPos.z, fpsPos.y);
    float minY        = groundY + eyeH;
    if (fpsPos.y < minY) {
        fpsPos.y = minY;          // step up instantly (stair rise)
    } else if (fpsPos.y > minY + 0.05f) {
        fpsPos.y -= 0.07f;        // gentle fall back to surface
        if (fpsPos.y < minY) fpsPos.y = minY;
    }
}

// ─── Keyboard handlers ─────────────────────────────────────────────────────
void handleKeyboardDown(unsigned char key, int, int)
{
    keyStates[key] = true;

    // Game input first (E interact, 1-6 ingredient, ESC pause ...)
    if (gameKeyDown(key)) { glutPostRedisplay(); return; }

    // ── Camera mode cycle (Orbit → FPS → Focused → Upper → Pond → Orbit) ──
    if (key == 'c' || key == 'C') {
        if      (currentCamMode == CAM_ORBIT)   currentCamMode = CAM_FPS;
        else if (currentCamMode == CAM_FPS)     currentCamMode = CAM_FOCUSED;
        else if (currentCamMode == CAM_FOCUSED) currentCamMode = CAM_UPPER;
        else if (currentCamMode == CAM_UPPER)   currentCamMode = CAM_POND;
        else                                    currentCamMode = CAM_ORBIT;
        updateWindowTitle();
    }

    // ── - / = / \ : show or hide ONE lighting term on every light (ambient / diffuse / specular) ──
    if (key == '-')  { toggleAmbient();  updateWindowTitle(); }
    if (key == '=')  { toggleDiffuse();  updateWindowTitle(); }
    if (key == 92) { toggleSpecular(); updateWindowTitle(); }          // backslash

    // ── 0 = sun / moon (directional light) ──
    if (key == '0') { toggleDirectional(); updateWindowTitle(); }

    // ── Keys 1-7 switch each light fixture on / off on its own ──
    //   1 dining pendants, 2 kitchen spotlight, 3 ceiling area light, 4 paper box lanterns,
    //   5 hanging lanterns, 6 floor lantern tower, 7 second-floor paper dome
    if (key >= '1' && key <= '7') {
        int f = key - '1';
        if (!groupOn(LIGHT_STOPS[f].group)) { toggleGroup(LIGHT_STOPS[f].group); fixtureOn[f] = true; }   // group was off: turn it on
        else fixtureOn[f] = !fixtureOn[f];
        updateWindowTitle();
    }

    // ── Light tour: 8 = next light source (zoom + switch on), 9 = toggle it ──
    if (key == '8') nextLightStop();
    if (key == '9' && currentCamMode == CAM_LIGHT && lightStop >= 0) {
        if (!groupOn(LIGHT_STOPS[lightStop].group)) toggleGroup(LIGHT_STOPS[lightStop].group);
        fixtureOn[lightStop] = !fixtureOn[lightStop];                                // this fixture only
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

    // ── B: bowl close-up camera on / back to the previous camera (not during the cooking game) ──
    if ((key == 'b' || key == 'B') && !gameActive) {
        if (currentCamMode != CAM_BOWL) { camBeforeBowl = currentCamMode; currentCamMode = CAM_BOWL; }
        else                              currentCamMode = camBeforeBowl;
        updateWindowTitle();
    }

    // ── Effect toggles: Y soft shadow mapping, Z glass refraction ──
    if (key == 'y' || key == 'Y') { toggleShadowMapping();   updateWindowTitle(); }
    if (key == 'z' || key == 'Z') { toggleGlassRefraction(); updateWindowTitle(); }
    if (key == 'r' || key == 'R') { toggleRayTracing();      updateWindowTitle(); }   // CPU ray tracer

    if (key == 'g' || key == 'G') { usePhongShading = !usePhongShading; updateWindowTitle(); }
    if (key == 'p' || key == 'P') { cycleLightingPreset(); updateWindowTitle(); }
    if (key == 't' || key == 'T') { toggleDayNight(); updateWindowTitle(); }

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
        if (key == GLUT_KEY_LEFT)                  camAngle   -= 5;
        if (key == GLUT_KEY_RIGHT)                 camAngle   += 5;
        if (key == GLUT_KEY_UP && camDistance > 3)  camDistance -= 1;
        if (key == GLUT_KEY_DOWN)                  camDistance += 1;
        if (key == GLUT_KEY_PAGE_UP)   { camHeight += 0.5f; camLookAtY += 0.4f; }
        if (key == GLUT_KEY_PAGE_DOWN) { camHeight -= 0.5f; camLookAtY -= 0.4f; }
    } else if (currentCamMode == CAM_POND) {
        if (key == GLUT_KEY_LEFT)                       pondAngle    -= 5;
        if (key == GLUT_KEY_RIGHT)                      pondAngle    += 5;
        if (key == GLUT_KEY_UP && pondDistance > 3)     pondDistance -= 0.5f;
        if (key == GLUT_KEY_DOWN && pondDistance < 25)  pondDistance += 0.5f;
        if (key == GLUT_KEY_PAGE_UP)   pondHeight += 0.3f;
        if (key == GLUT_KEY_PAGE_DOWN && pondHeight > 0.6f) pondHeight -= 0.3f;
    } else if (currentCamMode == CAM_UPPER) {
        if (key == GLUT_KEY_LEFT)                       upperAngle    -= 5;
        if (key == GLUT_KEY_RIGHT)                      upperAngle    += 5;
        if (key == GLUT_KEY_UP && upperDistance > 2)     upperDistance  -= 0.5f;
        if (key == GLUT_KEY_DOWN)                       upperDistance  += 0.5f;
        if (key == GLUT_KEY_PAGE_UP)   { upperHeight += 0.3f; upperLookAtY += 0.2f; }
        if (key == GLUT_KEY_PAGE_DOWN) { upperHeight -= 0.3f; upperLookAtY -= 0.2f; }
    } else if (currentCamMode == CAM_FOCUSED) {
        if (key == GLUT_KEY_LEFT)                           counterAngle    -= 5;
        if (key == GLUT_KEY_RIGHT)                          counterAngle    += 5;
        if (key == GLUT_KEY_UP && counterDistance > 1.5f)    counterDistance -= 0.5f;
        if (key == GLUT_KEY_DOWN)                           counterDistance += 0.5f;
        if (key == GLUT_KEY_PAGE_UP)   { counterHeight += 0.3f; counterLookAtY += 0.2f; }
        if (key == GLUT_KEY_PAGE_DOWN) { counterHeight -= 0.3f; counterLookAtY -= 0.2f; }
    } else if (currentCamMode == CAM_BOWL) {
        if (key == GLUT_KEY_LEFT)                          bowlAngle    -= 5;
        if (key == GLUT_KEY_RIGHT)                         bowlAngle    += 5;
        if (key == GLUT_KEY_UP && bowlDistance > 0.45f)    bowlDistance -= 0.05f;
        if (key == GLUT_KEY_DOWN && bowlDistance < 2.5f)   bowlDistance += 0.05f;
        if (key == GLUT_KEY_PAGE_UP && bowlHeight < 1.5f)  bowlHeight   += 0.05f;
        if (key == GLUT_KEY_PAGE_DOWN && bowlHeight > 0.06f) bowlHeight -= 0.05f;
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

        // Intersect ray with upper-room door plane x = 3.50
        if (fabs(rdx) > 0.0001) {
            double t = (3.50 - nx) / rdx;
            if (t > 0) {
                double hitY = ny + t * rdy;
                double hitZ = nz + t * rdz;
                // Door centre z=-0.1, half-w=0.6 → z [-0.7, 0.5]
                // Door y: 3.42 to 5.32
                if (hitZ > -0.7 && hitZ < 0.5 && hitY > 3.42 && hitY < 5.32) {
                    upperDoorOpen = !upperDoorOpen;
                    // Enter / exit the upper room view
                    if (upperDoorOpen) {
                        currentCamMode = CAM_UPPER;
                    } else {
                        currentCamMode = CAM_ORBIT;
                    }
                    updateWindowTitle();
                }
            }
        }
    }
    if (button == GLUT_LEFT_BUTTON) {
        isMouseDragging = (state == GLUT_DOWN);
        lastMouseX = x; lastMouseY = y;
    }
    if (button == 3 && state == GLUT_DOWN) {   // wheel up — zoom in
        if (currentCamMode == CAM_ORBIT && camDistance > 3)
            camDistance -= 0.8f;
        else if (currentCamMode == CAM_UPPER && upperDistance > 2)
            upperDistance -= 0.5f;
        else if (currentCamMode == CAM_POND && pondDistance > 3)
            pondDistance -= 0.5f;
        else if (currentCamMode == CAM_BOWL && bowlDistance > 0.45f)
            bowlDistance -= 0.05f;
        else if (currentCamMode == CAM_FOCUSED && counterDistance > 1.5f)
            counterDistance -= 0.5f;
        else if (currentCamMode == CAM_FPS)
            fpsPos.y += 0.2f;
        glutPostRedisplay();
    }
    if (button == 4 && state == GLUT_DOWN) {   // wheel down — zoom out
        if (currentCamMode == CAM_ORBIT)
            camDistance += 0.8f;
        else if (currentCamMode == CAM_UPPER)
            upperDistance += 0.5f;
        else if (currentCamMode == CAM_POND && pondDistance < 25)
            pondDistance += 0.5f;
        else if (currentCamMode == CAM_BOWL && bowlDistance < 2.5f)
            bowlDistance += 0.05f;
        else if (currentCamMode == CAM_FOCUSED)
            counterDistance += 0.5f;
        else if (currentCamMode == CAM_FPS && fpsPos.y > 0.8f)
            fpsPos.y -= 0.2f;
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
    } else if (currentCamMode == CAM_POND) {
        pondAngle  += dx * 0.35f;
        pondHeight += dy * 0.04f;
        if (pondHeight < 0.6f) pondHeight = 0.6f;
    } else if (currentCamMode == CAM_UPPER) {
        upperAngle  += dx * 0.35f;
        upperHeight += dy * 0.04f;
        if (upperHeight < 3.8f) upperHeight = 3.8f;
    } else if (currentCamMode == CAM_BOWL) {
        bowlAngle  += dx * 0.35f;
        bowlHeight += dy * 0.005f;
        if (bowlHeight < 0.06f) bowlHeight = 0.06f;
        if (bowlHeight > 1.5f)  bowlHeight = 1.5f;
    } else if (currentCamMode == CAM_FOCUSED) {
        counterAngle  += dx * 0.35f;
        counterHeight += dy * 0.04f;
        if (counterHeight < 0.5f) counterHeight = 0.5f;
    }
    lastMouseX = x; lastMouseY = y;
    glutPostRedisplay();
}
