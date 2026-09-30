#pragma once

#include "shapes.h"

enum CameraMode { CAM_ORBIT, CAM_FPS, CAM_FOCUSED };
extern CameraMode currentCamMode;

// Orbit Camera parameters
extern float camAngle;
extern float camDistance;
extern float camHeight;

// FPS Walkthrough Camera parameters
extern Vec3  fpsPos;
extern float fpsYaw;
extern float fpsPitch;

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
