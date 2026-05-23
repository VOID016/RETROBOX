# RETROBOX — Wiring Diagram \& Hardware Setup

If your screen is blank or buttons don't respond, this is the first place to check.

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
|Button A|One leg|GPIO 14|Other leg to GND|
|Button B|One leg|GPIO 27|Other leg to GND|
|Button START|One leg|GPIO 13|Other leg to GND|
|Passive Buzzer|+|GPIO 26||
|Passive Buzzer|−|GND||

> All buttons use internal pull-up resistors — no external resistors needed for debouncing.
> Do NOT use an active buzzer — it won't produce tones, only a single fixed frequency.

\---

## Passive Components

These components are not in the pin table above because they connect between existing nodes rather than to dedicated GPIO pins. All are strongly recommended — omitting them can cause display glitches, joystick drift, and noisy button presses.

|Component|Value|Connect Between|Purpose|
|-|-|-|-|
|Electrolytic capacitor|100 µF|ESP32 3.3V and GND (as close to the ESP32 power pins as possible)|Bulk decoupling — absorbs current spikes when the ESP32 transmits over BLE/Wi-Fi|
|Ceramic capacitor|100 nF (104)|OLED VCC and GND|High-frequency decoupling — stabilises the display power rail and prevents screen glitches|
|Resistor|4.7 kΩ|GPIO 22 (SCL) to 3.3V|I2C pull-up — required for reliable I2C communication with the OLED|
|Resistor|4.7 kΩ|GPIO 21 (SDA) to 3.3V|I2C pull-up — required for reliable I2C communication with the OLED|
|Resistor|10 kΩ|GPIO 25 (Joystick SW) to 3.3V|External pull-up on the joystick click line — reduces false triggers and ADC noise|

**Capacitor polarity:** The 100 µF electrolytic is polarised — connect the positive leg (longer leg / stripe side) to 3.3V and the negative leg to GND.  
**Ceramic capacitors** (100 nF) are non-polarised — either leg can go to either node.

\---

## Connections at a Glance

### ESP32 GPIO assignments

|GPIO|Connected to|
|-|-|
|GPIO 13|START button → GND|
|GPIO 14|A button → GND|
|GPIO 21|OLED SDA + 4.7 kΩ pull-up to 3.3V|
|GPIO 22|OLED SCL + 4.7 kΩ pull-up to 3.3V|
|GPIO 25|Joystick SW + 10 kΩ pull-up to 3.3V|
|GPIO 26|Buzzer (+)|
|GPIO 27|B button → GND|
|GPIO 34|Joystick VRX (analog)|
|GPIO 35|Joystick VRY (analog)|
|3.3V|OLED VCC, Joystick VCC, all pull-up resistors|
|GND|OLED GND, Joystick GND, all button legs, Buzzer (−)|

### OLED (SSD1306)

```
VCC  →  3.3V
GND  →  GND
SDA  →  GPIO 21
SCL  →  GPIO 22
```

Also place a 100 nF ceramic capacitor across VCC and GND on the OLED itself.

### Joystick (KY-023)

```
VCC  →  3.3V
GND  →  GND
VRX  →  GPIO 34
VRY  →  GPIO 35
SW   →  GPIO 25  (+ 10 kΩ resistor from SW to 3.3V)
```

### Buttons (A, B, START)

```
One leg   →  GPIO pin (14 / 27 / 13)
Other leg →  GND
```

Uses `INPUT\_PULLUP` — no external resistor needed on the signal line.

### Buzzer (passive)

```
(+)  →  GPIO 26
(−)  →  GND
```

Polarity doesn't matter on a passive buzzer — either leg can go to either pin.

### Decoupling capacitors

```
100 µF electrolytic  →  ESP32 3.3V and GND (near the power pins)
100 nF ceramic       →  OLED VCC and GND
```

\---

## Changing Pin Assignments

All pin definitions are at the top of `RETROBOX\_3\_7.ino`:

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

Change any of these to match your build. The rest of the code adapts automatically.

\---

## Troubleshooting

### Screen is completely blank

* Check SSD1306 is on 3.3V, not 5V
* Try changing `OLED\_ADDR` from `0x3C` to `0x3D` (some clones use a different address)
* Verify SDA → GPIO 21 and SCL → GPIO 22
* Confirm the two 4.7 kΩ I2C pull-up resistors are installed (GPIO 21 → 3.3V and GPIO 22 → 3.3V)

### Screen flickers or shows garbage

* Check the 100 nF ceramic capacitor across the OLED VCC and GND pins
* Re-seat the SDA/SCL wires at GPIO 21/22

### Joystick doesn't respond or drifts

* Joystick centers around analog value \~2048 (12-bit ADC)
* If cursor drifts when untouched, increase `JOY\_DEAD` in the `.ino`:

```cpp
#define JOY\_DEAD 600   // default is 400
```

* Confirm the 10 kΩ pull-up resistor is on GPIO 25 (Joystick SW)

### Buttons register double-presses

* This is a hardware debounce issue — add a 100 nF capacitor between the button pin and GND
* Or increase the software debounce delay (default 16 ms via loop timing)

### No sound / wrong sound

* Confirm you're using a passive buzzer (not active)
* Passive buzzers have two unmarked legs — polarity doesn't matter
* Active buzzers have a + marking and only play one frequency

### ESP32 resets randomly during gameplay

* Check the 100 µF electrolytic decoupling capacitor across the ESP32 3.3V and GND pins
* Try a different USB cable (data cable, not charge-only)

### Serial output for debugging

Open Serial Monitor at **115200 baud**. The firmware prints `\[ERROR] SSD1306 not found` if the display fails to init.

\---

## Power Options

|Option|Notes|
|-|-|
|USB (micro/USB-C)|Easiest — powers directly via PC or phone charger|
|3.7V LiPo battery|Add a TP4056 charge module with protection circuit|
|3× AA batteries|Use a voltage regulator to bring \~4.5V down to 3.3V|

The ESP32 draws \~80–100 mA during gameplay. A 1000 mAh LiPo gives \~8–10 hours.

\---

For build photos and community builds, check the GitHub Discussions tab.

