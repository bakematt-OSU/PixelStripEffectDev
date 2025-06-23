# PixelStrip

A modular C++ library for running advanced LED effects on addressable RGB strips (such as WS2812/NeoPixel) using Arduino-compatible boards.

## Features

* **Multi-zone control:** Divide your strip into independent "segments" for parallel effects (rainbow, fire, sound-reactive, etc).
* **Many built-in effects:** Includes wipes, rainbow, theater chase, fire, breathing, bouncing, lightning, sound-reactive lightning, and more.
* **Flexible API:** Start and stop effects per-segment with one call.
* **Customizable:** Add your own effects or segment logic easily.
* **Sound and sensor integration:** Works with microphone or IMU/accelerometer data.

## Supported Hardware

* Any Arduino-compatible microcontroller
* Any Adafruit NeoPixel (WS2812, SK6812, etc.) compatible LED strip or ring

## Wiring

* **Data Pin:** Connect to a digital pin on your microcontroller (e.g., D6)
* **Power:** Provide sufficient 5V power for your LED count
* **GND:** Connect GND of strip and microcontroller

## Installation

1. **Clone or download** this repository.
2. Copy `PixelStrip.h` and `PixelStrip.cpp` to your Arduino project folder.
3. Make sure to install [Adafruit NeoPixel Library](https://github.com/adafruit/Adafruit_NeoPixel) via Arduino Library Manager.

## Quick Start Example

```cpp
#include "PixelStrip.h"

PixelStrip leds(6, 60, 80, 3); // Pin 6, 60 LEDs, brightness 80, 3 slices

void setup() {
    leds.begin();
    // Start a rainbow effect on first segment ("all")
    leds.getSegments()[0]->rainbow(30, true);
}

void loop() {
    // Call update() on each segment you want animated
    for (auto seg : leds.getSegments()) {
        seg->update();
    }
    delay(10);
}
```

## API Overview

### Creating a Strip

```cpp
PixelStrip leds(dataPin, numLEDs, brightness, numSections);
```

* **dataPin**: Arduino pin connected to your strip's data input.
* **numLEDs**: Total number of LEDs.
* **brightness**: 0-255 (optional, default 50).
* **numSections**: Number of slices in addition to "all" (optional).

### Controlling Segments

Get all segments (including "all" and slices):

```cpp
auto segments = leds.getSegments();
```

Start effects (see full API in [PixelStrip.h](PixelStrip.h)):

```cpp
segments[0]->wipe(leds.Color(255,0,0), 40);  // Red wipe
segments[1]->fire(50, 5, true);              // Fire on section 1
segments[2]->solidRainbow();                 // Static rainbow
```

Update in your loop:

```cpp
for (auto seg : leds.getSegments()) seg->update();
```

## Effect List

* **wipe**: Chasing single color
* **rainbow**: Animated rainbow
* **solidRainbow**: Static rainbow fill
* **theaterChase**: Spaced pixel chase
* **jump**: Block color jump
* **fire / coloredFire**: Random flicker
* **breathe**: Pulse/fade
* **flash**: On/off flashes
* **fallingStar**: Comet with tail
* **intensityMeter**: Level meter (mic/analog input)
* **bounceWithAccel**: Bounce, with optional accelerometer/tilt input
* **tiltRainbow**: Rainbow that shifts with IMU data
* **lightningStrike**: Flash and flicker (with optional rainbow)
* **soundLightning**: Lightning that triggers from sound peaks

## Custom Segments

You can define custom sections with names:

```cpp
leds.addSection(0, 9, "Left");
leds.addSection(10, 19, "Right");
```

## Sound and Sensors

Sound-reactive effects require a microphone class with a `readPeak()` method.
IMU/acceleration effects require acceleration data.
(See example integrations in the [Sensors.h/Sensors.cpp](Sensors.h) files.)

## Advanced

* Add new effects by editing `PixelStrip::Segment` methods.
* Use `setBrightness()`, `allOn()`, `allOff()` per segment for manual control.

## License

MIT License.
See [LICENSE](LICENSE) file.

## Contributing

Pull requests, bug reports, and suggestions are welcome!
Open an issue or PR on GitHub.

## Credits

* [Adafruit NeoPixel Library](https://github.com/adafruit/Adafruit_NeoPixel)

---

### Questions or Help?

Open an issue or [contact the maintainer](mailto:your.email@example.com).
