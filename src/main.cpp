#include <Arduino.h>
#include "PixelStrip.h"
// #include "effects/Effects.h"
#include "Triggers.h"      
#include <PDM.h>           

// --- System-wide Audio Constants ---
#define SAMPLES 256
#define SAMPLING_FREQUENCY 16000

// --- Pin and LED Definitions ---
#define LED_PIN 4
#define LED_COUNT 300
#define BRIGHTNESS 25
#define SEGMENTS 1

// --- Bass Flash Color ---
uint8_t bassR = 128;
uint8_t bassG = 0;
uint8_t bassB = 128; 

// --- PDM Audio Buffer ---
volatile int16_t sampleBuffer[SAMPLES];
volatile int samplesRead;

// --- Global Objects ---
PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);
PixelStrip::Segment *seg;
AudioTrigger<SAMPLES> audioTrigger; 

// --- State and Callback Functions ---
// REMOVED: Redundant globals. State is now managed inside the Segment object.
// EffectType currentEffect = RAINBOW;
// bool flashTriggerActive = false;

// These are still needed by the FlashOnTrigger::update function
bool flashTriggerState = false;
uint8_t flashTriggerBrightness = 0;
void setFlashTrigger(bool value, uint8_t brightness = 0) {
  flashTriggerState = value;
  flashTriggerBrightness = brightness;
}

void ledFlashCallback(bool isActive, uint8_t brightness) {
    setFlashTrigger(isActive, brightness);
}

// REMOVED: This logic is now inside the PixelStrip::Segment class.
// void applyEffect(...) { ... }

void handleSerial() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();

        if (cmd.startsWith("setcolor")) {
            // This logic remains in main as it controls a variable used here.
            int firstSpace = cmd.indexOf(' ');
            if (firstSpace != -1) {
                int secondSpace = cmd.indexOf(' ', firstSpace + 1);
                int thirdSpace = cmd.indexOf(' ', secondSpace + 1);
                if (secondSpace != -1 && thirdSpace != -1) {
                    bassR = cmd.substring(firstSpace + 1, secondSpace).toInt();
                    bassG = cmd.substring(secondSpace + 1, thirdSpace).toInt();
                    bassB = cmd.substring(thirdSpace + 1).toInt();
                    Serial.println("Color set. Re-enable bassflash to see the change.");
                }
            }
        }
        else if (cmd == "bassflash") {
            Serial.println(">>> Trigger Mode: BASS");
            audioTrigger.onTrigger(ledFlashCallback); // Controller logic
            uint32_t flashColor = strip.Color(bassR, bassG, bassB);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER, flashColor);
        } else if (cmd == "stop") {
             audioTrigger.onTrigger(nullptr); // Turn off trigger
             seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
        } else if (cmd == "rainbow") {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
        } else if (cmd == "solid") {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::SOLID);
        }
        else if (cmd == "next") {
            audioTrigger.onTrigger(nullptr);
            // Get current effect from the segment and cycle to the next one
            auto current = seg->activeEffect;
            switch (current) {
                case PixelStrip::Segment::SegmentEffect::RAINBOW:
                    seg->startEffect(PixelStrip::Segment::SegmentEffect::SOLID);
                    break;
                case PixelStrip::Segment::SegmentEffect::SOLID:
                    // For 'next', we go to bassflash with the default color
                    audioTrigger.onTrigger(ledFlashCallback);
                    seg->startEffect(PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER);
                    break;
                case PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER:
                default:
                    seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
                    break;
            }
        }
    }
}
void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read((int16_t*)sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, SAMPLING_FREQUENCY)) {
    Serial.println("Failed to start PDM!");
    while (1);
  }
  
  strip.begin();
  if (strip.getSegments().size() > 1) {
      seg = strip.getSegments()[1];
  } else {
      seg = strip.getSegments()[0]; 
  }
  seg->begin();

  // Start with a default effect
  seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
}

void loop() {
  handleSerial();
  
  // UPDATED: Check the segment's flashTriggerActive state directly
  if (seg->flashTriggerActive && samplesRead > 0) {
    audioTrigger.update(sampleBuffer);
    samplesRead = 0; 
  }

  seg->update();
  strip.show();
}