#include "PixelStrip.h"
#include "./effects/Effects.h"

PixelStrip::PixelStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness, uint8_t numSections)
    : strip(ledCount, pin, NEO_GRB + NEO_KHZ800)
{
    strip.setBrightness(brightness);
    segments_.push_back(new Segment(*this, 0, ledCount - 1, String("all"), 0));

    if (numSections > 0) {
        uint16_t per = ledCount / numSections;
        for (uint8_t s = 0; s < numSections; ++s) {
            uint16_t start = s * per;
            uint16_t end = (s == numSections - 1) ? (ledCount - 1) : (start + per - 1);
            segments_.push_back(new Segment(*this, start, end, String("seg") + String(s + 1), s + 1));
        }
    }
}

void PixelStrip::addSection(uint16_t start, uint16_t end, const String &name) {
    uint8_t newId = segments_.size();
    segments_.push_back(new Segment(*this, start, end, name, newId));
}

void PixelStrip::begin() {
    strip.begin();
    strip.show();
}
void PixelStrip::show() { strip.show(); }
void PixelStrip::clear() {
    strip.clear();
    strip.show();
}
uint32_t PixelStrip::Color(uint8_t r, uint8_t g, uint8_t b) { return strip.Color(r, g, b); }
void PixelStrip::setPixel(uint16_t i, uint32_t col) { strip.setPixelColor(i, col); }
void PixelStrip::clearPixel(uint16_t i) { strip.setPixelColor(i, 0); }

PixelStrip::Segment::Segment(PixelStrip &p, uint16_t s, uint16_t e, const String &n, uint8_t i)
    : parent(p), startIdx(s), endIdx(e), name(n), id(i),
      brightness(p.strip.getBrightness())
{}

String PixelStrip::Segment::getName() const { return name; }
uint8_t PixelStrip::Segment::getId() const { return id; }

void PixelStrip::Segment::begin() {
    clear();
}

void PixelStrip::Segment::setBrightness(uint8_t b) {
    brightness = b;
    parent.strip.setBrightness(b);
}
uint8_t PixelStrip::Segment::getBrightness() const { return brightness; }

void PixelStrip::Segment::update() {
    if (rainbowActive) {
        RainbowChase::update(this);
    }
    if (allOnActive) {
        SolidColor::update(this);
    }
}

void PixelStrip::Segment::allOff() {
    for (uint16_t i = startIdx; i <= endIdx; ++i)
        parent.strip.setPixelColor(i, 0);
    parent.strip.show();
}
