#include <Arduino.h>
#include "PixelStrip.h"
#include "effects/Effects.h"

#define LED_PIN    4
#define LED_COUNT  300
#define BRIGHTNESS 25
#define SEGMENTS   1

PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);
PixelStrip::Segment* seg;

enum EffectType {
  RAINBOW,
  SOLID,
  EFFECT_COUNT
};

EffectType currentEffect = RAINBOW;

void applyEffect(EffectType effect) {
  // Stop all effects
  RainbowChase::stop(seg);
  SolidColor::stop(seg);

  seg->clear();
  currentEffect = effect;

  switch (effect) {
    case RAINBOW:
      RainbowChase::start(seg, 30, 175);
      break;
    case SOLID:
      SolidColor::start(seg, strip.Color(0, 255, 0), 200);
      break;
    default:
      break;
  }
}

void handleSerial() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "next") {
      currentEffect = static_cast<EffectType>((currentEffect + 1) % EFFECT_COUNT);
      applyEffect(currentEffect);
    } else if (cmd == "rainbow") {
      applyEffect(RAINBOW);
    } else if (cmd == "solid") {
      applyEffect(SOLID);
    } else {
      Serial.println("Unknown command. Try 'next', 'rainbow', or 'solid'.");
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  strip.begin();
  seg = strip.getSegments()[1];
  seg->begin();
  applyEffect(currentEffect);
}

void loop() {
  handleSerial();
  seg->update();
}
