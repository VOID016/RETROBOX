# RETROBOX

> An open-source handheld retro gaming console built on the \*\*ESP32\*\* microcontroller with a \*\*128×64 SSD1306 OLED\*\* display.

RETROBOX ships with **8 built-in games** spanning genres from classic arcade puzzlers to a real-time raycaster FPS — all rendered on a monochrome screen no larger than a postage stamp. It is designed for first-time visitors and hobbyists who want a fun, hackable platform to play with and build on.

\---

## Table of Contents

* [Hardware Controls](#hardware-controls)
* [Difficulty System](#difficulty-system)
* [Games](#games)

  * [🏎 Top-Down Racer](#-top-down-racer)
  * [🧱 Tetris](#-tetris)
  * [🐍 Snake](#-snake)
  * [🏓 Arkanoid](#-arkanoid)
  * [🚀 Vertical Shooter](#-vertical-shooter)
  * [🏰 Dungeon Crawler](#-dungeon-crawler)
  * [🔫 Doom-Nano](#-doom-nano)
  * [🍄 Mario Platformer](#-mario-platformer)
* [Enabling \& Disabling Games](#enabling--disabling-games)
* [Adding a Custom Game](#adding-a-custom-game)

\---

## Hardware Controls

|Input|Description|
|-|-|
|**Analog Joystick**|4-directional movement; sensitivity adjustable in the settings menu.|
|**Joystick SW (push)**|Click the thumbstick — mapped to a special action per game.|
|**Button A**|Primary action: jump / shoot / launch / rotate.|
|**Button B**|Secondary action: run / bomb / strafe-toggle.|
|**Button START**|Pause / unpause the current game.|

\---

## Difficulty System

Select your difficulty from the main menu before starting a game. High scores are stored **separately per game and per difficulty**, persisting across power cycles.

|Difficulty|Description|
|-|-|
|**Easy**|More forgiving timings, fewer enemies, slower pacing. Ideal for learning the controls.|
|**Medium**|The intended, balanced experience.|
|**Hard**|Faster enemies, higher damage, stricter resource pressure.|

\---

## Games

### 🏎 Top-Down Racer

*Weave through traffic, collect fuel, and activate nitro boosts to beat your high score.*

An endless vertical-scrolling road game. Pilot a car down a 3-lane highway, dodging oncoming traffic while managing your fuel gauge. The road speeds up automatically, and a nitro system lets you blast through dense traffic — at the cost of precious fuel.

**Mechanics**

* 3 lanes with procedurally placed opponents and roadside obstacles.
* Fuel gauge constantly drains; pick up glowing **F** canisters to refill.
* Up to **3 nitro charges** — activating nitro adds speed and widens your steering range. Charges replenish only after a crash.
* Two damage levels: first collision is cosmetic; second costs a life.
* Speed ramps up every 10 s (Easy: 15 s / Hard: 6 s) up to a difficulty-scaled cap.
* Score formula: `frameCount / 3 × current speed`.

**HUD**



|Element|Position|Meaning|
|-|-|-|
|`S:NNNN`|Top-left|Current score|
|`\[♥]\[♥]\[♥]`|Top-centre|Remaining lives (filled squares, 1 per life)|
|`LN`|Top-right|Current speed level|
|Fuel bar|Bottom-left half|Remaining fuel — depletes constantly; collect **F** pickups to refill|
|Nitro pips|Bottom-right half|Up to 3 charges; filled = available, outline = spent|
|`N!`|Bottom-right|Appears while nitro is active|

**Controls**

|Input|Action|
|-|-|
|Joystick L/R|Steer left / right|
|Button A|Activate nitro|
|Button START|Pause|

**Difficulty**

|Level|Details|
|-|-|
|Easy|0.55× multiplier, slow drain (\~375 ms/2 units), max speed 4.|
|Medium|1.0× multiplier, \~245 ms/2 units drain, max speed 6.|
|Hard|1.3× multiplier, \~140 ms/2 units drain, max speed 8, faster ramp intervals.|

**Tips**

* Stay in the middle lane when the road is clear — it gives you the most escape routes.
* Collect every fuel canister; fuel starvation is the #1 cause of death at high speeds.
* Save nitro for dense traffic clusters, not open road.

\---

### 🧱 Tetris

*The classic falling-block puzzle — clear lines, level up, and chase the high score.*

Seven classic tetrominoes (I, O, T, S, Z, L, J) fall from the top of a 10-column board. Slide and rotate them to complete full horizontal lines. The game ends when a new piece can no longer spawn.

**Mechanics**

* Standard 10-wide board, up to 20 rows tall.
* **Ghost piece** shows exactly where the current piece will land.
* **Next-piece preview** displayed to the right of the board.
* Scoring: 1 line = 100 pts, 2 = 300, 3 = 500, 4 (Tetris!) = 800 — all multiplied by current level.
* Level increases every 10 lines cleared; drop speed accelerates.
* Hold left/right with a 280 ms auto-repeat delay for fine positioning.
* Soft-drop forces the piece down at 60 ms intervals.

**HUD**



|Element|Position|Meaning|
|-|-|-|
|Board outline|Left side|10×(up to 20) cell play field|
|Ghost piece|In-board|Outline of where the active piece will land|
|`NXT` + preview|Right sidebar|The next tetromino shown at 3×3 px per block|
|`LV` + number|Right sidebar|Current level (increments every 10 lines)|
|`SC` + number|Right sidebar|Score ÷ 100 (truncated to fit)|

> \*\*Note:\*\* The sidebar is only drawn when `PLAY\_W` is wide enough (≥ \~54 px after the board). On very narrow displays the sidebar is hidden; score is not shown on-screen in that case — check it on the game-over screen.

**Controls**

|Input|Action|
|-|-|
|Joystick L/R|Move piece left / right|
|Joystick Down|Soft drop|
|Button A|Rotate 90° clockwise|
|Button START|Pause|

**Difficulty**

|Level|Details|
|-|-|
|Easy|Start 800 ms/drop, min 200 ms.|
|Medium|Start 600 ms, min 120 ms.|
|Hard|Start 400 ms, min 80 ms.|

**Tips**

* Keep one column open for I-pieces to score Tetrises.
* Watch the ghost piece, not the falling piece — it tells you exactly where to commit.
* Use soft-drop early on to bank points before speed ramps up.

\---

### 🐍 Snake

*Grow your snake by eating food, avoid the walls and your own tail — 3 lives to spend.*

Classic Snake on a grid. Your snake starts at length 3 and grows each time it eats the blinking food pellet. Hitting a wall or your own body costs a life; lose all 3 and the game ends.

**Mechanics**

* 4×4 pixel grid cells filling the full play area below the HUD.
* Snake grows by 1 segment per food eaten; +10 score per food.
* Every 5 foods collected, speed increases (move interval ×0.88) and the level ticks up.
* On collision, the snake resets to length 3 at centre — score carries over.
* Food blinks between solid and outline to make it easy to spot.
* Direction input is **buffered one step ahead** — you can pre-queue a turn.

**HUD**



|Element|Position|Meaning|
|-|-|-|
|`S:NNNN`|Top-left|Current score (10 pts per food eaten)|
|`L:N`|Top-right|Remaining lives (starts at 3; a wall or self-collision costs 1)|
|Separator line|Row 8|Divides HUD from play area|

> The level number is not shown in the HUD — speed increases are silent. Watch the snake's movement tempo to judge current level.

**Controls**

|Input|Action|
|-|-|
|Joystick|Change direction (180° reversal blocked)|
|Button START|Pause|

**Difficulty**

|Level|Details|
|-|-|
|Easy|Move every 200 ms.|
|Medium|Move every 150 ms.|
|Hard|Move every 100 ms.|

**Tips**

* Work in a consistent spiral or zigzag pattern to avoid boxing yourself in.
* Use the direction buffer to flick the joystick before the snake actually moves — useful for tight corners.

\---

### 🏓 Arkanoid

*Bounce the ball, break the bricks, survive as long as you can.*

A faithful brick-breaker. Control a paddle at the bottom to keep a ball bouncing. Clear all bricks to advance to the next wave, which adds a new row and slightly increases ball speed.

**Mechanics**

* Ball angle off the paddle is influenced by **where it lands** — centre = steep, edge = wide.
* Brick point value = `(BRICK\_ROWS - row) × 10 × level`; top-row bricks score the most.
* 3 lives. Ball falling below the paddle costs one life; resets to paddle centre requiring a manual launch.
* Ball speed: base `1.8 + level × 0.15`, capped at 3.5. Wave clear applies an extra 1.1× multiplier (cap 4.0).
* High score auto-saved whenever a new record is set mid-game.

**HUD**



|Element|Position|Meaning|
|-|-|-|
|`S:NNNN`|Top-left|Current score|
|`L:N`|Top-centre|Remaining lives (starts at 3)|
|`W:N`|Top-right|Current wave / level number|
|Separator line|Row 8|Divides HUD from play area|
|`A = LAUNCH`|Lower-centre|Reminder shown when ball is held on paddle, waiting for launch|

**Controls**

|Input|Action|
|-|-|
|Joystick L/R|Move paddle left / right|
|Button A|Launch ball|
|Button START|Pause|

**Difficulty**

|Level|Details|
|-|-|
|Easy|3 brick rows per wave.|
|Medium|4 brick rows per wave.|
|Hard|5 brick rows per wave.|

**Tips**

* Aim for top rows first — they score more and clear the board faster.
* A hit near the paddle edge creates a wide-angle shot useful for corner bricks.
* Wait for the ball to hover stably over the paddle before pressing A to launch.

\---

### 🚀 Vertical Shooter

*Pilot your ship through enemy waves, earn bombs, and face escalating threats.*

A classic vertical shoot-'em-up (SHMUP). Your triangle-shaped fighter moves freely in the lower half of the screen while enemy ships descend in waves. Every 10 kills earns a new wave designation and a bonus bomb charge.

**Mechanics**

* **3 enemy types:** Type 0 (basic descent), Type 1 (zigzag drift, fires back), Type 2 (armoured with 3 HP, fires back).
* Up to 4 player bullets and 6 enemy bullets on-screen simultaneously.
* **Bomb** destroys all on-screen enemies and clears enemy bullets. Starts with 2 charges; gains 1 (max 3) each wave.
* Fire rate and enemy speed both scale per wave.
* Scrolling star field background adds a sense of motion.

**HUD**



|Element|Position|Meaning|
|-|-|-|
|`S:NNNN`|Top-left|Current score|
|`WN`|Top-centre|Current wave number|
|`^^^`|Top-right|Remaining lives — one `^` (caret) per life|
|`B:` + `\*` pips|Bottom-left|Bomb charges: `\*` = available, blank = spent|

**Controls**

|Input|Action|
|-|-|
|Joystick|Move ship in all 4 directions|
|Button A / Joystick SW (held)|Continuous fire|
|Button B|Detonate bomb|
|Button START|Pause|

**Difficulty**

|Level|Details|
|-|-|
|Easy|0.6× speed/fire multiplier, fewer on-screen enemies.|
|Medium|1.0× baseline.|
|Hard|1.4× multiplier, faster enemies and bullets.|

**Tips**

* Stay near the bottom — it maximises the time for enemy bullets to miss you.
* Use bombs when surrounded, not when you're already safe.
* Prioritise Type 1 and Type 2 enemies; they shoot back and are the main source of damage.

\---

### 🏰 Dungeon Crawler

*Explore procedural rooms, battle monsters, find potions and chests, descend ever deeper.*

A turn-based top-down dungeon crawler with procedurally generated maps. Every action you take (move or attack) triggers an enemy turn. Find the exit tile **X** to descend to the next floor and grow stronger.

**Mechanics**

* Up to 8 random rooms per floor connected by L-shaped corridors.
* **Fog of war** — each move reveals a radius-3 circle. Revealed tiles remain visible.
* **4 enemy types:** Goblin (low HP/ATK), Orc (medium), Troll (high HP), Serpent (venomous extra damage).
* Enemies within 8 tiles chase you; beyond 8 they wander.
* **Special tiles:** Potion (restores 3–5 HP), Chest (score bonus), Trap (2 damage), Exit (descend).
* Player stats grow per floor: max HP +2, partially healed, attack +1.
* Camera follows the player, clamped to map edges.

**HUD**



|Element|Position|Meaning|
|-|-|-|
|`HP N/N`|Top-left|Current HP / maximum HP|
|`LvN`|Top-centre|Current dungeon floor / level|
|`ScN`|Top-right|Cumulative score|
|Separator line|Row 11|Divides status bar from dungeon view|
|`@`|In-map|Your character's position|
|Enemy symbols|In-map|`g` = Goblin, `o` = Orc, `T` = Troll, `S` = Serpent|
|`p`|In-map|Potion pickup|
|Chest outline|In-map|Treasure chest|
|`X` in a box|In-map|Exit to next floor|
|Message bar|Bottom 10 px|White-on-black inverted strip showing combat/event messages (e.g. *"Hit o for 2 dmg"*, *"Trap! -2HP!"*). Disappears after \~1.8 s.|

> \*\*Narrow display note:\*\* On screens narrower than 80 px, the status bar uses a shorter format (`HP8/10 L2 S340`) to fit.

**Controls**

|Input|Action|
|-|-|
|Joystick|Move / attack (bump into an enemy to attack)|
|Button A|Attack adjacent enemy while stationary|
|Button START|Pause|

**Difficulty**

|Level|Details|
|-|-|
|Easy|Fewer enemies (max −2), Serpent deals 2 damage.|
|Medium|3 + floor enemies, standard damage.|
|Hard|+2 extra enemies, Serpent deals 3 damage, more frequent spawns.|

**Tips**

* Attack by walking into an enemy — no separate button needed.
* Explore fully before heading to the exit: potions and chests are permanent score.
* Serpents (S) are the most dangerous; kill them first before engaging groups.
* Use corridors to bottleneck enemies and fight one at a time.

\---

### 🔫 Doom-Nano

*A real-time raycaster FPS — navigate 8 hand-crafted maps, kill Imps, find the exit.*

A first-person shooter rendered with a classic raycasting engine (in the spirit of Wolfenstein 3D / DOOM) directly on the OLED. Eight hand-crafted 20×14-tile maps are randomly selected each run. Shoot Imps before they close in, find the marked exit tile, and advance to deeper levels with more enemies.

**Mechanics**

* Per-column raycasting, distance-based 4-level dithered shading, fisheye correction, N/S wall phase-shift for visual depth.
* Enemy sprites (8×12 px) rendered with Z-buffer occlusion.
* Enemy HP bar shown above each Imp within 8 tiles.
* **Imp AI:** chases within 8 tiles at difficulty-scaled speed; wanders randomly beyond 8 tiles.
* **Shooting:** fires a cone of ±0.28 radians. Cooldown: 400 ms (Easy/Medium), 250 ms (Hard).
* Muzzle flash and gun recoil animation on every shot.
* **Toggle minimap** (Joystick SW) — 2 px/tile overlay with blinking exit marker and player direction arrow.
* **HUD compass** always points toward the level exit.
* Clearing all Imps spawns the next wave (+1 per 2 levels) and restores 30% health.
* Stepping on the exit tile loads a new random map and partially heals the player.

**HUD**



|Element|Position|Meaning|
|-|-|-|
|3-D view|Upper portion|Raycasted walls with 4-level distance shading; floor scanlines; black ceiling|
|Gun sprite|Lower-centre of 3-D view|Shotgun with recoil and muzzle-flash animation|
|Enemy HP bar|Above enemy sprite|12 px wide bar, visible when an Imp is within 8 tiles|
|Separator line|At `viewH`|Divides 3-D view from HUD strip|
|`HP` + filled bar|HUD left|Player health — full bar = 44 px wide; shrinks as you take damage|
|`LN`|HUD centre-left|Current level number|
|`SN`|HUD centre|Score|
|Compass `(⊙)`|HUD far-right|Rotating needle always pointing toward the exit tile|
|`ST`|Top-right (in-view)|Appears only while Button B is held (strafe mode active)|
|Minimap overlay|Top-left (when toggled)|2 px/tile map of the current floor with a blinking exit marker and player arrow|
|Hurt flash|Screen-centre|Brief white flash in the centre pixels when the player takes a hit|

**Controls**

|Input|Action|
|-|-|
|Joystick Up/Down|Move forward / backward|
|Joystick L/R|Turn left / right|
|Button A|Shoot|
|Button B (hold)|Strafe mode (L/R slides sideways)|
|Joystick SW|Toggle minimap overlay|
|Button START|Pause|

**Difficulty**

|Level|Details|
|-|-|
|Easy|2 Imps, chase speed 0.7 u/s, deal 0.07 HP/hit, slower fire cooldown.|
|Medium|4 Imps, 1.0 u/s, 0.12 HP/hit.|
|Hard|6 Imps, 1.5 u/s, 0.20 HP/hit, 250 ms fire cooldown.|

**Tips**

* Use the compass at all times — wandering wastes health.
* **Strafe-peek** around corners: hold B, tap the joystick to check for enemies without fully exposing yourself.
* Kill all Imps before exiting — wave clear gives a health bonus.
* Open the minimap when disoriented; the blinking marker shows the exact exit position.
* Shoot at close range — the hit cone is ±16°, so distant shots can miss.

\---

### 🍄 Mario Platformer

*16 hand-crafted levels, power-ups, a shop between stages, bosses, and persistent saves.*

The most feature-rich game on RETROBOX. A side-scrolling platformer across 16 levels divided into 4 worlds. Collect coins, break bricks, stomp enemies, and reach the flag to unlock the inter-level shop. **Progress is saved to non-volatile flash storage**, so you can pick up where you left off.

**Mechanics**

* Physics: fixed-point gravity, variable-height jumps, and **4-frame coyote time** after walking off an edge.
* **Power-up ladder:** Small → Mushroom (powered) → Fire Flower (fire mode). Taking a hit steps you down one level before losing a life.
* **5 enemy types:** Goomba, Koopa (shell-kick mechanic), Piranha Plant, Bullet Bill, Boss (multi-hit).
* **Question Block items:** Mushroom, Fire Flower, Star (380 frames of invincibility), Shield (absorbs one hit).
* Fireball bounces along the floor and kills most enemies; max 2 on-screen.
* Every 10 coins = +1 life. Coin state saved per-level.
* **Combo stomps** in the air multiply score (×2 → ×8).
* Hidden blocks revealed if **Map Reveal** is purchased in the shop.
* **Inter-level Item Shop** — 8 purchasable items: 1UP, Mushroom reserve, Fire Flower, Star, Shield, 2UP, Map Reveal, Coin Conversion.
* **Reserve item** — store one item and use it instantly mid-game with Joystick SW.
* After level 16, the game loops in **Endless Mode** with an increased cycle count.
* All progress saved to NVS namespace `mario2`.

**HUD**



|Element|Position|Meaning|
|-|-|-|
|`MxNN`|Top-left|Lives remaining (e.g. `Mx03` = 3 lives)|
|`CNNN`|After lives|Total coins collected across all levels (3-digit, e.g. `C012`)|
|`WN-N`|After coins|Current world and level (e.g. `W1-2` = World 1, Level 2; endless cycles count as extra worlds)|
|Reserve icon|Top-right area|One-letter abbreviation of the stored reserve item: `M` = Mushroom, `F` = Fire Flower, `T` = Star, `H` = Shield, blank = none|
|`\*`|Top-right|Appears when Star (invincibility) is active|
|`S`|Top-right|Appears when Shield is active|
|Separator line|Row 7|Divides HUD from play area|

**Shop HUD** (between levels):

|Element|Description|
|-|-|
|Inverted header bar|"ITEM SHOP" title in black-on-white|
|`Coins: NNN`|Your current coin balance|
|Item detail box|Name, description, and cost of the highlighted item|
|Item carousel|4 items shown at once; selected item is highlighted (inverted); `<` / `>` arrows when more items exist off-screen|
|`A=buy  B=next lvl`|Button reminder at the bottom|

**Controls**

|Input|Action|
|-|-|
|Joystick L/R|Walk (hold B to run)|
|Joystick Down|Duck|
|Button A|Jump (hold for higher jump)|
|Button B (hold)|Run / sprint|
|Button B (press)|Shoot fireball (Fire mode only)|
|Joystick SW|Use reserve item|
|Button START|Pause|
|**Shop** — Joystick L/R|Browse items|
|**Shop** — Button A|Buy selected item|
|**Shop** — Button B|Skip shop|

**Difficulty**

|Level|Details|
|-|-|
|Easy|Fewer enemies (cap 6), slower walk/run (320/540), enemies at 0.70× speed.|
|Medium|Standard speeds (380/620), standard enemies.|
|Hard|Faster walk/run (425/670), enemies at 1.42× speed, Boss has 6 HP.|

**Tips**

* Hold B to run — you need running speed to clear the wider gaps in later worlds.
* Store a Star or Shield in reserve before boss rooms.
* Combo-stomp enemies mid-air for exponential score multipliers.
* Buy Map Reveal early — hidden blocks in level 14 often contain Stars.
* Coins accumulate across lives; bank them in the shop for 1UP or 2UP items.

\---

## Enabling \& Disabling Games

RETROBOX uses a single configuration file — `games\_config.h` — to control which games are compiled in. Comment out a line to remove a game, or un-comment it to add it back:

```c
#define ENABLE\_RACER
#define ENABLE\_TETRIS
#define ENABLE\_SNAKE
#define ENABLE\_ARKANOID
#define ENABLE\_VSHOOTER
#define ENABLE\_DUNGEON
#define ENABLE\_DOOM      // high RAM usage
#define ENABLE\_MARIO     // high RAM usage
```

> \*\*Note:\*\* DOOM and MARIO use the most RAM. If you hit memory limits after adding custom games, disable one of these two first.

\---

## Adding a Custom Game

Each game lives in its own header file (e.g. `game\_snake.h`) and exposes these functions inside a C++ namespace:

```cpp
void init();          // Called once when the game is selected from the menu.
void update();        // Called every frame (\~30 fps). Reads inputs and advances game state.
void draw();          // Called every frame after update(). Renders to the display buffer.
bool isOver();        // Returns true when the game-over condition is met.
int  getScore();      // Returns the current score for high-score tracking.
bool isNewRecord();   // Returns true if the score just beat the stored high score.
```

See [`ADDING\_A\_GAME.md`](ADDING_A_GAME.md) in the repository root for a complete walkthrough and a starter template.

\---

*Happy gaming! If you find RETROBOX useful, drop a ⭐ on GitHub.*

