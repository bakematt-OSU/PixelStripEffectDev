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
// Add these variables near the top of your main.cpp
uint8_t bassR = 128;
uint8_t bassG = 0;
uint8_t bassB = 128; // Default to blue

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

// Replace your existing applyEffect function with this version
void applyEffect(EffectType effect, PixelStrip::Segment* targetSegment) {
  if (!targetSegment) return;
  currentEffect = effect;
  flashTriggerActive = (effect == FLASH_TRIGGER);

  if (flashTriggerActive) {
      audioTrigger.onTrigger(ledFlashCallback);
      // UPDATED: Now uses the global variables for color
      FlashOnTrigger::start(targetSegment, strip.Color(bassR, bassG, bassB), false, 100);
  } else {
      audioTrigger.onTrigger(nullptr); // Disable trigger for other effects
      if (effect == RAINBOW) RainbowChase::start(targetSegment, 30, 50);
      else if (effect == SOLID) SolidColor::start(targetSegment, strip.Color(0, 255, 0), 50);
  }
}

// Replace your existing handleSerial function with this version
void handleSerial() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();

        // NEW: Logic to handle the "setcolor" command
        if (cmd.startsWith("setcolor")) {
            // Example command: "setcolor 255 0 0" for red
            int firstSpace = cmd.indexOf(' ');
            if (firstSpace != -1) {
                int secondSpace = cmd.indexOf(' ', firstSpace + 1);
                int thirdSpace = cmd.indexOf(' ', secondSpace + 1);

                if (secondSpace != -1 && thirdSpace != -1) {
                    String r_str = cmd.substring(firstSpace + 1, secondSpace);
                    String g_str = cmd.substring(secondSpace + 1, thirdSpace);
                    String b_str = cmd.substring(thirdSpace + 1);

                    bassR = r_str.toInt();
                    bassG = g_str.toInt();
                    bassB = b_str.toInt();
                    
                    Serial.print("New bass flash color set to R: ");
                    Serial.print(bassR);
                    Serial.print(", G: ");
                    Serial.print(bassG);
                    Serial.print(", B: ");
                    Serial.println(bassB);

                    // If in bass flash mode, re-apply the effect to show the new color immediately
                    if (currentEffect == FLASH_TRIGGER) {
                        applyEffect(FLASH_TRIGGER, seg);
                    }
                } else {
                     Serial.println("Invalid format. Use: setcolor <r> <g> <b>");
                }
            } else {
                Serial.println("Invalid format. Use: setcolor <r> <g> <b>");
            }
        }
        else if (cmd == "bassflash") {
            Serial.println(">>> Trigger Mode: BASS (Onboard Mic)");
            applyEffect(FLASH_TRIGGER, seg);
        } else if (cmd == "stop") {
             applyEffect(RAINBOW, seg); // Switch to a default, non-trigger effect
        }
        else if (cmd == "next" || cmd == "rainbow" || cmd == "solid") {
            if (cmd == "next") currentEffect = static_cast<EffectType>((currentEffect + 1) % EFFECT_COUNT);
            if (cmd == "rainbow") currentEffect = RAINBOW;
            if (cmd == "solid") currentEffect = SOLID;
            applyEffect(currentEffect, seg);
        }
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
