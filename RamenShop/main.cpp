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
#include "lighting.h"
#include "camera.h"
#include "texture.h"
#include "shader.h"
#include "raytracer.h"
#include "game.h"
#include <cstdio>

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Wider field of view in game mode so the kitchen fits on screen at arm's length
    {
        int vw = glutGet(GLUT_WINDOW_WIDTH), vh = glutGet(GLUT_WINDOW_HEIGHT);
        if (vh < 1) vh = 1;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(gameActive ? 62.0 : 45.0, (double)vw / vh, 0.3, 200.0);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set up camera transformation (Orbit or First-Person)
    applyCameraView();
    updateOutdoorView();                // camera position for the distance detail of trees and bushes

    // Dynamically apply all light components & flicker parameters
    applyLightingParameters();

    // If Ray Tracing mode is active, render via real-time Ray Tracing engine
    if (useRayTracing) {
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        renderRayTracedFrame(w, h);
        drawGameHUD(w, h);
        glutSwapBuffers();
        return;
    }

    // Place all lights in world space (positions defined in lighting.cpp)
    placeLightsInWorldSpace();

    drawSky();

    // Enable Phong shader for lit scene geometry (G toggles Gouraud ↔ Phong)
    if (usePhongShading) {
        enablePhongShader();
        updatePhongUniforms();
    }

    resetMaterialGloss();               // soft default sheen on everything that sets no material
    drawGround();
    drawExterior();
    drawInterior();

    if (usePhongShading) {
        disablePhongShader();
    }

    // 2D game overlay (order board, prompts, results) - drawn last
    drawGameHUD(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

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

    // Game logic (state machine, cooking timers, customer) - uses real elapsed time
    updateGame();

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

    // Lighting setup — all light configuration in lighting.cpp
    initLighting();

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
    initGame();
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
