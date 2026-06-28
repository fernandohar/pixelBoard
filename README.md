# pixelBoard

ESP32-S3 Super Mini firmware for a 16x16 NeoPixel matrix with SD-card
animations, a DS3231 RTC clock, web control, BLE phone control, OTA updates,
games, mood lighting, and a hardware setup menu.

## Arduino / ESP32-S3 compatibility

This branch targets an ESP32-S3 Super Mini style board using the ESP32 Arduino
core. Recommended Arduino IDE board settings:

```text
Board: ESP32S3 Dev Module
Flash Size: 4MB
PSRAM: QSPI PSRAM / Enabled
USB CDC On Boot: Enabled
Partition Scheme: No FS 4MB (2MB APP x2)
```

The larger app partition is important because WiFi, WebServer, WebSockets, BLE,
OTA, games, SD card, and RTC support do not fit in the default 1.2 MB ESP32-S3
app partition.

The firmware uses the SdFat API:

```cpp
SdFat32 sd;
File32 file;
sd.begin(SdSpiConfig(...));
```

Install the Arduino `SdFat` library if it is not already available.

## ESP32-S3 Super Mini pin map

Default pin assignments for the Super Mini pinout used by this branch:

| Function | ESP32-S3 GPIO | Notes |
| --- | --- | --- |
| External NeoPixel matrix DIN | GPIO21 | Avoids GPIO48, which is used by many Super Mini onboard RGB LEDs. |
| RTC SDA | GPIO8 | I2C SDA |
| RTC SCL | GPIO9 | I2C SCL |
| SD SCK | GPIO12 | SPI clock |
| SD MISO | GPIO13 | SPI MISO |
| SD MOSI | GPIO11 | SPI MOSI |
| SD CS | GPIO10 | SPI chip select |
| Reset/Menu button | GPIO4 | Active-low, uses `INPUT_PULLUP` |
| Select button | GPIO5 | Active-low, uses `INPUT_PULLUP` |
| Up button | GPIO6 | Active-low, uses `INPUT_PULLUP` |
| Down button | GPIO7 | Active-low, uses `INPUT_PULLUP` |

The Super Mini board's onboard RGB LED is usually on GPIO48. The external matrix
is intentionally mapped to GPIO21 so it does not fight with the onboard LED.

## BLE phone control

This branch starts a BLE GATT service using the same generated device name as
the WiFi setup portal:

```text
pixelboard_XXXXXX
```

BLE service and characteristics:

| Purpose | UUID |
| --- | --- |
| Service | `7d8f0001-6f8a-4a5a-9d6b-40f520dc0001` |
| Command write characteristic | `7d8f0002-6f8a-4a5a-9d6b-40f520dc0001` |
| Status notify/read characteristic | `7d8f0003-6f8a-4a5a-9d6b-40f520dc0001` |

Initial phone-app command protocol:

| Command | Description |
| --- | --- |
| `WIFI:ssid|password` | Connect the board to WiFi from a phone over BLE. |
| `TEXT:hello` | Scroll text on the matrix. |
| `CLEAR` | Clear the display. |
| `PIX:x,y,RRGGBB` | Draw one pixel, useful for live phone drawing. |
| `ROW:y:<96 hex chars>` | Send one 16-pixel row, 6 hex chars per pixel. |
| `FRAME:<1536 hex chars>` | Send a full 16x16 RGB frame. |
| `BRIGHT:n` | Set brightness level. |

For a future iPhone/Android app, the recommended approach is for the phone to
decode/resize BMPs or drawings into a 16x16 RGB frame, then send either `ROW`
chunks or a `FRAME` command over BLE. This keeps the firmware small and avoids
doing heavy image decoding over BLE on the microcontroller.

## Menu overview

After flashing the firmware and copying the `SD Card/` contents to the SD card,
the board boots into the last saved display mode. If there is no saved mode yet,
it starts in pixel-art traverse mode when the SD card is available, or clock mode
when it is not.

Editable copies of the browser controller pages live in `resources/web/`. The
runtime copies remain in `SD Card/`, because the ESP32-S3 serves
`pixBoardController.htm` and `gameController.htm` directly from the SD card.

The hardware **Reset/Menu** button is a software menu button:

- Short press outside the menu: open the setup menu.
- Short press inside the menu: go back one level, or exit from the root menu.
- Hold for 3 seconds: save state and restart the ESP32-S3.

The hardware **Up** and **Down** buttons move through menu items. Outside the
menu, they cycle the board display modes, except in Mood Light mode where they
change the active solid color or RGB randomizer preset. Long labels scroll
across the 16x16 matrix.

Menu tree:

```text
MENU
|-- WIFI
|   |-- SHOW IP
|   |     Shows the current device IP, or NO WIFI.
|   |-- SHOW SSID
|   |     Shows the current WiFi SSID, or NO SSID.
|   |-- CONNECT SSID
|   |     Clears saved WiFi credentials and restarts.
|   |     The matrix scrolls:
|   |     WIFI CREDENTIAL CLEARED CONNECT TO pixelboard_XXXXXX TO SETUP WIFI
|   |     On next boot, WiFiManager opens the setup portal.
|   |     Connect your phone/computer to pixelboard_XXXXXX, then choose WiFi.
|   `-- BACK
|
|-- TIME
|   |-- WIFI CLOCK - ON/OFF
|   |   `-- ON / OFF
|   |       Select this item, use Up/Down to choose ON or OFF,
|   |       then Select again to save.
|   |       ON: NTP updates the DS3231 RTC when WiFi is connected.
|   |       OFF: time depends on the DS3231 RTC module only.
|   |
|   |-- 24 HOUR ON / 12 HOUR ON
|   |     Toggles the clock display format.
|   |
|   |-- TIMEZONE
|   |     Shows a short city label and UTC offset, such as YVR -8.
|   |     Use Up/Down to change the selected timezone.
|   |
|   `-- BACK
|
`-- EXIT
```

Off-the-shelf defaults in this branch:

- NTP server: `pool.ntp.org`
- WiFi clock: `ON`
- Timezone country config: Canada (`CA`)
- Vancouver label: `YVR -8`
- Button GPIOs: disabled until `HW_BUTTON_*_PIN` values are configured

The menu implementation is split into [`setupMenu.h`](setupMenu.h) and included
from `pixelBoard.ino`. Arduino supports multiple `.ino` sketch tabs, but an
explicit header include keeps this project from depending on Arduino's `.ino`
concatenation order.

## Web game controller

Some display modes are games and need directional controls that are easier to
use from the browser controller than from the four hardware setup buttons. This
includes Snake, Tetris, and Arkanoid.

The ESP32-S3 serves the controller pages from the SD card:

```text
http://<device-ip>/pixBoardController.htm
http://<device-ip>/gameController.htm
```

The default page is `pixBoardController.htm`, so you can usually open:

```text
http://<device-ip>/
```

To connect:

1. Put `pixBoardController.htm` and `gameController.htm` on the SD card root.
2. Boot the board and connect it to WiFi.
3. Use the hardware menu:

   ```text
   MENU -> WIFI -> SHOW IP
   ```

4. Open that IP address in a phone/computer browser.
5. Press **Snake Game** or open `/gameController.htm`.

`gameController.htm` opens a WebSocket connection to:

```text
ws://<device-ip>:81/
```

It sends controller state messages like:

```text
buttonStatus:0010000
```

The button order is:

```text
UP, DOWN, LEFT, RIGHT, SELECT/START, A, B
```

Keyboard arrow keys also control the D-pad when using a computer browser.

## NTP server configuration

The firmware defaults to the public NTP pool:

```text
pool.ntp.org
```

You can override it from the SD card by creating this file:

```text
/ntpserver.txt
```

The first non-empty, non-comment line is used as the NTP hostname or IP address:

```text
# Optional SD-card NTP override
time.google.com
```

If the file is missing, empty, or cannot be opened, the firmware falls back to
`pool.ntp.org`.

The WiFi clock sync interval defaults to 300 seconds. You can override it from
the SD card by creating:

```text
/ntpinterval.txt
```

Example:

```text
# NTP sync interval in seconds
300
```

The accepted range is 30 to 86400 seconds. If WiFi is disconnected or NTP does
not respond, the device keeps showing time from the DS3231 RTC and doubles the
delay before the next NTP attempt. A successful NTP sync resets the delay back
to the configured interval.

## Clock menu

The Time menu shows a single WiFi clock setting instead of separate manual/NTP
items:

```text
WIFI CLOCK - ON
WIFI CLOCK - OFF
```

Press `Select` on that item to open an ON/OFF picker. Use `Up` or `Down` to
change the selection, then press `Select` again to save and return to the Time
menu.

When WiFi clock is `ON`, NTP responses update the DS3231 RTC using the selected
timezone offset. When it is `OFF`, the RTC keeps running by itself and the
firmware does not request periodic NTP time updates.

## Timezone localization

The clock menu uses short city labels because they fit better on the 16x16 LED
matrix than long timezone names. The firmware starts with a global whole-hour
offset list, then applies an optional country-specific file from the SD card.

The shipped SD card config selects Canada:

```text
/timezone_country.conf
CA
```

Startup order:

1. Load the built-in global timezone list.
2. Apply `/timezone_global.conf` from SD card if present.
3. Read the country code from `/timezone_country.conf`.
4. Apply `/<COUNTRY>_timezone.conf`, for example `/CA_timezone.conf`.

Country files replace labels for matching offsets. For example, the global list
may contain:

```text
LA,-8
```

Canada can replace the `-8` label with Vancouver:

```text
YVR,-8
```

Both of these formats are accepted:

```text
YVR,-8
{"YVR", "-8"}
{"-8", "YVR"}
```

Included SD-card examples:

```text
/timezone_global.conf
/timezone_country.conf
/CA_timezone.conf
/US_timezone.conf
```

Current limitation: this firmware stores whole-hour UTC offsets only. It does
not automatically apply daylight saving time rules.

## Hardware wiring

This branch is wired for an ESP32-S3 Super Mini driving a 5V NeoPixel matrix.
The external matrix data pin is configured as GPIO21:

```cpp
#define NEOPIXEL_PIN 21
Adafruit_NeoPixel strip = Adafruit_NeoPixel(BOARDSIZE, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
```

The older wiring diagram files still show the ESP8266 version and should be
regenerated before using them for ESP32-S3 assembly. Use the pin table below as
the source of truth for this branch.

### Power wiring

| Connection | Wire to | Notes |
| --- | --- | --- |
| 5V power supply `+` | NeoPixel matrix `5V` | Do not power 256 LEDs from USB. |
| 5V power supply `-` | NeoPixel matrix `GND` | Must also connect to ESP32-S3 `GND`. |
| 5V power supply `-` | ESP32-S3 `GND` | Required common ground for data signal. |
| ESP32-S3 `5V` / `VBUS` | 5V supply `+` | Optional if not powering the ESP32-S3 by USB. |

For a 16x16 matrix, worst-case full white current is about:

```text
256 LEDs x 60 mA = 15.36 A
```

Use a 5V supply sized for your brightness. If you limit brightness in firmware,
the real current can be much lower, but the supply and wiring should still be
chosen safely.

Add these parts near the NeoPixel matrix:

| Part | Where | Why |
| --- | --- | --- |
| 330 to 470 ohm resistor | In series between ESP32-S3 GPIO21 and NeoPixel `DIN` | Protects the first pixel from data-line spikes. |
| 1000 uF electrolytic capacitor, 6.3V or higher | Across NeoPixel `5V` and `GND` | Absorbs LED power-up surges. Observe polarity. |
| 74AHCT125 / 74HCT245 level shifter | Between ESP32-S3 data and NeoPixel `DIN` | Recommended for reliable 3.3V-to-5V data conversion. |

### ESP32-S3 Super Mini pin map

| Module | ESP32-S3 GPIO | Notes |
| --- | --- | --- |
| NeoPixel `DIN` | GPIO21 | Current external matrix data pin. Put 330-470 ohm resistor in series. |
| NeoPixel `5V` | External 5V `+` | Use external LED power. |
| NeoPixel `GND` | External 5V `-` and ESP32-S3 `GND` | All grounds must be common. |
| DS3231 RTC `SDA` | GPIO8 | I2C SDA. |
| DS3231 RTC `SCL` | GPIO9 | I2C SCL. |
| DS3231 RTC `VCC` | `3V3` | Prefer 3.3V so I2C pullups do not pull ESP32-S3 pins to 5V. |
| DS3231 RTC `GND` | `GND` | Common ground. |
| SD card `SCK` | GPIO12 | SPI clock. |
| SD card `MISO` | GPIO13 | SPI MISO. |
| SD card `MOSI` | GPIO11 | SPI MOSI. |
| SD card `CS` | GPIO10 | `#define SD_CS 10` in the sketch. |

> Note: Many DS3231 modules include pullup resistors on SDA/SCL. If your module
> is powered from 5V, those pullups can put 5V on ESP32-S3 GPIO pins. Power the
> RTC from 3.3V or remove/change the pullups.

### Hardware button wiring

The firmware supports four configurable hardware buttons:

| Button | Default ESP32-S3 GPIO | Firmware purpose |
| --- | --- | --- |
| Reset/Menu | GPIO4 | Short press opens/back-outs of the menu. Hold 3 seconds to software-reset the ESP32-S3. |
| Select | GPIO5 | Selects the current menu item. |
| Up | GPIO6 | Menu up / increase value. Outside the menu, cycles to the previous board mode. |
| Down | GPIO7 | Menu down / decrease value. Outside the menu, cycles to the next board mode. |

Buttons are read as **active-low**:

```text
3.3V ---- 10k resistor ---- GPIO input ---- button ---- GND
```

The code also enables `INPUT_PULLUP`, so the external 10k pullup is optional for
short wires, but recommended for reliable hardware. Do **not** connect any
ESP32-S3 GPIO button input to 5V.

Optional button parts:

| Part | Where | Why |
| --- | --- | --- |
| 10k resistor | GPIO input to 3.3V | External pullup, keeps the input high when not pressed. |
| 100 nF capacitor | GPIO input to GND | Optional hardware debounce/noise filtering. |
| 220 ohm to 1k resistor | In series with GPIO input | Optional protection against wiring mistakes. |

Configure the button GPIOs in `pixelBoard.ino` before flashing if you want a
different pinout.

### Button and menu behavior

| State | Reset/Menu | Select | Up | Down |
| --- | --- | --- | --- | --- |
| Normal display/game mode | Short press opens menu; 3 second hold restarts | No action | Previous board mode | Next board mode |
| Mood Light mode | Short press opens menu; 3 second hold restarts | Next board mode | Previous mood preset | Next mood preset |
| Setup menu | Back/exit; 3 second hold restarts | Select item | Previous item / increase value | Next item / decrease value |

Current board mode order:

```text
Pixel Art Traverse
Pixel Art Single
Solid Fill
Clock
Snake
Tetris
Game of Life
Arkanoid
Mood Light / RGB Randomizer
```

Mood Light presets include solid colors, warm white, animated rainbow gradients,
blocky rainbow patterns, random RGB pixels, and sparkle-style randomized color.
