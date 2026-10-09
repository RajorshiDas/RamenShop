#pragma once

#include "shapes.h"

enum CameraMode { CAM_ORBIT, CAM_FPS, CAM_FOCUSED, CAM_UPPER, CAM_LIGHT, CAM_POND };
extern CameraMode currentCamMode;

// Orbit Camera parameters (exterior overview)
extern float camAngle;
extern float camDistance;
extern float camHeight;
extern float camLookAtY;

// FPS Walkthrough Camera parameters
extern Vec3  fpsPos;
extern float fpsYaw;
extern float fpsPitch;

// Upper Room orbit parameters
extern float upperAngle;
extern float upperDistance;
extern float upperHeight;
extern float upperLookAtY;

// Counter View orbit parameters
extern float counterAngle;
extern float counterDistance;
extern float counterHeight;
extern float counterLookAtY;

// Key state arrays for smooth continuous motion
extern bool keyStates[256];
extern bool specialKeyStates[256];

// Camera functions
void updateWindowTitle();
void applyCameraView();
void updateCameraMovement();

// Input handler functions
void handleKeyboardDown(unsigned char key, int x, int y);
void handleKeyboardUp(unsigned char key, int x, int y);
void handleSpecialDown(int key, int x, int y);
void handleSpecialUp(int key, int x, int y);
void handleMouseClick(int button, int state, int x, int y);
void handleMouseMotion(int x, int y);

// Light tour: key 8 flies the camera to the next light source and switches it on; key 9 toggles it
const char* getLightTourName();
