// ============================================================
//  game_arkanoid.h  —  RETROBOX built-in game
//  Arkanoid — ball, paddle, brick breaker
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

namespace ArkanoidGame {
  static int PADDLE_Y, PADDLE_W;
  static const int PADDLE_H  = 3;
  static const int BALL_R    = 2;
  static int BRICK_W, BRICK_H, BRICK_COLS, BRICK_ROWS, BRICK_XOFF, BRICK_YOFF;

  static int   paddleX;
  static float ballX, ballY, ballVx, ballVy;
  static bool  bricks[6][12];
  static int   score, lives, level;
  static bool  gameOver, newRecord, ballLaunched;

  int bricksRemaining() {
    int n = 0;
    for (int r = 0; r < BRICK_ROWS; r++)
      for (int c = 0; c < BRICK_COLS; c++)
        if (bricks[r][c]) n++;
    return n;
  }

  void resetBall() {
    ballX = paddleX + PADDLE_W / 2;
    ballY = PADDLE_Y - BALL_R - 2;
    float speed = 1.8f + level * 0.15f;
    if (speed > 3.5f) speed = 3.5f;
    ballVx = (random(0, 2) ? 1 : -1) * speed * 0.7f;
    ballVy = -speed;
    ballLaunched = false;
  }

  void initBricks() {
    for (int r = 0; r < BRICK_ROWS; r++)
      for (int c = 0; c < BRICK_COLS; c++)
        bricks[r][c] = true;
  }

  void init() {
    PADDLE_Y = PLAY_H - 6;
    PADDLE_W = max(14, PLAY_W / 7);
    BRICK_W  = max(9,  PLAY_W / 11);
    BRICK_H  = 5;
    BRICK_COLS = (PLAY_W - 8) / BRICK_W;
    if (BRICK_COLS > 12) BRICK_COLS = 12;
    BRICK_ROWS = (currentDiff == DIFF_EASY) ? 3 : (currentDiff == DIFF_HARD) ? 5 : 4;
    if (BRICK_ROWS > 6) BRICK_ROWS = 6;
    BRICK_XOFF = (PLAY_W - BRICK_COLS * BRICK_W) / 2;
    BRICK_YOFF = 10;

    paddleX = (PLAY_W - PADDLE_W) / 2;
    score = 0; lives = 3; level = 1; gameOver = false; newRecord = false;
    initBricks();
    resetBall();
  }

  void update() {
    if (gameOver) return;
    if (joyLeft()  && paddleX > 2)               paddleX -= joyMoveStep() + 1;
    if (joyRight() && paddleX < PLAY_W - PADDLE_W - 2) paddleX += joyMoveStep() + 1;

    if (!ballLaunched && btnA.pressed) { ballLaunched = true; sfxBeep(); }
    if (!ballLaunched) { ballX = paddleX + PADDLE_W / 2; return; }

    ballX += ballVx;
    ballY += ballVy;

    if (ballX <= BALL_R)            { ballX = BALL_R;            ballVx = fabs(ballVx); }
    if (ballX >= PLAY_W - BALL_R)   { ballX = PLAY_W - BALL_R;   ballVx = -fabs(ballVx); }
    if (ballY <= BALL_R + 9)        { ballY = BALL_R + 9;         ballVy = fabs(ballVy); }

    if (ballY > PLAY_H + 4) {
      sfxExplode(); lives--;
      if (lives <= 0) { gameOver = true; sfxGameOver(); return; }
      resetBall(); return;
    }

    if (ballY + BALL_R >= PADDLE_Y && ballY - BALL_R <= PADDLE_Y + PADDLE_H &&
        ballX >= paddleX && ballX <= paddleX + PADDLE_W) {
      ballVy = -fabs(ballVy);
      float offset = (ballX - (paddleX + PADDLE_W / 2.0f)) / (PADDLE_W / 2.0f);
      ballVx = offset * 2.5f;
      if (fabs(ballVx) < 0.2f) ballVx = 0.3f;
      sfxBeep();
    }

    for (int r = 0; r < BRICK_ROWS; r++) {
      for (int c = 0; c < BRICK_COLS; c++) {
        if (!bricks[r][c]) continue;
        int bx = BRICK_XOFF + c * BRICK_W;
        int by = BRICK_YOFF + r * BRICK_H;
        if (ballX + BALL_R > bx && ballX - BALL_R < bx + BRICK_W &&
            ballY + BALL_R > by && ballY - BALL_R < by + BRICK_H) {
          bricks[r][c] = false;
          sfxBeep();
          float oL = (ballX + BALL_R) - bx, oR = (bx + BRICK_W) - (ballX - BALL_R);
          float oT = (ballY + BALL_R) - by, oB = (by + BRICK_H) - (ballY - BALL_R);
          if (min(oL, oR) < min(oT, oB)) ballVx = -ballVx;
          else                            ballVy = -ballVy;
          score += (BRICK_ROWS - r) * 10 * level;
          if (score > highScores[selectedGame][(int)currentDiff]) {
            saveScore(selectedGame, (int)currentDiff, score); newRecord = true;
          }
          goto done_bricks;
        }
      }
    }
    done_bricks:;

    if (bricksRemaining() == 0) {
      level++; sfxLevelUp();
      initBricks();
      float spd = sqrtf(ballVx * ballVx + ballVy * ballVy) * 1.1f;
      if (spd > 4.0f) spd = 4.0f;
      float ang = atan2f(ballVy, ballVx);
      ballVx = cosf(ang) * spd;
      ballVy = sinf(ang) * spd;
    }
  }

  void draw() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);  display.print("S:"); display.print(score);
    display.setCursor(PLAY_W / 2 - 4, 0); display.print("L:"); display.print(lives);
    display.setCursor(PLAY_W - 18, 0); display.print("W:"); display.print(level);
    display.drawLine(0, 8, PLAY_W - 1, 8, SSD1306_WHITE);

    for (int r = 0; r < BRICK_ROWS; r++) {
      for (int c = 0; c < BRICK_COLS; c++) {
        if (!bricks[r][c]) continue;
        int bx = BRICK_XOFF + c * BRICK_W + 1;
        int by = BRICK_YOFF + r * BRICK_H + 1;
        if (r % 2 == 0) display.fillRect(bx, by, BRICK_W - 2, BRICK_H - 2, SSD1306_WHITE);
        else            display.drawRect(bx, by, BRICK_W - 2, BRICK_H - 2, SSD1306_WHITE);
      }
    }

    display.fillCircle((int)ballX, (int)ballY, BALL_R, SSD1306_WHITE);
    display.fillRoundRect(paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, 1, SSD1306_WHITE);
    if (!ballLaunched) drawCenteredText(PLAY_H - 18, "A = LAUNCH", 1);
    display.display();
  }

  bool isOver()      { return gameOver; }
  int  getScore()    { return score; }
  bool isNewRecord() { return newRecord; }
}
