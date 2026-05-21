// ============================================================
//  RETROBOX v3.7  —  ESP32 Multi-Game Handheld Console
//  Author:  VOID (https://github.com/VOID016)
//  License: MIT
//  Board:   ESP32 DevKit V1
//  Display: SSD1306 128×64 OLED (I2C)
// ============================================================
//
//  QUICK START
//  ──────────────────────────────────────────────────────────
//  1. Install libraries (see README.md → Dependencies)
//  2. Open games_config.h to enable/disable games
//  3. Flash to your ESP32
//  4. Plug in, play
//
//  ADDING A CUSTOM GAME → see ADDING_A_GAME.md
//  WIRING REFERENCE     → see WIRING_DIAGRAM.md
// ============================================================
// ============================================================
//  PIN ASSIGNMENTS:
//   GPIO21 → OLED SDA        GPIO22 → OLED SCL
//   GPIO34 → Joystick VRx    GPIO35 → Joystick VRy
//   GPIO25 → Joystick SW     GPIO14 → Button A (fire/confirm)
//   GPIO27 → Button B (back/special)
//   GPIO13 → Button START    GPIO26 → Buzzer (direct, 3.3V)
// ============================================================


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "retrobox_types.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include <math.h>

// ── Game modules (enable/disable in games_config.h) ──────────
#include "games_config.h"

#ifdef ENABLE_RACER
  #include "game_racer.h"
#endif
#ifdef ENABLE_TETRIS
  #include "game_tetris.h"
#endif
#ifdef ENABLE_SNAKE
  #include "game_snake.h"
#endif
#ifdef ENABLE_ARKANOID
  #include "game_arkanoid.h"
#endif
#ifdef ENABLE_VSHOOTER
  #include "game_vshooter.h"
#endif
#ifdef ENABLE_DUNGEON
  #include "game_dungeon.h"
#endif
#ifdef ENABLE_DOOM
  #include "game_doom.h"
#endif
#ifdef ENABLE_MARIO
  #include "game_mario.h"
#endif

// ── Hardware constants ────────────────────────────────────────
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_ADDR      0x3C
#define JOY_X_PIN       34
#define JOY_Y_PIN       35
#define JOY_SW_PIN      25
#define BTN_A_PIN       14
#define BTN_B_PIN       27
#define BTN_START_PIN   13
#define BUZZER_PIN      26
#define ONBOARD_LED      2

// ── Musical notes ─────────────────────────────────────────────
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_G3   196
#define NOTE_A3   220
#define NOTE_REST   0

// ── Global objects ────────────────────────────────────────────
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Preferences       prefs;

// ── Orientation ───────────────────────────────────────────────
enum Orientation { ORI_LANDSCAPE = 0, ORI_PORTRAIT = 1, ORI_LANDSCAPE_FLIP = 2, ORI_PORTRAIT_FLIP = 3 };
Orientation currentOri = ORI_LANDSCAPE;

int PLAY_W = 128;
int PLAY_H = 64;

void applyOrientation() {
  switch (currentOri) {
    case ORI_LANDSCAPE:      display.setRotation(0); PLAY_W = 128; PLAY_H = 64;  break;
    case ORI_PORTRAIT:       display.setRotation(1); PLAY_W = 64;  PLAY_H = 128; break;
    case ORI_LANDSCAPE_FLIP: display.setRotation(2); PLAY_W = 128; PLAY_H = 64;  break;
    case ORI_PORTRAIT_FLIP:  display.setRotation(3); PLAY_W = 64;  PLAY_H = 128; break;
  }
}

void resetOrientation() {
  display.setRotation(0);
  PLAY_W = 128;
  PLAY_H = 64;
}

// ── Game list ─────────────────────────────────────────────────
enum GameID {
  GAME_RACER = 0,
  GAME_TETRIS,
  GAME_SNAKE,
  GAME_ARKANOID,
  GAME_VSHOOTER,
  GAME_DUNGEON,
  GAME_DOOM,
  GAME_MARIO,
  GAME_COUNT
};
const char* GAME_NAMES[GAME_COUNT] = {
  "RACER", "TETRIS", "SNAKE", "ARKANOID", "V-SHOOTER", "DUNGEON", "DOOM", "MARIO"
};

// ── Difficulty ────────────────────────────────────────────────

const char* DIFF_NAMES[] = { "EASY", "MEDIUM", "HARD" };
Difficulty currentDiff = DIFF_MEDIUM;

// ── Top-level state machine ───────────────────────────────────
enum AppState {
  STATE_SPLASH,
  STATE_GAME_SELECT,
  STATE_TUTORIAL,
  STATE_DIFFICULTY_SELECT,
  STATE_ORI_SELECT,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_GAME_OVER,
  STATE_DEV_MENU
};
AppState appState = STATE_SPLASH;
int      selectedGame  = 0;
int      devMenuPage   = 0;

// ── Timing ────────────────────────────────────────────────────
unsigned long splashStart    = 0;
unsigned long lastFrameTime  = 0;
unsigned long gameOverEnteredAt = 0;


// ── Score storage ─────────────────────────────────────────────
int highScores[GAME_COUNT][3];  // [game][difficulty]
int joystickSensitivity = 7;     // 1 = least sensitive, 10 = most sensitive

// ── Joystick helpers ─────────────────────────────────────────
int  joyX()     { return analogRead(JOY_X_PIN); }
int  joyY()     { return analogRead(JOY_Y_PIN); }
// Deadzone: sens 1 = large dead zone (hard to trigger), sens 10 = tiny dead zone (hair-trigger)
// Range: sens1→offset=363(tight), sens10→offset=930(nearly full travel triggers)
int  joyOffset() { return constrain(300 + joystickSensitivity * 63, 363, 930); }
int  joyLow()    { return 2048 - joyOffset(); }
int  joyHigh()   { return 2048 + joyOffset(); }
bool joyBelow(int v) { return v < joyLow(); }
bool joyAbove(int v) { return v > joyHigh(); }
// Movement step size scales with sensitivity: 1px at sens 1-3, 2px at 4-7, 3px at 8-10
int  joyMoveStep()   { return (joystickSensitivity <= 3) ? 1 : (joystickSensitivity <= 7) ? 2 : 3; }
bool joyLeft() {
  if (currentOri == ORI_PORTRAIT)       return joyBelow(joyY());
  if (currentOri == ORI_LANDSCAPE_FLIP) return joyAbove(joyX());
  if (currentOri == ORI_PORTRAIT_FLIP)  return joyAbove(joyY());
  return joyBelow(joyX());
}
bool joyRight() {
  if (currentOri == ORI_PORTRAIT)       return joyAbove(joyY());
  if (currentOri == ORI_LANDSCAPE_FLIP) return joyBelow(joyX());
  if (currentOri == ORI_PORTRAIT_FLIP)  return joyBelow(joyY());
  return joyAbove(joyX());
}
bool joyUp() {
  if (currentOri == ORI_PORTRAIT)       return joyAbove(joyX());
  if (currentOri == ORI_LANDSCAPE_FLIP) return joyAbove(joyY());
  if (currentOri == ORI_PORTRAIT_FLIP)  return joyBelow(joyX());
  return joyBelow(joyY());
}
bool joyDown() {
  if (currentOri == ORI_PORTRAIT)       return joyBelow(joyX());
  if (currentOri == ORI_LANDSCAPE_FLIP) return joyBelow(joyY());
  if (currentOri == ORI_PORTRAIT_FLIP)  return joyAbove(joyX());
  return joyAbove(joyY());
}
bool joyMenuUp()   { return joyBelow(joyY()); }
bool joyMenuDown() { return joyAbove(joyY()); }
bool joyMenuLeft() { return joyBelow(joyX()); }
bool joyMenuRight(){ return joyAbove(joyX()); }

// ── Interrupt-driven jump button latch ────────────────────────
// ESP32 GPIO14 supports hardware interrupts. The ISR fires on the
// FALLING edge (button press = pin goes LOW) and sets a volatile flag.
// readButtons() consumes the flag and ORs it into btnA.pressed so that
// even a tap shorter than one 33ms frame is NEVER missed.
volatile bool btnA_isr_fired = false;
void IRAM_ATTR btnA_ISR() { btnA_isr_fired = true; }

// ── Button state (latched edge detector) ─────────────────────
// pressed latches TRUE on the rising edge and stays true until
// clearButtons() is called at the end of each frame.
// This survives any blocking sfx delays that run between
// readButtons() and the game logic that checks btnX.pressed.

Button btnA     = {BTN_A_PIN,    true, false, false};
Button btnB     = {BTN_B_PIN,    true, false, false};
Button btnStart = {BTN_START_PIN,true, false, false};
Button btnJoySW = {JOY_SW_PIN,   true, false, false};

void readButtons() {
  // Consume ISR flag FIRST so the rising-edge check below can't clobber it.
  // btnA_isr_fired is set by the FALLING-edge interrupt, guaranteeing even
  // sub-frame taps (< 33 ms) set btnA.pressed exactly once.
  if (btnA_isr_fired) {
    btnA.pressed     = true;
    btnA_isr_fired   = false;
  }

  auto update = [](Button& b) {
    bool cur = (digitalRead(b.pin) == LOW);
    if (cur && !b.lastState) b.pressed = true;  // latch, don't overwrite
    b.held      = cur;
    b.lastState = cur;
  };
  update(btnA);      // rising-edge latch (held / lastState still needed)
  update(btnB);
  update(btnStart);
  update(btnJoySW);
}

void clearButtons() {
  // Call at the END of each frame after all game logic has consumed the flags.
  btnA.pressed     = false;
  btnB.pressed     = false;
  btnStart.pressed = false;
  btnJoySW.pressed = false;
}

// ── Dev mode: B + START held 1.5 seconds ─────────────────────
unsigned long devHoldStart = 0;
bool devHolding = false;

void checkDevHold() {
  if (appState != STATE_GAME_SELECT) { devHolding = false; return; }
  bool both = btnB.held && btnStart.held;
  if (both) {
    if (!devHolding) { devHolding = true; devHoldStart = millis(); }
    else if (millis() - devHoldStart > 1500) {
      devHolding = false;
      devMenuPage = 0;
      appState = STATE_DEV_MENU;
      sfxLevelUp();
    }
  } else {
    devHolding = false;
  }
}

// ── Buzzer (LEDC PWM) ─────────────────────────────────────────
void sfxBeep()     { ledcWriteTone(0, NOTE_C5); delay(50);  ledcWriteTone(0, 0); }
void sfxConfirm()  { ledcWriteTone(0, NOTE_E4); delay(40);  ledcWriteTone(0, NOTE_G4); delay(60); ledcWriteTone(0, 0); }
void sfxBack()     { ledcWriteTone(0, NOTE_G4); delay(40);  ledcWriteTone(0, NOTE_C4); delay(60); ledcWriteTone(0, 0); }
void sfxShoot()    { ledcWriteTone(0, NOTE_A4); delay(30);  ledcWriteTone(0, 0); }
void sfxExplode()  { ledcWriteTone(0, NOTE_G3); delay(80);  ledcWriteTone(0, NOTE_A3); delay(60); ledcWriteTone(0, 0); }
void sfxGameOver() {
  int notes[] = {NOTE_C4, NOTE_B4, NOTE_A4, NOTE_G3};
  for (int n : notes) { ledcWriteTone(0, n); delay(120); }
  ledcWriteTone(0, 0);
}
void sfxLevelUp()  {
  int notes[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
  for (int n : notes) { ledcWriteTone(0, n); delay(80); }
  ledcWriteTone(0, 0);
}
void sfxMenu()  { ledcWriteTone(0, NOTE_D4); delay(35); ledcWriteTone(0, 0); }
void sfxPower() {
  int notes[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5, NOTE_G4, NOTE_E4, NOTE_C4};
  int lens[]  = {80, 80, 80, 160, 60, 60, 120};
  for (int i = 0; i < 7; i++) { ledcWriteTone(0, notes[i]); delay(lens[i]); }
  ledcWriteTone(0, 0);
}
void sfxHit()   { ledcWriteTone(0, 150); delay(40); ledcWriteTone(0, 0); }

// ── NVS helpers ───────────────────────────────────────────────
void loadAllScores() {
  prefs.begin("rb24", true);
  for (int g = 0; g < GAME_COUNT; g++)
    for (int d = 0; d < 3; d++) {
      char k[12]; sprintf(k, "g%dd%d", g, d);
      int v = prefs.getInt(k, 0);
      highScores[g][d] = (v < 0 || v > 9999999) ? 0 : v;
    }
  prefs.end();
}
void loadSettings() {
  prefs.begin("rb24", true);
  joystickSensitivity = prefs.getInt("joySens", 7);
  prefs.end();
  joystickSensitivity = constrain(joystickSensitivity, 1, 10);
}
void saveJoystickSensitivity() {
  joystickSensitivity = constrain(joystickSensitivity, 1, 10);
  prefs.begin("rb24", false);
  prefs.putInt("joySens", joystickSensitivity);
  prefs.end();
}
void saveScore(int game, int diff, int score) {
  if (score <= highScores[game][diff]) return;
  highScores[game][diff] = score;
  prefs.begin("rb24", false);
  char k[12]; sprintf(k, "g%dd%d", game, diff);
  prefs.putInt(k, score);
  prefs.end();
}
void factoryReset() {
  prefs.begin("rb24", false);
  prefs.clear();
  prefs.end();
  for (int g = 0; g < GAME_COUNT; g++)
    for (int d = 0; d < 3; d++)
      highScores[g][d] = 0;
  joystickSensitivity = 7;
}

// ── I2C bus recovery ─────────────────────────────────────────
void recoverI2CBus() {
  pinMode(21, OUTPUT); pinMode(22, OUTPUT);
  digitalWrite(21, HIGH); digitalWrite(22, HIGH);
  for (int i = 0; i < 9; i++) {
    digitalWrite(22, LOW); delayMicroseconds(5);
    digitalWrite(22, HIGH); delayMicroseconds(5);
  }
  digitalWrite(21, LOW); delayMicroseconds(5);
  digitalWrite(22, HIGH); delayMicroseconds(5);
  digitalWrite(21, HIGH); delayMicroseconds(5);
  Wire.begin();
}

// ============================================================
//  DRAW UTILITIES
// ============================================================
void drawCenteredText(int y, const char* str, int size =1) {
  display.setTextSize(size);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(str, 0, y, &x1, &y1, &w, &h);
  display.setCursor((PLAY_W - (int)w) / 2, y);
  display.print(str);
}
void drawBorderBox() {
  display.drawRect(0, 0, PLAY_W, PLAY_H, SSD1306_WHITE);
}

// Animated starfield
static int starX[24], starY[24];
static bool starsInited = false;
void initStars() {
  if (starsInited) return;
  for (int i = 0; i < 24; i++) {
    starX[i] = random(0, SCREEN_WIDTH);
    starY[i] = random(0, SCREEN_HEIGHT);
  }
  starsInited = true;
}
void scrollStarsH(int speed = 1) {
  for (int i = 0; i < 24; i++) {
    display.drawPixel(starX[i], starY[i], SSD1306_WHITE);
    starX[i] -= speed;
    if (starX[i] < 0) { starX[i] = SCREEN_WIDTH - 1; starY[i] = random(0, SCREEN_HEIGHT); }
  }
}
void scrollStarsV(int* sx, int* sy, int n, int speed = 1) {
  for (int i = 0; i < n; i++) {
    display.drawPixel(sx[i], sy[i], SSD1306_WHITE);
    sy[i] += speed;
    if (sy[i] >= PLAY_H) { sy[i] = 0; sx[i] = random(0, PLAY_W); }
  }
}

void drawShip(int x, int y) {
  display.fillRect(x+3, y,   2, 2, SSD1306_WHITE);
  display.fillRect(x+1, y+2, 6, 2, SSD1306_WHITE);
  display.fillRect(x,   y+4, 8, 1, SSD1306_WHITE);
  display.drawPixel(x+3, y-1, SSD1306_WHITE);
}

// ============================================================
//  SPLASH SCREEN
// ============================================================
void drawSplash() {
  // ──  SPECIAL HANDLER  ──
  if (btnB.held && btnJoySW.held) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    drawCenteredText(8,  "* * * * * * * * *");
    drawCenteredText(20, "BUILT BY");
    drawCenteredText(32, "V  O  I  D", 2);
    drawCenteredText(50, "* * * * * * * * *");
    display.display();
    delay(2200);
    return;   // skip normal splash draw this frame
  }

  display.clearDisplay();
  initStars();
  scrollStarsH(1);
  display.setTextColor(SSD1306_WHITE);
  drawCenteredText(6, "RETROBOX", 2);
  drawCenteredText(30, "MULTI-GAME CONSOLE");
  drawCenteredText(40, "v3.7");
  unsigned long elapsed = millis() - splashStart;
  int barW = (int)((elapsed / 2500.0f) * 100);
  if (barW > 100) barW = 100;
  display.drawRect(14, 52, 100, 6, SSD1306_WHITE);
  display.fillRect(15, 53, barW, 4, SSD1306_WHITE);
  display.display();
}

// ============================================================
//  GAME SELECT SCREEN
// ============================================================
void drawGameSelect() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(24, 1);
  display.print("SELECT GAME");
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  int start = max(0, min(selectedGame - 1, GAME_COUNT - 3));
  for (int i = 0; i < 3 && (start + i) < GAME_COUNT; i++) {
    int g = start + i;
    int y = 13 + i * 14;
    if (g == selectedGame) {
      display.fillRect(0, y - 1, 128, 12, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(6, y + 1);
    display.print(g == selectedGame ? "> " : "  ");
    display.print(GAME_NAMES[g]);
    int best = 0;
    for (int d = 0; d < 3; d++) best = max(best, highScores[g][d]);
    if (best > 0) {
      char buf[16]; sprintf(buf, "%d", best);
      display.setCursor(90, y + 1);
      display.print(buf);
    }
  }
  display.setTextColor(SSD1306_WHITE);
  for (int g = 0; g < GAME_COUNT; g++) {
    int dx = 38 + g * 9;
    if (g == selectedGame) display.fillCircle(dx, 61, 2, SSD1306_WHITE);
    else                   display.drawCircle(dx, 61, 2, SSD1306_WHITE);
  }
  display.display();
}

// ============================================================
//  DIFFICULTY SELECT
// ============================================================
void drawDifficultySelect() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  drawCenteredText(2, GAME_NAMES[selectedGame]);
  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);
  drawCenteredText(13, "DIFFICULTY");
  for (int i = 0; i < 3; i++) {
    int y = 26 + i * 12;
    if (i == (int)currentDiff) {
      display.fillRect(20, y - 1, 88, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(26, y + 1);
      display.print("> ");
    } else {
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(26, y + 1);
      display.print("  ");
    }
    display.print(DIFF_NAMES[i]);
    display.setTextColor(SSD1306_WHITE);
    if (highScores[selectedGame][i] > 0) {
      char buf[10]; sprintf(buf, "%d", highScores[selectedGame][i]);
      display.setCursor(90, y + 1);
      display.print(buf);
    }
  }
  display.setCursor(PLAY_W < 80 ? 2 : 6, 58);
  display.print(PLAY_W < 80 ? "A=OK B=Back" : "A=OK  B=Back");
  display.display();
}

// ============================================================
//  ORIENTATION SELECT
// ============================================================
void drawOriSelect() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  drawCenteredText(2, "ORIENTATION");
  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);
  drawCenteredText(5, "Choose layout:");

  const char* opts[] = {"LANDSCAPE", "PORTRAIT", "LAND.FLIP", "PORT.FLIP"};
  for (int i = 0; i < 4; i++) {
    int y = 14 + i * 11;
    if (i == (int)currentOri) {
      display.fillRect(4, y - 1, 120, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(8, y + 1);
      display.print("> ");
    } else {
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(8, y + 1);
      display.print("  ");
    }
    display.print(opts[i]);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(6, 58);
  display.print("A=Confirm  B=Back");
  display.display();
}

// ============================================================
//  PAUSE / GAME OVER
// ============================================================
void drawPause() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawCenteredText(PLAY_H / 2 - 20, "PAUSED", 2);
  drawCenteredText(PLAY_H / 2 + 4,  "START=resume");
  drawCenteredText(PLAY_H / 2 + 14, "B=Main menu");
  display.display();
}

void drawGameOverScreen(int score, bool newRecord) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  drawCenteredText(PLAY_H / 2 - 28, "GAME OVER", 2);
  display.drawLine(0, PLAY_H / 2 - 12, PLAY_W - 1, PLAY_H / 2 - 12, SSD1306_WHITE);
  char buf[24];
  sprintf(buf, "SCORE: %d", score);
  drawCenteredText(PLAY_H / 2 - 8, buf);
  sprintf(buf, "BEST:  %d", highScores[selectedGame][(int)currentDiff]);
  drawCenteredText(PLAY_H / 2 + 2, buf);
  if (newRecord) drawCenteredText(PLAY_H / 2 + 14, "** NEW RECORD! **");
  if (PLAY_W < 80)
    drawCenteredText(PLAY_H - 10, "ST=menu A=retry");
  else
    drawCenteredText(PLAY_H - 10, "START=menu  A=retry");
  display.display();
}

// ============================================================
//  DEV MENU
// ============================================================
void drawDevMenu() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.setCursor(4, 3);
  display.setTextSize(1);
  display.print("DEV [");
  display.print(devMenuPage + 1);
  display.print("/9]");
  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  char l1[32] = "", l2[32] = "", l3[32] = "";
  char bar[12] = "";
  switch (devMenuPage) {
    case 0: sprintf(l1, "FREE RAM"); sprintf(l2, "%d bytes", ESP.getFreeHeap()); break;
    case 1: sprintf(l1, "CPU FREQ"); sprintf(l2, "%d MHz", getCpuFrequencyMhz()); break;
    case 2: sprintf(l1, "UPTIME"); sprintf(l2, "%lu sec", millis() / 1000); break;
    case 3: sprintf(l1, "FLASH SIZE"); sprintf(l2, "%d KB", ESP.getFlashChipSize() / 1024);
            sprintf(l3, "Sketch:%d KB", ESP.getSketchSize() / 1024); break;
    case 4: sprintf(l1, "JOY RAW"); sprintf(l2, "X:%d  Y:%d", analogRead(JOY_X_PIN), analogRead(JOY_Y_PIN)); break;
    case 5:
            for (int i = 0; i < 10; i++) bar[i] = (i < joystickSensitivity) ? '#' : '-';
            bar[10] = '\0';
            sprintf(l1, "JOY SENS %d/10", joystickSensitivity);
            sprintf(l2, "[%s]", bar);
            sprintf(l3, "UP/DN adjust");
            break;
    case 6: sprintf(l1, "HIGH SCORES");
            sprintf(l2, "%s E:%d M:%d", GAME_NAMES[selectedGame], highScores[selectedGame][0], highScores[selectedGame][1]);
            sprintf(l3, "H:%d", highScores[selectedGame][2]); break;
    case 7: sprintf(l1, "FACTORY RESET"); sprintf(l2, "A=CONFIRM"); sprintf(l3, "Wipes all scores!"); break;
    case 8: sprintf(l1, "BUILD INFO"); sprintf(l2, "RetroBox v3.7"); sprintf(l3, "8 games  No WiFi"); break;
  }

  display.setCursor(4, 16); display.print(l1);
  display.setCursor(4, 28); display.print(l2);
  if (l3[0]) { display.setCursor(4, 40); display.print(l3); }
  display.setCursor(4, 54);
  if (devMenuPage == 5) display.print("L/R page U/D set");
  else display.print("JOY=page A=act B=exit");
  display.display();
}

// ============================================================
//  TUTORIAL SYSTEM
// ============================================================
static int tutorialPage = 0;

// ── Non-Mario games: 2-page tutorials (unchanged layout) ─────
struct TutLine { const char* a; const char* b; };
const TutLine TUTORIALS[GAME_COUNT][2][2] = {
  { {{"Dodge oncoming cars","Stay in 3 lanes"},{"L/R=steer  A=nitro","fuel bar=empty=game over"}},
    {{"Fuel pickups on road","3 HP, crashes drain HP"},{"Speed rises over time","Lv shown top-right"}} },
  { {{"Clear lines to score","stack pieces neatly"},{"Joy L/R = move","A = rotate  D = drop"}},
    {{"More lines = faster","pieces = harder"},{"Ghost piece shown","No lives -- survive!"}} },
  { {{"Eat food to grow","don't hit walls/self"},{"Joy = direction","(opposite = ignore)"}},
    {{"Snake speeds up","as you eat more"},{"3 lives","Respawns on hit"}} },
  { {{"Smash all bricks","don't let ball fall"},{"Joy L/R = paddle","A = launch ball"}},
    {{"Angle your paddle","to aim the ball"},{"3 lives","Bricks respawn each wave"}} },
  { {{"Shoot enemy waves","scrolling starfield"},{"Joy L/R = move","A/SW=fire  B=bomb"}},
    {{"Boss every 5 waves","enemies dive at you"},{"3 lives","Score x multiplier"}} },
  { {{"Explore rooms, find X","exit to reach next floor"},{"Joy=move  A=attack adj","Bump enemy to fight!"}},
    {{"g=goblin o=orc T=Troll","@=you  X=exit  p=potion"},{"[chest]=gold +50pts","^=trap -2HP  fog hides"}} },
  { {{"3D Raycaster FPS!","Compass arrow=EXIT dir"},{"Joy U/D=move  L/R=turn","A=shoot  B=strafe-mode"}},
    {{"SW btn=minimap toggle","Shoot enemies for score"},{"Easy=2  Hard=6 enemies","Exit door for next level"}} },
  // Mario slot unused — Mario uses MARIO_TUT below
  { {{"16 crafted levels","coins save for shop"},{"Joy L/R move  A=jump","B=run/fire SW=reserve"}},
    {{"Flag opens item shop","A=buy  B=next level"},{"Goomba/Koopa/Pipes","START pauses game"}} },
};

// ── Mario: 5-page detailed tutorial ──────────────────────────
// Each page has up to 4 lines of text drawn individually.
static const int MARIO_TUT_PAGES = 5;
struct MarioTutPage {
  const char* title;
  const char* lines[4];
};
static const MarioTutPage MARIO_TUT[MARIO_TUT_PAGES] = {
  {
    "MARIO - GOAL",
    {
      "Reach the FLAG at the end",
      "of each level to proceed.",
      "16 levels across 4 worlds.",
      "START to pause at any time."
    }
  },
  {
    "MOVEMENT",
    {
      "JOY LEFT/RIGHT  - Walk",
      "Hold B + JOY    - Run faster",
      "JOY DOWN        - Duck",
      "A               - Jump"
    }
  },
  {
    "JUMPING & COMBAT",
    {
      "Hold A longer = higher jump.",
      "Land ON TOP of Goombas/Koopas",
      "to stomp them for points.",
      "Touch from side = lose a life."
    }
  },
  {
    "POWER-UPS & SHOP",
    {
      "Hit ? blocks for power-ups.",
      "Collect 10 coins = +1 Life!",
      "Flag -> ITEM SHOP opens.",
      "SW = use reserve item slot."
    }
  },
  {
    "ENEMIES & TIPS",
    {
      "Goomba: stomp once to kill.",
      "Koopa: stomp->shell->kick it.",
      "Piranha: wait for it to dip.",
      "Bosses need multiple stomps!"
    }
  }
};

// Returns total pages for the current game's tutorial
static int tutTotalPages() {
  return (selectedGame == GAME_MARIO) ? MARIO_TUT_PAGES : 2;
}

// Helper: prints a string word-wrapped into the available width.
// Returns the Y position after the last printed line.
// maxCharsPerLine = floor(PLAY_W / 6) for size-1 (each char ~6px wide)
static int drawWrappedText(int x, int y, const char* str, int lineH = 9) {
  int maxW = PLAY_W - x * 2;          // usable width (mirror left margin on right)
  int cpl  = maxW / 6;                 // chars per line at text size 1
  if (cpl < 1) cpl = 1;

  const char* p = str;
  while (*p) {
    // Try to fit up to cpl chars; break at last space if line would overflow
    int len = strlen(p);
    if (len <= cpl) {
      // Whole remainder fits
      display.setCursor(x, y);
      display.print(p);
      y += lineH;
      break;
    }
    // Find last space within cpl chars
    int cut = cpl;
    for (int i = cpl; i > 0; i--) {
      if (p[i] == ' ') { cut = i; break; }
    }
    // Print up to cut
    char tmp[32];
    int n = min(cut, 31);
    strncpy(tmp, p, n);
    tmp[n] = '\0';
    display.setCursor(x, y);
    display.print(tmp);
    y += lineH;
    p += cut;
    if (*p == ' ') p++; // skip leading space
  }
  return y;
}

void drawTutorial() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  bool isPortrait = (PLAY_W <= 64);
  int total = tutTotalPages();

  // ── Header ───────────────────────────────────────────────
  // Portrait: show "NAME P/T" on one line (max ~10 chars)
  // Landscape: show "FULL TITLE (P/T)"
  char header[28];
  if (isPortrait) {
    // Short form: game abbreviation + page indicator
    char abbr[8];
    if (selectedGame == GAME_MARIO) {
      strncpy(abbr, "MARIO", 7);
    } else {
      strncpy(abbr, GAME_NAMES[selectedGame], 5);
      abbr[5] = '\0';
    }
    sprintf(header, "%s %d/%d", abbr, tutorialPage + 1, total);
  } else {
    if (selectedGame == GAME_MARIO) {
      sprintf(header, "%s (%d/%d)", MARIO_TUT[tutorialPage].title, tutorialPage + 1, total);
    } else {
      sprintf(header, "%s (%d/%d)", GAME_NAMES[selectedGame], tutorialPage + 1, total);
    }
    header[21] = '\0'; // hard cap at 21 chars for landscape
  }
  drawCenteredText(1, header);
  display.drawLine(0, 10, PLAY_W - 1, 10, SSD1306_WHITE);

  // ── Body ─────────────────────────────────────────────────
  int bodyMargin = isPortrait ? 2 : 4;
  int lineH      = isPortrait ? 10 : 9;

  if (selectedGame == GAME_MARIO) {
    const MarioTutPage& pg = MARIO_TUT[tutorialPage];
    int y = 13;
    for (int i = 0; i < 4; i++) {
      if (pg.lines[i] && pg.lines[i][0]) {
        y = drawWrappedText(bodyMargin, y, pg.lines[i], lineH);
        if (y > PLAY_H - 12) break; // stop before footer
      }
    }
  } else {
    const TutLine* pg = TUTORIALS[selectedGame][tutorialPage];
    int y = 13;
    y = drawWrappedText(bodyMargin, y, pg[0].a, lineH);
    y = drawWrappedText(bodyMargin, y, pg[0].b, lineH);
    // Divider only if there's room
    if (y + 4 < PLAY_H - 14) {
      display.drawLine(bodyMargin + 2, y + 2, PLAY_W - bodyMargin - 2, y + 2, SSD1306_WHITE);
      y += 6;
    } else {
      y += 3;
    }
    y = drawWrappedText(bodyMargin, y, pg[1].a, lineH);
    drawWrappedText(bodyMargin, y, pg[1].b, lineH);
  }

  // ── Footer: nav hints ────────────────────────────────────
  bool isLast = (tutorialPage >= total - 1);
  // Portrait needs very short hint; landscape can be verbose
  const char* hint;
  if (isPortrait) {
    hint = isLast ? "A=go B=bk" : "A=nx B=bk";
  } else {
    hint = isLast ? "A=play  B=back SW=skip" : "A=next  B=back SW=skip";
  }
  drawCenteredText(PLAY_H - 8, hint);

  display.display();
}

// ============================================================
//  GAME 1: TOP-DOWN RACER
// ============================================================


//  GAME DISPATCH
// ============================================================
void gameInit() {
  if (selectedGame == GAME_DOOM || selectedGame == GAME_MARIO) {
    if (currentOri == ORI_PORTRAIT)      currentOri = ORI_LANDSCAPE;
    if (currentOri == ORI_PORTRAIT_FLIP) currentOri = ORI_LANDSCAPE_FLIP;
    applyOrientation();
  }
  switch (selectedGame) {
    case GAME_RACER:    RacerGame::init();    break;
    case GAME_TETRIS:   TetrisGame::init();   break;
    case GAME_SNAKE:    SnakeGame::init();    break;
    case GAME_ARKANOID: ArkanoidGame::init(); break;
    case GAME_VSHOOTER: VShooterGame::init(); break;
    case GAME_DUNGEON:  DungeonGame::init();  break;
    case GAME_DOOM:     DoomGame::init();     break;
    case GAME_MARIO:    MarioGame::init();    break;
  }
}
void gameUpdate() {
  switch (selectedGame) {
    case GAME_RACER:    RacerGame::update();    break;
    case GAME_TETRIS:   TetrisGame::update();   break;
    case GAME_SNAKE:    SnakeGame::update();    break;
    case GAME_ARKANOID: ArkanoidGame::update(); break;
    case GAME_VSHOOTER: VShooterGame::update(); break;
    case GAME_DUNGEON:  DungeonGame::update();  break;
    case GAME_DOOM:     DoomGame::update();     break;
    case GAME_MARIO:    MarioGame::update();    break;
  }
}
void gameDraw() {
  switch (selectedGame) {
    case GAME_RACER:    RacerGame::draw();    break;
    case GAME_TETRIS:   TetrisGame::draw();   break;
    case GAME_SNAKE:    SnakeGame::draw();    break;
    case GAME_ARKANOID: ArkanoidGame::draw(); break;
    case GAME_VSHOOTER: VShooterGame::draw(); break;
    case GAME_DUNGEON:  DungeonGame::draw();  break;
    case GAME_DOOM:     DoomGame::draw();     break;
    case GAME_MARIO:    MarioGame::draw();    break;
  }
}
bool gameIsOver() {
  switch (selectedGame) {
    case GAME_RACER:    return RacerGame::isOver();
    case GAME_TETRIS:   return TetrisGame::isOver();
    case GAME_SNAKE:    return SnakeGame::isOver();
    case GAME_ARKANOID: return ArkanoidGame::isOver();
    case GAME_VSHOOTER: return VShooterGame::isOver();
    case GAME_DUNGEON:  return DungeonGame::isOver();
    case GAME_DOOM:     return DoomGame::isOver();
    case GAME_MARIO:    return MarioGame::isOver();
  }
  return false;
}
int gameGetScore() {
  switch (selectedGame) {
    case GAME_RACER:    return RacerGame::getScore();
    case GAME_TETRIS:   return TetrisGame::getScore();
    case GAME_SNAKE:    return SnakeGame::getScore();
    case GAME_ARKANOID: return ArkanoidGame::getScore();
    case GAME_VSHOOTER: return VShooterGame::getScore();
    case GAME_DUNGEON:  return DungeonGame::getScore();
    case GAME_DOOM:     return DoomGame::getScore();
    case GAME_MARIO:    return MarioGame::getScore();
  }
  return 0;
}
bool gameIsNewRecord() {
  switch (selectedGame) {
    case GAME_RACER:    return RacerGame::isNewRecord();
    case GAME_TETRIS:   return TetrisGame::isNewRecord();
    case GAME_SNAKE:    return SnakeGame::isNewRecord();
    case GAME_ARKANOID: return ArkanoidGame::isNewRecord();
    case GAME_VSHOOTER: return VShooterGame::isNewRecord();
    case GAME_DUNGEON:  return DungeonGame::isNewRecord();
    case GAME_DOOM:     return DoomGame::isNewRecord();
    case GAME_MARIO:    return MarioGame::isNewRecord();
  }
  return false;
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(80);

  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();

  ledcSetup(0, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  pinMode(BTN_A_PIN,     INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_A_PIN), btnA_ISR, FALLING);
  pinMode(BTN_B_PIN,     INPUT_PULLUP);
  pinMode(BTN_START_PIN, INPUT_PULLUP);
  pinMode(JOY_SW_PIN,    INPUT_PULLUP);
  pinMode(ONBOARD_LED,   OUTPUT);
  digitalWrite(ONBOARD_LED, LOW);

  recoverI2CBus();
  bool oledOk = false;
  for (int attempt = 0; attempt < 5 && !oledOk; attempt++) {
    if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) oledOk = true;
    else { recoverI2CBus(); delay(100); }
  }
  if (!oledOk) {
    while (true) {
      digitalWrite(ONBOARD_LED, HIGH); delay(200);
      digitalWrite(ONBOARD_LED, LOW);  delay(200);
    }
  }
  display.setTextColor(SSD1306_WHITE);
  display.setRotation(0);
  display.clearDisplay();
  display.display();

  loadAllScores();
  loadSettings();

  splashStart = millis();
  appState = STATE_SPLASH;
  sfxPower();
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  unsigned long now = millis();
  if (now - lastFrameTime < FRAME_MS) return;
  lastFrameTime = now;

  readButtons();
  checkDevHold();

  switch (appState) {

    case STATE_SPLASH:
      drawSplash();
      if (millis() - splashStart > 2800 || btnStart.pressed) {
        resetOrientation();
        appState = STATE_GAME_SELECT;
      }
      break;

    case STATE_GAME_SELECT: {
      static unsigned long lastNav = 0;
      if (millis() - lastNav > 160) {
        if (joyMenuUp()   && selectedGame > 0)              { selectedGame--; sfxMenu(); lastNav = millis(); }
        if (joyMenuDown() && selectedGame < GAME_COUNT - 1) { selectedGame++; sfxMenu(); lastNav = millis(); }
      }
      drawGameSelect();
      if (btnA.pressed) { sfxConfirm(); tutorialPage = 0; appState = STATE_TUTORIAL; }
      if (btnStart.pressed && !devHolding) { sfxConfirm(); tutorialPage = 0; appState = STATE_TUTORIAL; }
      break;
    }

    case STATE_TUTORIAL: {
      drawTutorial();
      int totPages = tutTotalPages();
      if (btnA.pressed) {
        if (tutorialPage < totPages - 1) { tutorialPage++; sfxMenu(); }
        else { sfxConfirm(); appState = STATE_DIFFICULTY_SELECT; }
      }
      if (btnB.pressed) {
        if (tutorialPage > 0) { tutorialPage--; sfxMenu(); }
        else { sfxBack(); appState = STATE_GAME_SELECT; }
      }
      // SW (joystick button) or START = skip ALL tutorial pages immediately
      if (btnJoySW.pressed || btnStart.pressed) { sfxConfirm(); tutorialPage = 0; appState = STATE_DIFFICULTY_SELECT; }
      break;
    }

    case STATE_DIFFICULTY_SELECT: {
      static unsigned long lastNav2 = 0;
      if (millis() - lastNav2 > 160) {
        if (joyMenuUp()   && (int)currentDiff > 0) { currentDiff = (Difficulty)((int)currentDiff - 1); sfxMenu(); lastNav2 = millis(); }
        if (joyMenuDown() && (int)currentDiff < 2) { currentDiff = (Difficulty)((int)currentDiff + 1); sfxMenu(); lastNav2 = millis(); }
      }
      drawDifficultySelect();
      if (btnA.pressed) { sfxConfirm(); appState = STATE_ORI_SELECT; }
      if (btnB.pressed) { sfxBack();    appState = STATE_GAME_SELECT; }
      break;
    }

    case STATE_ORI_SELECT: {
      static unsigned long lastNavO = 0;
      if (millis() - lastNavO > 160) {
        if (joyMenuUp()   && (int)currentOri > 0) { currentOri = (Orientation)((int)currentOri - 1); sfxMenu(); lastNavO = millis(); }
        if (joyMenuDown() && (int)currentOri < 3) { currentOri = (Orientation)((int)currentOri + 1); sfxMenu(); lastNavO = millis(); }
      }
      drawOriSelect();
      if (btnA.pressed) { sfxConfirm(); applyOrientation(); gameInit(); appState = STATE_PLAYING; }
      if (btnB.pressed) { sfxBack(); appState = STATE_DIFFICULTY_SELECT; }
      break;
    }

    case STATE_PLAYING:
      gameUpdate();
      gameDraw();
      if (gameIsOver()) {
        saveScore(selectedGame, (int)currentDiff, gameGetScore());
        gameOverEnteredAt = millis();
        appState = STATE_GAME_OVER;
        break;
      }
      if (btnStart.pressed) { appState = STATE_PAUSED; sfxMenu(); }
      break;

    case STATE_PAUSED:
      drawPause();
      if (btnStart.pressed) { appState = STATE_PLAYING; sfxConfirm(); }
      if (btnB.pressed) { sfxBack(); resetOrientation(); appState = STATE_GAME_SELECT; }
      break;

    case STATE_GAME_OVER:
      drawGameOverScreen(gameGetScore(), gameIsNewRecord());
      if (millis() - gameOverEnteredAt > 1500) {
        if (btnStart.pressed) { sfxBack(); resetOrientation(); appState = STATE_GAME_SELECT; }
        if (btnA.pressed) { sfxConfirm(); applyOrientation(); gameInit(); appState = STATE_PLAYING; }
      }
      break;

    case STATE_DEV_MENU: {
      static unsigned long lastDevNav = 0;
      drawDevMenu();
      if (millis() - lastDevNav > 220) {
        if (joyMenuRight() && devMenuPage < 8) { devMenuPage++; sfxMenu(); lastDevNav = millis(); }
        if (joyMenuLeft()  && devMenuPage > 0) { devMenuPage--; sfxMenu(); lastDevNav = millis(); }
        if (devMenuPage == 5 && joyMenuUp() && joystickSensitivity < 10) {
          joystickSensitivity++;
          saveJoystickSensitivity();
          sfxMenu();
          lastDevNav = millis();
        }
        if (devMenuPage == 5 && joyMenuDown() && joystickSensitivity > 1) {
          joystickSensitivity--;
          saveJoystickSensitivity();
          sfxMenu();
          lastDevNav = millis();
        }
      }
      if (btnA.pressed) {
        if (devMenuPage == 7) { factoryReset(); sfxGameOver(); }
        else sfxConfirm();
      }
      if (btnB.pressed || btnStart.pressed) { sfxBack(); appState = STATE_GAME_SELECT; }
      break;
    }
  }
  // Clear latched pressed flags AFTER all game logic for this frame has run.
  clearButtons();
}

// ============================================================
//  END OF FILE — RETROBOX v3.3  | built by VOID
// ============================================================
