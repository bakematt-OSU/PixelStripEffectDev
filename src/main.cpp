#include <Arduino.h>
#include "PixelStrip.h"
#include "effects/Effects.h" // UPDATED: Use the single aggregate header
#include "Triggers.h"      // Uses the version of Triggers.h with new defaults
#include <PDM.h>           // The library for the onboard microphone

// --- System-wide Audio Constants ---
#define SAMPLES 256
#define SAMPLING_FREQUENCY 16000

// --- Pin and LED Definitions ---
#define LED_PIN 4
#define LED_COUNT 300
#define BRIGHTNESS 25
#define SEGMENTS 1

// --- PDM Audio Buffer ---
volatile int16_t sampleBuffer[SAMPLES];
volatile int samplesRead;

// --- Global Objects ---
PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);
PixelStrip::Segment *seg;
// The AudioTrigger object is now created without arguments,
// because it's using the new default values from Triggers.h.
AudioTrigger<SAMPLES> audioTrigger; 

// --- State and Callback Functions ---
enum EffectType { RAINBOW, SOLID, FLASH_TRIGGER, EFFECT_COUNT };
EffectType currentEffect = RAINBOW;
bool flashTriggerActive = false;

// These are still needed by FlashOnTrigger.h
bool flashTriggerState = false;
uint8_t flashTriggerBrightness = 0;
void setFlashTrigger(bool value, uint8_t brightness = 0) {
  flashTriggerState = value;
  flashTriggerBrightness = brightness;
}

// This is the action that happens when the trigger fires.
void ledFlashCallback(bool isActive, uint8_t brightness) {
    setFlashTrigger(isActive, brightness);
}

// This function now correctly enables/disables the audio trigger callback.
void applyEffect(EffectType effect, PixelStrip::Segment* targetSegment) {
  if (!targetSegment) return;
  currentEffect = effect;
  flashTriggerActive = (effect == FLASH_TRIGGER);

  if (flashTriggerActive) {
      audioTrigger.onTrigger(ledFlashCallback);
      FlashOnTrigger::start(targetSegment, strip.Color(0, 0, 255), false, 100);
  } else {
      audioTrigger.onTrigger(nullptr); // Disable trigger for other effects
      if (effect == RAINBOW) RainbowChase::start(targetSegment, 30, 50);
      else if (effect == SOLID) SolidColor::start(targetSegment, strip.Color(0, 255, 0), 50);
  }
}

void handleSerial() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();

        if (cmd == "bassflash") {
            Serial.println(">>> Trigger Mode: BASS (Onboard Mic)");
            // You can still override the default threshold for testing if needed:
            // audioTrigger.setThreshold(50000); 
            applyEffect(FLASH_TRIGGER, seg);
        } else if (cmd == "stop") {
             applyEffect(RAINBOW, seg); // Switch to a default, non-trigger effect
        }
        // Add other commands here...
    }
}

// This is the PDM library's callback function.
void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read((int16_t*)sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2; // Set the flag that new data is ready
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // --- Initialize Onboard Microphone ---
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, SAMPLING_FREQUENCY)) {
    Serial.println("Failed to start PDM!");
    while (1); // Halt if microphone fails
  }
  
  // --- Initialize LED Strip ---
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
  
  if (flashTriggerActive && samplesRead > 0) {
    audioTrigger.update(sampleBuffer);
    samplesRead = 0; 
  }

  seg->update();
  strip.show();
}
