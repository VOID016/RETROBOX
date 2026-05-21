// ============================================================
//  game_tetris.h  —  RETROBOX built-in game
//  Tetris — falling tetrominoes, line clears, levels
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

namespace TetrisGame {
  static int BOARD_W_CELLS, BOARD_H_CELLS, BLOCK, BOARD_X;
  static uint8_t* board = nullptr;
  static int  score, lives, level, lines;
  static bool gameOver, newRecord;

  static const int8_t PIECES[7][4][4][2] = {
    {{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}},
    {{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}}},
    {{{1,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{0,1},{1,1},{1,2}}},
    {{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}},
    {{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}},
    {{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}},
    {{{2,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,1},{0,2}},{{0,0},{1,0},{1,1},{1,2}}}
  };

  static int  pieceType, pieceRot, pieceX, pieceY;
  static int  nextType;
  static unsigned long lastDrop, dropInterval;
  static bool leftHeld, rightHeld;
  static unsigned long leftHeldAt, rightHeldAt;

  bool fits(int type, int rot, int px, int py) {
    for (int b = 0; b < 4; b++) {
      int x = px + PIECES[type][rot][b][0];
      int y = py + PIECES[type][rot][b][1];
      if (x < 0 || x >= BOARD_W_CELLS || y >= BOARD_H_CELLS) return false;
      if (y >= 0 && board[y * BOARD_W_CELLS + x]) return false;
    }
    return true;
  }

  void newPiece() {
    pieceType = nextType;
    nextType  = random(0, 7);
    pieceRot  = 0;
    pieceX    = BOARD_W_CELLS / 2 - 2;
    pieceY    = -1;
    if (!fits(pieceType, pieceRot, pieceX, pieceY)) {
      gameOver = true; sfxGameOver();
    }
  }

  void lockPiece() {
    for (int b = 0; b < 4; b++) {
      int x = pieceX + PIECES[pieceType][pieceRot][b][0];
      int y = pieceY + PIECES[pieceType][pieceRot][b][1];
      if (y >= 0 && y < BOARD_H_CELLS && x >= 0 && x < BOARD_W_CELLS)
        board[y * BOARD_W_CELLS + x] = pieceType + 1;
    }
    int cleared = 0;
    for (int row = BOARD_H_CELLS - 1; row >= 0; row--) {
      bool full = true;
      for (int c = 0; c < BOARD_W_CELLS; c++) if (!board[row * BOARD_W_CELLS + c]) { full = false; break; }
      if (full) {
        cleared++;
        for (int r = row; r > 0; r--)
          for (int c = 0; c < BOARD_W_CELLS; c++)
            board[r * BOARD_W_CELLS + c] = board[(r-1) * BOARD_W_CELLS + c];
        for (int c = 0; c < BOARD_W_CELLS; c++) board[c] = 0;
        row++;
      }
    }
    if (cleared > 0) {
      sfxLevelUp();
      static const int pts[] = {0, 100, 300, 500, 800};
      score += pts[min(cleared, 4)] * level;
      lines += cleared;
      level = 1 + lines / 10;
      unsigned long minDrop = (currentDiff == DIFF_EASY) ? 200UL : (currentDiff == DIFF_HARD) ? 80UL : 120UL;
      dropInterval = max(minDrop, (unsigned long)(800 - level * 40));
      if (score > highScores[selectedGame][(int)currentDiff]) {
        saveScore(selectedGame, (int)currentDiff, score);
        newRecord = true;
      }
    }
    newPiece();
  }

  void init() {
    BLOCK = (PLAY_W < 80) ? 5 : 4;
    BOARD_W_CELLS = 10;
    int boardPixH = PLAY_H;
    BOARD_H_CELLS = boardPixH / BLOCK;
    if (BOARD_H_CELLS > 20) BOARD_H_CELLS = 20;
    BOARD_X = 2;

    if (board) free(board);
    board = (uint8_t*)calloc(BOARD_W_CELLS * BOARD_H_CELLS, 1);

    score = 0; lives = 0; level = 1; lines = 0;
    gameOver = false; newRecord = false;
    dropInterval = (currentDiff == DIFF_EASY) ? 800UL : (currentDiff == DIFF_HARD) ? 400UL : 600UL;
    nextType = random(0, 7);
    newPiece();
    lastDrop = millis();
    leftHeld = rightHeld = false;
  }

  void update() {
    if (gameOver) return;
    if (joyLeft()) {
      if (!leftHeld) { leftHeld = true; leftHeldAt = millis(); if (fits(pieceType, pieceRot, pieceX - 1, pieceY)) { pieceX--; sfxMenu(); } }
      else if (millis() - leftHeldAt > 280) { if (fits(pieceType, pieceRot, pieceX - 1, pieceY)) pieceX--; }
    } else leftHeld = false;
    if (joyRight()) {
      if (!rightHeld) { rightHeld = true; rightHeldAt = millis(); if (fits(pieceType, pieceRot, pieceX + 1, pieceY)) { pieceX++; sfxMenu(); } }
      else if (millis() - rightHeldAt > 280) { if (fits(pieceType, pieceRot, pieceX + 1, pieceY)) pieceX++; }
    } else rightHeld = false;
    if (btnA.pressed) {
      int nr = (pieceRot + 1) % 4;
      if (fits(pieceType, nr, pieceX, pieceY)) { pieceRot = nr; sfxBeep(); }
    }
    unsigned long interval = joyDown() ? 60 : dropInterval;
    if (millis() - lastDrop > interval) {
      lastDrop = millis();
      if (fits(pieceType, pieceRot, pieceX, pieceY + 1)) pieceY++;
      else lockPiece();
    }
  }

  void draw() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    int hx = BOARD_X + BOARD_W_CELLS * BLOCK + 4;
    if (hx + 20 < PLAY_W) {
      display.setCursor(hx, 0);  display.print("NXT");
      for (int b = 0; b < 4; b++) {
        int px = hx + PIECES[nextType][0][b][0] * 3;
        int py = 9  + PIECES[nextType][0][b][1] * 3;
        display.fillRect(px, py, 2, 2, SSD1306_WHITE);
      }
      display.setCursor(hx, 24); display.print("LV");
      display.setCursor(hx, 32); display.print(level);
      display.setCursor(hx, 42); display.print("SC");
      display.setCursor(hx, 50); display.print(score / 100);
    }

    display.drawRect(BOARD_X - 1, 0, BOARD_W_CELLS * BLOCK + 2, BOARD_H_CELLS * BLOCK + 1, SSD1306_WHITE);
    for (int r = 0; r < BOARD_H_CELLS; r++)
      for (int c = 0; c < BOARD_W_CELLS; c++)
        if (board[r * BOARD_W_CELLS + c])
          display.fillRect(BOARD_X + c * BLOCK, r * BLOCK, BLOCK - 1, BLOCK - 1, SSD1306_WHITE);

    for (int b = 0; b < 4; b++) {
      int x = pieceX + PIECES[pieceType][pieceRot][b][0];
      int y = pieceY + PIECES[pieceType][pieceRot][b][1];
      if (y >= 0)
        display.fillRect(BOARD_X + x * BLOCK, y * BLOCK, BLOCK - 1, BLOCK - 1, SSD1306_WHITE);
    }

    int gy = pieceY;
    while (fits(pieceType, pieceRot, pieceX, gy + 1)) gy++;
    if (gy != pieceY) {
      for (int b = 0; b < 4; b++) {
        int x = pieceX + PIECES[pieceType][pieceRot][b][0];
        int y = gy + PIECES[pieceType][pieceRot][b][1];
        if (y >= 0)
          display.drawRect(BOARD_X + x * BLOCK, y * BLOCK, BLOCK - 1, BLOCK - 1, SSD1306_WHITE);
      }
    }
    display.display();
  }

  bool isOver()      { return gameOver; }
  int  getScore()    { return score; }
  bool isNewRecord() { return newRecord; }
}
