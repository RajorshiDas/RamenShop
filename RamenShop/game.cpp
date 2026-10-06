#include "game.h"
#include "scene.h"
#include "camera.h"
#include "food.h"
#include "shader.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

bool gameActive = false;   // off at launch: the shop is shown exactly as before; ENTER starts the game

// ═══════════════════════════════════════════════════════════════════════════
//  World layout (matches drawKitchen / drawCounter in furniture.cpp, scene.cpp)
// ═══════════════════════════════════════════════════════════════════════════
static const float COUNTER_TOP = FLOOR_Y + 1.06f;          // dining counter surface
static const float POT_BASE_Y  = FLOOR_Y + 0.94f + 0.07f;  // pots sit on the stove grate
static const float POT_TOP_Y   = POT_BASE_Y + 0.58f;       // just above the pot rim
static const float KITCHEN_Z   = -3.2f;
static const float NOODLE_POT_X = -0.55f;
static const float BROTH_POT_X  =  0.55f;
static const float PREP_X       = -2.2f;
static const float SERVE_Z      = -0.6f;                   // dining counter (z of aisle side)

// ═══════════════════════════════════════════════════════════════════════════
//  Game data
// ═══════════════════════════════════════════════════════════════════════════
enum Held { H_NONE, H_RAW, H_BOWL };
enum Topping { T_PORK, T_EGG, T_ONION, T_NORI, T_COUNT };
static const char* TOPPING_NAME[T_COUNT] = { "Pork", "Egg", "Green Onion", "Seaweed" };

struct BowlState {
    bool  broth   = false;
    bool  noodles = false;
    float cook    = 0.0f;                 // seconds the noodles spent in the pot
    bool  top[T_COUNT] = { false, false, false, false };
};

struct Order {
    bool  top[T_COUNT] = { false, false, false, false };
    float patience = 90.0f;               // seconds before the customer leaves
    float elapsed  = 0.0f;
    int   number   = 0;
};

struct Result {
    float accuracy = 0, cookQ = 0, timeQ = 0, composite = 0;
    int   stars = 0, yen = 0;
    const char* cookName = "-";
    float secs = 0;
    bool  failed = false;
};

static GameState gs = GS_MENU;
static GameState gsBeforePause = GS_COOKING;
static float stateT = 0.0f;
static float clk    = 0.0f;               // game clock (animations)
static int   lastMs = 0;

static Held      held = H_NONE;
static BowlState bowl;
static bool  potOn = false;               // noodles are boiling in the pot
static float potT  = 0.0f;                // how long (s)
static int   selIng = 1;                  // 1..6 chosen at the prep area

static int   level = 1, served = 0, yen = 0;
static Order  order;
static Result res;
static int   orderCounter = 0;

// Customer
static bool  custPresent = false, custLeaving = false;
static float custD = 0.0f;                // distance walked along the entry path
static int   seatIdx = 2, outfit = 0;
static bool  bowlOnCounter = false;
static BowlState servedBowl;

// Toast message
static char  toastMsg[96] = "";
static float toastT = 0.0f;

static const float WALK_SPEED = 1.7f;

// ═══════════════════════════════════════════════════════════════════════════
//  Small helpers
// ═══════════════════════════════════════════════════════════════════════════
static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static float smooth(float t)  { t = clamp01(t); return t * t * (3.0f - 2.0f * t); }
static float lerpf(float a, float b, float t) { return a + (b - a) * t; }
static Color lerpC(Color a, Color b, float t) {
    return { lerpf(a.r, b.r, t), lerpf(a.g, b.g, t), lerpf(a.b, b.b, t) };
}
static void toast(const char* msg, float secs = 2.0f) {
    snprintf(toastMsg, sizeof(toastMsg), "%s", msg);
    toastT = secs;
}

// 0 RAW, 1 UNDERCOOKED, 2 PERFECT, 3 OVERCOOKED, 4 RUINED
static int cookClass(float t) {
    if (t < 3.0f)  return 0;
    if (t < 6.0f)  return 1;
    if (t < 9.0f)  return 2;
    if (t < 12.0f) return 3;
    return 4;
}
static const char* COOK_NAME[5]  = { "Raw", "Undercooked", "Perfect", "Overcooked", "Ruined" };
static const float COOK_SCORE[5] = { 0.15f, 0.60f, 1.00f, 0.60f, 0.10f };

static Color noodleColor(float t)
{
    const Color RAW  = { 0.92f, 0.92f, 0.86f };
    const Color SOFT = { 0.98f, 0.92f, 0.66f };
    const Color PERF = { 1.00f, 0.88f, 0.45f };
    const Color OVER = { 0.80f, 0.60f, 0.28f };
    const Color RUIN = { 0.42f, 0.28f, 0.15f };
    if (t < 3.0f)  return lerpC(RAW,  SOFT, smooth(t / 3.0f));
    if (t < 6.0f)  return lerpC(SOFT, PERF, smooth((t - 3.0f) / 3.0f));
    if (t < 9.0f)  return PERF;
    if (t < 12.0f) return lerpC(PERF, OVER, smooth((t - 9.0f) / 3.0f));
    return lerpC(OVER, RUIN, smooth((t - 12.0f) / 3.0f));
}

static float stoolX(int i) { return -2.8f + i * 1.12f; }

static void gameReset()
{
    held = H_NONE; bowl = BowlState(); potOn = false; potT = 0; selIng = 1;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Flying-ingredient effects (translate along an arc + spin)
// ═══════════════════════════════════════════════════════════════════════════
struct Fx { bool on; int kind; Vec3 a, b; float t, dur, arc; int sub; };
static Fx fx[8];

static void spawnFx(int kind, Vec3 a, Vec3 b, float arc, float dur, int sub = 0)
{
    for (auto& f : fx) if (!f.on) { f = { true, kind, a, b, 0.0f, dur, arc, sub }; return; }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Customer path:  door -> aisle -> stool.  Everything walks toward -Z.
// ═══════════════════════════════════════════════════════════════════════════
struct PathInfo { float x[5], z[5], len[4], total; };

static PathInfo buildPath()
{
    PathInfo p;
    float sx = stoolX(seatIdx);
    p.x[0] = 0;  p.z[0] = 4.9f;      // outside the door
    p.x[1] = 0;  p.z[1] = 2.8f;      // just inside
    p.x[2] = sx; p.z[2] = 2.2f;      // behind the stools
    p.x[3] = sx; p.z[3] = 1.45f;     // behind chosen stool
    p.x[4] = sx; p.z[4] = 0.80f;     // seated
    p.total = 0;
    for (int i = 0; i < 4; i++) {
        p.len[i] = hypotf(p.x[i + 1] - p.x[i], p.z[i + 1] - p.z[i]);
        p.total += p.len[i];
    }
    return p;
}

// Returns position/yaw/sit amount for a distance d walked along the path
static void samplePath(const PathInfo& p, float d, float& x, float& z, float& yaw, float& sit)
{
    d = std::max(0.0f, std::min(d, p.total));
    float acc = 0;
    int seg = 3;
    for (int i = 0; i < 4; i++) {
        if (d <= acc + p.len[i] || i == 3) { seg = i; break; }
        acc += p.len[i];
    }
    float t = (p.len[seg] > 0) ? clamp01((d - acc) / p.len[seg]) : 1.0f;
    x = lerpf(p.x[seg], p.x[seg + 1], t);
    z = lerpf(p.z[seg], p.z[seg + 1], t);
    float dx = p.x[seg + 1] - p.x[seg], dz = p.z[seg + 1] - p.z[seg];
    yaw = atan2f(dx, dz) * 180.0f / PI;
    sit = (seg == 3) ? smooth(t) : 0.0f;
    if (seg == 3) yaw = 180.0f;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Orders and scoring
// ═══════════════════════════════════════════════════════════════════════════
static void newOrder()
{
    order = Order();
    order.number   = ++orderCounter;
    order.patience = std::max(50.0f, 105.0f - 14.0f * (level - 1));
    order.top[T_ONION] = true;                                  // level 1: green onion only
    if (level >= 2) {
        order.top[T_PORK] = (rand() % 100) < 75;
        order.top[T_EGG]  = (rand() % 100) < 60;
        order.top[T_ONION] = (rand() % 100) < 80;
        if (!order.top[T_PORK] && !order.top[T_EGG] && !order.top[T_ONION]) order.top[T_PORK] = true;
    }
    if (level >= 3) order.top[T_NORI] = (rand() % 100) < 60;
}

static void spawnCustomer()
{
    newOrder();
    seatIdx = 1 + rand() % 4;                 // stools 1-4 (in view of the cook)
    outfit  = (outfit + 1) % 4;
    custPresent = true; custLeaving = false; custD = 0;
    bowlOnCounter = false;
    gameReset();
    gs = GS_TAKING_ORDER; stateT = 0;
    toast("A customer is coming in...", 2.5f);
}

static void startGame()
{
    level = 1; served = 0; yen = 0; orderCounter = 0;
    spawnCustomer();
}

static void finishOrder(bool failed)
{
    res = Result();
    res.failed = failed;
    res.secs   = order.elapsed;

    if (!failed) {
        int required = 2, matched = 0, extras = 0;
        if (bowl.noodles) matched++;
        if (bowl.broth)   matched++;
        for (int t = 0; t < T_COUNT; t++) {
            if (order.top[t]) { required++; if (bowl.top[t]) matched++; }
            else if (bowl.top[t]) extras++;
        }
        res.accuracy = matched / (float)(required + extras);
        int cc = bowl.noodles ? cookClass(bowl.cook) : 0;
        res.cookName = bowl.noodles ? COOK_NAME[cc] : "No noodles";
        res.cookQ    = bowl.noodles ? COOK_SCORE[cc] : 0.0f;
        res.timeQ    = clamp01(1.0f - order.elapsed / order.patience);
        res.composite = 0.5f * res.accuracy + 0.35f * res.cookQ + 0.15f * res.timeQ;
        res.stars = std::max(1, std::min(5, (int)floorf(res.composite * 5.0f + 0.5f)));
        float base = res.composite * 100.0f * (1.0f + 0.1f * (level - 1)) + res.stars * 5.0f;
        res.yen = ((int)(base / 10.0f)) * 10 + (res.stars == 5 ? 20 : 0);

        servedBowl = bowl; bowlOnCounter = true;
        yen += res.yen; served++;
        int newLevel = std::min(4, 1 + served / 2);
        if (newLevel > level) { level = newLevel; toast("LEVEL UP!  New ingredients and orders.", 3.0f); }
    } else {
        res.cookName = "-";
        toast("The customer got tired of waiting and left!", 3.0f);
    }

    gameReset();
    gs = GS_RESULT; stateT = 0;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Player / stations / interaction
// ═══════════════════════════════════════════════════════════════════════════
enum Station { ST_NONE, ST_PREP, ST_NOODLE_POT, ST_BROTH_POT, ST_SERVE };

void gameClampPlayer(Vec3& p)
{
    if (!gameActive) return;
    p.x = std::max(-3.9f, std::min(3.3f, p.x));
    p.z = std::max(-2.30f, std::min(-1.05f, p.z));
}

static Station nearestStation()
{
    float px = fpsPos.x, pz = fpsPos.z;
    float yaw = fpsYaw * PI / 180.0f;
    float fx = sinf(yaw), fz = cosf(yaw);          // horizontal facing direction
    float best = 1e9f; Station bs = ST_NONE;
    // A station is usable when it is within reach AND roughly in front of the player,
    // so the kitchen (behind) and the counter (in front, after turning) never compete.
    auto consider = [&](Station s, float sx, float sz, float reach) {
        float dx = sx - px, dz = sz - pz, d = hypotf(dx, dz);
        if (d >= reach) return;
        float facing = (d > 0.001f) ? (fx * dx + fz * dz) / d : 1.0f;
        if (facing < 0.35f) return;
        if (d < best) { best = d; bs = s; }
    };
    consider(ST_PREP,       PREP_X,       KITCHEN_Z, 2.7f);
    consider(ST_NOODLE_POT, NOODLE_POT_X, KITCHEN_Z, 2.5f);
    consider(ST_BROTH_POT,  BROTH_POT_X,  KITCHEN_Z, 2.5f);
    consider(ST_SERVE,      px,           SERVE_Z,   1.5f);    // anywhere along the counter
    return bs;
}

// Hand position in front of the camera (also used for the held item)
static Vec3 handPos()
{
    float yaw = fpsYaw * PI / 180.0f, pit = fpsPitch * PI / 180.0f;
    float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pit), sp = sinf(pit);
    Vec3 F = { cp * sy, sp, cp * cy };
    Vec3 R = { -cy, 0.0f, sy };
    Vec3 U = { R.y * F.z - R.z * F.y, R.z * F.x - R.x * F.z, R.x * F.y - R.y * F.x };
    bool moving = keyStates['w'] || keyStates['W'] || keyStates['a'] || keyStates['A'] ||
                  keyStates['s'] || keyStates['S'] || keyStates['d'] || keyStates['D'];
    float bob = moving ? sinf(clk * 9.0f) * 0.012f : sinf(clk * 1.6f) * 0.004f;
    return { fpsPos.x + F.x * 0.80f + R.x * 0.24f + U.x * (-0.30f + bob),
             fpsPos.y + F.y * 0.80f + R.y * 0.24f + U.y * (-0.30f + bob),
             fpsPos.z + F.z * 0.80f + R.z * 0.24f + U.z * (-0.30f + bob) };
}

struct Act { Station st; char text[112]; bool ok; };

static void setAct(Act& a, bool ok, const char* fmt, const char* arg = nullptr)
{
    a.ok = ok;
    if (arg) snprintf(a.text, sizeof(a.text), fmt, arg); else snprintf(a.text, sizeof(a.text), "%s", fmt);
}

// Builds the context prompt for the nearest station; performs the action if exec.
static Act queryAct(bool exec)
{
    Act a; a.st = nearestStation(); a.ok = false; a.text[0] = 0;
    bool active = (gs == GS_TAKING_ORDER || gs == GS_COOKING || gs == GS_SERVING);
    if (!active || a.st == ST_NONE) return a;

    Vec3 hand = handPos();
    Vec3 nPot = { NOODLE_POT_X, POT_TOP_Y, KITCHEN_Z };
    Vec3 bPot = { BROTH_POT_X,  POT_TOP_Y, KITCHEN_Z };

    switch (a.st) {
    case ST_PREP: {
        if (selIng == 1) {
            if (held == H_NONE) { setAct(a, true, "Take raw noodles");
                if (exec) { held = H_RAW; toast("Raw noodles - drop them in the left pot"); } }
            else setAct(a, false, "Your hands are full");
        } else if (selIng == 2) {
            if (held == H_NONE) { setAct(a, true, "Take an empty bowl");
                if (exec) { held = H_BOWL; bowl = BowlState(); } }
            else setAct(a, false, "Your hands are full");
        } else {
            int t = selIng - 3;
            if (held != H_BOWL) setAct(a, false, "Hold a bowl first (press 2, then E)");
            else if (bowl.top[t]) setAct(a, false, "%s is already in the bowl", TOPPING_NAME[t]);
            else { setAct(a, true, "Add %s to the bowl", TOPPING_NAME[t]);
                if (exec) { bowl.top[t] = true;
                    spawnFx(2, { hand.x, hand.y + 0.7f, hand.z }, hand, 0.0f, 0.35f, t); } }
        }
        break;
    }
    case ST_NOODLE_POT: {
        if (held == H_RAW) {
            setAct(a, true, "Drop the noodles into the boiling pot");
            if (exec) { potOn = true; potT = 0; held = H_NONE;
                spawnFx(0, hand, nPot, 0.55f, 0.5f); toast("Noodles are cooking!  Scoop them out when they look PERFECT"); }
        } else if (held == H_BOWL) {
            if (!potOn)            setAct(a, false, "The pot is empty");
            else if (bowl.noodles) setAct(a, false, "The bowl already has noodles");
            else { setAct(a, true, "Scoop the noodles into your bowl");
                if (exec) { bowl.noodles = true; bowl.cook = potT; potOn = false; potT = 0;
                    spawnFx(0, nPot, hand, 0.55f, 0.5f); } }
        } else {
            if (potOn) setAct(a, false, "Noodles are cooking...  take a bowl (2) to scoop them");
            else       setAct(a, false, "The pot is empty - take raw noodles (1)");
        }
        break;
    }
    case ST_BROTH_POT: {
        if (held != H_BOWL)  setAct(a, false, "Take a bowl first (press 2 at the prep area)");
        else if (bowl.broth) setAct(a, false, "The bowl already has broth");
        else { setAct(a, true, "Ladle tonkotsu broth into the bowl");
            if (exec) { bowl.broth = true; spawnFx(1, bPot, hand, 0.45f, 0.45f); } }
        break;
    }
    case ST_SERVE: {
        if (held != H_BOWL)                 setAct(a, false, "Nothing to serve");
        else if (gs == GS_TAKING_ORDER)     setAct(a, false, "The customer is still sitting down");
        else { setAct(a, true, "Serve the ramen to the customer");
            if (exec) { finishOrder(false); } }
        break;
    }
    default: break;
    }
    return a;
}

// ─── Switching between explore mode (default) and game mode ─────────────────
static CameraMode prevCam = CAM_ORBIT;     // camera to return to when leaving the game

static void enterGameMode()
{
    prevCam = currentCamMode;
    gameActive = true;
    gameReset();
    custPresent = false; custLeaving = false; bowlOnCounter = false;
    currentCamMode = CAM_FPS;
    fpsPos   = { 0.0f, 1.75f, -1.25f };
    fpsYaw   = 180.0f;                     // look toward the kitchen
    fpsPitch = -20.0f;
    gs = GS_MENU;
    lastMs = glutGet(GLUT_ELAPSED_TIME);
    updateWindowTitle();
}

static void exitGameMode()
{
    gameActive = false;
    gameReset();
    custPresent = false; custLeaving = false; bowlOnCounter = false;
    doorOpen = false;
    gs = GS_MENU;
    currentCamMode = prevCam;
    updateWindowTitle();
}

bool gameKeyDown(unsigned char key)
{
    // Explore mode: the shop is shown like before.  ENTER (or M) offers the game.
    if (!gameActive) {
        if (key == 13 || key == 'm' || key == 'M') { enterGameMode(); return true; }
        return false;
    }
    // Game mode: M always goes back to exploring
    if (key == 'm' || key == 'M') { exitGameMode(); return true; }

    if (key == 'v' || key == 'V') {                 // turn around: kitchen <-> counter
        fpsYaw = (cosf(fpsYaw * PI / 180.0f) > 0.0f) ? 180.0f : 0.0f;
        fpsPitch = -20.0f;
        return true;
    }

    bool playing = (gs == GS_TAKING_ORDER || gs == GS_COOKING || gs == GS_SERVING);

    if (gs == GS_MENU) {
        if (key == 13 || key == ' ') { startGame(); return true; }
        if (key == 27) { exitGameMode(); return true; }       // back to exploring
        return false;
    }
    if (gs == GS_PAUSED) {
        if (key == 27 || key == 13) { gs = gsBeforePause; lastMs = glutGet(GLUT_ELAPSED_TIME); return true; }
        if (key == 'q' || key == 'Q') exit(0);
        return false;
    }
    if (gs == GS_RESULT) {
        if (key == ' ' || key == 13) { stateT = std::max(stateT, 4.4f); return true; }
        if (key == 27) { gsBeforePause = gs; gs = GS_PAUSED; return true; }
        return (key == 'e' || key == 'E' || (key >= '1' && key <= '6') || key == 'q' || key == 'Q');
    }
    if (playing) {
        if (key == 27) { gsBeforePause = gs; gs = GS_PAUSED; return true; }
        if (key == 'e' || key == 'E') { Act a = queryAct(true);
            if (!a.ok && a.text[0] == 0) toast("Walk up to a station and press E", 1.5f);
            else if (!a.ok) toast(a.text, 1.5f);
            return true; }
        if (key >= '1' && key <= '6') { selIng = key - '0'; return true; }
        if (key == 'q' || key == 'Q') {
            if (held != H_NONE) { held = H_NONE; bowl = BowlState(); toast("Thrown away", 1.2f); }
            return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Init / update
// ═══════════════════════════════════════════════════════════════════════════
void initGame()
{
    srand(12345);
    gs = GS_MENU;                      // camera stays as before until the player opts in
    lastMs = glutGet(GLUT_ELAPSED_TIME);
}

void updateGame()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastMs) * 0.001f;
    lastMs = now;
    if (dt > 0.1f) dt = 0.1f;
    if (dt < 0.0f) dt = 0.0f;
    if (!gameActive) return;
    if (gs == GS_PAUSED) return;
    clk += dt;
    if (toastT > 0) toastT -= dt;

    for (auto& f : fx) if (f.on) { f.t += dt; if (f.t >= f.dur) f.on = false; }

    if (gs == GS_MENU) return;
    stateT += dt;
    if (potOn) potT += dt;

    // Open the shop door while a customer is coming in / going out
    PathInfo path = buildPath();
    if (custPresent) doorOpen = (custD < path.len[0] + path.len[1] * 0.6f);

    switch (gs) {
    case GS_TAKING_ORDER:
        order.elapsed += dt;
        custD += WALK_SPEED * dt;
        if (custD >= path.total) {
            custD = path.total; gs = GS_COOKING; stateT = 0;
            toast("Order up!  Check the board on the left", 2.5f);
        }
        break;
    case GS_COOKING:
    case GS_SERVING:
        order.elapsed += dt;
        gs = (held == H_BOWL && bowl.broth && bowl.noodles) ? GS_SERVING : GS_COOKING;
        if (order.elapsed > order.patience) finishOrder(true);
        break;
    case GS_RESULT:
        if (stateT > 2.2f) custLeaving = true;
        if (custLeaving && custPresent) {
            custD -= WALK_SPEED * dt;
            if (custD <= 0) { custD = 0; custPresent = false; bowlOnCounter = false; doorOpen = false; }
        }
        if (stateT > 4.4f && !custPresent) spawnCustomer();
        break;
    default: break;
    }
}

float gameSteamScale(int pot)
{
    if (!gameActive || gs == GS_MENU) return 1.0f;
    if (pot == 0) return potOn ? 1.0f + 0.9f * clamp01(potT / 9.0f) : 0.55f;   // more steam as it cooks
    return 1.0f;
}

// ═══════════════════════════════════════════════════════════════════════════
//  3D drawing
// ═══════════════════════════════════════════════════════════════════════════
static void drawNoodlesColored(Vec3 pos, Color c, float s)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glScalef(s, s, s);
    setMaterialPBR(Materials::Noodle, c);
    for (int i = 0; i < 6; i++) {
        float x = 0.04f * (i % 3) - 0.04f;
        float z = 0.03f * (i % 2) - 0.015f;
        drawTorus({ x, 0.01f * (i % 2), z }, NO_ROT, ONE, c, 0.025f, 0.08f + 0.04f * i);
    }
    for (int i = 0; i < 3; i++)
        drawCylinderCustom({ -0.05f, 0.03f, -0.1f + 0.1f * i }, { 0, 40.0f * i, -90 }, ONE, c, 0.02f, 0.02f, 0.3f);
    resetMaterialGloss();
    glPopMatrix();
}

static void drawToppingMesh(int t, Vec3 pos, float s)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glScalef(s, s, s);
    switch (t) {
    case T_PORK:  setMaterialPBR(Materials::RolledMeat, MEAT);        drawMeat({ 0, 0, 0 }, { 10, 0, 0 });   break;
    case T_EGG:   setMaterialPBR(Materials::EggYolk, EGG_WHITE);      drawEgg({ 0, 0, 0 }, { 0, 30, 0 });    break;
    case T_ONION: setMaterialPBR(Materials::SeaweedNori, ONION_GREEN); drawGreenOnion({ 0, 0, 0 });           break;
    case T_NORI:  setMaterialPBR(Materials::SeaweedNori, NORI);       drawSeaweed({ 0, 0, 0 }, { -20, 0, 0 }); break;
    }
    resetMaterialGloss();
    glPopMatrix();
}

// Ramen bowl showing exactly what has been put in it
static void drawBowlContents(const BowlState& b, Vec3 pos, float s)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glScalef(s, s, s);

    setMaterialGloss(0.90f, 0.90f, 0.90f, 110.0f);
    drawBowl({ 0, 0, 0 }, NO_ROT, ONE, BOWL_RED);
    resetMaterialGloss();

    float y0 = b.broth ? 0.0f : -0.20f;       // without broth everything sits at the bottom
    if (b.broth) {
        setMaterialPBR(Materials::Broth, BROTH);
        setMaterialGloss(0.90f, 0.75f, 0.50f, 85.0f);
        drawCylinder({ 0, 0.29f, 0 }, NO_ROT, { 0.83f, 0.02f, 0.83f }, BROTH);
        resetMaterialGloss();
    }
    if (b.noodles) drawNoodlesColored({ 0, 0.31f + y0, 0 }, noodleColor(b.cook), 1.0f);
    if (b.top[T_PORK])  drawToppingMesh(T_PORK,  { -0.17f, 0.33f + y0,  0.12f }, 1.0f);
    if (b.top[T_EGG])   drawToppingMesh(T_EGG,   {  0.18f, 0.32f + y0,  0.10f }, 1.0f);
    if (b.top[T_NORI])  drawToppingMesh(T_NORI,  {  0.00f, 0.38f + y0, -0.28f }, 1.0f);
    if (b.top[T_ONION]) drawToppingMesh(T_ONION, {  0.05f, 0.33f + y0, -0.08f }, 1.0f);
    glPopMatrix();
}

static void drawRing(float x, float y, float z, float r, float w, float a, Color c)
{
    setLighting(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glColor4f(c.r, c.g, c.b, a);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 48; i++) {
        float ang = i * 2.0f * PI / 48.0f;
        float ca = cosf(ang), sa = sinf(ang);
        glVertex3f(x + ca * (r - w), y, z + sa * (r - w));
        glVertex3f(x + ca * (r + w), y, z + sa * (r + w));
    }
    glEnd();
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    setLighting(true);
}

// Customer built from primitives with a small joint hierarchy (hips, knees, shoulders)
static void drawCustomer()
{
    static const Color SHIRT[4] = { { 0.20f, 0.35f, 0.65f }, { 0.70f, 0.25f, 0.25f },
                                    { 0.30f, 0.55f, 0.35f }, { 0.55f, 0.40f, 0.65f } };
    static const Color SKIN = { 0.93f, 0.76f, 0.62f }, PANTS = { 0.17f, 0.17f, 0.22f },
                       HAIR = { 0.08f, 0.06f, 0.05f };
    PathInfo p = buildPath();
    float x, z, yaw, sit;
    samplePath(p, custD, x, z, yaw, sit);
    if (custLeaving) yaw += 180.0f;                // walking back out

    bool walking = (custD > 0.01f && custD < p.total - 0.01f) && (gs == GS_TAKING_ORDER || custLeaving);
    float phase = custD * 4.2f;
    float swing = walking ? sinf(phase) * 28.0f : 0.0f;
    float bounce = walking ? fabsf(sinf(phase)) * 0.035f : 0.0f;
    float react = 0.0f;                            // little hop when the result is great
    if (gs == GS_RESULT && !res.failed && res.stars >= 4 && !custLeaving)
        react = fabsf(sinf(stateT * 6.0f)) * 0.06f * clamp01(1.0f - stateT / 2.2f);

    float seatTop = FLOOR_Y + 0.76f;               // top of the stool seat
    float hipY = lerpf(FLOOR_Y + 0.56f, seatTop + 0.02f, sit) + bounce + react;

    glPushMatrix();
    glTranslatef(x, hipY, z);
    glRotatef(yaw, 0, 1, 0);
    setMaterialGloss(0.15f, 0.15f, 0.15f, 20.0f);

    // Torso + shoulders + head
    drawCylinder({ 0, 0.0f, 0 }, NO_ROT, { 0.40f, 0.60f, 0.26f }, SHIRT[outfit]);
    drawSphere({ 0, 0.62f, 0 }, NO_ROT, { 0.42f, 0.16f, 0.28f }, SHIRT[outfit]);
    drawCylinder({ 0, 0.64f, 0 }, NO_ROT, { 0.09f, 0.10f, 0.09f }, SKIN);                 // neck
    glPushMatrix();
    glTranslatef(0, 0.88f, 0);
    glRotatef(sit * 8.0f * sinf(clk * 1.3f), 1, 0, 0);                                  // idle head nod
    drawSphere({ 0, 0, 0 }, NO_ROT, { 0.27f, 0.30f, 0.27f }, SKIN);                      // head
    drawSphere({ 0, 0.05f, -0.015f }, NO_ROT, { 0.285f, 0.24f, 0.285f }, HAIR);          // hair
    drawSphere({ -0.06f, 0.0f, 0.125f }, NO_ROT, { 0.035f, 0.045f, 0.03f }, BLACK);      // eyes
    drawSphere({  0.06f, 0.0f, 0.125f }, NO_ROT, { 0.035f, 0.045f, 0.03f }, BLACK);
    glPopMatrix();

    // Legs: hip -> knee hierarchy.  Seated: thighs forward, shins down.
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.10f, 0.0f, 0.0f);
        float thigh = -90.0f * sit + swing * (side) * (1.0f - sit);
        glRotatef(thigh, 1, 0, 0);
        drawCylinder({ 0, -0.28f, 0 }, NO_ROT, { 0.15f, 0.28f, 0.15f }, PANTS);          // thigh
        glTranslatef(0, -0.28f, 0);
        glRotatef(90.0f * sit + (walking ? fmaxf(0.0f, -swing * side) * 0.6f : 0.0f), 1, 0, 0);
        drawCylinder({ 0, -0.28f, 0 }, NO_ROT, { 0.13f, 0.28f, 0.13f }, PANTS);          // shin
        drawSphere({ 0, -0.30f, 0.05f }, NO_ROT, { 0.14f, 0.08f, 0.22f }, BLACK);        // shoe
        glPopMatrix();
    }
    // Arms: shoulder -> elbow.  Seated: forearms rest forward on the counter.
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * 0.26f, 0.60f, 0.0f);
        float upper = -swing * side * 0.9f * (1.0f - sit) + (-35.0f) * sit;
        glRotatef(upper, 1, 0, 0);
        drawCylinder({ 0, -0.26f, 0 }, NO_ROT, { 0.10f, 0.26f, 0.10f }, SHIRT[outfit]);
        glTranslatef(0, -0.26f, 0);
        glRotatef(-60.0f * sit, 1, 0, 0);
        drawCylinder({ 0, -0.24f, 0 }, NO_ROT, { 0.09f, 0.24f, 0.09f }, SKIN);
        glPopMatrix();
    }
    resetMaterialGloss();
    glPopMatrix();
}

static void drawFxItems()
{
    for (const auto& f : fx) {
        if (!f.on) continue;
        float e = smooth(f.t / f.dur);
        Vec3 p = { lerpf(f.a.x, f.b.x, e), lerpf(f.a.y, f.b.y, e) + f.arc * sinf(e * PI), lerpf(f.a.z, f.b.z, e) };
        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glRotatef(f.t * 420.0f, 0, 1, 0);
        if (f.kind == 0) {
            drawNoodlesColored({ 0, 0, 0 }, noodleColor(0.0f), 0.45f);
        } else if (f.kind == 1) {
            setMaterialGloss(0.9f, 0.8f, 0.5f, 90.0f);
            drawSphere({ 0, 0, 0 }, NO_ROT, { 0.10f, 0.12f, 0.10f }, BROTH);
            resetMaterialGloss();
        } else {
            drawToppingMesh(f.sub, { 0, 0, 0 }, 0.28f);
        }
        glPopMatrix();
    }
}

void drawGameWorld()
{
    if (!gameActive) return;

    // Floating icon over the prep area showing the selected ingredient
    if (gs != GS_MENU) {
        glPushMatrix();
        glTranslatef(PREP_X, 1.50f + 0.04f * sinf(clk * 2.2f), KITCHEN_Z + 0.25f);
        glRotatef(clk * 55.0f, 0, 1, 0);
        switch (selIng) {
        case 1: drawNoodlesColored({ 0, 0, 0 }, noodleColor(0.0f), 0.45f); break;
        case 2: { BowlState empty; drawBowlContents(empty, { 0, 0, 0 }, 0.20f); } break;
        default: drawToppingMesh(selIng - 3, { 0, 0, 0 }, 0.45f); break;
        }
        glPopMatrix();
    }

    // Station highlight (pulsing ring) for the station in reach
    bool active = (gs == GS_TAKING_ORDER || gs == GS_COOKING || gs == GS_SERVING);
    if (active) {
        Act a = queryAct(false);
        float pulse = 0.35f + 0.25f * sinf(clk * 5.0f);
        Color c = a.ok ? Color{ 1.0f, 0.85f, 0.30f } : Color{ 0.75f, 0.75f, 0.80f };
        switch (a.st) {
        case ST_PREP:       drawRing(PREP_X, POT_BASE_Y - 0.06f, KITCHEN_Z, 0.55f, 0.03f, pulse, c); break;
        case ST_NOODLE_POT: drawRing(NOODLE_POT_X, POT_BASE_Y - 0.06f, KITCHEN_Z, 0.42f, 0.03f, pulse, c); break;
        case ST_BROTH_POT:  drawRing(BROTH_POT_X,  POT_BASE_Y - 0.06f, KITCHEN_Z, 0.42f, 0.03f, pulse, c); break;
        case ST_SERVE:      drawRing(stoolX(seatIdx), COUNTER_TOP + 0.01f, -0.25f, 0.34f, 0.025f, pulse, c); break;
        default: break;
        }
    }

    // Noodles boiling in the left pot (colour follows the cooking stage)
    if (potOn) {
        Color nc = noodleColor(potT);
        glPushMatrix();
        glTranslatef(NOODLE_POT_X, POT_BASE_Y + 0.545f, KITCHEN_Z);
        glRotatef(clk * 18.0f, 0, 1, 0);
        for (int i = 0; i < 6; i++) {
            float bob = 0.010f * sinf(clk * 4.0f + i * 1.3f);
            drawTorus({ 0, bob, 0 }, NO_ROT, ONE, nc, 0.022f, 0.05f + 0.038f * i);
        }
        glPopMatrix();
        // floating indicator: a noodle bundle that changes colour with the cooking stage
        glPushMatrix();
        glTranslatef(NOODLE_POT_X, POT_TOP_Y + 0.22f + 0.025f * sinf(clk * 3.0f), KITCHEN_Z);
        glRotatef(clk * 70.0f, 0, 1, 0);
        drawNoodlesColored({ 0, 0, 0 }, nc, 0.30f);
        glPopMatrix();
        // bubbles
        for (int i = 0; i < 7; i++) {
            float ph = fmodf(clk * (0.8f + 0.1f * i) + i * 0.37f, 1.0f);
            float ang = i * 2.4f + clk * 0.3f;
            drawSphere({ NOODLE_POT_X + cosf(ang) * 0.20f * (0.4f + 0.6f * (i % 3) / 2.0f),
                         POT_BASE_Y + 0.55f + 0.05f * ph,
                         KITCHEN_Z + sinf(ang) * 0.20f * (0.4f + 0.6f * (i % 3) / 2.0f) },
                       NO_ROT, { 0.035f * (1.0f - ph) + 0.01f, 0.025f, 0.035f * (1.0f - ph) + 0.01f },
                       { 0.95f, 0.95f, 0.95f });
        }
        // "ding" pulse when the noodles are perfect
        if (cookClass(potT) == 2) {
            float ph = fmodf(clk * 1.4f, 1.0f);
            drawRing(NOODLE_POT_X, POT_TOP_Y + 0.08f, KITCHEN_Z, 0.30f + 0.18f * ph, 0.02f,
                     0.7f * (1.0f - ph), { 0.4f, 1.0f, 0.5f });
        }
    }

    if (custPresent) drawCustomer();

    // The served bowl in front of the customer
    if (bowlOnCounter) {
        drawBowlContents(servedBowl, { stoolX(seatIdx), COUNTER_TOP, -0.25f }, 0.24f);
        if (showSteam) drawSteam({ stoolX(seatIdx), COUNTER_TOP + 0.09f, -0.25f }, 0.22f, 0.7f);
    }

    // Item in the player's hands
    if (gs != GS_MENU && held != H_NONE) {
        Vec3 h = handPos();
        glPushMatrix();
        glTranslatef(h.x, h.y, h.z);
        glRotatef(fpsYaw, 0, 1, 0);
        if (held == H_RAW) drawNoodlesColored({ 0, 0, 0 }, noodleColor(0.0f), 0.7f);
        else               drawBowlContents(bowl, { 0, 0, 0 }, 0.30f);
        glPopMatrix();
    }

    drawFxItems();
}

// ═══════════════════════════════════════════════════════════════════════════
//  HUD
// ═══════════════════════════════════════════════════════════════════════════
static void hudBegin(int w, int h)
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_LINE_BIT | GL_TEXTURE_BIT);
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D); glDisable(GL_CULL_FACE); glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
}
static void hudEnd()
{
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glPopAttrib();
}

static void* F_SMALL = GLUT_BITMAP_HELVETICA_12;
static void* F_MED   = GLUT_BITMAP_HELVETICA_18;
static void* F_BIG   = GLUT_BITMAP_TIMES_ROMAN_24;

static float textW(const char* s, void* f) { return (float)glutBitmapLength(f, (const unsigned char*)s); }
static void text(float x, float y, const char* s, void* f, float r, float g, float b, float a = 1.0f)
{
    glColor4f(r, g, b, a);
    glRasterPos2f(x, y);
    glutBitmapString(f, (const unsigned char*)s);
}
static void textC(float cx, float y, const char* s, void* f, float r, float g, float b, float a = 1.0f)
{
    text(cx - textW(s, f) * 0.5f, y, s, f, r, g, b, a);
}
static void panel(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS); glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1); glEnd();
}
static void panelBorder(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP); glVertex2f(x0, y0); glVertex2f(x1, y0); glVertex2f(x1, y1); glVertex2f(x0, y1); glEnd();
}
static void star(float cx, float cy, float R, bool filled)
{
    float r = R * 0.45f;
    if (filled) glColor4f(1.0f, 0.82f, 0.15f, 1.0f); else glColor4f(0.45f, 0.45f, 0.50f, 0.8f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 10; i++) {
        float ang = PI / 2.0f + i * PI / 5.0f;
        float rr = (i % 2 == 0) ? R : r;
        glVertex2f(cx + cosf(ang) * rr, cy + sinf(ang) * rr);
    }
    glEnd();
}

static void drawCookMeter(float x, float y)
{
    // 0..15 s split into RAW / UNDERCOOKED / PERFECT / OVERCOOKED / RUINED
    const float W = 330.0f, H = 16.0f, MAXT = 15.0f;
    panel(x - 10, y - 30, x + W + 10, y + H + 34, 0.05f, 0.05f, 0.08f, 0.72f);
    text(x, y + H + 12, "NOODLES IN POT", F_SMALL, 1, 1, 1);
    const Color seg[5] = { { 0.65f, 0.65f, 0.65f }, { 0.95f, 0.85f, 0.40f }, { 0.30f, 0.85f, 0.35f },
                           { 0.95f, 0.55f, 0.20f }, { 0.55f, 0.20f, 0.15f } };
    for (int i = 0; i < 5; i++) {
        panel(x + W * (i * 3.0f / MAXT), y, x + W * ((i + 1) * 3.0f / MAXT) - 2, y + H,
              seg[i].r, seg[i].g, seg[i].b, 0.85f);
    }
    float mx = x + W * clamp01(potT / MAXT);
    panel(mx - 2, y - 6, mx + 2, y + H + 6, 1, 1, 1, 1);
    int cc = cookClass(potT);
    char buf[64]; snprintf(buf, sizeof(buf), "%s   %.1fs", COOK_NAME[cc], potT);
    text(x, y - 20, buf, F_MED, seg[cc].r, seg[cc].g, seg[cc].b);
}

void drawGameHUD(int w, int h)
{
    if (!gameActive) {
        // Explore mode: just a small invitation at the bottom of the screen
        hudBegin(w, h);
        const char* msg = "Press  ENTER  to play the Ramen Cooking Game";
        float tw = textW(msg, F_MED);
        float a = 0.70f + 0.30f * sinf(glutGet(GLUT_ELAPSED_TIME) * 0.004f);
        float cx = w * 0.5f;
        panel(cx - tw * 0.5f - 18, 14, cx + tw * 0.5f + 18, 50, 0.05f, 0.05f, 0.08f, 0.72f);
        panelBorder(cx - tw * 0.5f - 18, 14, cx + tw * 0.5f + 18, 50, 0.95f, 0.70f, 0.25f, 0.9f);
        text(cx - tw * 0.5f, 26, msg, F_MED, 1.0f, 0.85f, 0.35f, a);
        hudEnd();
        return;
    }
    hudBegin(w, h);

    bool active = (gs == GS_TAKING_ORDER || gs == GS_COOKING || gs == GS_SERVING);

    if (gs == GS_MENU) {
        panel(0, 0, (float)w, (float)h, 0.02f, 0.02f, 0.05f, 0.62f);
        float cx = w * 0.5f, cy = h * 0.5f;
        panel(cx - 330, cy - 200, cx + 330, cy + 190, 0.10f, 0.06f, 0.05f, 0.88f);
        panelBorder(cx - 330, cy - 200, cx + 330, cy + 190, 0.95f, 0.70f, 0.25f, 1.0f);
        textC(cx, cy + 140, "RAMEN SHOP", F_BIG, 1.0f, 0.78f, 0.25f);
        textC(cx, cy + 112, "Cook it right.  Serve it fast.", F_MED, 1, 1, 1);
        const char* lines[] = {
            "W A S D               Move        Arrows / mouse drag: look around",
            "V                      Turn around (kitchen <-> counter)",
            "E                      Interact with the station in front of you",
            "1-6                    Choose ingredient at the prep area",
            "Q                      Throw away what you are holding",
            "C                      Change camera",
            "ESC                    Pause      (on this screen: back to exploring)      M  Back to exploring",
            "",
            "1 Noodles   2 Bowl   3 Pork   4 Egg   5 Green onion   6 Seaweed",
            "Boil the noodles, ladle the broth, add the toppings, serve!" };
        for (int i = 0; i < 10; i++) text(cx - 300, cy + 70 - i * 22, lines[i], F_SMALL, 0.92f, 0.92f, 0.92f);
        float pulse = 0.65f + 0.35f * sinf(clk * 4.0f + 1.0f);
        textC(cx, cy - 165, "Press  ENTER  to start cooking", F_MED, 1.0f, 0.85f, 0.35f, pulse);
        (void)clk;
        hudEnd();
        return;
    }

    // ── Order board (top-left) ───────────────────────────────────────────
    if (gs != GS_RESULT) {
        float x = 18, top = h - 18;
        int items = 2;
        for (int t = 0; t < T_COUNT; t++) if (order.top[t]) items++;
        float ph = 112 + items * 22;
        panel(x - 8, top - ph, x + 292, top, 0.07f, 0.05f, 0.04f, 0.80f);
        panelBorder(x - 8, top - ph, x + 292, top, 0.85f, 0.60f, 0.22f, 1.0f);
        char buf[96];
        snprintf(buf, sizeof(buf), "ORDER #%d", order.number);
        text(x, top - 24, buf, F_MED, 1.0f, 0.80f, 0.30f);
        text(x, top - 46, "Tonkotsu Ramen", F_MED, 1, 1, 1);
        bool hb = (held == H_BOWL);
        float yy = top - 72;
        auto line = [&](bool done, const char* name) {
            snprintf(buf, sizeof(buf), "[%c] %s", done ? 'x' : ' ', name);
            if (done) text(x + 6, yy, buf, F_SMALL, 0.45f, 1.0f, 0.55f);
            else      text(x + 6, yy, buf, F_SMALL, 0.92f, 0.92f, 0.92f);
            yy -= 22;
        };
        line(hb && bowl.noodles, "Noodles (boiled just right)");
        line(hb && bowl.broth,   "Tonkotsu broth");
        for (int t = 0; t < T_COUNT; t++) if (order.top[t]) line(hb && bowl.top[t], TOPPING_NAME[t]);
        if (hb) {
            for (int t = 0; t < T_COUNT; t++)
                if (bowl.top[t] && !order.top[t]) {
                    snprintf(buf, sizeof(buf), "  extra: %s", TOPPING_NAME[t]);
                    text(x + 6, yy, buf, F_SMALL, 1.0f, 0.45f, 0.40f); yy -= 18;
                }
        }
        // patience bar
        float frac = clamp01(1.0f - order.elapsed / order.patience);
        float by = top - ph + 30;
        panel(x, by, x + 276, by + 10, 0.2f, 0.2f, 0.2f, 0.9f);
        Color pc = lerpC({ 0.90f, 0.25f, 0.20f }, { 0.35f, 0.85f, 0.35f }, frac);
        panel(x, by, x + 276 * frac, by + 10, pc.r, pc.g, pc.b, 1.0f);
        snprintf(buf, sizeof(buf), "%.0fs", std::max(0.0f, order.patience - order.elapsed));
        text(x + 282 - textW(buf, F_SMALL), by - 14, buf, F_SMALL, 0.9f, 0.9f, 0.9f);
        text(x, by - 14, "Customer patience", F_SMALL, 0.8f, 0.8f, 0.8f);
    }

    // ── Stats (top-right) ────────────────────────────────────────────────
    {
        char buf[64];
        float x1 = w - 18.0f, top = h - 18.0f;
        panel(x1 - 230, top - 76, x1 + 8, top, 0.07f, 0.05f, 0.04f, 0.80f);
        panelBorder(x1 - 230, top - 76, x1 + 8, top, 0.85f, 0.60f, 0.22f, 1.0f);
        snprintf(buf, sizeof(buf), "LEVEL %d", level);          text(x1 - 218, top - 24, buf, F_MED, 1.0f, 0.80f, 0.30f);
        snprintf(buf, sizeof(buf), "Served: %d", served);       text(x1 - 218, top - 46, buf, F_SMALL, 1, 1, 1);
        snprintf(buf, sizeof(buf), "%d Yen", yen);              text(x1 - 218, top - 66, buf, F_MED, 0.55f, 1.0f, 0.60f);
    }

    // ── Cooking meter (bottom-left) ──────────────────────────────────────
    if (potOn && active) drawCookMeter(24, 60);

    // ── Interaction prompt + inventory (bottom-center) ───────────────────
    if (active) {
        Act a = queryAct(false);
        float cx = w * 0.5f;
        if (a.text[0]) {
            char buf[160]; snprintf(buf, sizeof(buf), "[E]  %s", a.text);
            float tw = textW(buf, F_MED);
            panel(cx - tw * 0.5f - 16, 78, cx + tw * 0.5f + 16, 112, 0.05f, 0.05f, 0.08f, 0.78f);
            if (a.ok) text(cx - tw * 0.5f, 88, buf, F_MED, 1.0f, 0.88f, 0.35f);
            else      text(cx - tw * 0.5f, 88, buf, F_MED, 0.75f, 0.75f, 0.78f);
        }
        // ingredient selector
        const char* names[6] = { "1 Noodles", "2 Bowl", "3 Pork", "4 Egg", "5 Onion", "6 Seaweed" };
        float total = 0; for (int i = 0; i < 6; i++) total += textW(names[i], F_SMALL) + 26;
        float x = cx - total * 0.5f;
        panel(x - 8, 30, x + total, 62, 0.05f, 0.05f, 0.08f, 0.78f);
        for (int i = 0; i < 6; i++) {
            float tw = textW(names[i], F_SMALL);
            if (selIng == i + 1) { panel(x - 4, 34, x + tw + 4, 58, 0.95f, 0.70f, 0.20f, 0.95f);
                                   text(x, 41, names[i], F_SMALL, 0.1f, 0.05f, 0.0f); }
            else                   text(x, 41, names[i], F_SMALL, 0.9f, 0.9f, 0.9f);
            x += tw + 26;
        }
        const char* hs = (held == H_NONE) ? "Hands: empty" : (held == H_RAW ? "Hands: raw noodles" : "Hands: ramen bowl");
        text(w - 18 - textW(hs, F_MED), 38, hs, F_MED, 0.95f, 0.95f, 0.95f);
        if (gs == GS_SERVING) textC(cx, 128, "Ready!  Take it to the counter and serve", F_MED, 0.55f, 1.0f, 0.6f);
    }

    // ── Toast ────────────────────────────────────────────────────────────
    if (toastT > 0 && toastMsg[0]) {
        float a = clamp01(toastT / 0.5f);
        float tw = textW(toastMsg, F_MED);
        panel(w * 0.5f - tw * 0.5f - 18, h - 120, w * 0.5f + tw * 0.5f + 18, h - 84, 0.05f, 0.05f, 0.08f, 0.8f * a);
        textC(w * 0.5f, h - 109, toastMsg, F_MED, 1.0f, 1.0f, 1.0f, a);
    }

    // ── Result card ──────────────────────────────────────────────────────
    if (gs == GS_RESULT) {
        float cx = w * 0.5f, cy = h * 0.55f;
        panel(cx - 230, cy - 150, cx + 230, cy + 150, 0.10f, 0.06f, 0.05f, 0.90f);
        panelBorder(cx - 230, cy - 150, cx + 230, cy + 150, 0.95f, 0.70f, 0.25f, 1.0f);
        char buf[96];
        if (res.failed) {
            textC(cx, cy + 100, "CUSTOMER LEFT!", F_BIG, 1.0f, 0.40f, 0.35f);
            textC(cx, cy + 50, "You took too long.", F_MED, 1, 1, 1);
            for (int i = 0; i < 5; i++) star(cx - 100 + i * 50, cy - 10, 18, false);
            textC(cx, cy - 80, "+0 Yen", F_BIG, 0.8f, 0.8f, 0.8f);
        } else {
            textC(cx, cy + 112, "RAMEN COMPLETED!", F_BIG, 1.0f, 0.80f, 0.28f);
            snprintf(buf, sizeof(buf), "Order Accuracy:  %.0f%%", res.accuracy * 100.0f);
            text(cx - 150, cy + 66, buf, F_MED, 1, 1, 1);
            snprintf(buf, sizeof(buf), "Cooking:  %s", res.cookName);
            text(cx - 150, cy + 38, buf, F_MED, 1, 1, 1);
            snprintf(buf, sizeof(buf), "Time:  %.0f sec", res.secs);
            text(cx - 150, cy + 10, buf, F_MED, 1, 1, 1);
            for (int i = 0; i < 5; i++) star(cx - 100 + i * 50, cy - 40, 20, i < res.stars);
            snprintf(buf, sizeof(buf), "+%d Yen", res.yen);
            textC(cx, cy - 110, buf, F_BIG, 0.55f, 1.0f, 0.60f);
        }
        textC(cx, cy - 138, "SPACE to continue", F_SMALL, 0.75f, 0.75f, 0.75f);
    }

    // ── Pause ────────────────────────────────────────────────────────────
    if (gs == GS_PAUSED) {
        panel(0, 0, (float)w, (float)h, 0.0f, 0.0f, 0.0f, 0.55f);
        textC(w * 0.5f, h * 0.5f + 30, "PAUSED", F_BIG, 1.0f, 0.85f, 0.35f);
        textC(w * 0.5f, h * 0.5f - 6,  "ESC or ENTER: resume     Q: quit", F_MED, 1, 1, 1);
    }

    hudEnd();
}
