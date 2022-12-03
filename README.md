# GPS tracker
---
Yet another GPS tracker based on arduino/nodemcu platform. The main reason for publishing this project is to provide a complete, step by step, no doubts/investigation process for building and programing a complete and fully functional GPS tracker for ~~everyone~~ ...well mostly sailors.

### How it works
After connecting to power supply tracker will immediately display received data (placeholders for missing data). You should be able to see:
* UTC - time and date (in UTC)
* COG - course over ground (i degrees)
* SOG - speed over ground (in KT)
* lat - latitude dd° mm.mm' N/S
* lng - longitude ddd° mm.mm' E/W

depending on what is currently available there will be real data or placeholders. Display can be turned on and off with use of the button - this is very important to keep the display off when not used to increase batery live.

When movement is detected and happens continuously for at least 10s the additional indicator will be displayed on the right hand side of lat/lng (3 cocentric circles). This indicator states that the data will actually be logged into SD card.

Loggin interval is 5s - data logged is latitude, longitude, time, date, speed and heading. Time interval assures that log file (csv) does not exceed 4MB (FAT32 limitation) even if logging happens all the time for 24h. File naming convention will switch to new file for each day - file format name is dd-mm-yyy.csv.

SD card is updated each minute (when there are log entries stored in memory). Display update period is 2s.

### Display
![no picture][display_picture]

[display_picture]: ./pictures/display_picture.png "Display overview"

### Environment
This project has been created using *PlatformIO* and *VS Code*. Dependencies (libraries) needed for this project to run are defined in *platformio.ini* file. After the project is built the libraries will be restored in *.pio/libdeps/nodemcuv2/* folder. Look [here](https://www.youtube.com/watch?v=dany7ae_0ks) for IDE setup. Find below the dependencies listed:
| dependencies                                                          |
|-----------------------------------------------------------------------|
| thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays@^4.3.0  |
| plerup/EspSoftwareSerial@^6.16.1                                      |  
| slashdevin/NeoGPS@^4.2.9                                              |
| ivanseidel/LinkedList@0.0.0-alpha+sha.dac3874d28                      |

### Components
* NodemCU v3 board (ESP8266 controller),
* NEO 6-M GPS module (with antenna),
* HW-125 SD card module (**remove / unsolder voltage regulator to make it usable with 3.3V - google that one!**),
* SH1106 based SPI OLED 128x64 display - it can be SSD1306 but you will need some code changes (**most probably you will get a random one when shopping for OLED display 128x64**),
* switch button,
* 10k ohm resistor, 
* female to female breadboard wires (pack),
* usb-a to micro cable with data lines,
* powerbank.

#### optionally:
* breadboard,
* male to male, female to male wires.

### Circuit connections

| NEO 6-M | NodemCU v3 |
|---------|-----------:|
| VCC     | 3V         |  
| GND     | G          |
| RX      | D4         |
| TX      | D3         |

| SH1106  | NodemCU v3 |
|---------|-----------:|
| VDD     | 3V         |  
| GND     | G          |
| SCK     | D1         |
| SDA     | D2         |

| HW-125  | NodemCU v3 |
|---------|-----------:|
| VCC     | 3V         |  
| GND     | G          |
| CS      | D8         |
| SCK     | D5         |
| MOSI    | D7         |
| MISO    | D6         |

| switch / button     | NodemCU v3 |
|---------------------|-----------:|
| gnd before resistor | D0         |
| gnd after resistor  | G          |
| other leg           | 3V         |

### HW-125 modification
You need to remove voltage controller (unsolder) and then short (solder together) marked pins. This is only required for NodemCU as it is 3.3V. For Arduino it can stay as is.

![no picture][sd_picture]

[sd_picture]: ./pictures/hw_125_picture.png "SD module picture"

### Changes in libraries
Some GPS settings need to be changed directly in files installed from dependencies. Namely (after build) in folder *.pio/libdeps/nodemcuv2/NeoGPS/src*:

```c++
//in file NMEAGPS_cfg.h uncomment:
#define NMEAGPS_PARSE_GGA
#define NMEAGPS_PARSE_GLL
#define NMEAGPS_PARSE_GSA
#define NMEAGPS_PARSE_GSV
//#define NMEAGPS_PARSE_GST
#define NMEAGPS_PARSE_RMC
#define NMEAGPS_PARSE_VTG
#define NMEAGPS_PARSE_ZDA

//and set

#define LAST_SENTENCE_IN_INTERVAL NMEAGPS::NMEA_GLL
//instead of
//#define LAST_SENTENCE_IN_INTERVAL NMEAGPS::NMEA_RMC
```

### Settings in code
For "production" mode - if you want logger to work only when there is movement.

```c++
//source file: src/main.cpp
bool VALID_FIX_ONLY_WITH_HEADING = true;
bool ENABLE_SERIAL_DEBUGGING     = false;
```

If you happen to have SSD1306 display driver you will have to change the import accordingly

```c++
//source file: src/main.cpp
#include <SH1106Wire.h> // look up the library and change accordingly
```
#### Monospace font
It is much easier to make the display aligned when you are using monospace font. This project uses *Monospaced_plain_10*. This one is not available by default so it's been included in */fonts/Monospaced_plain_10.txt*, you need to add this one to: *.pio\libdeps\nodemcuv2\ESP8266 and ESP32 OLED driver for SSD1306 displays\src\OLEDDisplayFonts.h*

## Debugging
By leaving *ENABLE_SERIAL_DEBUGGING* set to true it is possible to observe debug data via serial monitor (baud rate 9600).

```c++
//source file: src/main.cpp
bool ENABLE_SERIAL_DEBUGGING     = true;
```