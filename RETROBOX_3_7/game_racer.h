// ============================================================
//  game_racer.h  —  RETROBOX built-in game
//  Top-Down Racer — 3 lanes, fuel, nitro, obstacles
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

namespace RacerGame {
  static int ROAD_L, ROAD_R, ROAD_MID, CAR_Y;
  static const int CAR_W  = 8;
  static const int CAR_H  = 10;
  static const int MAX_CARS = 5;
  static const int MAX_TREES = 6;
  static const int NITRO_MAX = 3;

  struct OtherCar { int x, y; bool alive; int type; };
  struct FuelPickup { int x, y; bool alive; };
  struct Tree { int x, y; };

  static int   carX;
  static int   score, lives, level, speed, frameCount;
  static bool  gameOver, newRecord;
  static OtherCar others[MAX_CARS];
  static FuelPickup fuels[2];
  static Tree  trees[MAX_TREES];
  static int   roadOffset;
  static float diffMult;
  static int   fuel;
  static int   nitro;
  static bool  nitroing;
  static unsigned long nitroEnd;
  static int   damage;
  static unsigned long lastFuelDrain;
  static unsigned long lastRamp;

  int laneX(int lane) {
    int laneW = (ROAD_R - ROAD_L) / 3;
    return ROAD_L + lane * laneW + (laneW - CAR_W) / 2;
  }
  static int carLanes[MAX_CARS];

  void spawnOtherCar(int idx) {
    int laneCounts[3] = {0, 0, 0};
    for (int i = 0; i < MAX_CARS; i++) {
      if (i != idx && others[i].alive && carLanes[i] >= 0 && carLanes[i] < 3)
        laneCounts[carLanes[i]]++;
    }
    int best = 0;
    for (int l = 1; l < 3; l++)
      if (laneCounts[l] < laneCounts[best]) best = l;
    if (laneCounts[best] > 0 && random(0,3) == 0) best = random(0, 3);
    carLanes[idx] = best;
    int yOff = CAR_H + 20 + idx * 28 + random(0, 20);
    others[idx] = { laneX(best), -yOff, true, random(0, 2) };
  }
  void spawnFuel(int idx) {
    int lane = random(0, 3);
    fuels[idx] = { laneX(lane) + CAR_W/4, -random(20, 80), true };
  }
  void initTrees() {
    for (int i = 0; i < MAX_TREES; i++) {
      if (ROAD_L < 4 || ROAD_R > PLAY_W - 4) {
        trees[i].x = -10; trees[i].y = -10;
      } else {
        trees[i].x = (i % 2 == 0) ? random(2, ROAD_L - 4) : random(ROAD_R + 2, PLAY_W - 4);
        trees[i].y = random(8, PLAY_H);
      }
    }
  }

  void init() {
    int roadMargin = (PLAY_W < 80) ? 0 : PLAY_W / 5;
    ROAD_L   = roadMargin;
    ROAD_R   = PLAY_W - roadMargin;
    ROAD_MID = PLAY_W / 2;
    CAR_Y    = PLAY_H - 14;
    diffMult = (currentDiff == DIFF_EASY) ? 0.55f : (currentDiff == DIFF_HARD) ? 1.3f : 1.0f;
    carX = ROAD_MID - CAR_W / 2;
    score = 0; lives = 3; level = 1; gameOver = false; newRecord = false;
    speed = (currentDiff == DIFF_EASY) ? 1 : (currentDiff == DIFF_HARD) ? 3 : 2;
    frameCount = 0; roadOffset = 0;
    fuel = 100; nitro = NITRO_MAX; nitroing = false; damage = 0;
    lastFuelDrain = millis();
    lastRamp = millis();
    memset(carLanes, -1, sizeof(carLanes));
    for (int i = 0; i < MAX_CARS; i++) spawnOtherCar(i);
    for (int i = 0; i < 2; i++) spawnFuel(i);
    initTrees();
  }

  void drawCar(int x, int y, bool isPlayer, int dmg) {
    display.fillRect(x + 1, y, CAR_W - 2, CAR_H, SSD1306_WHITE);
    if (isPlayer) {
      display.fillRect(x + 2, y + 2, CAR_W - 4, 4, SSD1306_BLACK);
      if (dmg == 1) {
        display.drawPixel(x + 2, y + 2, SSD1306_WHITE);
        display.drawPixel(x + 5, y + 4, SSD1306_WHITE);
      } else if (dmg >= 2) {
        display.fillRect(x + 2, y + 2, CAR_W - 4, 4, SSD1306_WHITE);
        display.drawPixel(x + 3, y + 3, SSD1306_BLACK);
        display.drawPixel(x + 4, y + 3, SSD1306_BLACK);
      }
    } else {
      display.fillRect(x + 2, y + 3, CAR_W - 4, 3, SSD1306_BLACK);
    }
    display.drawPixel(x,         y + 1, SSD1306_WHITE);
    display.drawPixel(x,         y + 3, SSD1306_WHITE);
    display.drawPixel(x + CAR_W - 1, y + 1, SSD1306_WHITE);
    display.drawPixel(x + CAR_W - 1, y + 3, SSD1306_WHITE);
  }

  void drawTree(int x, int y) {
    display.fillRect(x + 1, y,     3, 5, SSD1306_WHITE);
    display.fillRect(x + 2, y + 4, 1, 3, SSD1306_WHITE);
  }

  void update() {
    if (gameOver) return;
    frameCount++;
    unsigned long now = millis();

    unsigned long rampMs = (unsigned long)(currentDiff == DIFF_EASY ? 15000 : currentDiff == DIFF_HARD ? 6000 : 10000);
    int maxSpeed  = (currentDiff == DIFF_EASY) ? 4 : (currentDiff == DIFF_HARD) ? 8 : 6;
    if (now - lastRamp > rampMs && speed < maxSpeed) {
      speed++; level++; sfxBeep(); lastRamp = now;
    }

    int steerSpd = (nitroing ? 3 : 2) + (joyMoveStep() - 2); // sens scales 1..3 extra
    steerSpd = constrain(steerSpd, 1, 5);
    if (joyLeft()  && carX > ROAD_L + 2)          carX -= steerSpd;
    if (joyRight() && carX < ROAD_R - CAR_W - 2)  carX += steerSpd;

    if (btnA.pressed && nitro > 0 && !nitroing) {
      nitroing = true;
      nitroEnd = now + 800;
      nitro--;
      sfxLevelUp();
    }
    if (nitroing && now > nitroEnd) nitroing = false;

    int effectiveSpeed = nitroing ? speed + 3 : speed;
    roadOffset = (roadOffset + effectiveSpeed) % 16;

    // Drain intervals: midpoint between previous fast (150/95/55ms) and old slow (600/400/220ms)
    // Easy: ~375ms/2u (~14s empty), Medium: ~245ms/2u (~12s empty), Hard: ~140ms/2u (~7s empty)
    int drainInterval = (currentDiff == DIFF_EASY) ? 375 : (currentDiff == DIFF_HARD) ? 140 : 245;
    if (now - lastFuelDrain > (unsigned long)drainInterval) {
      fuel -= 2;   // drain 2 units per tick
      lastFuelDrain = now;
      if (fuel <= 0) {
        fuel = 0;
        gameOver = true;
        sfxGameOver();
        saveScore(selectedGame, (int)currentDiff, score);
        return;
      }
    }

    for (int i = 0; i < MAX_CARS; i++) {
      others[i].y += effectiveSpeed;
      if (others[i].y > PLAY_H) spawnOtherCar(i);

      if (others[i].y + CAR_H > CAR_Y && others[i].y < CAR_Y + CAR_H &&
          others[i].x + CAR_W > carX  && others[i].x < carX + CAR_W) {
        sfxExplode();
        damage++;
        lives--;
        others[i].y = -CAR_H - 30;
        others[i].x = ROAD_MID - CAR_W / 2;
        if (lives <= 0) {
          gameOver = true; sfxGameOver();
          saveScore(selectedGame, (int)currentDiff, score);
          return;
        }
      }
    }

    for (int i = 0; i < 2; i++) {
      fuels[i].y += effectiveSpeed;
      if (fuels[i].y > PLAY_H) spawnFuel(i);
      if (fuels[i].alive &&
          fuels[i].y + 5 > CAR_Y && fuels[i].y < CAR_Y + CAR_H &&
          abs(fuels[i].x - carX) < 10) {
        fuel = min(100, fuel + 35);
        fuels[i].alive = false;
        sfxBeep();
        spawnFuel(i);
      }
    }

    for (int i = 0; i < MAX_TREES; i++) {
      trees[i].y += effectiveSpeed;
      if (trees[i].y > PLAY_H) {
        trees[i].y = -8;
        if (ROAD_L < 4 || ROAD_R > PLAY_W - 4) {
          trees[i].x = -10;
        } else {
          trees[i].x = (i % 2 == 0) ? random(2, ROAD_L - 4) : random(ROAD_R + 2, PLAY_W - 4);
        }
      }
    }

    score = frameCount / 3 * speed;
    if (score > highScores[selectedGame][(int)currentDiff]) {
      saveScore(selectedGame, (int)currentDiff, score);
      newRecord = true;
    }
  }

  void draw() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    char buf[12];
    display.setCursor(0, 0);
    display.print("S:"); display.print(score);
    sprintf(buf, "L%d", level);
    display.setCursor(PLAY_W - 18, 0);
    display.print(buf);

    for (int i = 0; i < lives; i++) {
      display.fillRect(PLAY_W / 2 - 10 + i * 7, 1, 5, 5, SSD1306_WHITE);
    }

    int fuelW = (int)(fuel / 100.0f * (PLAY_W / 2 - 4));
    display.drawRect(0, PLAY_H - 5, PLAY_W / 2 - 2, 5, SSD1306_WHITE);
    display.fillRect(1, PLAY_H - 4, fuelW, 3, SSD1306_WHITE);

    for (int i = 0; i < NITRO_MAX; i++) {
      int nx = PLAY_W / 2 + 2 + i * 7;
      if (i < nitro) display.fillRect(nx, PLAY_H - 4, 5, 3, SSD1306_WHITE);
      else           display.drawRect(nx, PLAY_H - 4, 5, 3, SSD1306_WHITE);
    }
    if (nitroing) {
      display.setCursor(PLAY_W / 2 + 22, PLAY_H - 5);
      display.print("N!");
    }

    display.drawLine(0, 8, PLAY_W - 1, 8, SSD1306_WHITE);

    for (int i = 0; i < MAX_TREES; i++) drawTree(trees[i].x, trees[i].y);

    display.drawLine(ROAD_L, 8, ROAD_L, PLAY_H - 6, SSD1306_WHITE);
    display.drawLine(ROAD_R, 8, ROAD_R, PLAY_H - 6, SSD1306_WHITE);

    int laneW = (ROAD_R - ROAD_L) / 3;
    for (int l = 1; l <= 2; l++) {
      int lx = ROAD_L + l * laneW;
      for (int y = 8 + roadOffset; y < PLAY_H - 6; y += 14)
        display.fillRect(lx, y, 1, 6, SSD1306_WHITE);
    }

    for (int i = 0; i < 2; i++) {
      if (!fuels[i].alive) continue;
      display.fillRect(fuels[i].x, fuels[i].y, 6, 6, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(fuels[i].x + 1, fuels[i].y);
      display.print("F");
      display.setTextColor(SSD1306_WHITE);
    }

    for (int i = 0; i < MAX_CARS; i++)
      if (others[i].y > 8 && others[i].y < PLAY_H - 6)
        drawCar(others[i].x, others[i].y, false, 0);

    drawCar(carX, CAR_Y, true, damage);

    display.display();
  }

  bool isOver()      { return gameOver; }
  int  getScore()    { return score; }
  bool isNewRecord() { return newRecord; }
}
