// ============================================================
//  game_snake.h  —  RETROBOX built-in game
//  Snake — grow, 3 lives, speed increases
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

namespace SnakeGame {
  static const int CELL = 4;
  static int COLS, ROWS;
  static const int HUD_H = 10;
  static const int MAX_LEN = 512;

  struct Pt { int16_t x, y; };
  static Pt   snake[MAX_LEN];
  static int  snakeLen;
  static Pt   food;
  static int  dx, dy;
  static int  score, level, lives;
  static bool gameOver, newRecord;
  static unsigned long lastMove;
  static int  moveInterval;

  void placeFood() {
    bool ok = false;
    while (!ok) {
      food.x = random(0, COLS);
      food.y = random(0, ROWS);
      ok = true;
      for (int i = 0; i < snakeLen; i++)
        if (snake[i].x == food.x && snake[i].y == food.y) { ok = false; break; }
    }
  }

  void init() {
    COLS = PLAY_W / CELL;
    ROWS = (PLAY_H - HUD_H) / CELL;
    snakeLen = 3;
    dx = 1; dy = 0;
    int mx = COLS / 2, my = ROWS / 2;
    for (int i = 0; i < snakeLen; i++) snake[i] = {(int16_t)(mx - i), (int16_t)my};
    score = 0; level = 1; lives = 3;
    gameOver = false; newRecord = false;
    moveInterval = (currentDiff == DIFF_EASY) ? 200 : (currentDiff == DIFF_HARD) ? 100 : 150;
    placeFood();
    lastMove = millis();
  }

  void update() {
    if (gameOver) return;
    static int ndx = 1, ndy = 0;
    if (joyLeft()  && dx ==  0) { ndx = -1; ndy =  0; }
    if (joyRight() && dx ==  0) { ndx =  1; ndy =  0; }
    if (joyUp()    && dy ==  0) { ndx =  0; ndy = -1; }
    if (joyDown()  && dy ==  0) { ndx =  0; ndy =  1; }

    if (millis() - lastMove < (unsigned long)moveInterval) return;
    lastMove = millis();
    dx = ndx; dy = ndy;

    Pt head = {(int16_t)(snake[0].x + dx), (int16_t)(snake[0].y + dy)};
    bool wallHit = head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS;
    bool selfHit = false;
    for (int i = 0; i < snakeLen - 1; i++)
      if (snake[i].x == head.x && snake[i].y == head.y) { selfHit = true; break; }

    if (wallHit || selfHit) {
      sfxExplode();
      lives--;
      if (lives <= 0) { gameOver = true; sfxGameOver(); return; }
      snakeLen = 3;
      int mx = COLS / 2, my = ROWS / 2;
      for (int i = 0; i < snakeLen; i++) snake[i] = {(int16_t)(mx - i), (int16_t)my};
      dx = 1; dy = 0; ndx = 1; ndy = 0;
      placeFood();
      return;
    }

    for (int i = snakeLen - 1; i > 0; i--) snake[i] = snake[i-1];
    snake[0] = head;

    if (head.x == food.x && head.y == food.y) {
      if (snakeLen < MAX_LEN) snakeLen++;
      score += 10;
      sfxBeep();
      if (snakeLen % 5 == 0) {
        level++;
        moveInterval = max(60, (int)(moveInterval * 0.88f));
        sfxLevelUp();
      }
      if (score > highScores[selectedGame][(int)currentDiff]) {
        saveScore(selectedGame, (int)currentDiff, score);
        newRecord = true;
      }
      placeFood();
    }
  }

  void draw() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0); display.print("S:"); display.print(score);
    display.setCursor(PLAY_W - 28, 0); display.print("L:"); display.print(lives);
    display.drawLine(0, 8, PLAY_W - 1, 8, SSD1306_WHITE);

    for (int i = 0; i < snakeLen; i++) {
      int px = snake[i].x * CELL;
      int py = snake[i].y * CELL + HUD_H + 1;
      if (i == 0) display.fillRect(px, py, CELL - 1, CELL - 1, SSD1306_WHITE);
      else        display.drawRect(px, py, CELL - 1, CELL - 1, SSD1306_WHITE);
    }

    int fx = food.x * CELL + 1, fy = food.y * CELL + HUD_H + 1;
    if ((millis() / 300) % 2 == 0) {
      display.fillRect(fx + 1, fy, CELL - 3, CELL - 1, SSD1306_WHITE);
      display.drawPixel(fx + 1, fy - 1, SSD1306_WHITE);
    } else {
      display.drawRect(fx, fy, CELL - 1, CELL - 1, SSD1306_WHITE);
    }
    display.display();
  }

  bool isOver()      { return gameOver; }
  int  getScore()    { return score; }
  bool isNewRecord() { return newRecord; }
}
