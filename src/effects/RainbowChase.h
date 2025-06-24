#ifndef RAINBOWCHASE_H
#define RAINBOWCHASE_H

#include "../PixelStrip.h"

namespace RainbowChase {

inline void start(PixelStrip::Segment* seg, unsigned long frameDelay, uint8_t brightness) {
    seg->setEffect(PixelStrip::Segment::SegmentEffect::RAINBOW);
    seg->rainbowDelay = frameDelay;
    seg->rainbowLastUpdate = millis();
    seg->rainbowFirstPixelHue = 0;
    seg->rainbowActive = true;
    seg->setBrightness(brightness);
}

inline void update(PixelStrip::Segment* seg) {
    if (!seg->rainbowActive) return;

    unsigned long now = millis();
    if (now - seg->rainbowLastUpdate < seg->rainbowDelay) return;

    seg->rainbowLastUpdate = now;

    for (int i = seg->startIndex(); i <= seg->endIndex(); ++i) {
        // Calculate the hue for each pixel to create the rainbow effect.
        int pixelHue = seg->rainbowFirstPixelHue + ((i - seg->startIndex()) * 65536L / (seg->endIndex() - seg->startIndex() + 1));
        
        // Convert the HSV color to a 32-bit RGB color.
        uint32_t color = seg->getParent().ColorHSV(pixelHue);
        
        // --- CORRECTED LINE ---
        // Set the pixel. The parent's setPixel will handle brightness automatically.
        seg->getParent().setPixel(i, color);
    }

    // REMOVED: seg->getParent().show(); - This should be called in your main loop.

    // Advance the starting hue for the next frame's animation.
    seg->rainbowFirstPixelHue += 256;
    if (seg->rainbowFirstPixelHue >= 5 * 65536UL)
        seg->rainbowFirstPixelHue = 0;
}

inline void stop(PixelStrip::Segment* seg) {
    seg->rainbowActive = false;
    seg->clear();
}

}

#endif
