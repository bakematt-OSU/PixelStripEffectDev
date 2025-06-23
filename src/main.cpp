#include <Arduino.h>
#include "PixelStrip.h"
#include "effects/Effects.h"

// LED config
#define LED_PIN    4
#define LED_COUNT  300
#define BRIGHTNESS 100
#define SEGMENTS   2

PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);
PixelStrip::Segment* seg;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial monitor
  strip.begin();

  // Get first segment and start effect
  seg = strip.getSegments()[1];  // index 1 is first user segment (after "all")
  strip.getSegments()[1]->begin();
  RainbowChase::start(seg, 10, 100);  // 10ms delay, 100 brightness
  SolidColor::start(seg, strip.Color(0, 255, 0), 150);
}

void loop() {
  seg->update();  // Run the effect
}
