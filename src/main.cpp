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

// The callback now calls the method on the strip object, which broadcasts the state.
void ledFlashCallback(bool isActive, uint8_t brightness) {
    strip.propagateTriggerState(isActive, brightness);
}

void handleSerial() {
    if (Serial.available()) {
        String cmd_full = Serial.readStringUntil('\n');
        cmd_full.trim();
        
        // This splits the command into a base command word and its parameters.
        String cmd_base = cmd_full;
        String cmd_params = "";
        
        int space_index = cmd_full.indexOf(' ');
        if (space_index != -1) {
            cmd_base = cmd_full.substring(0, space_index);
            cmd_params = cmd_full.substring(space_index + 1);
        }
        cmd_base.toLowerCase();

        // --- Command Handling ---

        if (cmd_base == "clearsegments") {
            Serial.println("Clearing all user-defined segments...");
            strip.clearUserSegments();
            seg = strip.getSegments()[0]; // Default back to the 'all' segment
            seg->startEffect(PixelStrip::Segment::SegmentEffect::NONE);
            Serial.println("Active segment is now 0 (full strip).");
        }
        else if (cmd_base == "addsegment") {
            int firstSpace = cmd_params.indexOf(' ');
            if (firstSpace != -1) {
                int start = cmd_params.substring(0, firstSpace).toInt();
                int end = cmd_params.substring(firstSpace + 1).toInt();
                if (end >= start) {
                    String name = "seg" + String(strip.getSegments().size());
                    strip.addSection(start, end, name);
                    Serial.print("Added new segment (index ");
                    Serial.print(strip.getSegments().size() - 1);
                    Serial.print(") from pixel "); Serial.print(start);
                    Serial.print(" to "); Serial.println(end);
                } else { Serial.println("Error: End pixel must be >= start pixel."); }
            } else { Serial.println("Invalid format. Use: addsegment <start> <end>"); }
        }
        else if (cmd_base == "select") {
            int segmentIndex = cmd_params.toInt();
            if (segmentIndex >= 0 && segmentIndex < strip.getSegments().size()) {
                 seg = strip.getSegments()[segmentIndex];
                 Serial.print("Segment "); Serial.print(segmentIndex); Serial.println(" selected.");
            } else { Serial.println("Invalid segment index."); }
        }
        else if (cmd_base == "setcolor") {
            int firstSpace = cmd_params.indexOf(' ');
            if (firstSpace != -1) {
                int secondSpace = cmd_params.indexOf(' ', firstSpace + 1);
                int thirdSpace = cmd_params.indexOf(' ', secondSpace + 1);
                if (secondSpace != -1 && thirdSpace != -1) {
                    bassR = cmd_params.substring(0, secondSpace).toInt();
                    bassG = cmd_params.substring(secondSpace + 1, thirdSpace).toInt();
                    bassB = cmd_params.substring(thirdSpace + 1).toInt();
                    Serial.println("Bass flash color set. Re-enable bassflash on a segment to see the change.");
                } else { Serial.println("Invalid format. Use: setcolor <r> <g> <b>"); }
            } else { Serial.println("Invalid format. Use: setcolor <r> <g> <b>"); }
        }
        else if (cmd_base == "bassflash") {
            seg->startEffect(PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER, strip.Color(bassR, bassG, bassB));
        }
        else if (cmd_base == "fire") {
            uint32_t p1 = 0, p2 = 0; // 0 tells the effect to use its default
            int space_idx = cmd_params.indexOf(' ');
            if (cmd_params.length() > 0) {
                if (space_idx != -1) { // Two+ parameters
                    p1 = cmd_params.substring(0, space_idx).toInt();
                    p2 = cmd_params.substring(space_idx + 1).toInt();
                } else { // One parameter
                    p1 = cmd_params.toInt();
                }
            }
            seg->startEffect(PixelStrip::Segment::SegmentEffect::FIRE, p1, p2);
        }
        else if (cmd_base == "rainbowcycle" || cmd_base == "theaterchase") {
            uint32_t p1 = 0; // 0 tells the effect to use its default
            if (cmd_params.length() > 0) {
                p1 = cmd_params.toInt();
            }
            if (cmd_base == "rainbowcycle") seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW_CYCLE, p1);
            else seg->startEffect(PixelStrip::Segment::SegmentEffect::THEATER_CHASE, p1);
        }
        else if (cmd_base == "rainbow" || cmd_base == "stop") {
            seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
        }
        else if (cmd_base == "solid") {
            seg->startEffect(PixelStrip::Segment::SegmentEffect::SOLID, strip.Color(0,255,0));
        }
        else if (cmd_base == "next") {
            int current_val = static_cast<int>(seg->activeEffect);
            int next_val = current_val + 1;
            if (next_val >= static_cast<int>(PixelStrip::Segment::SegmentEffect::EFFECT_COUNT)) {
                next_val = 1;
            }
            auto next_effect = static_cast<PixelStrip::Segment::SegmentEffect>(next_val);
            
            // Start each effect with its correct default parameters
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
                    seg->startEffect(next_effect, 20); // Using a default speed for fire
                    break;
                default: // Includes RAINBOW
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

  // Set up the audio trigger ONCE and leave it on
  PDM.onReceive(onPDMdata);
  audioTrigger.onTrigger(ledFlashCallback); 
  if (!PDM.begin(1, SAMPLING_FREQUENCY)) {
    Serial.println("Failed to start PDM!");
    while (1);
  }
  
  strip.begin();
  
  // Set the default active segment to the main segment (index 0)
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