// ============================================================
//  game_vshooter.h  —  RETROBOX built-in game
//  Vertical Shooter — enemy waves, bombs, boss
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

namespace VShooterGame {
  static const int PLAYER_W  = 8;
  static const int PLAYER_H  = 6;
  static const int MAX_PB    = 4;
  static const int MAX_EB    = 6;
  static const int MAX_ENEMIES = 12;

  struct Bullet { int x, y; bool alive; };
  struct Enemy  { int x, y; bool alive; int type; float vx, vy; int hp; };

  static int   playerX, playerY;
  static Bullet pb[MAX_PB], eb[MAX_EB];
  static Enemy  enemies[MAX_ENEMIES];
  static int   score, lives, wave, kills;
  static bool  gameOver, newRecord;
  static int   bombCharges;
  static float diffMult;
  static unsigned long lastPShot, lastESpawn, lastEShot;

  static int sx[20], sy[20];
  static bool starsInit = false;

  void initVStars() {
    if (starsInit) return;
    for (int i = 0; i < 20; i++) { sx[i] = random(0, PLAY_W); sy[i] = random(0, PLAY_H); }
    starsInit = true;
  }

  void spawnEnemy(int idx) {
    int type = (wave >= 3) ? random(0, 3) : random(0, 2);
    float spd = 1.0f + wave * 0.2f * diffMult;
    if (spd > 3.0f) spd = 3.0f;
    enemies[idx] = { random(4, PLAY_W - 10), -8, true, type, 0, spd, (type == 2) ? 3 : 1 };
    if (type == 1) enemies[idx].vx = (random(0, 2) ? 1.0f : -1.0f) * spd * 0.5f;
  }

  void init() {
    diffMult = (currentDiff == DIFF_EASY) ? 0.6f : (currentDiff == DIFF_HARD) ? 1.4f : 1.0f;
    playerX = PLAY_W / 2 - PLAYER_W / 2;
    playerY = PLAY_H - PLAYER_H - 4;
    score = 0; lives = 3; wave = 1; kills = 0;
    gameOver = false; newRecord = false;
    bombCharges = 2;
    for (int i = 0; i < MAX_PB; i++) pb[i].alive = false;
    for (int i = 0; i < MAX_EB; i++) eb[i].alive = false;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].alive = false;
    lastPShot = lastESpawn = lastEShot = 0;
    starsInit = false;
    initVStars();
  }

  void triggerBomb() {
    if (bombCharges <= 0) return;
    bombCharges--;
    for (int i = 0; i < MAX_ENEMIES; i++) if (enemies[i].alive) { enemies[i].alive = false; score += 50; kills++; sfxExplode(); }
    for (int i = 0; i < MAX_EB; i++) eb[i].alive = false;
    sfxLevelUp();
  }

  void update() {
    if (gameOver) return;
    unsigned long now = millis();

    if (joyLeft()  && playerX > 2)               playerX -= joyMoveStep();
    if (joyRight() && playerX < PLAY_W - PLAYER_W - 2) playerX += joyMoveStep();
    if (joyUp()    && playerY > PLAY_H / 2)      playerY -= joyMoveStep();
    if (joyDown()  && playerY < PLAY_H - PLAYER_H - 2) playerY += joyMoveStep();

    int fireInterval = (int)(250 / diffMult);
    if ((btnA.held || btnJoySW.held) && now - lastPShot > (unsigned long)fireInterval) {
      lastPShot = now;
      sfxShoot();
      for (int i = 0; i < MAX_PB; i++) {
        if (!pb[i].alive) { pb[i] = { playerX + PLAYER_W / 2, playerY - 2, true }; break; }
      }
    }

    if (btnB.pressed) triggerBomb();

    for (int i = 0; i < MAX_PB; i++) {
      if (!pb[i].alive) continue;
      pb[i].y -= 4;
      if (pb[i].y < 0) pb[i].alive = false;
    }

    for (int i = 0; i < MAX_EB; i++) {
      if (!eb[i].alive) continue;
      eb[i].y += 2;
      if (eb[i].y > PLAY_H) eb[i].alive = false;
    }

    int maxOnScreen = 2 + wave;
    if (maxOnScreen > MAX_ENEMIES) maxOnScreen = MAX_ENEMIES;
    int aliveCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) if (enemies[i].alive) aliveCount++;
    int spawnInterval = (int)(1200 / diffMult) - wave * 60;
    if (spawnInterval < 300) spawnInterval = 300;
    if (aliveCount < maxOnScreen && now - lastESpawn > (unsigned long)spawnInterval) {
      for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) { spawnEnemy(i); lastESpawn = now; break; }
      }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].alive) continue;
      enemies[i].x += (int)enemies[i].vx;
      enemies[i].y += (int)enemies[i].vy;
      if (enemies[i].x <= 0 || enemies[i].x >= PLAY_W - 8) enemies[i].vx = -enemies[i].vx;
      if (enemies[i].y > PLAY_H) { enemies[i].alive = false; }
    }

    int eShootInterval = (int)(2000 / diffMult) - wave * 80;
    if (eShootInterval < 500) eShootInterval = 500;
    if (now - lastEShot > (unsigned long)eShootInterval) {
      lastEShot = now;
      for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive && enemies[i].type != 0) {
          for (int j = 0; j < MAX_EB; j++) {
            if (!eb[j].alive) { eb[j] = { enemies[i].x + 4, enemies[i].y + 6, true }; break; }
          }
          break;
        }
      }
    }

    for (int b = 0; b < MAX_PB; b++) {
      if (!pb[b].alive) continue;
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (!enemies[e].alive) continue;
        if (abs(pb[b].x - enemies[e].x - 4) < 6 && abs(pb[b].y - enemies[e].y - 4) < 6) {
          pb[b].alive = false;
          enemies[e].hp--;
          if (enemies[e].hp <= 0) {
            enemies[e].alive = false;
            sfxExplode();
            score += (enemies[e].type + 1) * 20 * wave;
            kills++;
            if (score > highScores[selectedGame][(int)currentDiff]) {
              saveScore(selectedGame, (int)currentDiff, score); newRecord = true;
            }
          } else sfxHit();
          break;
        }
      }
    }

    for (int i = 0; i < MAX_EB; i++) {
      if (!eb[i].alive) continue;
      if (eb[i].y + 3 >= playerY && eb[i].y <= playerY + PLAYER_H &&
          eb[i].x >= playerX - 2 && eb[i].x <= playerX + PLAYER_W + 2) {
        eb[i].alive = false;
        sfxExplode(); lives--;
        if (lives <= 0) { gameOver = true; sfxGameOver(); return; }
      }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].alive) continue;
      if (enemies[i].y + 8 >= playerY && enemies[i].y <= playerY + PLAYER_H &&
          abs(enemies[i].x - playerX) < PLAYER_W + 2) {
        enemies[i].alive = false;
        sfxExplode(); lives--;
        if (lives <= 0) { gameOver = true; sfxGameOver(); return; }
      }
    }

    if (kills >= wave * 10) {
      wave++;
      sfxLevelUp();
      kills = 0;
      bombCharges = min(bombCharges + 1, 3);
    }
  }

  void drawPlayerShip(int x, int y) {
    display.drawLine(x + PLAYER_W / 2, y, x, y + PLAYER_H, SSD1306_WHITE);
    display.drawLine(x + PLAYER_W / 2, y, x + PLAYER_W, y + PLAYER_H, SSD1306_WHITE);
    display.drawLine(x, y + PLAYER_H, x + PLAYER_W, y + PLAYER_H, SSD1306_WHITE);
    display.fillRect(x + PLAYER_W / 2 - 1, y + 1, 2, 3, SSD1306_WHITE);
  }

  void drawEnemy(int x, int y, int type) {
    switch (type) {
      case 0:
        display.fillRect(x + 1, y,     6, 4, SSD1306_WHITE);
        display.drawPixel(x,     y,     SSD1306_WHITE);
        display.drawPixel(x + 7, y,     SSD1306_WHITE);
        display.drawPixel(x + 3, y + 4, SSD1306_WHITE);
        display.drawPixel(x + 4, y + 4, SSD1306_WHITE);
        break;
      case 1:
        display.drawLine(x + 4, y, x, y + 5, SSD1306_WHITE);
        display.drawLine(x + 4, y, x + 7, y + 5, SSD1306_WHITE);
        display.fillRect(x + 2, y + 2, 4, 3, SSD1306_WHITE);
        break;
      case 2:
        display.drawRect(x, y, 9, 7, SSD1306_WHITE);
        display.fillRect(x + 2, y + 2, 5, 3, SSD1306_WHITE);
        display.drawPixel(x + 4, y + 3, SSD1306_BLACK);
        break;
    }
  }

  void draw() {
    display.clearDisplay();
    initVStars();
    for (int i = 0; i < 20; i++) {
      display.drawPixel(sx[i], sy[i], SSD1306_WHITE);
      sy[i] += 1;
      if (sy[i] >= PLAY_H) { sy[i] = 0; sx[i] = random(0, PLAY_W); }
    }

    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0); display.print("S:"); display.print(score);
    char buf[12]; sprintf(buf, "W%d", wave);
    display.setCursor(PLAY_W / 2 - 6, 0); display.print(buf);
    display.setCursor(PLAY_W - 12, 0);
    for (int i = 0; i < lives; i++) display.print("^");

    display.setCursor(0, PLAY_H - 7);
    display.print("B:");
    for (int i = 0; i < bombCharges; i++) display.print("*");

    for (int i = 0; i < MAX_ENEMIES; i++)
      if (enemies[i].alive) drawEnemy(enemies[i].x, enemies[i].y, enemies[i].type);
    for (int i = 0; i < MAX_EB; i++)
      if (eb[i].alive) display.fillRect(eb[i].x, eb[i].y, 2, 3, SSD1306_WHITE);
    for (int i = 0; i < MAX_PB; i++)
      if (pb[i].alive) display.fillRect(pb[i].x - 1, pb[i].y, 2, 4, SSD1306_WHITE);

    drawPlayerShip(playerX, playerY);
    display.display();
  }

  bool isOver()      { return gameOver; }
  int  getScore()    { return score; }
  bool isNewRecord() { return newRecord; }
}
