#ifndef FLASH_ON_TRIGGER_H
#define FLASH_ON_TRIGGER_H

#include "../PixelStrip.h"

namespace FlashOnTrigger {

inline void start(PixelStrip::Segment* seg, uint32_t baseColor, bool useRainbow, unsigned long flashInterval) {
    seg->setEffect(PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER);
    seg->flashTriggerActive = true;
    seg->flashUseRainbow = useRainbow;
    seg->flashBaseColor = baseColor;
    seg->flashLastUpdate = millis();
    seg->flashInterval = flashInterval;

    if (!useRainbow) {
        for (int i = seg->startIndex(); i <= seg->endIndex(); ++i) {
            // --- CORRECTED LINE ---
            // Set the initial color. Brightness will be applied by setPixel.
            seg->getParent().setPixel(i, baseColor);
        }
        // REMOVED: seg->getParent().show(); - This should be called in your main loop.
    }
}

inline void stop(PixelStrip::Segment* seg) {
    seg->flashTriggerActive = false;
    seg->allOff();
}

inline void update(PixelStrip::Segment* seg, bool trigger, uint8_t brightness) {
    if (!seg->flashTriggerActive) return;

    if (trigger) {
        seg->setBrightness(brightness);  // Apply new brightness for this frame.

        for (int i = seg->startIndex(); i <= seg->endIndex(); ++i) {
            // Determine the color, either from the rainbow or the base color.
            uint32_t color = seg->flashUseRainbow
                ? seg->getParent().ColorHSV((i - seg->startIndex()) * 65536UL / (seg->endIndex() - seg->startIndex() + 1))
                : seg->flashBaseColor;

            // --- CORRECTED LINE ---
            // Set the pixel color. Brightness is handled automatically.
            seg->getParent().setPixel(i, color);
        }

        // REMOVED: seg->getParent().show(); - This should be called in your main loop.
    } else {
        // If not triggered, turn the segment off.
        seg->allOff();
    }
}

}

#endif
