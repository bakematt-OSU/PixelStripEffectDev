#include <Arduino.h>
#include "PixelStrip.h"
#include "effects/Effects.h"
#include "Sensors.h"
#include "Triggers.h" // <-- INCLUDE THE NEW MODULAR TRIGGER FILE

// --- Pin and LED Definitions ---
#define LED_PIN 4
#define LED_COUNT 300
#define BRIGHTNESS 25
#define SEGMENTS 1
#define MIC_PIN A0 // Define the analog pin for the microphone

// --- Global Objects ---
PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);
PixelStrip::Segment *seg;

// Create an instance of our new AudioTrigger.
// We pass it the microphone instance, default mode, and the analog pin.
AudioTrigger audioTrigger(Microphone::instance(), AudioTrigger::PEAK, MIC_PIN, 400, 1500, 20);

// --- State Enums and Variables ---
enum EffectType { RAINBOW, SOLID, FLASH_TRIGGER, EFFECT_COUNT };
EffectType currentEffect = RAINBOW;

// These global variables are still needed because the FlashOnTrigger effect reads them directly.
bool flashTriggerState = false;
uint8_t flashTriggerBrightness = 0;
void setFlashTrigger(bool value, uint8_t brightness = 0)
{
  flashTriggerState = value;
  flashTriggerBrightness = brightness;
}

// --- CALLBACK FUNCTION ---
// This is the specific action for the LEDs.
// The AudioTrigger will call this function when a sound is detected.
void ledFlashCallback(bool isActive, uint8_t brightness) {
    setFlashTrigger(isActive, brightness);
}

void applyEffect(EffectType effect, PixelStrip::Segment* targetSegment)
{
  if (!targetSegment) return;
  currentEffect = effect;

  switch (effect)
  {
    case RAINBOW:
      // When switching to a non-trigger effect, disable the audio trigger.
      audioTrigger.onTrigger(nullptr); 
      RainbowChase::start(targetSegment, 30, 50);
      break;
    case SOLID:
      audioTrigger.onTrigger(nullptr);
      SolidColor::start(targetSegment, strip.Color(0, 255, 0), 50);
      break;
    case FLASH_TRIGGER:
      // When we select this effect, we also tell the trigger
      // to use our LED callback function. The specific mode (PEAK vs BASS)
      // is set by the serial command.
      audioTrigger.onTrigger(ledFlashCallback);
      FlashOnTrigger::start(targetSegment, strip.Color(0, 0, 255), false, 100);
      break;
  }
}

void handleSerial() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();

        if (cmd == "micflash") {
            Serial.println(">>> Trigger Mode: PEAK volume");
            Microphone::instance().begin(); // Needed for peak detection
            audioTrigger.setDetectionMode(AudioTrigger::PEAK);
            // You can adjust the threshold for peak detection here
            audioTrigger.setThreshold(400); 
            applyEffect(FLASH_TRIGGER, seg);
        } 
        else if (cmd == "bassflash") {
            Serial.println(">>> Trigger Mode: BASS frequency");
            audioTrigger.setDetectionMode(AudioTrigger::BASS);
            // Bass detection often requires a different threshold. Tune this value.
            audioTrigger.setThreshold(1500); 
            applyEffect(FLASH_TRIGGER, seg);
        }
        else if (cmd == "micstop") {
            Serial.println(">>> Disabling Audio Trigger");
            applyEffect(RAINBOW, seg); // Switches to a default effect, which also disables the trigger
        }
        else if (cmd == "next" || cmd == "rainbow" || cmd == "solid") {
             // Handle other commands to switch effects
            if (cmd == "next") currentEffect = static_cast<EffectType>((currentEffect + 1) % EFFECT_COUNT);
            if (cmd == "rainbow") currentEffect = RAINBOW;
            if (cmd == "solid") currentEffect = SOLID;
            applyEffect(currentEffect, seg);
        }
    }
}


void setup()
{
  Serial.begin(115200);
  while (!Serial);

  strip.begin();
  if (strip.getSegments().size() > 1) {
      seg = strip.getSegments()[1];
  } else {
      seg = strip.getSegments()[0]; 
  }
  seg->begin();

  applyEffect(currentEffect, seg);
}


void loop() {
  handleSerial();

  // --- Simplified Main Loop ---
  // 1. Check for trigger events. If a callback is registered, it will be called.
  audioTrigger.update();

  // 2. Update the LED strip based on the current state.
  seg->update();

  // 3. Show the result.
  strip.show();
  
  delay(5); 
}
