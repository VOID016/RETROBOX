# RETROBOX — ESP32 Handheld Retro Console

> A DIY handheld game console you build yourself — 8 games, one ESP32, under ₹500 in parts.

Built on an **ESP32 DevKit V1** with a 128×64 OLED, analog joystick, 3 buttons, and a piezo buzzer. Flash it, wire it, play it. Adding your own game takes about 30 minutes and zero changes to the core firmware.

\---

## 🎮 Games Included

|Game|Genre|Highlights|
|-|-|-|
|**Mario Platformer**|Platformer|4 worlds · 16 levels · shop system · bosses|
|**DOOM-Nano**|Raycaster FPS|Ray-cast renderer · enemy AI · minimap|
|**Tetris**|Puzzle|Classic falling-block, escalating speed|
|**Snake**|Classic Arcade|3 lives · grows faster as you score|
|**Top-Down Racer**|Racing|Scrolling road · fuel · nitro boost|
|**Arkanoid**|Breakout|Multi-level brick breaker|
|**Vertical Shooter**|Shoot-em-up|Enemy waves · bomb charges · boss scaling|
|**Dungeon Crawler**|RPG|Procedural rooms · fog of war · turn-based combat|

All games support **Easy / Normal / Hard** difficulty and save high scores to flash memory.

\---

## ⚡ Hardware

|Component|Part|Notes|
|-|-|-|
|Microcontroller|ESP32 DevKit V1|38-pin version recommended|
|Display|SSD1306 128×64 OLED|I2C — any brand works|
|Joystick|KY-023 analog module|2-axis + click button|
|Buttons|Tactile switches|3× momentary push buttons|
|Audio|Passive piezo buzzer|NOT active buzzer|
|Power|Micro-USB or 3.7V LiPo|LiPo needs a TP4056 charge module|

> ⚠️ \*\*Passive buzzer only.\*\* An active buzzer will not produce tones — only a fixed click.

\---

## 📌 Wiring

All connections at a glance:

|ESP32 GPIO|Connect to|
|-|-|
|3.3V|OLED VCC, Joystick VCC|
|GND|OLED GND, Joystick GND, all button legs (×1 per button), Buzzer −|
|GPIO 21|OLED SDA|
|GPIO 22|OLED SCL|
|GPIO 34|Joystick VRx (horizontal axis)|
|GPIO 35|Joystick VRy (vertical axis)|
|GPIO 25|Joystick SW (click)|
|GPIO 14|Button A (fire / confirm)|
|GPIO 27|Button B (back / special)|
|GPIO 13|Button START|
|GPIO 26|Buzzer +|

> Buttons use \*\*internal pull-up resistors\*\* — no external resistors needed.  
> Full schematic and troubleshooting: see \[WIRING\_DIAGRAM.md](WIRING\_DIAGRAM.md)

\---

## 🚀 Flashing (Step-by-Step)

### 1 — Install the tools

* [Arduino IDE 2.x](https://www.arduino.cc/en/software)
* ESP32 board package: follow [this guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
* Two libraries via **Tools → Manage Libraries**:

  * `Adafruit SSD1306`
  * `Adafruit GFX Library`

### 2 — Open the project

Open `RETROBOX\_3\_7.ino` in Arduino IDE. All other `.h` files must stay in the **same folder**.

### 3 — Select your board

**Tools → Board → ESP32 Arduino → ESP32 Dev Module**

Set Upload Speed to `921600` for faster flashing.

### 4 — Flash

Connect your ESP32 via USB, select the correct COM port, and click **Upload** (→).

The console boots directly to the game select menu.

\---

## 🎛️ Customizing Your Build

RETROBOX is designed to be modified. Here are the most common things you'll want to change.

### Enable / Disable Games

Open `games\_config.h`. Each game is one line:

```cpp
#define ENABLE\_MARIO
#define ENABLE\_DOOM
#define ENABLE\_SNAKE
// ... etc.
```

Comment out any game you don't want:

```cpp
// #define ENABLE\_DOOM    ← disabled, won't compile in
#define ENABLE\_SNAKE      ← enabled
```

> 💡 If you're running low on RAM, disable `DOOM` or `MARIO` first — they use the most memory.

### Change Pin Assignments

If your wiring differs from the default, edit the `#define` block at the top of `RETROBOX\_3\_7.ino`:

```cpp
#define PIN\_JOY\_X     34
#define PIN\_JOY\_Y     35
#define PIN\_JOY\_SW    25
#define PIN\_BTN\_A     14
#define PIN\_BTN\_B     27
#define PIN\_BTN\_START 13
#define PIN\_BUZZER    26
#define OLED\_SDA      21
#define OLED\_SCL      22
#define OLED\_ADDR   0x3C
```

Change any value — the rest of the firmware adapts automatically.

### Tune Joystick Sensitivity

If the joystick drifts or feels sluggish, adjust `JOY\_DEAD` in `RETROBOX\_3\_7.ino`:

```cpp
#define JOY\_DEAD 400   // increase to 600 if cursor drifts untouched
```

### Add Your Own Game

The architecture is built for this. Every game is a self-contained C++ namespace in its own `.h` file, exposing exactly 5 functions. The main firmware never needs structural changes.

See [**ADDING\_A\_GAME.md**](ADDING_A_GAME.md) — full walkthrough with a starter template, all available APIs, and tips for hitting 60fps on the ESP32.

\---

## 🎮 Controls

|Action|Control|
|-|-|
|Navigate menus|Joystick|
|Confirm / Fire|Button A|
|Back / Special action|Button B|
|Pause / Start|START button|
|Skip tutorial|Joystick click (SW)|

**Developer Menu:** Hold **B + START** for 1.5 seconds on the game select screen — gives access to joystick sensitivity calibration, high score reset, and system info.

> 🕹️ There may or may not be something hidden in this firmware. Poke around.

\---

## 📁 Project Structure

```
retrobox-esp32/
├── RETROBOX\_3\_7.ino       Main firmware
├── games\_config.h         ← Start here to enable/disable games
├── game\_mario.h           Mario Platformer
├── game\_doom.h            DOOM-Nano raycaster
├── game\_tetris.h          Tetris
├── game\_snake.h           Snake
├── game\_racer.h           Top-Down Racer
├── game\_arkanoid.h        Arkanoid
├── game\_vshooter.h        Vertical Shooter
├── game\_dungeon.h         Dungeon Crawler
├── ADDING\_A\_GAME.md       How to write your own game
├── WIRING\_DIAGRAM.md      Hardware assembly + troubleshooting
└── README.md              This file
```

\---

## 🔋 Power Options

|Option|Notes|
|-|-|
|USB (micro/USB-C)|Easiest — power from any phone charger or PC|
|3.7V LiPo battery|Add a TP4056 charge module with protection circuit|
|3× AA batteries|Use a 3.3V regulator to step down from \~4.5V|

> The ESP32 draws \~80–100mA during gameplay. A 1000mAh LiPo lasts roughly 8–10 hours.

\---

## 🐛 Common Issues

\## 🐛 Known Issues \& Limitations



\### Hardware



\## 🐛 Known Issues \& Limitations



\### Hardware



| Issue                            | Cause                     | Fix                                                                                 |

| -------------------------------- | ------------------------- | ----------------------------------------------------------------------------------- |

| Screen flickers or shows garbage | Loose SDA/SCL wire        | Re-seat the OLED connections at GPIO 21/22                                          |

| Screen blank after flashing      | Wrong I2C address         | Change `OLED\_ADDR` from `0x3C` to `0x3D` in the `.ino`                              |

| Screen blank, address correct    | OLED on 5V                | Move OLED VCC to the \*\*3.3V\*\* pin — 5V will damage it                               |

| Joystick cursor drifts at rest   | ADC noise / clone module  | Increase `JOY\_DEAD` from `400` to `600–800`                                         |

| Joystick feels sluggish          | Dead zone too wide        | Decrease `JOY\_DEAD` to `300`                                                        |

| Buttons register double presses  | No hardware debounce      | Solder a 100nF capacitor between the button pin and GND                             |

| No sound at all                  | Active buzzer used        | Replace with a \*\*passive\*\* buzzer                                                   |

| Sound plays but wrong pitch      | Wrong pin                 | Check `PIN\_BUZZER` matches your actual wiring                                       |

| ESP32 won't enter upload mode    | Auto-reset not triggering | Hold the \*\*BOOT\*\* button while clicking Upload, release after “Connecting…” appears |

| Upload fails on Linux/Mac        | Serial port permissions   | Run `sudo chmod 666 /dev/ttyUSB0` (or your port)                                    |



\---



\### Software / Firmware



| Issue                                      | Cause                             | Fix                                                                                             |

| ------------------------------------------ | --------------------------------- | ----------------------------------------------------------------------------------------------- |

| Compilation error: "too large for RAM"     | Too many games enabled            | Disable `DOOM` and/or `MARIO` in `games\_config.h` — they use the most RAM                       |

| Compilation error after adding custom game | Missing dispatcher entry          | Re-read Step 5 of `ADDING\_A\_GAME.md` — all 5 dispatchers must have a `case`                     |

| High scores reset on every boot            | Preferences namespace collision   | Make sure no two games use the same key string in `prefs.putInt()`                              |

| Game crashes / reboots mid-play            | Stack overflow or heap exhaustion | Reduce `MAX\_LEN` / array sizes in your custom game, or disable another game to free RAM         |

| Dungeon Crawler freezes on boot            | `malloc()` failed                 | Not enough heap — disable one or two other games                                                |

| DOOM runs slowly                           | Normal on lower clock speeds      | Set ESP32 CPU frequency to \*\*240 MHz\*\*: Tools → CPU Frequency → 240MHz                          |

| Menu shows blank game name slot            | `GAME\_NAMES` array out of sync    | Make sure your game's name entry matches its position in the `GameID` enum                      |

| Difficulty setting not saving              | Expected behavior                 | Difficulty resets to Normal on reboot by design — selected fresh each game launch from the menu |



\---



\### Clone Hardware Quirks (Common with Cheap Modules)



| Symptom                                  | Likely Cause                                 | Fix                                                                                      |

| ---------------------------------------- | -------------------------------------------- | ---------------------------------------------------------------------------------------- |

| MPU-6050 / GY-521 gyro reads all zeros   | Clone chip — offset registers non-functional | Do \*\*not\*\* write to offset registers (`0x13–0x18`); use software bias correction instead |

| OLED shows mirrored or upside-down image | Clone with different orientation             | Add `display.setRotation(2)` after `display.begin()`                                     |

| ESP32 resets randomly during gameplay    | Insufficient power from USB                  | Try a different USB cable (data cable, not charge-only) or a higher-current charger      |

| ADC readings noisy or unstable           | ESP32 ADC2 used while Wi-Fi active           | Wi-Fi is not used in RETROBOX — if enabled elsewhere, disable it                         |



Full troubleshooting: [WIRING\_DIAGRAM.md](WIRING_DIAGRAM.md)

\---

## 📜 License

MIT — free to use, modify, build upon, and distribute. See [LICENSE](LICENSE).

\---

*Built with ❤️ for the maker community.*

