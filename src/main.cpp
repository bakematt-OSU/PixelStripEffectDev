#include <Arduino.h>
#include "PixelStrip.h"
#include "Triggers.h"      
#include <PDM.h>           

// --- System-wide Audio Constants ---
#define SAMPLES 256
#define SAMPLING_FREQUENCY 16000

// --- Pin and LED Definitions ---
#define LED_PIN 4
#define LED_COUNT 300
#define BRIGHTNESS 25
// Set to 0 to disable automatic segment division and allow for manual creation
#define SEGMENTS 0

// --- Bass Flash Color ---
uint8_t bassR = 128;
uint8_t bassG = 0;
uint8_t bassB = 128; 

// --- PDM Audio Buffer ---
volatile int16_t sampleBuffer[SAMPLES];
volatile int samplesRead;

// --- Global Objects ---
PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);

// This pointer refers to the "currently selected" segment for commands.
PixelStrip::Segment *seg;

AudioTrigger<SAMPLES> audioTrigger; 

// The callback calls the method on the currently selected segment object.
void ledFlashCallback(bool isActive, uint8_t brightness) {
    if (seg) {
        seg->setTriggerState(isActive, brightness);
    }
}

void handleSerial() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();

        // --- NEW: Dynamic Segment Commands ---
        if (cmd == "clearsegments") {
            Serial.println("Clearing all user-defined segments...");
            strip.clearUserSegments();
            seg = strip.getSegments()[0]; // Default back to the 'all' segment
            seg->startEffect(PixelStrip::Segment::SegmentEffect::NONE);
            Serial.println("Active segment is now 0 (full strip).");
        }
        else if (cmd.startsWith("addsegment")) {
            // Example command: "addsegment 0 99"
            int firstSpace = cmd.indexOf(' ');
            if (firstSpace != -1) {
                int secondSpace = cmd.indexOf(' ', firstSpace + 1);
                if (secondSpace != -1) {
                    int start = cmd.substring(firstSpace + 1, secondSpace).toInt();
                    int end = cmd.substring(secondSpace + 1).toInt();

                    if (end >= start) {
                        String name = "seg" + String(strip.getSegments().size());
                        strip.addSection(start, end, name);
                        Serial.print("Added new segment (index ");
                        Serial.print(strip.getSegments().size() - 1);
                        Serial.print(") from pixel ");
                        Serial.print(start);
                        Serial.print(" to ");
                        Serial.println(end);
                    } else {
                        Serial.println("Error: End pixel must be >= start pixel.");
                    }
                } else {
                    Serial.println("Invalid format. Use: addsegment <start> <end>");
                }
            }
        }
        // --- Effect Control Commands ---
        else if (cmd.startsWith("select")) {
            int segmentIndex = cmd.substring(7).toInt();
            if (segmentIndex >= 0 && segmentIndex < strip.getSegments().size()) {
                 seg = strip.getSegments()[segmentIndex];
                 Serial.print("Segment ");
                 Serial.print(segmentIndex);
                 Serial.println(" selected.");
            } else {
                 Serial.println("Invalid segment index.");
            }
        }
        else if (cmd.startsWith("setcolor")) {
            int firstSpace = cmd.indexOf(' ');
            if (firstSpace != -1) {
                int secondSpace = cmd.indexOf(' ', firstSpace + 1);
                int thirdSpace = cmd.indexOf(' ', secondSpace + 1);
                if (secondSpace != -1 && thirdSpace != -1) {
                    bassR = cmd.substring(firstSpace + 1, secondSpace).toInt();
                    bassG = cmd.substring(secondSpace + 1, thirdSpace).toInt();
                    bassB = cmd.substring(thirdSpace + 1).toInt();
                    Serial.println("Bass flash color set. Re-enable bassflash on a segment to see the change.");
                }
            }
        }
        else if (cmd == "bassflash") {
            Serial.print(">>> Applying BASS TRIGGER to selected segment: ");
            Serial.println(seg->getId());
            audioTrigger.onTrigger(ledFlashCallback);
            uint32_t flashColor = strip.Color(bassR, bassG, bassB);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER, flashColor);
        } else if (cmd == "stop") {
             audioTrigger.onTrigger(nullptr);
             seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
        } else if (cmd == "rainbow") {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
        } else if (cmd == "solid") {
            audioTrigger.onTrigger(nullptr);
            uint32_t solidColor = strip.Color(0, 255, 0);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::SOLID, solidColor);
        }
        else if (cmd == "rainbowcycle") {
            audioTrigger.onTrigger(nullptr); 
            seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW_CYCLE, 10);
        }
        else if (cmd == "theaterchase") {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::THEATER_CHASE, 50);
        }
        else if (cmd == "fire") {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::FIRE, 20);
        }
        else if (cmd == "next") {
            audioTrigger.onTrigger(nullptr);
            int current_val = static_cast<int>(seg->activeEffect);
            int next_val = current_val + 1;
            if (next_val >= static_cast<int>(PixelStrip::Segment::SegmentEffect::EFFECT_COUNT)) {
                next_val = 1;
            }
            auto next_effect = static_cast<PixelStrip::Segment::SegmentEffect>(next_val);

            if (next_effect == PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER) {
                 audioTrigger.onTrigger(ledFlashCallback);
            }
            
            switch(next_effect) {
                case PixelStrip::Segment::SegmentEffect::SOLID:
                    seg->startEffect(next_effect, strip.Color(0, 255, 0));
                    break;
                case PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER:
                    seg->startEffect(next_effect, strip.Color(bassR, bassG, bassB));
                    break;
                case PixelStrip::Segment::SegmentEffect::RAINBOW_CYCLE:
                    seg->startEffect(next_effect, 10);
                    break;
                case PixelStrip::Segment::SegmentEffect::THEATER_CHASE:
                    seg->startEffect(next_effect, 50);
                    break;
                case PixelStrip::Segment::SegmentEffect::FIRE:
                    seg->startEffect(next_effect, 20);
                    break;
                default:
                    seg->startEffect(next_effect);
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
  
  // Set the default active segment to the main segment (index 0), which is the whole strip.
  // Use the new terminal commands to add more segments.
  seg = strip.getSegments()[0]; 
  seg->begin();
  seg->startEffect(PixelStrip::Segment::SegmentEffect::NONE);
}

void loop() {
  handleSerial();
  
  if (samplesRead > 0) {
    audioTrigger.update(sampleBuffer);
    samplesRead = 0; 
  }

  // Loop through all segments and call update() on each one.
  for (auto* s : strip.getSegments()) {
    s->update();
  }

  strip.show();
}