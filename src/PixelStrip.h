#ifndef PIXELSTRIP_H
#define PIXELSTRIP_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <vector>

class PixelStrip
{
public:
    class Segment
    {
    public:
        // EFFECT ENUM TYPE FOR TRACKING ACTIVE EFFECT
        enum class SegmentEffect
        {
            NONE,
            RAINBOW,
            SOLID,
            FLASH_TRIGGER
        };
        // CONSTRUCTOR
        Segment(PixelStrip &parent, uint16_t startIdx, uint16_t endIdx, const String &name, uint8_t id);

        // VARIABLES
        uint16_t startIndex() const { return startIdx; }
        uint16_t endIndex() const { return endIdx; }
        String getName() const;
        uint8_t getId() const;
        SegmentEffect activeEffect = SegmentEffect::NONE;

        // METHODS
        void begin();
        void update();
        void allOn(uint32_t color);
        void allOff();
        inline void clear() { allOff(); }
        void setEffect(SegmentEffect effect);
        void setBrightness(uint8_t b);
        uint8_t getBrightness() const;

        


        // Access parent PixelStrip
        PixelStrip &getParent() { return parent; }

        // RainbowChase state
        unsigned long rainbowDelay = 0;
        unsigned long rainbowLastUpdate = 0;
        unsigned long rainbowFirstPixelHue = 0;
        bool rainbowActive = false;

        // SolidColorEffect state
        bool allOnActive = false;
        uint32_t allOnColor = 0;

        // FlashOnTrigger state
        bool flashTriggerActive = false;
        bool flashUseRainbow = false;
        bool flashToggle = false;
        uint32_t flashBaseColor = 0;
        unsigned long flashInterval = 100;
        unsigned long flashLastUpdate = 0;

    private:
        PixelStrip &parent;
        uint16_t startIdx, endIdx;
        String name;
        uint8_t id;
        uint8_t brightness;
    };

    PixelStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness = 50, uint8_t numSections = 0);

    void begin();
    void show();
    void clear();

    uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
    uint32_t ColorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255);
    void setPixel(uint16_t idx, uint32_t color);
    void clearPixel(uint16_t idx);

    const std::vector<Segment *> &getSegments() const { return segments_; }
    Adafruit_NeoPixel &getStrip() { return strip; }
    void addSection(uint16_t start, uint16_t end, const String &name);

private:
    Adafruit_NeoPixel strip;
    std::vector<Segment *> segments_;
};

#endif // PIXELSTRIP_H
