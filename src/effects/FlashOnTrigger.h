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
        for (int i = seg->startIndex(); i <= seg->endIndex(); ++i)
            seg->getParent().setPixel(i, baseColor);
        seg->getParent().show();
    }
}


inline void stop(PixelStrip::Segment* seg) {
    seg->flashTriggerActive = false;
    seg->allOff();
}

inline void update(PixelStrip::Segment* seg, bool trigger, uint8_t brightness)
{
    if (!seg->flashTriggerActive) return;

    if (trigger)
    {
        seg->setBrightness(brightness);  // 🔸 Set new brightness

        for (int i = seg->startIndex(); i <= seg->endIndex(); ++i)
        {
            uint32_t color = seg->flashUseRainbow
                ? seg->getParent().ColorHSV((i - seg->startIndex()) * 65536UL / (seg->endIndex() - seg->startIndex() + 1))
                : seg->flashBaseColor;
            seg->getParent().setPixel(i, color);
        }

        seg->getParent().show();
    }
    else
    {
        seg->allOff();
    }
}

}

#endif
