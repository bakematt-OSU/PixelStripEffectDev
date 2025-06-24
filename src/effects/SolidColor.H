#ifndef SOLIDCOLOR_H
#define SOLIDCOLOR_H

#include "../PixelStrip.h"

namespace SolidColor {

inline void start(PixelStrip::Segment* seg, uint32_t color, uint8_t brightness = 255) {
    seg->setEffect(PixelStrip::Segment::SegmentEffect::SOLID);
    seg->setBrightness(brightness);
    seg->allOnColor = color;
    seg->allOnActive = true;
}

inline void update(PixelStrip::Segment* seg) {
    if (!seg->allOnActive) return;

    // Loop through each pixel in the segment.
    for (uint16_t i = seg->startIndex(); i <= seg->endIndex(); ++i) {
        // --- CORRECTED LINE ---
        // Set the pixel to the segment's stored color. Brightness is handled automatically.
        seg->getParent().setPixel(i, seg->allOnColor);
    }

    // REMOVED: seg->getParent().show(); - This should be called in your main loop.
}

inline void stop(PixelStrip::Segment* seg) {
    seg->allOnActive = false;
    seg->clear();
}

}

#endif
