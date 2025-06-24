#include "PixelStrip.h"
#include "./effects/Effects.h" // Includes all effect start/update headers

PixelStrip::PixelStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness, uint8_t numSections)
    : strip(ledCount, pin)
{
    segments_.push_back(new Segment(*this, 0, ledCount - 1, String("all"), 0));
    segments_[0]->setBrightness(brightness);

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

void PixelStrip::begin() { strip.Begin(); }
void PixelStrip::show() { if (strip.CanShow()) { strip.Show(); } }
void PixelStrip::clear() { strip.ClearTo(RgbColor(0)); }

uint32_t PixelStrip::Color(uint8_t r, uint8_t g, uint8_t b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; }

void PixelStrip::setPixel(uint16_t i, uint32_t col) {
    RgbColor color((col >> 16) & 0xFF, (col >> 8) & 0xFF, col & 0xFF);
    color.Dim(activeBrightness_);
    strip.SetPixelColor(i, color);
}

void PixelStrip::clearPixel(uint16_t i) { strip.SetPixelColor(i, RgbColor(0)); }

PixelStrip::Segment::Segment(PixelStrip &p, uint16_t s, uint16_t e, const String &n, uint8_t i)
    : parent(p), startIdx(s), endIdx(e), name(n), id(i), brightness(255) {}

String PixelStrip::Segment::getName() const { return name; }
uint8_t PixelStrip::Segment::getId() const { return id; }
void PixelStrip::Segment::begin() { clear(); }
void PixelStrip::Segment::setBrightness(uint8_t b) { brightness = b; }
uint8_t PixelStrip::Segment::getBrightness() const { return brightness; }

void PixelStrip::Segment::update()
{
    parent.setActiveBrightness(brightness);
    switch (activeEffect)
    {
    case SegmentEffect::RAINBOW: RainbowChase::update(this); break;
    case SegmentEffect::SOLID: SolidColor::update(this); break;
    case SegmentEffect::FLASH_TRIGGER:
        extern bool flashTriggerState;
        extern uint8_t flashTriggerBrightness;
        FlashOnTrigger::update(this, flashTriggerState, flashTriggerBrightness);
        break;
    case SegmentEffect::NONE: default: break;
    }
}

void PixelStrip::Segment::allOff()
{
    for (uint16_t i = startIdx; i <= endIdx; ++i) {
        parent.getStrip().SetPixelColor(i, RgbColor(0));
    }
}

void PixelStrip::Segment::setEffect(SegmentEffect effect)
{
    rainbowActive = false;
    allOnActive = false;
    flashTriggerActive = false;
    clear();
    activeEffect = effect;
}

// ADDED: Implementation for the new primary effect-starting method
void PixelStrip::Segment::startEffect(SegmentEffect effect, uint32_t color1, uint32_t color2)
{
    // This sets the activeEffect variable and clears the old state
    setEffect(effect);

    switch (effect)
    {
    case SegmentEffect::RAINBOW:
        RainbowChase::start(this, 30, 50);
        break;
    case SegmentEffect::SOLID:
        SolidColor::start(this, color1 != 0 ? color1 : parent.Color(0, 255, 0), 50);
        break;
    case SegmentEffect::FLASH_TRIGGER:
        // Note: The parent PixelStrip class provides the `Color` helper method.
        FlashOnTrigger::start(this, color1 != 0 ? color1 : parent.Color(0, 0, 255), false, 100);
        break;
    case SegmentEffect::NONE:
    default:
        clear();
        break;
    }
}


uint32_t PixelStrip::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
    uint8_t r, g, b;
    uint8_t region = hue / 10923;
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