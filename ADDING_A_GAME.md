# Adding a Custom Game to RETROBOX

This guide walks you through adding your own game in **5 steps**.  
You don't need to touch the main `.ino` file much — the architecture handles the plumbing.

---

## Before You Start

Every RETROBOX game is a **C++ namespace** inside a `.h` file.  
It exposes exactly **5 functions**:

```cpp
void init(int diff)  — called once when the game starts
void update()        — called every frame (input + logic)
void draw()          — called every frame (render to display)
bool isOver()        — returns true when the game should end
int  getScore()      — returns the player's score
```

That's the entire contract. Everything else is up to you.

---

## Step 1 — Create Your Game File

Create a new file: `game_MYGAME.h` (replace MYGAME with your game name, lowercase).

Use this starter template:

```cpp
// ============================================================
//  game_mygame.h  —  RETROBOX custom game
//  [Your game description here]
// ============================================================

#pragma once
#include <Adafruit_SSD1306.h>

// External references — provided by the main .ino
extern Adafruit_SSD1306 display;
extern bool joyLeft(), joyRight(), joyUp(), joyDown();
extern struct Button btnA, btnB, btnStart;
extern void sfxBeep(), sfxShoot(), sfxExplode(), sfxGameOver(), sfxLevelUp();

namespace MyGame {

// ── Your game state variables ─────────────────────────────
static bool over  = false;
static int  score = 0;

// ── init() — called once at game start ───────────────────
void init(int diff) {
  over  = false;
  score = 0;
  // diff: 0=Easy, 1=Normal, 2=Hard
  // Use diff to set speed, enemy count, lives, etc.
}

// ── update() — called every frame ────────────────────────
void update() {
  if (over) return;

  // Read input
  if (joyLeft())  { /* move left  */ }
  if (joyRight()) { /* move right */ }
  if (btnA.pressed) { /* action   */ }

  // Game logic here

  // When game ends:
  // over = true;
}

// ── draw() — called every frame ──────────────────────────
void draw() {
  display.clearDisplay();
  // Draw your game here using the Adafruit GFX API (see below)
  display.display(); // always call this at the end of draw()
}

// ── Required accessors ────────────────────────────────────
bool isOver()     { return over;  }
int  getScore()   { return score; }

} // namespace MyGame
```

---

## Step 2 — Register in `games_config.h`

Open `games_config.h` and add one line:

```cpp
#define ENABLE_MYGAME
```

---

## Step 3 — Add to the `GameID` enum

In `RETROBOX_3_7.ino`, find the `GameID` enum and add:

```cpp
#ifdef ENABLE_MYGAME
  GAME_MYGAME,
#endif
```

---

## Step 4 — Add to `GAME_NAMES`

Find the `GAME_NAMES` array and add:

```cpp
#ifdef ENABLE_MYGAME
  "MYGAME",   // This is the name shown in the menu (max ~9 chars)
#endif
```

---

## Step 5 — Wire into the Dispatchers

Find each of the 4 dispatcher functions and add a case:

**`gameInit()`**
```cpp
#ifdef ENABLE_MYGAME
  case GAME_MYGAME: MyGame::init(diff); break;
#endif
```

**`gameUpdate()`**
```cpp
#ifdef ENABLE_MYGAME
  case GAME_MYGAME: MyGame::update(); break;
#endif
```

**`gameDraw()`**
```cpp
#ifdef ENABLE_MYGAME
  case GAME_MYGAME: MyGame::draw(); break;
#endif
```

**`gameIsOver()`**
```cpp
#ifdef ENABLE_MYGAME
  case GAME_MYGAME: return MyGame::isOver();
#endif
```

**`gameGetScore()`**
```cpp
#ifdef ENABLE_MYGAME
  case GAME_MYGAME: return MyGame::getScore();
#endif
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

Screen is 128×64 pixels. Origin (0,0) is top-left.

### Input

```cpp
// Joystick (analog with dead-zone)
joyLeft()   // returns true while pushed left
joyRight()  // returns true while pushed right
joyUp()     // returns true while pushed up
joyDown()   // returns true while pushed down

// Buttons (debounced)
btnA.pressed   // true for ONE frame when first pressed
btnB.pressed   // true for ONE frame when first pressed
btnA.held      // true every frame while held down
btnB.held      // true every frame while held down
```

Use `.pressed` for single actions (shoot, jump). Use `.held` for held actions (charge, move).

### Audio

```cpp
sfxBeep()      // short high beep — pick-up, menu move
sfxShoot()     // quick mid beep — firing
sfxExplode()   // low rumble — death, collision
sfxGameOver()  // long low tone — game end
sfxLevelUp()   // ascending notes — level clear, achievement
```

### Screen Dimensions

```cpp
PLAY_W   // 128 — usable pixel width
PLAY_H   //  64 — usable pixel height
```

### Timing

```cpp
millis()         // ms since boot — use for frame timing
delay(ms)        // blocking delay (avoid in update() if possible)
```

---

## Tips

- **Target 60fps**: your `update()` + `draw()` should complete in under 16ms total
- **Avoid `delay()` in `update()`** — use `millis()` timestamps instead
- **Use `static` variables** inside your namespace — they persist between frames without needing global declarations
- **Test with difficulty 1 (Normal) first**, then tune 0 and 2
- Check `game_snake.h` for a complete, well-commented working example

---

*Questions? Open an issue on the repo.*
