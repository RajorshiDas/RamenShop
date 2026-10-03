/*
 * ============================================================================
 *  3D RAMEN SHOP  -  OpenGL / freeglut  (C++17)
 *
 *  Modules:
 *      shapes.h/.cpp       - Vectors, colors, quadric, transforms, primitives, steam
 *      texture.h/.cpp      - Procedural texture generation (wood, tile, wall, asphalt, concrete)
 *      objects.h/.cpp      - Scene-object selection and delta-transform system
 *      furniture.h/.cpp    - Stools, counter, chairs, tables, shelves, kitchen equipment
 *      food.h/.cpp         - Ramen bowls, chashu, eggs, nori, green onions, chopsticks
 *      decorations.h/.cpp  - Paper lanterns, noren curtains, menu boards, wall art
 *      exterior.h/.cpp     - Shop structure, roof, windows, street, sidewalk, trees
 *      scene.h/.cpp        - High-level drawGround(), drawExterior(), drawInterior(), light toggles
 *      camera.h/.cpp       - Orbit, FPS Walkthrough, and Focused counter cameras
 *      main.cpp            - Application entry point, window, GL init, FreeGLUT loop
 *
 *  Camera Controls:
 *      C                   : cycle camera mode (Orbit → Walkthrough → Focused counter → Orbit)
 *      Left Mouse Drag     : orbit/tilt (Orbit) or look around (Walkthrough)
 *      Mouse Scroll Wheel  : zoom (Orbit) or height adjust (Walkthrough)
 *      LEFT / RIGHT arrows : orbit horizontally (Orbit mode)
 *      UP / DOWN arrows    : zoom in / out (Orbit mode)
 *      PAGE UP / PAGE DOWN : camera higher / lower (Orbit mode)
 *      W / A / S / D       : walk forward / strafe left / backward / strafe right (Walkthrough)
 *      Q / E               : fly down / up (Walkthrough)
 *
 *  Scene Controls:
 *      R                   : toggle Real-Time Ray Tracing ON / OFF
 *      B                   : cycle Ray Tracing bounce depth (1 to 5 bounces)
 *      Y                   : toggle Ray Traced soft shadows on / off
 *      G                   : toggle Phong / Gouraud shading
 *      T                   : toggle Day / Night (sun ↔ moon)
 *      H                   : hide / show the roof
 *      O                   : dark edge outlines on / off
 *      F                   : atmospheric fog on / off
 *      X                   : rising steam particles on / off
 *      SPACE               : pause / resume animation
 *      ESC                 : quit
 *
 *  Lighting Toggles:
 *      1                   : toggle Ambient Light (global + per-light ambient)
 *      2                   : toggle Diffuse Reflection (Lambertian cosine shading)
 *      3                   : toggle Specular Highlights (Phong gloss on glass, metals, ceramics)
 *      4                   : toggle Sky Light (Moon / Sun directional)
 *      5                   : toggle Lamp Lights (dining pendant + exterior chochin lanterns)
 *      6                   : toggle Kitchen Light (chef counter spotlight + volumetric beam)
 *      7                   : toggle Decor Lights (street lamps + hanging lanterns + 2nd floor)
 *      P                   : cycle Lighting Presets (Full Realism, Spotlight Focus, Decor Only,
 *                            Cozy Night, Specular Only, Diffuse Only, Ambient Only)
 *
 *  Object Selection & Manipulation (TAB to cycle, then use keys below):
 *      TAB                 : cycle selected object (Bowl → Kettle → Lantern L → Lantern R → Noren → Stool → none)
 *      I / K               : move selected object +Z / -Z
 *      J / L               : move selected object -X / +X
 *      U / N               : move selected object +Y / -Y
 *      [ / ]               : rotate selected object -Y / +Y (yaw)
 *      ; / '               : rotate selected object -X / +X (pitch)
 *      , / .               : scale selected object down / up
 * ============================================================================
 */

#include "shapes.h"
#include "scene.h"
#include "camera.h"
#include "texture.h"
#include "shader.h"
#include "raytracer.h"
#include <cstdio>

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set up camera transformation (Orbit or First-Person)
    applyCameraView();

    // Dynamically apply all light components & flicker parameters
    applyLightingParameters();

    // If Ray Tracing mode is active, render via real-time Ray Tracing engine
    if (useRayTracing) {
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        renderRayTracedFrame(w, h);
        glutSwapBuffers();
        return;
    }

    // ── Place lights in world space (after camera transform) ─────────────
    // Each GL light corresponds to a visible object in the scene.
    //
    // LIGHT0  Directional  Moon / Sun                 (outdoor sky)
    // LIGHT1  Point        Pendant lamps + box cluster (dining center)
    // LIGHT2  Point        Left exterior chochin       (entrance)
    // LIGHT3  Point        Right exterior chochin      (entrance)
    // LIGHT4  Spot         Kitchen spotlight fixture    (chef counter)
    // LIGHT5  Point        Street lamps                (sidewalk)
    // LIGHT6  Point        Hanging red lanterns         (dining atmosphere)
    // LIGHT7  Point        Second-floor ceiling dome    (upstairs room)

    // 1. Directional sky light (w = 0)
    GLfloat pos0[] = { 0.5f, 0.35f, -0.7f, 0.0f };

    // 2. Interior / Exterior point lights (w = 1)
    GLfloat pos1[] = {  0.3f, 2.80f,  0.0f, 1.0f };     // dining pendant center
    GLfloat pos2[] = { -3.8f, 2.60f,  4.5f, 1.0f };     // left chochin
    GLfloat pos3[] = {  3.8f, 2.60f,  4.5f, 1.0f };     // right chochin

    // 3. Kitchen spot (pointing straight down)
    GLfloat pos4[] = { -1.5f, 3.25f, -0.3f, 1.0f };
    GLfloat dir4[] = {  0.0f, -1.0f,  0.0f };

    // 4. Street / Atmosphere / Second-floor point lights
    GLfloat pos5[] = {  0.0f, 3.40f,  7.0f, 1.0f };     // street lamp (avg of both)
    GLfloat pos6[] = {  0.0f, 2.90f,  0.3f, 1.0f };     // hanging lanterns center
    GLfloat pos7[] = {  0.0f, 5.15f,  0.0f, 1.0f };     // 2nd floor ceiling dome

    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
    glLightfv(GL_LIGHT2, GL_POSITION, pos2);
    glLightfv(GL_LIGHT3, GL_POSITION, pos3);

    glLightfv(GL_LIGHT4, GL_POSITION, pos4);
    glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, dir4);

    glLightfv(GL_LIGHT5, GL_POSITION, pos5);
    glLightfv(GL_LIGHT6, GL_POSITION, pos6);
    glLightfv(GL_LIGHT7, GL_POSITION, pos7);

    drawSky();

    // Enable Phong shader for lit scene geometry
    enablePhongShader();
    updatePhongUniforms();

    drawGround();
    drawExterior();
    drawInterior();

    disablePhongShader();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 0.5, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

void updateTimer(int value)
{
    if (!animPaused) {
        animTime += 0.016f;
    }

    // Smoothly animate entrance door open/close
    float targetAngle = doorOpen ? 90.0f : 0.0f;
    if (doorAngle < targetAngle) {
        doorAngle += 2.5f;
        if (doorAngle > targetAngle) doorAngle = targetAngle;
    } else if (doorAngle > targetAngle) {
        doorAngle -= 2.5f;
        if (doorAngle < targetAngle) doorAngle = targetAngle;
    }

    // Smoothly animate sliding shoji door open/close
    float targetSlide = slideDoorOpen ? 1.0f : 0.0f;
    if (slideDoorOffset < targetSlide) {
        slideDoorOffset += 0.025f;
        if (slideDoorOffset > targetSlide) slideDoorOffset = targetSlide;
    } else if (slideDoorOffset > targetSlide) {
        slideDoorOffset -= 0.025f;
        if (slideDoorOffset < targetSlide) slideDoorOffset = targetSlide;
    }

    // Smoothly animate upper room sliding door open/close
    float targetUpper = upperDoorOpen ? 1.0f : 0.0f;
    if (upperDoorOffset < targetUpper) {
        upperDoorOffset += 0.025f;
        if (upperDoorOffset > targetUpper) upperDoorOffset = targetUpper;
    } else if (upperDoorOffset > targetUpper) {
        upperDoorOffset -= 0.025f;
        if (upperDoorOffset < targetUpper) upperDoorOffset = targetUpper;
    }

    // Process continuous WASD movement when in FPS mode
    updateCameraMovement();

    glutPostRedisplay();
    glutTimerFunc(16, updateTimer, 0);
}

void init()
{
    // Initialise GLEW (must come after GL context creation)
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        fprintf(stderr, "GLEW init failed: %s\n", glewGetErrorString(glewErr));
    }

    glClearColor(SKY.r, SKY.g, SKY.b, 1.0f);
    glEnable(GL_DEPTH_TEST);
    initTextures();
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glLineWidth(1.5f);
    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);

    // Atmospheric Night Fog
    GLfloat fogColor[] = { SKY.r, SKY.g, SKY.b, 1.0f };
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogf(GL_FOG_DENSITY, 0.022f);
    glHint(GL_FOG_HINT, GL_NICEST);
    if (showFog) glEnable(GL_FOG);

    // Lighting setup — each GL light matches a visible object
    setLighting(true);
    glEnable(GL_LIGHT0); // Directional moon / sun
    glEnable(GL_LIGHT1); // Interior dining pendant + box lantern cluster
    glEnable(GL_LIGHT2); // Left exterior chochin lantern
    glEnable(GL_LIGHT3); // Right exterior chochin lantern
    glEnable(GL_LIGHT4); // Kitchen spotlight fixture
    glEnable(GL_LIGHT5); // Street lamps (outdoor)
    glEnable(GL_LIGHT6); // Hanging red lanterns (dining atmosphere)
    glEnable(GL_LIGHT7); // Second-floor ceiling dome light

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // Local eye viewer for realistic specular highlights
    glShadeModel(GL_SMOOTH);

    // ── Attenuation: atten = 1 / (Kc + Kl*d + Kq*d²) ────────────────────
    // Values chosen per light based on room size (~10m across) and
    // the realistic range each fixture would illuminate.

    // LIGHT1: Interior dining pendant — warm ~5m range, primary indoor source
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.09f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.032f);

    // LIGHT2 & LIGHT3: Exterior chochin lanterns — small ~3m range, strong falloff
    for (int i = 2; i <= 3; i++) {
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION,    0.14f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.07f);
    }

    // LIGHT4: Kitchen spotlight — focused ~3m, narrow beam pointing down
    glLightf(GL_LIGHT4, GL_SPOT_CUTOFF,    30.0f);
    glLightf(GL_LIGHT4, GL_SPOT_EXPONENT,  25.0f);
    glLightf(GL_LIGHT4, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT4, GL_LINEAR_ATTENUATION,    0.05f);
    glLightf(GL_LIGHT4, GL_QUADRATIC_ATTENUATION, 0.015f);

    // LIGHT5: Street lamps — outdoor ~6m range
    glLightf(GL_LIGHT5, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT5, GL_LINEAR_ATTENUATION,    0.07f);
    glLightf(GL_LIGHT5, GL_QUADRATIC_ATTENUATION, 0.017f);

    // LIGHT6: Hanging lanterns — soft atmospheric ~3m range
    glLightf(GL_LIGHT6, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT6, GL_LINEAR_ATTENUATION,    0.12f);
    glLightf(GL_LIGHT6, GL_QUADRATIC_ATTENUATION, 0.05f);

    // LIGHT7: Second-floor ceiling dome — gentle ~3m range
    glLightf(GL_LIGHT7, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT7, GL_LINEAR_ATTENUATION,    0.14f);
    glLightf(GL_LIGHT7, GL_QUADRATIC_ATTENUATION, 0.07f);

    applyLightingParameters();

    // Compile Phong GLSL shader (press G to toggle)
    initPhongShader();

    // Initialise Real-Time GPU Ray Tracer (press R to toggle)
    initRayTracer();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    glutInitWindowSize(1200, 800);
    glutCreateWindow("3D Ramen Shop");

    init();
    updateWindowTitle();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(handleKeyboardDown);
    glutKeyboardUpFunc(handleKeyboardUp);
    glutSpecialFunc(handleSpecialDown);
    glutSpecialUpFunc(handleSpecialUp);
    glutMouseFunc(handleMouseClick);
    glutMotionFunc(handleMouseMotion);
    glutTimerFunc(16, updateTimer, 0);

    glutMainLoop();
    return 0;
}
