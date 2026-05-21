# RETROBOX — Wiring Diagram \& Hardware Setup

> If your screen is blank or buttons don't respond, this is the first place to check.

\---

## Pin Reference Table

|Component|Component Pin|ESP32 Pin|Notes|
|-|-|-|-|
|SSD1306 OLED|VCC|3.3V|Do NOT use 5V — will fry the display|
|SSD1306 OLED|GND|GND||
|SSD1306 OLED|SDA|GPIO 21|I2C data|
|SSD1306 OLED|SCL|GPIO 22|I2C clock|
|KY-023 Joystick|VCC|3.3V||
|KY-023 Joystick|GND|GND||
|KY-023 Joystick|VRX|GPIO 34|Analog — horizontal axis|
|KY-023 Joystick|VRY|GPIO 35|Analog — vertical axis|
|KY-023 Joystick|SW|GPIO 25|Digital — joystick click|
|Button A|One leg|GPIO 26|Other leg to GND|
|Button B|One leg|GPIO 27|Other leg to GND|
|Button START|One leg|GPIO 14|Other leg to GND|
|Passive Buzzer|+|GPIO 26||
|Passive Buzzer|−|GND||

> \*\*All buttons use internal pull-up resistors\*\* — no external resistors needed.  
> \*\*Do NOT use an active buzzer\*\* — it won't produce tones, only a single fixed frequency.

\---

## ASCII Schematic

```
                        ESP32 DevKit V1
                   ┌─────────────────────┐
             3.3V ─┤ 3V3             GND ├─ GND
                   │                     │
      OLED SDA ────┤ GPIO21       GPIO14 ├──── START button → GND
      OLED SCL ────┤ GPIO22       GPIO27 ├──── B button → GND
                   │             GPIO26  ├──── A button → GND
   Joystick SW ────┤ GPIO25       GPIO25 │
                   │             GPIO26  ├──── Buzzer (+)
  Joystick VRX ────┤ GPIO34              │
  Joystick VRY ────┤ GPIO35              │
                   └─────────────────────┘

  OLED:          Joystick KY-023:       Buttons:
  VCC → 3.3V     VCC → 3.3V            One leg → GPIO pin
  GND → GND      GND → GND             Other leg → GND
  SDA → GPIO21   VRX → GPIO34          (uses INPUT\_PULLUP)
  SCL → GPIO22   VRY → GPIO35
                 SW  → GPIO25
```

\---

## Changing Pin Assignments

All pin definitions are at the top of `RETROBOX\_3\_7.ino`:

```cpp
#define PIN\_JOY\_X     34
#define PIN\_JOY\_Y     35
#define PIN\_JOY\_SW    25
#define PIN\_BTN\_A     26
#define PIN\_BTN\_B     27
#define PIN\_BTN\_START 14
#define PIN\_BUZZER    32
#define OLED\_SDA      21
#define OLED\_SCL      22
#define OLED\_ADDR   0x3C
```

Change any of these to match your build. The rest of the code adapts automatically.

\---

## Troubleshooting

### Screen is completely blank

* Check SSD1306 is on 3.3V, not 5V
* Try changing `OLED\_ADDR` from `0x3C` to `0x3D` (some clones use a different address)
* Verify SDA → GPIO21 and SCL → GPIO22

### Joystick doesn't respond or drifts

* Joystick centers around analog value \~2048 (12-bit ADC)
* If cursor drifts when untouched, increase `JOY\_DEAD` in the `.ino`:

```cpp
  #define JOY\_DEAD 600   // default is 400
  ```

### Buttons register double-presses

* This is a hardware debounce issue — add a 100nF capacitor between the button pin and GND
* Or increase the software debounce delay (default 16ms via loop timing)

### No sound / wrong sound

* Confirm you're using a **passive** buzzer (not active)
* Passive buzzers have two unmarked legs — polarity doesn't matter
* Active buzzers have a + marking and only play one frequency

### Serial output for debugging

* Open Serial Monitor at **115200 baud**
* The firmware prints `\[ERROR] SSD1306 not found` if the display fails to init

\---

## Power Options

|Option|Notes|
|-|-|
|USB (micro/USB-C)|Easiest — powers directly via PC or phone charger|
|3.7V LiPo battery|Add a TP4056 charge module with protection circuit|
|3× AA batteries|Use a voltage regulator to bring \~4.5V down to 3.3V|

> The ESP32 draws \~80–100mA during gameplay. A 1000mAh LiPo gives \~8–10 hours.

\---

*For build photos and community builds, check the GitHub Discussions tab.*

