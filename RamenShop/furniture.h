#pragma once

#include "shapes.h"

void drawTable(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawChair(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawStool(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawBench(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawCounter(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawShelf(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawCabinet(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawSink(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawCookingPot(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawKitchen(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawCashRegister(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawLadle(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawSpatula(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawStrainer(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawTongs(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawWok(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawKitchenHood(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);

// ── Modular kitchen equipment (each independently repositionable) ───────────
void drawStoveTop(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawLargeRamenPot(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawIngredientContainer(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, Color fill = { 0.40f, 0.80f, 0.30f });
void drawStackedBowls(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE, int count = 5);
void drawRefrigerator(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawDishRack(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawKitchenKnife(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawTicketRail(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawGasBurner(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawNoodleStation(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
void drawServingCounter(Vec3 pos, Vec3 rot = NO_ROT, Vec3 scale = ONE);
