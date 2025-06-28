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

// --- Heartbeat Effect State Variables ---
enum HeartbeatColorState
{
    HEARTBEAT_RED,
    HEARTBEAT_GREEN,
    HEARTBEAT_BLUE
};
HeartbeatColorState heartbeatColorState = HEARTBEAT_RED;
unsigned long lastHeartbeatColorChange = 0;
const unsigned long HEARTBEAT_COLOR_CHANGE_INTERVAL = 2000;

// --- Callback and Command Functions ---

void ledFlashCallback(bool isActive, uint8_t brightness)
{
    strip.propagateTriggerState(isActive, brightness);
}

void updateHeartbeat()
{
    if (millis() - lastHeartbeatColorChange < HEARTBEAT_COLOR_CHANGE_INTERVAL)
    {
        return;
    }
    lastHeartbeatColorChange = millis();

    WiFiDrv::analogWrite(LEDR, 0);
    WiFiDrv::analogWrite(LEDG, 0);
    WiFiDrv::analogWrite(LEDB, 0);

    switch (heartbeatColorState)
    {
    case HEARTBEAT_RED:
        WiFiDrv::analogWrite(LEDR, 255);
        heartbeatColorState = HEARTBEAT_GREEN;
        break;
    case HEARTBEAT_GREEN:
        WiFiDrv::analogWrite(LEDG, 255);
        heartbeatColorState = HEARTBEAT_BLUE;
        break;
    case HEARTBEAT_BLUE:
        WiFiDrv::analogWrite(LEDB, 255);
        heartbeatColorState = HEARTBEAT_RED;
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
        else if (cmd_base == "setfirecolors")
        {
            // Expects 9 integer values: R1 G1 B1 R2 G2 B2 R3 G3 B3
            int r1, g1, b1, r2, g2, b2, r3, g3, b3;
            int n = sscanf(cmd_params.c_str(), "%d %d %d %d %d %d %d %d %d", &r1, &g1, &b1, &r2, &g2, &b2, &r3, &g3, &b3);
            if (n == 9)
            {
                seg->fireColor1 = strip.Color(r1, g1, b1);
                seg->fireColor2 = strip.Color(r2, g2, b2);
                seg->fireColor3 = strip.Color(r3, g3, b3);
                Serial.println("Fire colors updated.");
                Serial.print("C1: ");
                Serial.print(r1);
                Serial.print(",");
                Serial.print(g1);
                Serial.print(",");
                Serial.println(b1);
                Serial.print("C2: ");
                Serial.print(r2);
                Serial.print(",");
                Serial.print(g2);
                Serial.print(",");
                Serial.println(b2);
                Serial.print("C3: ");
                Serial.print(r3);
                Serial.print(",");
                Serial.print(g3);
                Serial.print(",");
                Serial.println(b3);
            }
            else
            {
                Serial.println("Invalid format. Use: setfirecolors <r1> <g1> <b1> <r2> <g2> <b2> <r3> <g3> <b3>");
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
        else if (cmd_base == "coloredfire")
        {
            // This effect now uses the colors set by 'setfirecolors'
            seg->startEffect(PixelStrip::Segment::SegmentEffect::COLORED_FIRE, 0, 0);
            Serial.println("Starting Colored Fire effect.");
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

    WiFiDrv::analogWrite(LEDR, 0);
    WiFiDrv::analogWrite(LEDG, 0);
    WiFiDrv::analogWrite(LEDB, 0);

    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");
        WiFiDrv::analogWrite(LEDR, 255);
        WiFiDrv::analogWrite(LEDG, 0);
        WiFiDrv::analogWrite(LEDB, 0);
        while (true)
            ;
    }

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