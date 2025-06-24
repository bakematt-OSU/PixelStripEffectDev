#include "PixelStrip.h"
#include "./effects/Effects.h"

PixelStrip::PixelStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness, uint8_t numSections)
    : strip(ledCount, pin, NEO_GRB + NEO_KHZ800)
{
    strip.setBrightness(brightness);
    segments_.push_back(new Segment(*this, 0, ledCount - 1, String("all"), 0));

    if (numSections > 0)
    {
        uint16_t per = ledCount / numSections;
        for (uint8_t s = 0; s < numSections; ++s)
        {
            uint16_t start = s * per;
            uint16_t end = (s == numSections - 1) ? (ledCount - 1) : (start + per - 1);
            segments_.push_back(new Segment(*this, start, end, String("seg") + String(s + 1), s + 1));
        }
    }
}

void PixelStrip::addSection(uint16_t start, uint16_t end, const String &name)
{
    uint8_t newId = segments_.size();
    segments_.push_back(new Segment(*this, start, end, name, newId));
}

void PixelStrip::begin()
{
    strip.begin();
    // strip.show();
}
void PixelStrip::show() { /*strip.show()*/; }
void PixelStrip::clear()
{
    strip.clear();
    // strip.show();
}
uint32_t PixelStrip::Color(uint8_t r, uint8_t g, uint8_t b) { return strip.Color(r, g, b); }
void PixelStrip::setPixel(uint16_t i, uint32_t col) { strip.setPixelColor(i, col); }
void PixelStrip::clearPixel(uint16_t i) { strip.setPixelColor(i, 0); }

PixelStrip::Segment::Segment(PixelStrip &p, uint16_t s, uint16_t e, const String &n, uint8_t i)
    : parent(p), startIdx(s), endIdx(e), name(n), id(i),
      brightness(p.strip.getBrightness())
{
}

String PixelStrip::Segment::getName() const { return name; }
uint8_t PixelStrip::Segment::getId() const { return id; }

void PixelStrip::Segment::begin()
{
    clear();
}

void PixelStrip::Segment::setBrightness(uint8_t b)
{
    brightness = b;
    parent.strip.setBrightness(b);
}
uint8_t PixelStrip::Segment::getBrightness() const { return brightness; }

void PixelStrip::Segment::update()
{
    switch (activeEffect)
    {
    case SegmentEffect::RAINBOW:
        RainbowChase::update(this);
        break;

    case SegmentEffect::SOLID:
        SolidColor::update(this);
        break;

    case SegmentEffect::FLASH_TRIGGER:
        extern bool flashTriggerState;
        extern uint8_t flashTriggerBrightness;
        FlashOnTrigger::update(this, flashTriggerState, flashTriggerBrightness);
        break;

    case SegmentEffect::NONE:
    default:
        break;
    }
}

void PixelStrip::Segment::allOff()
{
    for (uint16_t i = startIdx; i <= endIdx; ++i)
        parent.strip.setPixelColor(i, 0);
    // parent.strip.show();
}

void PixelStrip::Segment::setEffect(SegmentEffect effect)
{
    // 🔸 Turn off all effects
    rainbowActive = false;
    allOnActive = false;
    flashTriggerActive = false;

    // Optionally clear the LEDs (remove this if you want to retain visuals during switch)
    clear();

    // 🔸 Set new active effect
    activeEffect = effect;
}


uint32_t PixelStrip::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
    uint8_t r, g, b;

    uint8_t region = hue / 10923; // 65536 / 6
    uint16_t remainder = (hue - (region * 10923)) * 6;

    uint8_t p = (val * (255 - sat)) >> 8;
    uint8_t q = (val * (255 - ((sat * remainder) >> 16))) >> 8;
    uint8_t t = (val * (255 - ((sat * (65535 - remainder)) >> 16))) >> 8;

    switch (region) {
        case 0: r = val; g = t; b = p; break;
        case 1: r = q; g = val; b = p; break;
        case 2: r = p; g = val; b = t; break;
        case 3: r = p; g = q; b = val; break;
        case 4: r = t; g = p; b = val; break;
        default: r = val; g = p; b = q; break;
    }

    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
