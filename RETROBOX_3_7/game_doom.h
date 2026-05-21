// ============================================================
//  game_doom.h  —  RETROBOX built-in game
//  DOOM-Nano — raycaster FPS, 8 maps, enemies
// ============================================================
#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <math.h>

extern Adafruit_SSD1306 display;
extern Preferences       prefs;
extern int  PLAY_W, PLAY_H;
extern int  highScores[][3];
extern int  selectedGame;
extern Difficulty currentDiff;
extern bool joyLeft(), joyRight(), joyUp(), joyDown();
extern int  joyMoveStep();
extern Button btnA, btnB, btnStart, btnJoySW;
extern void sfxBeep(), sfxConfirm(), sfxBack(), sfxShoot();
extern void sfxExplode(), sfxGameOver(), sfxLevelUp(), sfxMenu(), sfxHit();
extern void saveScore(int game, int diff, int score);
extern void drawCenteredText(int y, const char* str, int size);
extern void scrollStarsV(int* sx, int* sy, int n, int speed);
extern void drawShip(int x, int y);

namespace DoomGame {

  // ── Map ──────────────────────────────────────────────────
  static const int MAP_W = 20;
  static const int MAP_H = 14;
  static uint8_t DMAP[MAP_W * MAP_H];

  static float EXIT_X, EXIT_Y;
  static const int VIEW_HUD_H = 12;  // HUD height at bottom

  static int mapIndex = 0;  // which pre-defined map was chosen at init()

  // ── 8 Pre-defined maps (0=floor, 1=wall, 2=exit) ─────────
  // Each is MAP_W*MAP_H = 280 bytes. Stored in PROGMEM.
  // Layout key: 1=wall, 0=open, 2=exit (placed in last room area)
  // All maps have a player-start region near top-left open area.
  static const uint8_t DMAP_0[MAP_W * MAP_H] PROGMEM = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,1,1,0,1,0,0,0,0,0,0,1,
    1,0,0,0,0,1,0,0,1,1,1,0,0,0,0,0,0,0,0,1,
    1,1,1,0,1,1,1,0,1,1,1,0,1,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,1,1,0,1,1,0,0,1,1,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,1,
    1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,2,1,
    1,0,0,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,1,1,0,1,1,1,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };
  static const uint8_t DMAP_1[MAP_W * MAP_H] PROGMEM = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1,
    1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1,
    1,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,
    1,1,1,0,1,1,1,1,0,0,0,0,0,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,
    1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1,0,0,1,
    1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1,0,0,1,
    1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,2,1,
    1,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,1,
    1,0,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };
  static const uint8_t DMAP_2[MAP_W * MAP_H] PROGMEM = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,
    1,0,0,1,1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,
    1,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,1,
    1,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,1,
    1,0,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,1,
    1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };
  static const uint8_t DMAP_3[MAP_W * MAP_H] PROGMEM = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1,
    1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1,
    1,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,1,
    1,1,0,1,1,0,0,0,1,1,0,0,1,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,
    1,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,1,1,0,1,
    1,0,0,1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,1,
    1,0,0,0,0,0,1,1,0,0,0,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,
    1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,2,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };
  static const uint8_t DMAP_4[MAP_W * MAP_H] PROGMEM = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,1,0,0,0,0,0,0,0,1,1,1,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,1,1,0,1,0,1,1,1,0,0,0,0,1,
    1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,
    1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,
    1,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,2,1,
    1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };
  static const uint8_t DMAP_5[MAP_W * MAP_H] PROGMEM = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,
    1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,
    1,1,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,1,1,1,
    1,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,0,1,1,1,1,0,0,0,1,1,0,0,1,
    1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,2,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };
  static const uint8_t DMAP_6[MAP_W * MAP_H] PROGMEM = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,
    1,0,0,1,1,0,0,0,0,0,0,0,0,0,1,1,0,0,0,1,
    1,0,0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0,1,
    1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,
    1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,
    1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,1,0,0,0,1,
    1,0,0,0,1,1,0,0,1,1,1,1,0,0,0,1,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };
  static const uint8_t DMAP_7[MAP_W * MAP_H] PROGMEM = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,1,
    1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,
    1,0,0,1,1,0,0,0,0,0,1,0,0,0,0,1,1,0,0,1,
    1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,1,1,1,0,1,1,1,1,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,
    1,0,0,1,0,0,0,0,1,1,0,1,1,0,0,0,1,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };

  // Pointer table for the 8 maps
  static const uint8_t* const DMAP_TABLE[8] PROGMEM = {
    DMAP_0, DMAP_1, DMAP_2, DMAP_3,
    DMAP_4, DMAP_5, DMAP_6, DMAP_7,
  };

  // Load a pre-defined map by index into the mutable DMAP[] buffer.
  // Scans for the exit tile (2) to set EXIT_X/EXIT_Y.
  void loadPredefMap(int idx) {
    const uint8_t* src = (const uint8_t*)pgm_read_ptr(&DMAP_TABLE[idx]);
    memcpy_P(DMAP, src, MAP_W * MAP_H);
    // Find exit cell
    EXIT_X = MAP_W - 2.5f; EXIT_Y = MAP_H - 2.5f;  // fallback
    for (int y = 0; y < MAP_H; y++)
      for (int x = 0; x < MAP_W; x++)
        if (DMAP[y * MAP_W + x] == 2) { EXIT_X = x + 0.5f; EXIT_Y = y + 0.5f; }
  }

  // ── BSP room data removed — maps are now pre-defined (DMAP_0..7) ─

  // ── Player state ─────────────────────────────────────────
  static float px, py, pa;
  static float phealth;

  // ── Dither: 4 levels, N/S walls phase-shifted ────────────
  // lv 0 = closest (bright) → lv 3 = farthest (dark)
  bool ditherPixel(int sx, int sy, int lv, int phase = 0) {
    static const uint8_t bayer[4][4] = {
      { 0, 8, 2,10},
      {12, 4,14, 6},
      { 3,11, 1, 9},
      {15, 7,13, 5},
    };
    // Thresholds: 13(81%), 9(56%), 5(31%), 2(13%)
    static const int thresholds[4] = {13, 9, 5, 2};
    int threshold = thresholds[lv < 4 ? lv : 3];
    int bx = (sx + phase) & 3;
    int by = (sy + phase) & 3;
    return (int)bayer[by][bx] < threshold;
  }

  // ── Imp sprite (8×12) ────────────────────────────────────
  static const uint8_t IMP_SPRITE[] PROGMEM = {
    0b00111100,
    0b01111110,
    0b01011010,
    0b01111110,
    0b00111100,
    0b00111100,
    0b01111110,
    0b11111111,
    0b11111111,
    0b01111110,
    0b01000010,
    0b11000011,
  };
  static const int IMP_H = 12;
  static const int IMP_W =  8;

  // ── Enemy ─────────────────────────────────────────────────
  struct Enemy {
    float x, y;
    bool  alive;
    int   hp;
    float stateTimer;
    bool  attacking;
    float patrolAngle;   // wander direction when idle
    float patrolTimer;
  };
  static const int MAX_ENEMIES = 6;
  static Enemy enemies[MAX_ENEMIES];

  static int   score, level;
  static bool  gameOver, newRecord;
  static bool  strafeMode;
  static unsigned long lastShot;
  static unsigned long lastHurt;
  static bool  muzzleFlash;
  static unsigned long muzzleEnd;
  static int   diffEnemyCount;
  static bool  showMap;

  static float gunRecoil;
  static float bobPhase;
  static float bobAmt;
  static bool  wasMoving;

  bool isWall(int mx, int my) {
    if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return true;
    return DMAP[my * MAP_W + mx] == 1;
  }
  bool isExit(int mx, int my) {
    if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return false;
    return DMAP[my * MAP_W + mx] == 2;
  }


  void spawnEnemies(int count) {
    if (count > MAX_ENEMIES) count = MAX_ENEMIES;
    int n = 0;
    // Scatter enemies in open floor tiles, minimum distance from player
    for (int attempt = 0; attempt < 400 && n < count; attempt++) {
      int tx = 1 + random(0, MAP_W - 2);
      int ty = 1 + random(0, MAP_H - 2);
      if (DMAP[ty * MAP_W + tx] != 0) continue;
      float dx = tx + 0.5f - px, dy = ty + 0.5f - py;
      if (dx*dx + dy*dy < 16.0f) continue;  // at least 4 tiles away
      enemies[n++] = { tx + 0.5f, ty + 0.5f, true,
                       (int)(2 + level + (int)currentDiff),
                       0.0f, false,
                       (float)(random(0, 628)) / 100.0f,
                       0.0f };
    }
    for (int i = n; i < MAX_ENEMIES; i++) enemies[i].alive = false;
  }

  bool allEnemiesDead() {
    for (int i = 0; i < MAX_ENEMIES; i++)
      if (enemies[i].alive) return false;
    return true;
  }

  void init() {
    // Pick a random pre-defined map
    mapIndex = random(0, 8);
    loadPredefMap(mapIndex);

    // Find player start: first open cell near top-left
    px = 1.5f; py = 1.5f;
    for (int ty = 1; ty < MAP_H - 1 && DMAP[(int)py * MAP_W + (int)px] != 0; ty++)
      for (int tx = 1; tx < MAP_W - 1; tx++)
        if (DMAP[ty * MAP_W + tx] == 0) { px = tx + 0.5f; py = ty + 0.5f; goto found_start; }
    found_start:
    pa = 0.0f;
    phealth = 1.0f;
    score = 0; level = 1;
    gameOver = false; newRecord = false;
    strafeMode = false; showMap = false;
    lastShot = 0; lastHurt = 0;
    muzzleFlash = false;
    gunRecoil = 0.0f;
    bobPhase  = 0.0f;
    bobAmt    = 0.0f;
    wasMoving = false;

    diffEnemyCount = (currentDiff == DIFF_EASY) ? 2
                   : (currentDiff == DIFF_HARD) ? 6 : 4;
    spawnEnemies(diffEnemyCount);
  }

  void tryMovePlayer(float nx, float ny) {
    if (!isWall((int)nx, (int)py)) px = nx;
    if (!isWall((int)px, (int)ny)) py = ny;
  }

  float castRay(float ox, float oy, float angle,
                bool* hitExit = nullptr, bool* hitSide = nullptr) {
    float rx = cosf(angle), ry = sinf(angle);
    float mapX = floorf(ox), mapY = floorf(oy);
    float deltaX = (rx == 0) ? 1e30f : fabsf(1.0f / rx);
    float deltaY = (ry == 0) ? 1e30f : fabsf(1.0f / ry);
    float sideX, sideY;
    int stepX, stepY;

    if (rx < 0) { stepX = -1; sideX = (ox - mapX) * deltaX; }
    else        { stepX =  1; sideX = (mapX + 1.0f - ox) * deltaX; }
    if (ry < 0) { stepY = -1; sideY = (oy - mapY) * deltaY; }
    else        { stepY =  1; sideY = (mapY + 1.0f - oy) * deltaY; }

    bool hit = false, side = false;
    int mx, my;
    for (int i = 0; i < 40 && !hit; i++) {
      if (sideX < sideY) { sideX += deltaX; mapX += stepX; side = false; }
      else               { sideY += deltaY; mapY += stepY; side = true;  }
      mx = (int)mapX; my = (int)mapY;
      if (isWall(mx, my)) hit = true;
      if (hitExit && isExit(mx, my)) { *hitExit = true; hit = true; }
    }

    if (hitSide) *hitSide = side;
    float dist = side ? (sideY - deltaY) : (sideX - deltaX);
    if (dist < 0.1f) dist = 0.1f;
    return dist;
  }

  void updateEnemies(float dt) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].alive) continue;
      Enemy& e = enemies[i];
      float dx = px - e.x, dy = py - e.y;
      float dist = sqrtf(dx*dx + dy*dy);
      if (dist < 0.05f) dist = 0.05f;

      e.stateTimer += dt;
      e.patrolTimer += dt;

      if (dist < 8.0f) {
        // Chase player
        float speed = (currentDiff == DIFF_HARD) ? 1.5f : (currentDiff == DIFF_EASY) ? 0.7f : 1.0f;
        float nx = e.x + (dx / dist) * speed * dt;
        float ny = e.y + (dy / dist) * speed * dt;
        if (!isWall((int)nx, (int)e.y)) e.x = nx;
        if (!isWall((int)e.x, (int)ny)) e.y = ny;

        // Melee attack
        if (dist < 0.7f && e.stateTimer > 1.0f) {
          e.stateTimer = 0;
          float dmg = (currentDiff == DIFF_EASY) ? 0.07f
                    : (currentDiff == DIFF_HARD) ? 0.20f : 0.12f;
          phealth -= dmg;
          sfxHit();
          lastHurt = millis();
          if (phealth <= 0) {
            gameOver = true;
            sfxGameOver();
            return;
          }
        }
      } else {
        // Patrol: wander in a direction, change every ~2s
        if (e.patrolTimer > 2.0f) {
          e.patrolAngle = (float)(random(0, 628)) / 100.0f;
          e.patrolTimer = 0;
        }
        float nx = e.x + cosf(e.patrolAngle) * 0.4f * dt;
        float ny = e.y + sinf(e.patrolAngle) * 0.4f * dt;
        if (isWall((int)nx, (int)e.y) || isWall((int)e.x, (int)ny)) {
          // Bounce: reverse and turn
          e.patrolAngle += 3.14159f + (float)(random(-50, 50)) / 100.0f;
          e.patrolTimer = 1.5f;  // pick new angle soon
        } else {
          e.x = nx;
          e.y = ny;
        }
      }
    }
  }

  void drawImpWithOutline(int screenX, int screenY, int sprW, int sprH, int viewH) {
    // Pass 1: black border
    for (int row = 0; row < IMP_H; row++) {
      uint8_t rowData = pgm_read_byte(&IMP_SPRITE[row]);
      int y0 = screenY + (row * sprH) / IMP_H;
      int y1 = screenY + ((row+1) * sprH) / IMP_H;
      for (int col = 0; col < IMP_W; col++) {
        if (!((rowData >> (7 - col)) & 1)) continue;
        int x0 = screenX + (col * sprW) / IMP_W;
        int x1 = screenX + ((col+1) * sprW) / IMP_W;
        for (int sy = y0-1; sy <= y1; sy++) {
          if (sy < 0 || sy >= viewH) continue;
          for (int sx = x0-1; sx <= x1; sx++) {
            if (sx < 0 || sx >= PLAY_W) continue;
            display.drawPixel(sx, sy, SSD1306_BLACK);
          }
        }
      }
    }
    // Pass 2: white sprite
    for (int row = 0; row < IMP_H; row++) {
      uint8_t rowData = pgm_read_byte(&IMP_SPRITE[row]);
      int y0 = screenY + (row * sprH) / IMP_H;
      int y1 = screenY + ((row+1) * sprH) / IMP_H;
      for (int col = 0; col < IMP_W; col++) {
        if (!((rowData >> (7 - col)) & 1)) continue;
        int x0 = screenX + (col * sprW) / IMP_W;
        int x1 = screenX + ((col+1) * sprW) / IMP_W;
        for (int sy = y0; sy < y1 && sy >= 0 && sy < viewH; sy++)
          for (int sx = x0; sx < x1 && sx >= 0 && sx < PLAY_W; sx++)
            display.drawPixel(sx, sy, SSD1306_WHITE);
      }
    }
  }

  // Compass: arrow points toward EXIT relative to player's current facing.
  // worldAngle = absolute direction to exit.
  // relAngle   = worldAngle minus pa = angle in player-local space.
  // On OLED: x right = cosf, y DOWN = sinf (screen y is inverted vs math y),
  // so we negate the y component when projecting onto the circle.
  void drawCompass(int cx, int cy, int r) {
    display.drawCircle(cx, cy, r, SSD1306_WHITE);
    float dx = EXIT_X - px;
    float dy = EXIT_Y - py;
    float worldAngle = atan2f(dy, dx);          // absolute bearing to exit
    float relAngle   = worldAngle - pa;         // rotate into player-local frame
    // Normalize to [-π, π]
    while (relAngle >  3.14159f) relAngle -= 6.28318f;
    while (relAngle < -3.14159f) relAngle += 6.28318f;
    // Project: screen-x = cos(rel), screen-y = -sin(rel) (OLED y is down)
    int ax = cx + (int)(cosf(relAngle) * (r - 1));
    int ay = cy - (int)(sinf(relAngle) * (r - 1));
    display.drawLine(cx, cy, ax, ay, SSD1306_WHITE);
    display.fillCircle(ax, ay, 1, SSD1306_WHITE);
    display.drawPixel(cx, cy - r, SSD1306_WHITE);  // 12 o'clock tick = "forward"
  }

  // ── Minimap: outline-only walls, 1px per 2 tiles ─────────
  // Each 2×2 tile block → 1 screen pixel. Wall cells draw a 1px
  // outline via drawRect on the corresponding mini-cell.
  void drawMinimap() {
    // Scale: 2 screen pixels per map tile for a bigger, more readable map.
    // MAP_W=20 → 40px wide, MAP_H=14 → 28px tall. Fits top-left corner.
    const int SCALE = 2;
    const int MW = MAP_W * SCALE;   // 40
    const int MH = MAP_H * SCALE;   // 28
    const int MX = 2;
    const int MY = 2;

    // Background + outer border
    display.fillRect(MX - 1, MY - 1, MW + 2, MH + 2, SSD1306_BLACK);
    display.drawRect(MX - 1, MY - 1, MW + 2, MH + 2, SSD1306_WHITE);

    // Draw each map tile as a SCALE×SCALE block.
    for (int ty = 0; ty < MAP_H; ty++) {
      for (int tx = 0; tx < MAP_W; tx++) {
        uint8_t cell = DMAP[ty * MAP_W + tx];
        int sx = MX + tx * SCALE;
        int sy = MY + ty * SCALE;
        if (cell == 1) {
          // Wall: fill solid
          display.fillRect(sx, sy, SCALE, SCALE, SSD1306_WHITE);
        } else if (cell == 2) {
          // Exit: blink
          if ((millis() / 300) & 1)
            display.fillRect(sx, sy, SCALE, SCALE, SSD1306_WHITE);
          else
            display.drawRect(sx, sy, SCALE, SCALE, SSD1306_WHITE);
        }
        // Floor: leave black
      }
    }

    // Player dot (scale player position to minimap coords)
    int ppx = MX + (int)(px * SCALE);
    int ppy = MY + (int)(py * SCALE);
    ppx = constrain(ppx, MX, MX + MW - 1);
    ppy = constrain(ppy, MY, MY + MH - 1);
    display.fillCircle(ppx, ppy, 1, SSD1306_WHITE);
    // Direction tick (2 pixels ahead)
    int tdx = ppx + (int)(cosf(pa) * 2.5f);
    int tdy = ppy + (int)(sinf(pa) * 2.5f);
    tdx = constrain(tdx, MX, MX + MW - 1);
    tdy = constrain(tdy, MY, MY + MH - 1);
    display.drawPixel(tdx, tdy, SSD1306_WHITE);
  }

  void update() {
    if (gameOver) return;

    unsigned long now = millis();
    float dt = FRAME_MS / 1000.0f;

    // Joystick SW toggles minimap
    if (btnJoySW.pressed) showMap = !showMap;

    float sensScale = 0.6f + joyMoveStep() * 0.2f; // 0.8 at step=1, 1.0 at step=2, 1.2 at step=3
    float moveSpd  = ((currentDiff == DIFF_HARD) ? 2.2f : (currentDiff == DIFF_EASY) ? 1.6f : 1.9f) * sensScale;
    float turnSpd  = 3.4f * sensScale;

    bool moving = false;
    bool strafing = btnB.held;   // B HOLD = strafe mode (no toggle)

    if (strafing) {
      // Strafe: L/R slide (flipped per user request), U/D move
      if (joyLeft())  { tryMovePlayer(px + sinf(pa)*moveSpd*dt, py - cosf(pa)*moveSpd*dt); moving = true; }
      if (joyRight()) { tryMovePlayer(px - sinf(pa)*moveSpd*dt, py + cosf(pa)*moveSpd*dt); moving = true; }
      if (joyUp())    { tryMovePlayer(px + cosf(pa)*moveSpd*dt, py + sinf(pa)*moveSpd*dt); moving = true; }
      if (joyDown())  { tryMovePlayer(px - cosf(pa)*moveSpd*dt, py - sinf(pa)*moveSpd*dt); moving = true; }
    } else {
      // Normal: U/D move, L/R turn
      if (joyUp())    { tryMovePlayer(px + cosf(pa)*moveSpd*dt, py + sinf(pa)*moveSpd*dt); moving = true; }
      if (joyDown())  { tryMovePlayer(px - cosf(pa)*moveSpd*dt, py - sinf(pa)*moveSpd*dt); moving = true; }
      if (joyLeft())  pa -= turnSpd * dt;
      if (joyRight()) pa += turnSpd * dt;
    }

    // Screen bob
    float targetBob = moving ? 1.0f : 0.0f;
    bobAmt += (targetBob - bobAmt) * 0.15f;
    if (moving) bobPhase += 6.0f * dt;
    wasMoving = moving;

    // Shoot
    unsigned long shootCd = (currentDiff == DIFF_HARD) ? 250UL : 400UL;
    if (btnA.pressed && now - lastShot > shootCd) {
      lastShot = now;
      sfxShoot();
      muzzleFlash = true;
      muzzleEnd = now + 90;
      gunRecoil = 4.0f;

      bool hitExit = false;
      float shotDist = castRay(px, py, pa, &hitExit);
      for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        float ex = enemies[i].x - px, ey = enemies[i].y - py;
        float eDist = sqrtf(ex*ex + ey*ey);
        float eAngle = atan2f(ey, ex);
        float angleDiff = eAngle - pa;
        while (angleDiff >  3.14159f) angleDiff -= 6.28318f;
        while (angleDiff < -3.14159f) angleDiff += 6.28318f;
        if (fabsf(angleDiff) < 0.28f && eDist < shotDist + 0.5f) {
          int dmg = 1 + (int)currentDiff;
          enemies[i].hp -= dmg;
          sfxExplode();
          if (enemies[i].hp <= 0) {
            enemies[i].alive = false;
            score += 100 * level;
            sfxLevelUp();
            if (score > highScores[selectedGame][(int)currentDiff]) {
              saveScore(selectedGame, (int)currentDiff, score);
              newRecord = true;
            }
          }
        }
      }
    }

    if (muzzleFlash && now > muzzleEnd) muzzleFlash = false;

    // Gun recoil return
    if (gunRecoil > 0.0f) {
      gunRecoil -= dt * 22.0f;
      if (gunRecoil < 0.0f) gunRecoil = 0.0f;
    }

    updateEnemies(dt);

    // All enemies dead → next wave on same map
    if (allEnemiesDead()) {
      score += 50 * level;
      sfxLevelUp();
      int nextCount = min(diffEnemyCount + (level / 2), MAX_ENEMIES);
      spawnEnemies(nextCount);
      phealth = min(1.0f, phealth + 0.3f);
    }

    // Step on exit → new map
    if (isExit((int)px, (int)py)) {
      level++;
      score += 200 * level;
      sfxLevelUp();
      mapIndex = random(0, 8);
      loadPredefMap(mapIndex);
      // Find new player start
      px = 1.5f; py = 1.5f;
      for (int ty = 1; ty < MAP_H - 1; ty++)
        for (int tx = 1; tx < MAP_W - 1; tx++)
          if (DMAP[ty * MAP_W + tx] == 0) { px = tx + 0.5f; py = ty + 0.5f; goto new_start; }
      new_start:
      pa = 0.0f;
      int nextCount = min(diffEnemyCount + (level / 2), MAX_ENEMIES);
      spawnEnemies(nextCount);
      phealth = min(1.0f, phealth + 0.5f);
    }
  }

  void draw() {
    display.clearDisplay();

    int viewH = PLAY_H - VIEW_HUD_H;
    int halfH = viewH / 2;

    int bobOff = (int)(sinf(bobPhase) * 2.0f * bobAmt);

    // ── Ceiling: sparse dither (darker = more immersive) ──
    // No ceiling fill — keep it black for performance and contrast

    // ── Floor: scanline pattern (every 3rd row) ───────────
    // Cleaner than dither — less pixel noise, clear floor/wall split
    for (int y = halfH + 1; y < viewH; y++)
      if (y % 3 == 0)
        display.drawFastHLine(0, y, PLAY_W, SSD1306_WHITE);

    // ── Wall raycasting ───────────────────────────────────
    static float zBuf[128];
    float fov = 1.0f;

    for (int col = 0; col < PLAY_W; col++) {
      float rayAngle = pa - fov * 0.5f + fov * (float)col / (float)PLAY_W;
      bool hitExit = false, hitSide = false;
      float dist = castRay(px, py, rayAngle, &hitExit, &hitSide);
      dist *= cosf(rayAngle - pa);   // fisheye correction
      if (dist < 0.1f) dist = 0.1f;
      zBuf[col] = dist;

      int lineH = (int)(viewH / dist);
      if (lineH > viewH) lineH = viewH;

      int drawTop = halfH - lineH / 2 + bobOff;
      int drawBot = halfH + lineH / 2 + bobOff;
      drawTop = max(0, drawTop);
      drawBot = min(viewH, drawBot);

      // Distance-based shading: 4 bands
      int shade;
      if (hitExit) {
        shade = 0;  // Exit always bright
      } else {
        shade = (dist < 1.5f) ? 0 : (dist < 3.5f) ? 1 : (dist < 6.0f) ? 2 : 3;
      }
      // N/S wall faces are phase-shifted for visual distinction
      int phase = hitSide ? 2 : 0;

      for (int y = drawTop; y < drawBot; y++)
        if (ditherPixel(col, y, shade, phase)) display.drawPixel(col, y, SSD1306_WHITE);
    }

    // ── Sprites (enemies) ─────────────────────────────────
    int order[MAX_ENEMIES];
    float dists[MAX_ENEMIES];
    for (int i = 0; i < MAX_ENEMIES; i++) {
      order[i] = i;
      float dx = enemies[i].x - px, dy = enemies[i].y - py;
      dists[i] = dx*dx + dy*dy;
    }
    // Sort back-to-front
    for (int i = 0; i < MAX_ENEMIES-1; i++)
      for (int j = i+1; j < MAX_ENEMIES; j++)
        if (dists[order[j]] > dists[order[i]]) { int t=order[i]; order[i]=order[j]; order[j]=t; }

    for (int si = 0; si < MAX_ENEMIES; si++) {
      int i = order[si];
      if (!enemies[i].alive) continue;

      float sdx = enemies[i].x - px, sdy = enemies[i].y - py;
      float spriteDist = sqrtf(sdx*sdx + sdy*sdy);
      if (spriteDist < 0.2f) spriteDist = 0.2f;

      float spriteAngle = atan2f(sdy, sdx) - pa;
      while (spriteAngle >  3.14159f) spriteAngle -= 6.28318f;
      while (spriteAngle < -3.14159f) spriteAngle += 6.28318f;
      if (fabsf(spriteAngle) > fov*0.5f + 0.2f) continue;

      int spriteScreenX = (int)((0.5f + spriteAngle / fov) * PLAY_W);
      int spriteH = (int)(viewH / spriteDist);
      if (spriteH > viewH * 2) spriteH = viewH * 2;  // clamp: very close = large but visible
      if (spriteH < 2) continue;                      // was < 4; lower cutoff prevents disappear
      int spriteW = spriteH * IMP_W / IMP_H;

      int drawStartY = halfH - spriteH/2 + bobOff;
      int drawEndY   = halfH + spriteH/2 + bobOff;
      if (drawStartY < 0)     drawStartY = 0;
      if (drawEndY   > viewH) drawEndY   = viewH;

      int drawStartX = spriteScreenX - spriteW/2;
      int drawEndX   = spriteScreenX + spriteW/2;

      bool visible = false;
      for (int x = max(0, drawStartX); x < min(PLAY_W, drawEndX); x++) {
        if (zBuf[x] >= spriteDist) { visible = true; break; }
      }
      if (!visible) continue;

      drawImpWithOutline(drawStartX, drawStartY, spriteW, spriteH, viewH);

      // ── Enemy HP bar (12×3, above sprite, visible up to 8 tiles) ─
      if (spriteDist <= 8.0f) {
        int maxHp = 2 + level + (int)currentDiff;
        if (maxHp < 1) maxHp = 1;
        int barW = 12;
        int barX = spriteScreenX - barW / 2;
        int barY = drawStartY - 5;
        if (barY >= 0 && barX >= 0 && barX + barW <= PLAY_W) {
          // Black background for contrast
          display.fillRect(barX - 1, barY - 1, barW + 2, 5, SSD1306_BLACK);
          // Outline
          display.drawRect(barX, barY, barW, 3, SSD1306_WHITE);
          // Fill proportional to remaining HP
          int fillW = (int)((float)barW * enemies[i].hp / maxHp);
          if (fillW > barW) fillW = barW;
          if (fillW > 0) display.fillRect(barX + 1, barY + 1, fillW - 1, 1, SSD1306_WHITE);
        }
      }

    }  // end sprite loop

    // ── Shotgun with recoil ───────────────────────────────
    int gunX  = PLAY_W / 2 - 4;
    int gunY  = viewH - 9 - (int)gunRecoil;

    display.fillRect(gunX,     gunY + 5, 12, 4, SSD1306_WHITE);  // stock
    display.fillRect(gunX + 1, gunY + 2, 9,  4, SSD1306_WHITE);  // receiver
    display.fillRect(gunX + 3, gunY,     4,  3, SSD1306_WHITE);  // barrel
    display.drawRect(gunX + 2, gunY + 3, 3,  2, SSD1306_WHITE);  // pump grip
    display.drawPixel(gunX + 9, gunY + 6, SSD1306_BLACK);
    display.drawPixel(gunX + 10,gunY + 6, SSD1306_BLACK);

    if (muzzleFlash) {
      display.fillRect(gunX + 4, gunY - 5, 2, 5, SSD1306_WHITE);
      display.drawPixel(gunX + 2, gunY - 4, SSD1306_WHITE);
      display.drawPixel(gunX + 7, gunY - 4, SSD1306_WHITE);
      display.drawPixel(gunX + 3, gunY - 6, SSD1306_WHITE);
      display.drawPixel(gunX + 6, gunY - 6, SSD1306_WHITE);
      display.drawPixel(gunX + 5, gunY - 7, SSD1306_WHITE);
    }

    // ── Hurt flash ────────────────────────────────────────
    if (millis() - lastHurt < 200) {
      for (int y = viewH/2-4; y < viewH/2+4; y++)
        for (int x = PLAY_W/2-4; x < PLAY_W/2+4; x++)
          display.drawPixel(x, y, SSD1306_WHITE);
    }

    // ── HUD ───────────────────────────────────────────────
    int hudY = viewH;
    display.drawLine(0, hudY, PLAY_W-1, hudY, SSD1306_WHITE);
    int hpBarW = (int)(phealth * 44);
    display.setCursor(0, hudY + 2);
    display.setTextSize(1);
    display.print("HP");
    display.drawRect(12, hudY + 2, 46, 5, SSD1306_WHITE);
    if (hpBarW > 0) display.fillRect(13, hudY + 3, hpBarW, 3, SSD1306_WHITE);
    char buf[16]; sprintf(buf, "L%d S%d", level, score);
    display.setCursor(60, hudY + 2);
    display.print(buf);
    drawCompass(PLAY_W - 6, hudY + 6, 5);

    // Strafe indicator (B held)
    if (btnB.held) {
      display.setCursor(PLAY_W - 14, 0);
      display.setTextColor(SSD1306_WHITE);
      display.print("ST");
    }

    if (showMap) drawMinimap();

    display.display();
  }

  bool isOver()      { return gameOver; }
  int  getScore()    { return score; }
  bool isNewRecord() { return newRecord; }
}
