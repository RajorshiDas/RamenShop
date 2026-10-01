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
 *      1                   : global ambient light on / off
 *      2                   : directional moonlight (LIGHT0) on / off
 *      3                   : point lights (interior + lanterns) on / off
 *      4                   : specular highlights on / off
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

    // Place lights in world space (after camera transform)
    // 1. Directional lights (w = 0.0f)
    // Direction matches the fixed sun/moon position in drawSky()
    GLfloat pos0[] = {  0.5f, 0.35f, -0.7f, 0.0f };     // directional sun/moonlight
    GLfloat pos1[] = { -0.2f, 0.20f,  0.3f, 0.0f };     // fill from opposite side

    // 2. Point lights (w = 1.0f)
    GLfloat pos2[] = {  0.0f, 2.75f, -0.2f, 1.0f };     // warm interior pendant light
    GLfloat pos3[] = { -3.8f, 2.60f,  4.5f, 1.0f };     // left exterior lantern
    GLfloat pos4[] = {  3.8f, 2.60f,  4.5f, 1.0f };     // right exterior lantern

    // 3. Focused Spot Light (pointing straight down onto prep station)
    GLfloat pos5[] = { -1.5f, 3.25f, -0.3f, 1.0f };
    GLfloat dir5[] = {  0.0f, -1.0f,  0.0f };

    // 4. Rectangular Area Light (dual distributed emitters along ceiling fixture)
    GLfloat pos6[] = { -1.4f, 3.22f, -1.2f, 1.0f };
    GLfloat pos7[] = {  1.4f, 3.22f, -1.2f, 1.0f };
    GLfloat dirArea[] = { 0.0f, -1.0f, 0.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, pos0);
    glLightfv(GL_LIGHT1, GL_POSITION, pos1);
    glLightfv(GL_LIGHT2, GL_POSITION, pos2);
    glLightfv(GL_LIGHT3, GL_POSITION, pos3);
    glLightfv(GL_LIGHT4, GL_POSITION, pos4);

    glLightfv(GL_LIGHT5, GL_POSITION, pos5);
    glLightfv(GL_LIGHT5, GL_SPOT_DIRECTION, dir5);

    glLightfv(GL_LIGHT6, GL_POSITION, pos6);
    glLightfv(GL_LIGHT6, GL_SPOT_DIRECTION, dirArea);
    glLightfv(GL_LIGHT7, GL_POSITION, pos7);
    glLightfv(GL_LIGHT7, GL_SPOT_DIRECTION, dirArea);

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

    // Lighting setup
    setLighting(true);
    glEnable(GL_LIGHT0); // Directional moonlight
    glEnable(GL_LIGHT1); // Directional fill
    glEnable(GL_LIGHT2); // Warm interior point light
    glEnable(GL_LIGHT3); // Left lantern point light
    glEnable(GL_LIGHT4); // Right lantern point light
    glEnable(GL_LIGHT5); // Focused spot light
    glEnable(GL_LIGHT6); // Area light panel emitter 1
    glEnable(GL_LIGHT7); // Area light panel emitter 2

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // Local eye viewer for realistic specular highlights
    glShadeModel(GL_SMOOTH);

    // Initial Light Attenuation & Spotlight/Area Light setup
    // Uses inverse-square falloff: attenuation = 1 / (Kc + Kl*d + Kq*d^2)
    // With realistic values for physically-based rendering

    // LIGHT2: Warm central point light (pendant ~4m range)
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION,    0.035f);  // decreased for better reach
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.008f);  // realistic quadratic rolloff

    // LIGHT3 & LIGHT4: Exterior lanterns (smaller range, ~5m)
    for (int i = 3; i <= 4; i++) {
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION,    0.08f);   // more attenuation than pendant
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.018f);  // stronger quadratic curve
    }

    // LIGHT5: Focused Spot Light (Chef counter downlight, ~3m sharp falloff)
    glLightf(GL_LIGHT5, GL_SPOT_CUTOFF, 28.0f);          // narrower beam
    glLightf(GL_LIGHT5, GL_SPOT_EXPONENT, 32.0f);        // sharper edge definition
    glLightf(GL_LIGHT5, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf(GL_LIGHT5, GL_LINEAR_ATTENUATION,    0.04f);
    glLightf(GL_LIGHT5, GL_QUADRATIC_ATTENUATION, 0.012f);

    // LIGHT6 & LIGHT7: Overhead Area Light Panel (large soft falloff, ~6m range)
    for (int i = 6; i <= 7; i++) {
        glLightf(GL_LIGHT0 + i, GL_SPOT_CUTOFF, 85.0f);        // very wide soft spread
        glLightf(GL_LIGHT0 + i, GL_SPOT_EXPONENT, 1.5f);       // very soft edge
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION,  1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION,    0.025f);  // gentler falloff
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.004f);  // smooth quadratic
    }

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
