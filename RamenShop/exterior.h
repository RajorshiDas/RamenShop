#pragma once

#include "shapes.h"

void drawShopBuilding(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawRoof(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawWindow(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawGridWindow(Vec3 pos, Vec3 rot, Vec3 scale, float w, float h, int cols, int rows);
void drawStreet(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawSidewalk(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawLamp(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawPlant(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawFence(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawTree(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawCherryBlossomTree(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawVendingMachine(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawShojiDoor(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, float width = 1.6f, float height = 2.6f);
void drawShojiWindow(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, float width = 2.8f, float height = 2.6f);
void drawEntranceDoor(float angle);
void drawSlidingShoji(Vec3 pos, Vec3 rot, float offset, float totalW = 2.4f, float height = 2.8f);
