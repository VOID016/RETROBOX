# Adding a Custom Game to RETROBOX

This guide walks you through adding your own game in **6 steps**.  
You don't need to touch the main `.ino` file much — the architecture handles the plumbing.

---

## Before You Start

Every RETROBOX game is a **C++ namespace** inside a `.h` file.  
It exposes exactly **6 functions**:

```cpp
void init()          — called once when the game starts
void update()        — called every frame (input + logic)
void draw()          — called every frame (render to display)
bool isOver()        — returns true when the game should end
int  getScore()      — returns the player's score
bool isNewRecord()   — returns true if the score beat the stored high score
```

That's the entire contract. Everything else is up to you.

---

## Step 1 — Create Your Game File

Create a new file: `game_mygame.h` (replace `mygame` with your game name, lowercase).

Use this starter template:

```cpp
// ============================================================
//  game_mygame.h  —  RETROBOX custom game
//  [Your game description here]
// ============================================================

#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <math.h>
#include "retrobox_types.h"

// ── External references — provided by the main .ino ──────────
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

namespace MyGame {

// ── Game state ────────────────────────────────────────────────
static bool over      = false;
static bool newRecord = false;
static int  score     = 0;

// ── init() — called once at game start ───────────────────────
void init() {
  over      = false;
  newRecord = false;
  score     = 0;
  // Use currentDiff to scale difficulty:
  //   (int)currentDiff == 0 → Easy
  //   (int)currentDiff == 1 → Medium
  //   (int)currentDiff == 2 → Hard
}

// ── update() — called every frame ────────────────────────────
void update() {
  if (over) return;

  // Read input
  if (joyLeft())     { /* move left  */ }
  if (joyRight())    { /* move right */ }
  if (btnA.pressed)  { /* action     */ }

  // Game logic here

  // When score increases, check for a new record:
  // if (score > highScores[selectedGame][(int)currentDiff]) {
  //   saveScore(selectedGame, (int)currentDiff, score);
  //   newRecord = true;
  // }

  // When game ends:
  // over = true;
}

// ── draw() — called every frame ──────────────────────────────
void draw() {
  display.clearDisplay();
  // Draw your game here using the Adafruit GFX API (see below)
  display.display(); // always call this at the end of draw()
}

// ── Required accessors ────────────────────────────────────────
bool isOver()      { return over; }
int  getScore()    { return score; }
bool isNewRecord() { return newRecord; }

} // namespace MyGame
```

---

## Step 2 — Register in `games_config.h`

Open `games_config.h` and add one line under the custom games section:

```cpp
#define ENABLE_MYGAME
```

Then open `RETROBOX_3_7.ino` and add the corresponding `#ifdef` include block **after the other game includes** (around line 64):

```cpp
#ifdef ENABLE_MYGAME
  #include "game_mygame.h"
#endif
```

Without this block the `#define` in `games_config.h` does nothing and the game is never compiled in.

---

## Step 3 — Add to the `GameID` enum

In `RETROBOX_3_7.ino`, find the `GameID` enum and add your entry **before** `GAME_COUNT`:

```cpp
  GAME_MYGAME,   // ← add this line
  GAME_COUNT
```

---

## Step 4 — Add to `GAME_NAMES`

Find the `GAME_NAMES` array and add your entry at the matching position (same index as your enum entry):

```cpp
"MYGAME",   // shown in the menu (max ~9 chars)
```

---

## Step 5 — Add a `TUTORIALS[]` entry

The firmware contains a compile-time array sized to exactly `GAME_COUNT`:

```cpp
const TutLine TUTORIALS[GAME_COUNT][2][2] = { /* one entry per game */ };
```

When you add a new `GameID` entry, `GAME_COUNT` increases by 1 and the array needs a matching initializer — otherwise the compiler will error with *too few initializers*.

In `RETROBOX_3_7.ino`, find the `TUTORIALS` array and insert a new entry at the position that matches your game's enum index. Use this format:

```cpp
{ {{"Your hint line 1","hint line 2"},{"control hint 1","control hint 2"}},
  {{"tip or mechanic 1","tip 2"},{"tip 3","tip 4"}} },
```

Each entry has **2 pages** and each page has **2 text pairs** (`a` / `b`). Keep each string short enough to fit on the 128×64 screen at text size 1 (~21 characters per line).

---

## Step 6 — Wire into the Dispatchers

Find each of the 6 dispatcher functions and add a `case` for your game:

**`gameInit()`**
```cpp
case GAME_MYGAME: MyGame::init(); break;
```

**`gameUpdate()`**
```cpp
case GAME_MYGAME: MyGame::update(); break;
```

**`gameDraw()`**
```cpp
case GAME_MYGAME: MyGame::draw(); break;
```

**`gameIsOver()`**
```cpp
case GAME_MYGAME: return MyGame::isOver();
```

**`gameGetScore()`**
```cpp
case GAME_MYGAME: return MyGame::getScore();
```

**`gameIsNewRecord()`**
```cpp
case GAME_MYGAME: return MyGame::isNewRecord();
```

That's it. Flash, and your game appears in the menu.

---

## Available APIs

### Display (Adafruit GFX)

```cpp
display.clearDisplay();                              // clear frame buffer
display.setCursor(x, y);                            // set text position
display.setTextSize(1);                             // text scale (1 or 2)
display.setTextColor(SSD1306_WHITE);
display.print("text");                              // print string
display.fillRect(x, y, w, h, SSD1306_WHITE);       // filled rectangle
display.drawRect(x, y, w, h, SSD1306_WHITE);       // outline rectangle
display.fillCircle(x, y, r, SSD1306_WHITE);        // filled circle
display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);  // line
display.drawPixel(x, y, SSD1306_WHITE);            // single pixel
display.display();                                  // push buffer to screen ← REQUIRED
```

Screen dimensions at runtime (use these instead of hardcoded 128/64 — they change with orientation):

```cpp
PLAY_W   // pixel width  (128 in landscape, 64 in portrait)
PLAY_H   // pixel height (64 in landscape, 128 in portrait)
```

### Input

```cpp
// Joystick (analog with dead-zone)
joyLeft()   // returns true while pushed left
joyRight()  // returns true while pushed right
joyUp()     // returns true while pushed up
joyDown()   // returns true while pushed down

// Buttons (debounced)
btnA.pressed      // true for ONE frame when first pressed
btnB.pressed      // true for ONE frame when first pressed
btnJoySW.pressed  // true for ONE frame when joystick is clicked
btnA.held         // true every frame while held down
btnB.held         // true every frame while held down
```

Use `.pressed` for single actions (shoot, jump). Use `.held` for held actions (charge, move).

### Audio

```cpp
sfxBeep()      // short high beep — pick-up, menu move
sfxShoot()     // quick mid beep — firing
sfxHit()       // short low blip — taking damage
sfxExplode()   // low rumble — death, collision
sfxGameOver()  // descending tone sequence — game end
sfxLevelUp()   // ascending notes — level clear, achievement
sfxMenu()      // short tick — menu navigation
sfxConfirm()   // two-note up — confirm / select
sfxBack()      // two-note down — cancel / back
```

### Timing

```cpp
millis()     // ms since boot — use for frame timing
delay(ms)    // blocking delay (avoid in update() if possible)
```

---

## Tips

- **Target 30fps**: your `update()` + `draw()` should complete in under 33ms per frame
- **Avoid `delay()` in `update()`** — use `millis()` timestamps instead
- **Use `static` variables** inside your namespace — they persist between frames without needing global declarations
- **Scale difficulty from `currentDiff`** — cast to `int` (0=Easy, 1=Medium, 2=Hard) and use it to tune speed, enemy count, lives, etc. Do this in `init()` and store the result in a `static` variable
- **Note on orientation** — DOOM and Mario lock to landscape. All other games support all four orientations. Use `PLAY_W`/`PLAY_H` instead of hardcoded 128/64 so your game works correctly in portrait mode too
- Check `game_snake.h` for a complete, well-commented working example

---

*Questions? Open an issue on the repo.*
