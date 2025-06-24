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
// Microphone mic(400);

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

void applyEffect(EffectType effect)
{
  currentEffect = effect;

  switch (effect)
  {
  case RAINBOW:
    RainbowChase::start(seg, 30, 50);
    break;

  case SOLID:
    SolidColor::start(seg, strip.Color(0, 255, 0), 50);
    break;

  case FLASH_TRIGGER:
    FlashOnTrigger::start(seg, strip.Color(0, 0, 255), false, 100);
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
      applyEffect(currentEffect);
    }
    else if (cmd == "rainbow")
      applyEffect(RAINBOW);
    else if (cmd == "solid")
      applyEffect(SOLID);
    else if (cmd == "micflash")
    {
      Serial.println(">>> micflash: beginning mic");
      // mic.begin(); // ← might freeze here
      Microphone::instance().begin();
      Microphone::instance().setThreshold(400);
      Serial.println(">>> micflash: mic started");
      applyEffect(FLASH_TRIGGER);
    }
    else if (cmd.startsWith("micflash "))
    {
      int threshold = cmd.substring(9).toInt();
      Serial.print("Setting mic threshold to: ");
      Serial.println(threshold);
      Microphone::instance().setThreshold(threshold); // ✅ updated
      applyEffect(FLASH_TRIGGER);
    }
    else if (cmd == "micstop")
    {
      Serial.println("Stopping microphone");
      Microphone::instance().stop();
      applyEffect(RAINBOW); // Reset to rainbow effect
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

void updateFlashTriggerFromMic(int threshold, int peakMax, int minBrightness = 20)
{
  Microphone &mic = Microphone::instance();
  if (mic.available())
  {
    int peak = mic.readPeak();
    Serial.print("[Mic] Peak = ");
    Serial.println(peak);

    if (peak >= threshold)
    {
      // Map peak amplitude to brightness (minBrightness–255)
      int brightness = map(peak, threshold, peakMax, minBrightness, 255);
      brightness = constrain(brightness, minBrightness, 255);

      Serial.print("→ Triggering at brightness: ");
      Serial.println(brightness);

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

  pinMode(TRIGGER_PIN, INPUT); // 🔸 Set up trigger input pin

  strip.begin();
  seg = strip.getSegments()[1];
  seg->begin();

  applyEffect(currentEffect);
}


void loop() {
  static unsigned long lastBeat = 0;
  if (millis() - lastBeat > 1000) {
    Serial.println("[Loop] alive");
    lastBeat = millis();
  }

  // Handle any user input
  handleSerial();

  // If the current effect is flash, check the microphone
  if (currentEffect == FLASH_TRIGGER) {
    updateFlashTriggerFromMic(400, 1500);
  }

  // Update the color data in the memory buffer based on the active effect
  seg->update();
  
  // --- THIS IS THE CORRECT PLACE ---
  // Push the updated color data to the physical LED strip.
  strip.show();

//   // The delay is likely no longer needed, but you can keep it if it helps
//   // with other parts of your code. You can try removing it.
//   delay(5);
}
