#pragma once

#include "shapes.h"

// ═══════════════════════════════════════════════════════════════════════════
//  RAMEN COOKING GAME
//
//  A thin gameplay layer on top of the 3D shop.  It owns no rendering code of
//  the shop itself: it only adds
//     - a state machine (menu -> order -> cooking -> serving -> result)
//     - interactable stations (prep area, noodle pot, broth pot, serve counter)
//     - animated 3D objects driven by transforms (customer, held item,
//       noodles in the pot, flying ingredients)
//     - a 2D HUD (order board, prompts, timers, result card)
//
//  The game is OPT-IN.  The program starts in explore mode (orbit camera, the
//  whole shop as before) and shows "Press ENTER to play".  M or ESC on the title
//  screen leaves the game again.
//
//  Controls   W A S D / mouse drag : move / look      C : change camera
//             E : interact           1-6 : choose ingredient at the prep area
//             Q : discard held item  ESC : pause      M : back to exploring
// ═══════════════════════════════════════════════════════════════════════════

enum GameState {
    GS_MENU,
    GS_TAKING_ORDER,   // customer walks in and sits down, order appears
    GS_COOKING,        // player prepares the order
    GS_SERVING,        // player is carrying a finished bowl
    GS_RESULT,         // customer evaluates the ramen
    GS_PAUSED
};

extern bool gameActive;          // false = explore mode (default), true = game mode

void initGame();                 // call once after GL init
void updateGame();               // call every frame (uses real elapsed time)
bool gameKeyDown(unsigned char key);   // true = key was consumed by the game
void gameClampPlayer(Vec3& p);   // keep the cook inside the kitchen aisle

void drawGameWorld();            // 3D: customer, noodles in pot, held item, effects
void drawGameHUD(int w, int h);  // 2D overlay (call last, after the scene)

// Steam multiplier for a pot (0 = noodle pot, 1 = broth pot); 1.0 when the game is off
float gameSteamScale(int pot);
