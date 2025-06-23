#ifndef RAINBOWCHASE_H
#define RAINBOWCHASE_H

#include "../PixelStrip.h"

namespace RainbowChase {

inline void start(PixelStrip::Segment* seg, unsigned long frameDelay, uint8_t brightness) {
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
        int pixelHue = seg->rainbowFirstPixelHue + ((i - seg->startIndex()) * 65536L / (seg->endIndex() - seg->startIndex() + 1));
        seg->getParent().setPixel(i, seg->getParent().getStrip().ColorHSV(pixelHue));
    }
    seg->getParent().show();

    seg->rainbowFirstPixelHue += 256;
    if (seg->rainbowFirstPixelHue >= 5 * 65536UL)
        seg->rainbowFirstPixelHue = 0;
}

inline void stop(PixelStrip::Segment* seg) {
    seg->rainbowActive = false;
}

}

#endif
