#include "PixelStrip.h"
// The Effects.h header likely includes all your individual effect files.
// The error originates from the code inside these files.
#include "./effects/Effects.h"

PixelStrip::PixelStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness, uint8_t numSections)
    // Initializer now uses NeoPixelBus constructor
    : strip(ledCount, pin)
{
    // The initial brightness is passed to the default "all" segment.
    segments_.push_back(new Segment(*this, 0, ledCount - 1, String("all"), 0));
    segments_[0]->setBrightness(brightness);

    if (numSections > 0)
    {
        uint16_t per = ledCount / numSections;
        for (uint8_t s = 0; s < numSections; ++s)
        {
            uint16_t start = s * per;
            uint16_t end = (s == numSections - 1) ? (ledCount - 1) : (start + per - 1);
            // New segments will inherit the default brightness of 255, which can be set later.
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
    // Uses NeoPixelBus method to initialize the strip
    strip.Begin();
}

void PixelStrip::show()
{
    // This is now a non-blocking call.
    // It checks if the hardware is ready and only sends data if it is.
    if (strip.CanShow()) {
        strip.Show();
    }
}

void PixelStrip::clear()
{
    // Uses NeoPixelBus method to clear all pixels to black
    strip.ClearTo(RgbColor(0, 0, 0));
}

uint32_t PixelStrip::Color(uint8_t r, uint8_t g, uint8_t b) {
    // This helper function remains the same.
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void PixelStrip::setPixel(uint16_t i, uint32_t col) {
    // This is the new brightness handling logic.
    // 1. Deconstruct the 32-bit color into an RgbColor object.
    RgbColor color( (col >> 16) & 0xFF, (col >> 8) & 0xFF, col & 0xFF );
    // 2. Apply the segment's brightness using the Dim() method.
    color.Dim(activeBrightness_);
    // 3. Set the pixel color in the strip's buffer.
    strip.SetPixelColor(i, color);
}

void PixelStrip::clearPixel(uint16_t i) {
    // Uses NeoPixelBus method with a black color object.
    strip.SetPixelColor(i, RgbColor(0));
}

PixelStrip::Segment::Segment(PixelStrip &p, uint16_t s, uint16_t e, const String &n, uint8_t i)
    : parent(p), startIdx(s), endIdx(e), name(n), id(i),
      // Set default brightness to max. Can be changed via setBrightness().
      brightness(255)
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
}

uint8_t PixelStrip::Segment::getBrightness() const { return brightness; }

void PixelStrip::Segment::update()
{
    // Set the parent strip's active brightness to this segment's value.
    // This provides the correct brightness context for all subsequent setPixel() calls.
    parent.setActiveBrightness(brightness);

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
    // Set all pixels in the segment to black.
    RgbColor black(0);
    for (uint16_t i = startIdx; i <= endIdx; ++i) {
        parent.getStrip().SetPixelColor(i, black);
    }
}

void PixelStrip::Segment::setEffect(SegmentEffect effect)
{
    // Turn off all effects
    rainbowActive = false;
    allOnActive = false;
    flashTriggerActive = false;

    clear();

    // Set new active effect
    activeEffect = effect;
}


uint32_t PixelStrip::ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
    // This function is self-contained and does not need changes.
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
