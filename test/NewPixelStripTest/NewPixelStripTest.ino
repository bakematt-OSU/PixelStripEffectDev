
#include <Arduino.h>
#include "PixelStrip.h"

// Include each effect
#include "WipeEffect.h"
#include "RainbowEffect.h"
#include "SolidRainbowEffect.h"
#include "TheaterChaseEffect.h"
#include "JumpEffect.h"
#include "FireEffect.h"
#include "ColoredFireEffect.h"
#include "BreatheEffect.h"
#include "FlashEffect.h"
#include "FallingStarEffect.h"
#include "IntensityMeterEffect.h"
#include "BounceAccelEffect.h"
#include "TiltRainbowEffect.h"
#include "LightningStrikeEffect.h"
#include "SoundLightningEffect.h"

#define LED_PIN    4
#define LED_COUNT  60
#define BRIGHTNESS 100
#define SEGMENTS   1

PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);
PixelStrip::Segment* seg;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  strip.begin();
  seg = strip.getSegments()[1]; // segment 1 (not "all")
  seg->begin();

  // Choose a test effect here
  seg->wipe(strip.Color(255, 0, 0), 50, true);  // red wipe loop
}

void loop() {
  seg->update();
  delay(10);
}
