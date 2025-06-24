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

// The callback now calls the method on the segment object.
void ledFlashCallback(bool isActive, uint8_t brightness)
{
    if (seg)
    {
        seg->setTriggerState(isActive, brightness);
    }
}

// UPDATED: The handleSerial function with corrected logic for the "next" command
void handleSerial()
{
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toLowerCase();

        if (cmd.startsWith("setcolor"))
        {
            int firstSpace = cmd.indexOf(' ');
            if (firstSpace != -1)
            {
                int secondSpace = cmd.indexOf(' ', firstSpace + 1);
                int thirdSpace = cmd.indexOf(' ', secondSpace + 1);
                if (secondSpace != -1 && thirdSpace != -1)
                {
                    bassR = cmd.substring(firstSpace + 1, secondSpace).toInt();
                    bassG = cmd.substring(secondSpace + 1, thirdSpace).toInt();
                    bassB = cmd.substring(thirdSpace + 1).toInt();
                    Serial.println("Color set. Re-enable bassflash to see the change.");
                }
            }
        }
        else if (cmd == "bassflash")
        {
            Serial.println(">>> Trigger Mode: BASS");
            audioTrigger.onTrigger(ledFlashCallback);
            uint32_t flashColor = strip.Color(bassR, bassG, bassB);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER, flashColor);
        }
        else if (cmd == "stop")
        {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
        }
        else if (cmd == "rainbow")
        {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
        }
        else if (cmd == "solid")
        {
            audioTrigger.onTrigger(nullptr);
            uint32_t solidColor = strip.Color(0, 255, 0); // Start with green
            seg->startEffect(PixelStrip::Segment::SegmentEffect::SOLID, solidColor);
        }
        else if (cmd == "rainbowcycle")
        {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW_CYCLE, 10);
        }
        else if (cmd == "theaterchase")
        {
            audioTrigger.onTrigger(nullptr);
            seg->startEffect(PixelStrip::Segment::SegmentEffect::THEATER_CHASE, 50);
        }
        else if (cmd == "fire")
        {
            audioTrigger.onTrigger(nullptr); // Make sure audio trigger is off
            // We can pass a value to control the speed, e.g. 20ms
            seg->startEffect(PixelStrip::Segment::SegmentEffect::FIRE, 20);
        }
        else if (cmd == "next")
        {
            // --- THIS IS THE CORRECTED LOGIC ---
            audioTrigger.onTrigger(nullptr); // Turn off trigger by default for most effects

            int current_val = static_cast<int>(seg->activeEffect);
            int next_val = current_val + 1;
            if (next_val >= static_cast<int>(PixelStrip::Segment::SegmentEffect::EFFECT_COUNT))
            {
                next_val = 1; // 1 is the first effect after NONE
            }
            auto next_effect = static_cast<PixelStrip::Segment::SegmentEffect>(next_val);

            // Use a switch to start each effect with its correct default parameters
            switch (next_effect)
            {
            case PixelStrip::Segment::SegmentEffect::RAINBOW:
                seg->startEffect(next_effect); // No color needed
                break;
            case PixelStrip::Segment::SegmentEffect::SOLID:
                seg->startEffect(next_effect, strip.Color(0, 255, 0)); // Default to green
                break;
            case PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER:
                audioTrigger.onTrigger(ledFlashCallback);                        // Re-enable trigger
                seg->startEffect(next_effect, strip.Color(bassR, bassG, bassB)); // Use bass color
                break;
            case PixelStrip::Segment::SegmentEffect::RAINBOW_CYCLE:
                seg->startEffect(next_effect, 10); // Pass the wait time
                break;
            case PixelStrip::Segment::SegmentEffect::THEATER_CHASE:
                seg->startEffect(next_effect, 50); // Pass the wait time
                break;
            default:
                seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
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

    PDM.onReceive(onPDMdata);
    if (!PDM.begin(1, SAMPLING_FREQUENCY))
    {
        Serial.println("Failed to start PDM!");
        while (1)
            ;
    }

    strip.begin();
    if (strip.getSegments().size() > 1)
    {
        seg = strip.getSegments()[1];
    }
    else
    {
        seg = strip.getSegments()[0];
    }
    seg->begin();
    seg->startEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
}

void loop()
{
    handleSerial();

    if (samplesRead > 0)
    {
        audioTrigger.update(sampleBuffer);
        samplesRead = 0;
    }

    seg->update();
    strip.show();
}