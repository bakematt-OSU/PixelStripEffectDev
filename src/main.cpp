#include <Arduino.h>
#include "PixelStrip.h"
#include "effects/Effects.h"
#include "Sensors.h"

#define LED_PIN 4
#define LED_COUNT 300
#define BRIGHTNESS 25
#define SEGMENTS 1
#define TRIGGER_PIN 2 // 🔸 Example digital pin for trigger input

PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);
PixelStrip::Segment *seg;
// Microphone mic(400); // This line is commented out, assuming you use the singleton Microphone::instance()

enum EffectType
{
  RAINBOW,
  SOLID,
  FLASH_TRIGGER,
  EFFECT_COUNT
};

EffectType currentEffect = RAINBOW;

bool flashTriggerState = false;
uint8_t flashTriggerBrightness = 0;
void setFlashTrigger(bool value, uint8_t brightness = 0)
{
  flashTriggerState = value;
  flashTriggerBrightness = brightness;
}

void applyEffect(EffectType effect, PixelStrip::Segment* targetSegment)
{
  if (!targetSegment) return; // Safety check
  currentEffect = effect;

  switch (effect)
  {
  case RAINBOW:
    RainbowChase::start(targetSegment, 30, 50);
    break;

  case SOLID:
    SolidColor::start(targetSegment, strip.Color(0, 255, 0), 50);
    break;

  case FLASH_TRIGGER:
    FlashOnTrigger::start(targetSegment, strip.Color(0, 0, 255), false, 100);
    break;

  default:
    break;
  }
}

void handleSerial()
{
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "next")
    {
      currentEffect = static_cast<EffectType>((currentEffect + 1) % EFFECT_COUNT);
      applyEffect(currentEffect, seg);
    }
    else if (cmd == "rainbow")
      applyEffect(RAINBOW, seg);
    else if (cmd == "solid")
      applyEffect(SOLID, seg);
    else if (cmd == "micflash")
    {
      Serial.println(">>> micflash: beginning mic");
      Microphone::instance().begin();
      Microphone::instance().setThreshold(400); // Default threshold
      Serial.println(">>> micflash: mic started");
      applyEffect(FLASH_TRIGGER, seg);
    }
    else if (cmd.startsWith("micflash "))
    {
      int threshold = cmd.substring(9).toInt();
      Serial.print("Setting mic threshold to: ");
      Serial.println(threshold);
      Microphone::instance().setThreshold(threshold);
      applyEffect(FLASH_TRIGGER, seg);
    }
    else if (cmd == "micstop")
    {
      Serial.println("Stopping microphone");
      Microphone::instance().stop();
      applyEffect(RAINBOW, seg); // Reset to a default effect
    }
    else if (cmd.startsWith("triggeron "))
    {
      int val = cmd.substring(10).toInt();
      setFlashTrigger(true, constrain(val, 0, 255));
    }
    else if (cmd == "triggeron")
    {
      setFlashTrigger(true, 128); // default brightness
    }
    else if (cmd == "triggeroff")
    {
      setFlashTrigger(false);
    }
    else
      Serial.println("Unknown command. Try 'next', 'rainbow', 'solid', or 'micflash'.");
  }
}

// REVERTED: This function now calculates brightness based on volume.
void updateFlashTriggerFromMic(int threshold, int peakMax, int minBrightness = 20)
{
  Microphone &mic = Microphone::instance();
  if (mic.available())
  {
    int peak = mic.readPeak();
    // Uncomment the line below for debugging microphone values
    // Serial.print("[Mic] Peak = "); Serial.println(peak);

    if (peak >= threshold)
    {
      // Map peak amplitude to brightness (minBrightness–255)
      int brightness = map(peak, threshold, peakMax, minBrightness, 255);
      brightness = constrain(brightness, minBrightness, 255);

      // Uncomment for debugging trigger events
      // Serial.print("→ Triggering at brightness: "); Serial.println(brightness);

      setFlashTrigger(true, brightness);
    }
    else
    {
      setFlashTrigger(false);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ; // wait for USB

  pinMode(TRIGGER_PIN, INPUT);

  strip.begin();
  
  // Correctly get the first user-defined segment (index 1)
  if (strip.getSegments().size() > 1) {
      seg = strip.getSegments()[1];
  } else {
      // Fallback to the 'all' segment if no others exist
      seg = strip.getSegments()[0]; 
  }
  seg->begin();

  applyEffect(currentEffect, seg);
}


void loop() {
  static unsigned long lastBeat = 0;
  if (millis() - lastBeat > 1000) {
    // A simple heartbeat to know the loop is running
    // Serial.println("[Loop] alive");
    lastBeat = millis();
  }

  handleSerial();

  // If the current effect is flash, check the microphone peak volume
  if (currentEffect == FLASH_TRIGGER) {
    // REVERTED: Call the function with all parameters for variable brightness.
    // Adjust these values to tune the sensitivity.
    // updateFlashTriggerFromMic(threshold, peak_max, min_brightness)
    updateFlashTriggerFromMic(2000, 15000, 20); 
  }

  // Update segment state and push to strip
  seg->update();
  strip.show();

  // A small delay can help with stability and prevent overwhelming the serial port
  delay(5); 
}
