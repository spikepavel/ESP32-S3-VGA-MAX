# spikepavel's ESP32-S3 VGA library
The ultimate high-performance VGA graphics library for ESP32-S3 N16R8. The resolution is 800x600 pixels with a screen buffer (up to 8). The maximum possible resolution is 1024x600 pixels(theoretically 4000x1000). The number of colors is 65536(R5G6B5 format).\
\
Check out https://youtube.com/@BASICOS-COMPUTER and https://basicos.ru for project updates.\
\
If you found it useful please consider supporting my work on https://donationalerts.com/r/basicos
<br />
## Installation
This library only supports the ESP32-S3.\
\
To use in the Arduino IDE, you must have ESP32-S3 support pre-installed(3.1.0-RC1).\
\
The ESP32-S3 library can be found in the Library Manager(Sketch -> Include Library -> Manage Libaries).\
<br />
```
#include <VGA.h>
```
## Pin configuration
An VGA cable can be used to connect to the ESP32-S3.\
\
The connector pins can be found here: https://en.wikipedia.org/wiki/VGA_connector
<br />
<br />
You can reassign any pins yourself, but keep in mind that some ESP32-S3 pins may already be occupied by something else.\
<br />

## Usage
```
void setup() {
  vga.init();
  vga.setFont(FONT_9x16);
  vga.setTextColor(65535,0);
}
```
## Gratitude
NKros\