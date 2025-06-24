#ifndef PIXELSTRIP_H
#define PIXELSTRIP_H

#include <Arduino.h>
#include <NeoPixelBus.h> 
#include <vector>

using PixelBus = NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod>;

class PixelStrip
{
public:
    class Segment
    {
    public:
        enum class SegmentEffect
        {
            NONE,
            RAINBOW,
            SOLID,
            FLASH_TRIGGER
        };

        Segment(PixelStrip &parent, uint16_t startIdx, uint16_t endIdx, const String &name, uint8_t id);

        uint16_t startIndex() const { return startIdx; }
        uint16_t endIndex() const { return endIdx; }
        String getName() const;
        uint8_t getId() const;
        SegmentEffect activeEffect = SegmentEffect::NONE;

        void begin();
        void update();
        void allOff();
        inline void clear() { allOff(); }
        
        // OLD METHOD (we will still use it internally)
        void setEffect(SegmentEffect effect);
        
        // ADDED: New primary method for starting effects
        void startEffect(SegmentEffect effect, uint32_t color1 = 0, uint32_t color2 = 0);

        void setBrightness(uint8_t b);
        uint8_t getBrightness() const;
        
        PixelStrip &getParent() { return parent; }

        // Effect-specific state variables
        unsigned long rainbowDelay, rainbowLastUpdate, rainbowFirstPixelHue;
        bool rainbowActive = false;
        bool allOnActive = false;
        uint32_t allOnColor = 0;
        bool flashTriggerActive = false;
        bool flashUseRainbow = false;
        bool flashToggle = false;
        uint32_t flashBaseColor = 0;
        unsigned long flashInterval, flashLastUpdate;

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

    void setActiveBrightness(uint8_t b) { activeBrightness_ = b; }
    const std::vector<Segment *> &getSegments() const { return segments_; }
    PixelBus &getStrip() { return strip; }
    void addSection(uint16_t start, uint16_t end, const String &name);

private:
    PixelBus strip;
    std::vector<Segment *> segments_;
    uint8_t activeBrightness_ = 128;
};

#endif // PIXELSTRIP_H