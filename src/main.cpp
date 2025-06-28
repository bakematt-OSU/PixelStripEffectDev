#include <Arduino.h>
#include "PixelStrip.h"
#include "Triggers.h"
#include <PDM.h>
#include <WiFiNINA.h>

#define LEDR 25
#define LEDG 26
#define LEDB 27

// --- System-wide Audio Constants ---
#define SAMPLES 256
#define SAMPLING_FREQUENCY 16000

// --- Pin and LED Definitions ---
#define LED_PIN 4
#define LED_COUNT 300
#define BRIGHTNESS 25
#define SEGMENTS 0

// --- Active Color Variables ---
// These are now primarily for other effects, not the heartbeat directly.
uint8_t activeR = 128;
uint8_t activeG = 0;
uint8_t activeB = 128;

// --- PDM Audio Buffer ---
volatile int16_t sampleBuffer[SAMPLES];
volatile int samplesRead;

// --- Global Objects ---
PixelStrip strip(LED_PIN, LED_COUNT, BRIGHTNESS, SEGMENTS);
PixelStrip::Segment *seg;
AudioTrigger<SAMPLES> audioTrigger;

// --- Heartbeat Effect State Variables (MODIFIED) ---
enum HeartbeatColorState
{
    HEARTBEAT_RED,
    HEARTBEAT_GREEN,
    HEARTBEAT_BLUE
};
HeartbeatColorState heartbeatColorState = HEARTBEAT_RED;    // Start with red
unsigned long lastHeartbeatColorChange = 0;                 // Tracks when the color last changed
const unsigned long HEARTBEAT_COLOR_CHANGE_INTERVAL = 2000; // 2 seconds

// REMOVED: Old heartbeat pulse states and related variables are no longer needed.
// enum HeartbeatState { PULSE_1_UP, PULSE_1_DOWN, PAUSE_1, PULSE_2_UP, PULSE_2_DOWN, PAUSE_2 };
// HeartbeatState heartbeatState = PULSE_1_UP;
// unsigned long lastHeartbeatUpdate = 0;
// int heartbeatBrightness = 0;
// const int HEARTBEAT_STEP_DELAY = 10;
// const int HEARTBEAT_PAUSE_1 = 100;
// const int HEARTBEAT_PAUSE_2 = 700;

// --- Callback and Command Functions ---

void ledFlashCallback(bool isActive, uint8_t brightness)
{
    strip.propagateTriggerState(isActive, brightness);
}

// MODIFIED: updateHeartbeat function for individual color flashing
void updateHeartbeat()
{
    if (millis() - lastHeartbeatColorChange < HEARTBEAT_COLOR_CHANGE_INTERVAL)
    {
        return;
    }
    lastHeartbeatColorChange = millis();

    // Turn off all LEDs initially before setting the new one
    WiFiDrv::analogWrite(LEDR, 0);
    WiFiDrv::analogWrite(LEDG, 0);
    WiFiDrv::analogWrite(LEDB, 0);

    switch (heartbeatColorState)
    {
    case HEARTBEAT_RED:
        WiFiDrv::analogWrite(LEDR, 255);       // Turn Red on
        heartbeatColorState = HEARTBEAT_GREEN; // Next state will be Green
        break;
    case HEARTBEAT_GREEN:
        WiFiDrv::analogWrite(LEDG, 255);      // Turn Green on
        heartbeatColorState = HEARTBEAT_BLUE; // Next state will be Blue
        break;
    case HEARTBEAT_BLUE:
        WiFiDrv::analogWrite(LEDB, 255);     // Turn Blue on
        heartbeatColorState = HEARTBEAT_RED; // Next state will be Red (cycle back)
        break;
    }
}

void handleSerial()
{
    if (Serial.available())
    {
        String cmd_full = Serial.readStringUntil('\n');
        cmd_full.trim();

        String cmd_base = cmd_full;
        String cmd_params = "";

        int space_index = cmd_full.indexOf(' ');
        if (space_index != -1)
        {
            cmd_base = cmd_full.substring(0, space_index);
            cmd_params = cmd_full.substring(space_index + 1);
        }
        cmd_base.toLowerCase();

        // ... (All of your existing serial command logic remains here)
        if (cmd_base == "clearsegments")
        {
            Serial.println("Clearing all user-defined segments...");
            strip.clearUserSegments();
            seg = strip.getSegments()[0];
            seg->startEffect(PixelStrip::Segment::SegmentEffect::NONE);
            Serial.println("Active segment is now 0 (full strip).");
        }
        else if (cmd_base == "addsegment")
        {
            int firstSpace = cmd_params.indexOf(' ');
            if (firstSpace != -1)
            {
                int start = cmd_params.substring(0, firstSpace).toInt();
                int end = cmd_params.substring(firstSpace + 1).toInt();
                if (end >= start)
                {
                    String name = "seg" + String(strip.getSegments().size());
                    strip.addSection(start, end, name);
                    Serial.print("Added new segment (index ");
                    Serial.print(strip.getSegments().size() - 1);
                    Serial.print(") from pixel ");
                    Serial.print(start);
                    Serial.print(" to ");
                    Serial.println(end);
                }
                else
                {
                    Serial.println("Error: End pixel must be >= start pixel.");
                }
            }
            else
            {
                Serial.println("Invalid format. Use: addsegment <start> <end>");
            }
        }
        else if (cmd_base == "select")
        {
            int segmentIndex = cmd_params.toInt();
            if (segmentIndex >= 0 && segmentIndex < strip.getSegments().size())
            {
                seg = strip.getSegments()[segmentIndex];
                Serial.print("Segment ");
                Serial.print(segmentIndex);
                Serial.println(" selected.");
            }
            else
            {
                Serial.println("Invalid segment index.");
            }
        }
        else if (cmd_base == "setcolor")
        {
            int firstSpace = cmd_params.indexOf(' ');
            int secondSpace = cmd_params.indexOf(' ', firstSpace + 1);
            if (firstSpace > 0 && secondSpace > 0)
            {
                activeR = cmd_params.substring(0, firstSpace).toInt();
                activeG = cmd_params.substring(firstSpace + 1, secondSpace).toInt();
                activeB = cmd_params.substring(secondSpace + 1).toInt();
                Serial.print("Active color set to: R=");
                Serial.print(activeR);
                Serial.print(" G=");
                Serial.print(activeG);
                Serial.print(" B=");
                Serial.println(activeB);
            }
            else
            {
                Serial.println("Invalid format. Use: setcolor <r> <g> <b>");
            }
        }
        else if (cmd_base == "bassflash")
        {
            seg->startEffect(PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER, strip.Color(activeR, activeG, activeB));
        }
        else if (cmd_base == "solid")
        {
            seg->startEffect(PixelStrip::Segment::SegmentEffect::SOLID, strip.Color(activeR, activeG, activeB));
        }
        else if (cmd_base == "rainbow" || cmd_base == "stop")
        {
            seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
        }
        else if (cmd_base == "fire" || cmd_base == "flare")
        {
            uint32_t p1 = 0, p2 = 0;
            int space_idx = cmd_params.indexOf(' ');
            if (cmd_params.length() > 0)
            {
                if (space_idx != -1)
                {
                    p1 = cmd_params.substring(0, space_idx).toInt();
                    p2 = cmd_params.substring(space_idx + 1).toInt();
                }
                else
                {
                    p1 = cmd_params.toInt();
                }
            }
            if (cmd_base == "fire")
                seg->startEffect(PixelStrip::Segment::SegmentEffect::FIRE, p1, p2);
            else
                seg->startEffect(PixelStrip::Segment::SegmentEffect::FLARE, p1, p2);
        }
        else if (cmd_base == "rainbowcycle" || cmd_base == "theaterchase")
        {
            uint32_t p1 = 0;
            if (cmd_params.length() > 0)
            {
                p1 = cmd_params.toInt();
            }
            if (cmd_base == "rainbowcycle")
                seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW_CYCLE, p1);
            else
                seg->startEffect(PixelStrip::Segment::SegmentEffect::THEATER_CHASE, p1);
        }
        else if (cmd_base == "coloredfire")
        {
            // This effect uses the currently set active color
            uint32_t base_color = strip.Color(activeR, activeG, activeB);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::COLORED_FIRE, base_color, 0);
            Serial.println("Starting Colored Fire effect.");
        }
        else if (cmd_base == "next")
        {
            int current_val = static_cast<int>(seg->activeEffect);
            int next_val = current_val + 1;
            if (next_val >= static_cast<int>(PixelStrip::Segment::SegmentEffect::EFFECT_COUNT))
            {
                next_val = 1;
            }
            auto next_effect = static_cast<PixelStrip::Segment::SegmentEffect>(next_val);

            switch (next_effect)
            {
            case PixelStrip::Segment::SegmentEffect::SOLID:
            case PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER:
                seg->startEffect(next_effect, strip.Color(activeR, activeG, activeB));
                break;
            case PixelStrip::Segment::SegmentEffect::RAINBOW_CYCLE:
                seg->startEffect(next_effect, 10);
                break;
            case PixelStrip::Segment::SegmentEffect::THEATER_CHASE:
                seg->startEffect(next_effect, 50);
                break;
            case PixelStrip::Segment::SegmentEffect::FIRE:
            case PixelStrip::Segment::SegmentEffect::FLARE:
                seg->startEffect(next_effect, 0, 0);
                break;
            default:
                seg->startEffect(next_effect);
                break;
            }
        }
    }
}

void onPDMdata()
{
    int bytesAvailable = PDM.available();
    PDM.read((int16_t *)sampleBuffer, bytesAvailable);
    samplesRead = bytesAvailable / 2;
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    // Initialize the onboard LED pins (Common Cathode: 0 = off, 255 = brightest)
    // Ensure the LED starts off by setting all channels to 0.
    WiFiDrv::analogWrite(LEDR, 0);
    WiFiDrv::analogWrite(LEDG, 0);
    WiFiDrv::analogWrite(LEDB, 0);

    // Check for the WiFi module and firmware
    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");
        // Set onboard LED to solid red to indicate an error (full red, green/blue off)
        WiFiDrv::analogWrite(LEDR, 255); // Full brightness red
        WiFiDrv::analogWrite(LEDG, 0);   // Off green
        WiFiDrv::analogWrite(LEDB, 0);   // Off blue
        while (true)
            ;
    }

    // Set up the audio trigger
    PDM.onReceive(onPDMdata);
    audioTrigger.onTrigger(ledFlashCallback);
    if (!PDM.begin(1, SAMPLING_FREQUENCY))
    {
        Serial.println("Failed to start PDM!");
        while (1)
            ;
    }

    strip.begin();

    seg = strip.getSegments()[0];
    seg->begin();
    seg->startEffect(PixelStrip::Segment::SegmentEffect::NONE);
}

void loop()
{
    handleSerial();

    if (samplesRead > 0)
    {
        audioTrigger.update(sampleBuffer);
        samplesRead = 0;
    }

    updateHeartbeat();

    for (auto *s : strip.getSegments())
    {
        s->update();
    }

    strip.show();
}