// ============================================================
//  game_dungeon.h  —  RETROBOX built-in game
//  Dungeon Crawler — procedural rooms, combat, fog
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

namespace DungeonGame {
  static const int CELL = 6;
  static int MAP_W, MAP_H;

  enum Tile : uint8_t {
    T_WALL = 0, T_FLOOR = 1, T_EXIT = 2,
    T_POTION = 3, T_CHEST = 4, T_TRAP = 5
  };

  static const int MAX_ROOMS   = 8;
  static const int MAX_ENEMIES = 8;

  struct Room  { int x, y, w, h; };
  struct Enemy {
    int x, y;
    bool alive;
    int hp, maxhp, atk;
    char sym;
  };

  static uint8_t* tiles = nullptr;
  static bool*    fog   = nullptr;
  static Room     rooms[MAX_ROOMS];
  static int      numRooms;
  static Enemy    enemies[MAX_ENEMIES];

  static int px, py;
  static int php, phmax, patk;
  static int score, level, turnCount;
  static bool gameOver, newRecord;

  static unsigned long lastInput;
  static const unsigned long INPUT_DELAY = 160UL;

  static int camX, camY;

  static char msg[24];
  static unsigned long msgUntil;

  void showMsg(const char* m) {
    strncpy(msg, m, 23); msg[23] = 0;
    msgUntil = millis() + 1800;
  }

  bool inBounds(int x, int y) { return x >= 0 && x < MAP_W && y >= 0 && y < MAP_H; }
  uint8_t& tileAt(int x, int y) { return tiles[y * MAP_W + x]; }
  bool&    fogAt (int x, int y) { return fog  [y * MAP_W + x]; }

  void revealAround(int x, int y, int r = 3) {
    for (int dy = -r; dy <= r; dy++)
      for (int dx = -r; dx <= r; dx++)
        if (dx*dx + dy*dy <= r*r + r)
          if (inBounds(x + dx, y + dy))
            fogAt(x + dx, y + dy) = false;
  }

  void carveRoom(Room& rm) {
    for (int ry = rm.y; ry < rm.y + rm.h && ry < MAP_H; ry++)
      for (int rx = rm.x; rx < rm.x + rm.w && rx < MAP_W; rx++)
        tileAt(rx, ry) = T_FLOOR;
  }

  void hCorridor(int x1, int x2, int y) {
    int a = min(x1, x2), b = max(x1, x2);
    for (int x = a; x <= b && x < MAP_W; x++)
      if (inBounds(x, y)) tileAt(x, y) = T_FLOOR;
  }
  void vCorridor(int y1, int y2, int x) {
    int a = min(y1, y2), b = max(y1, y2);
    for (int y = a; y <= b && y < MAP_H; y++)
      if (inBounds(x, y)) tileAt(x, y) = T_FLOOR;
  }

  int enemyAt(int x, int y) {
    for (int i = 0; i < MAX_ENEMIES; i++)
      if (enemies[i].alive && enemies[i].x == x && enemies[i].y == y) return i;
    return -1;
  }

  void generateMap() {
    memset(tiles, T_WALL, (size_t)MAP_W * MAP_H);
    for (int i = 0; i < MAP_W * MAP_H; i++) fog[i] = true;

    numRooms = 0;
    for (int attempt = 0; attempt < 40 && numRooms < MAX_ROOMS; attempt++) {
      int rw = random(3, 8), rh = random(3, 6);
      int rx = random(1, MAP_W - rw - 1);
      int ry = random(1, MAP_H - rh - 1);
      bool ok = true;
      for (int i = 0; i < numRooms && ok; i++) {
        if (rx < rooms[i].x + rooms[i].w + 1 && rx + rw + 1 > rooms[i].x &&
            ry < rooms[i].y + rooms[i].h + 1 && ry + rh + 1 > rooms[i].y)
          ok = false;
      }
      if (!ok) continue;
      Room rm = {rx, ry, rw, rh};
      carveRoom(rm);
      if (numRooms > 0) {
        int cx1 = rooms[numRooms-1].x + rooms[numRooms-1].w / 2;
        int cy1 = rooms[numRooms-1].y + rooms[numRooms-1].h / 2;
        int cx2 = rx + rw / 2, cy2 = ry + rh / 2;
        if (random(0, 2)) { hCorridor(cx1, cx2, cy1); vCorridor(cy1, cy2, cx2); }
        else              { vCorridor(cy1, cy2, cx1); hCorridor(cx1, cx2, cy2); }
      }
      rooms[numRooms++] = rm;
    }

    if (numRooms > 0) {
      Room& last = rooms[numRooms - 1];
      tileAt(last.x + last.w / 2, last.y + last.h / 2) = T_EXIT;
    }

    for (int i = 1; i < numRooms - 1; i++) {
      if (random(0, 3) == 0) tileAt(rooms[i].x + 1, rooms[i].y + 1) = T_POTION;
      if (random(0, 3) == 0) tileAt(rooms[i].x + rooms[i].w - 2, rooms[i].y + 1) = T_CHEST;
      if (random(0, 4) == 0) tileAt(rooms[i].x + random(1, rooms[i].w - 1),
                                    rooms[i].y + random(1, rooms[i].h - 1)) = T_TRAP;
    }

    int baseCount = 3 + min(level, 5);
    if (currentDiff == DIFF_EASY) baseCount = max(2, baseCount - 1);
    if (currentDiff == DIFF_HARD) baseCount = min(MAX_ENEMIES, baseCount + 2);
    int numEnemies = min(baseCount, MAX_ENEMIES);

    for (int e = 0; e < MAX_ENEMIES; e++) enemies[e].alive = false;
    int placed = 0;
    for (int attempt = 0; attempt < 80 && placed < numEnemies; attempt++) {
      int ri = random(1, numRooms);
      int ex = rooms[ri].x + random(1, max(1, rooms[ri].w - 1));
      int ey = rooms[ri].y + random(1, max(1, rooms[ri].h - 1));
      if (tileAt(ex, ey) != T_FLOOR) continue;
      if (enemyAt(ex, ey) >= 0) continue;

      int roll = random(0, 10);
      char sym; int ehp, eatk;
      if (roll < 4)       { sym = 'g'; ehp = 2 + level; eatk = 1; }
      else if (roll < 7)  { sym = 'o'; ehp = 4 + level * 2; eatk = 2; }
      else if (roll < 9)  { sym = 'T'; ehp = 8 + level * 2; eatk = 3; }
      else                { sym = 'S'; ehp = 5 + level * 3; eatk = 2; }

      enemies[placed++] = {ex, ey, true, ehp, ehp, eatk, sym};
    }

    if (numRooms > 0) {
      px = rooms[0].x + rooms[0].w / 2;
      py = rooms[0].y + rooms[0].h / 2;
    } else {
      px = MAP_W / 2; py = MAP_H / 2;
    }
    tileAt(px, py) = T_FLOOR;
    revealAround(px, py, 4);
  }

  void updateCamera() {
    int viewW = PLAY_W / CELL;
    int viewH = (PLAY_H - 12) / CELL;
    camX = px - viewW / 2;
    camY = py - viewH / 2;
    camX = max(0, min(camX, MAP_W - viewW));
    camY = max(0, min(camY, MAP_H - viewH));
  }

  bool runEnemyTurn() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].alive) continue;
      int dist = abs(enemies[i].x - px) + abs(enemies[i].y - py);
      if (dist > 8) continue;

      int ex = enemies[i].x, ey = enemies[i].y;
      int dx = (px > ex) ? 1 : (px < ex) ? -1 : 0;
      int dy = (py > ey) ? 1 : (py < ey) ? -1 : 0;

      if (ex + dx == px && ey + dy == py) {
        int dmg = enemies[i].atk;
        if (enemies[i].sym == 'S') dmg = (currentDiff == DIFF_HARD) ? 3 : 2;
        php -= dmg;
        char buf[22]; sprintf(buf, "%c hits you -%dHP!", enemies[i].sym, dmg);
        showMsg(buf);
        sfxHit();
        if (php <= 0) {
          php = 0; gameOver = true; sfxGameOver();
          return true;
        }
      } else {
        int nx = ex + dx, ny = ey + dy;
        if (inBounds(nx, ny) && tileAt(nx, ny) != T_WALL && enemyAt(nx, ny) < 0) {
          enemies[i].x = nx; enemies[i].y = ny;
        } else {
          nx = ex + dx; ny = ey;
          if (dx != 0 && inBounds(nx, ny) && tileAt(nx, ny) != T_WALL && enemyAt(nx, ny) < 0)
            { enemies[i].x = nx; }
          else {
            nx = ex; ny = ey + dy;
            if (dy != 0 && inBounds(nx, ny) && tileAt(nx, ny) != T_WALL && enemyAt(nx, ny) < 0)
              { enemies[i].y = ny; }
          }
        }
      }
    }
    return false;
  }

  bool attackEnemy(int ei) {
    int dmg = patk + random(0, 2);
    enemies[ei].hp -= dmg;
    sfxShoot();
    if (enemies[ei].hp <= 0) {
      enemies[ei].alive = false;
      score += enemies[ei].maxhp * 10;
      char buf[22]; sprintf(buf, "Slew %c! +%dpts", enemies[ei].sym, enemies[ei].maxhp * 10);
      showMsg(buf);
      sfxLevelUp();
      if (score > highScores[selectedGame][(int)currentDiff]) {
        saveScore(selectedGame, (int)currentDiff, score);
        newRecord = true;
      }
    } else {
      char buf[22]; sprintf(buf, "Hit %c for %d dmg", enemies[ei].sym, dmg);
      showMsg(buf);
    }
    return true;
  }

  bool tryMove(int dx, int dy) {
    int nx = px + dx, ny = py + dy;
    if (!inBounds(nx, ny)) return false;
    uint8_t t = tileAt(nx, ny);
    if (t == T_WALL) { sfxBack(); return false; }

    int ei = enemyAt(nx, ny);
    if (ei >= 0) return attackEnemy(ei);

    px = nx; py = ny;
    revealAround(px, py, 3);
    updateCamera();

    switch (t) {
      case T_POTION: {
        int heal = 3 + random(0, 3);
        php = min(phmax, php + heal);
        tileAt(px, py) = T_FLOOR;
        char buf[20]; sprintf(buf, "Potion! +%dHP", heal);
        showMsg(buf); sfxBeep();
        break;
      }
      case T_CHEST:
        score += 50 + level * 10;
        tileAt(px, py) = T_FLOOR;
        showMsg("Chest! +50pts"); sfxLevelUp();
        break;
      case T_TRAP:
        php -= 2;
        tileAt(px, py) = T_FLOOR;
        showMsg("Trap! -2HP!"); sfxExplode();
        if (php <= 0) { gameOver = true; sfxGameOver(); return true; }
        break;
      case T_EXIT:
        level++;
        score += 100 * level;
        phmax += 2; php = min(phmax, php + 3);
        patk++;
        showMsg("Next floor! +HP!");
        sfxLevelUp();
        generateMap();
        updateCamera();
        return true;
    }
    return true;
  }

  void init() {
    MAP_W = (PLAY_W / CELL) + 8;
    MAP_H = ((PLAY_H - 12) / CELL) + 8;
    if (MAP_W < 14) MAP_W = 14;
    if (MAP_H < 12) MAP_H = 12;

    if (tiles) { free(tiles); tiles = nullptr; }
    if (fog)   { free(fog);   fog   = nullptr; }
    tiles = (uint8_t*)malloc((size_t)MAP_W * MAP_H);
    fog   = (bool*)   malloc((size_t)MAP_W * MAP_H * sizeof(bool));
    if (!tiles || !fog) {
      if (tiles) { free(tiles); tiles = nullptr; }
      if (fog)   { free(fog);   fog   = nullptr; }
      return;
    }

    score = 0; level = 1; turnCount = 0;
    gameOver = false; newRecord = false;
    msg[0] = 0; msgUntil = 0;

    phmax = (currentDiff == DIFF_EASY) ? 15 : (currentDiff == DIFF_HARD) ? 7 : 10;
    php   = phmax;
    patk  = (currentDiff == DIFF_HARD) ? 3 : 2;

    randomSeed(millis());
    generateMap();
    updateCamera();
    lastInput = millis() + 400;
  }

  void update() {
    if (gameOver) return;
    if (!tiles || !fog) return;
    if (millis() - lastInput < INPUT_DELAY) return;

    bool turnSpent = false;

    if      (joyLeft())  turnSpent = tryMove(-1,  0);
    else if (joyRight()) turnSpent = tryMove( 1,  0);
    else if (joyUp())    turnSpent = tryMove( 0, -1);
    else if (joyDown())  turnSpent = tryMove( 0,  1);

    if (!turnSpent && btnA.pressed) {
      const int dirs[4][2] = {{0,-1},{0,1},{-1,0},{1,0}};
      bool hit = false;
      for (auto& d : dirs) {
        int ei = enemyAt(px + d[0], py + d[1]);
        if (ei >= 0) { attackEnemy(ei); hit = true; break; }
      }
      if (!hit) showMsg("Nothing to attack!");
      turnSpent = true;
    }

    if (turnSpent) {
      lastInput = millis();
      turnCount++;
      runEnemyTurn();
    }
  }

  void draw() {
    if (!tiles || !fog) return;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    int viewW = PLAY_W / CELL;
    int viewH = (PLAY_H - 12) / CELL;

    for (int ty = 0; ty < viewH; ty++) {
      for (int tx = 0; tx < viewW; tx++) {
        int wx = tx + camX, wy = ty + camY;
        if (!inBounds(wx, wy)) continue;
        if (fogAt(wx, wy)) continue;
        int sx = tx * CELL, sy = ty * CELL + 12;
        uint8_t t = tileAt(wx, wy);
        switch (t) {
          case T_FLOOR:
            break;
          case T_WALL:
            display.fillRect(sx, sy, CELL, CELL, SSD1306_WHITE);
            display.drawPixel(sx + 1, sy + 1, SSD1306_BLACK);
            break;
          case T_EXIT:
            display.drawRect(sx, sy, CELL, CELL, SSD1306_WHITE);
            display.setCursor(sx + 1, sy);
            display.setTextColor(SSD1306_WHITE); display.print("X");
            break;
          case T_POTION:
            display.setCursor(sx, sy);
            display.print("p");
            break;
          case T_CHEST:
            display.drawRect(sx, sy + 1, CELL, CELL - 2, SSD1306_WHITE);
            break;
          case T_TRAP:
            display.drawPixel(sx + 2, sy + 2, SSD1306_WHITE);
            display.drawPixel(sx + 3, sy + 3, SSD1306_WHITE);
            break;
        }
      }
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (!enemies[i].alive) continue;
      int tx = enemies[i].x - camX, ty = enemies[i].y - camY;
      if (tx < 0 || tx >= viewW || ty < 0 || ty >= viewH) continue;
      if (fogAt(enemies[i].x, enemies[i].y)) continue;
      display.setCursor(tx * CELL, ty * CELL + 12);
      display.print((char)enemies[i].sym);
    }

    int ppx = (px - camX) * CELL, ppy = (py - camY) * CELL + 12;
    display.setCursor(ppx, ppy); display.print("@");

    display.drawLine(0, 11, PLAY_W - 1, 11, SSD1306_WHITE);
    display.setCursor(0, 1);
    char buf[24];
    if (PLAY_W < 80)
      sprintf(buf, "HP%d/%d L%d S%d", php, phmax, level, score);
    else
      sprintf(buf, "HP:%d/%d Lv%d Sc:%d", php, phmax, level, score);
    display.print(buf);

    if (millis() < msgUntil && msg[0]) {
      display.fillRect(0, PLAY_H - 10, PLAY_W, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(2, PLAY_H - 9);
      display.print(msg);
      display.setTextColor(SSD1306_WHITE);
    }

    display.display();
  }

  bool isOver()      { return gameOver; }
  int  getScore()    { return score; }
  bool isNewRecord() { return newRecord; }
}
