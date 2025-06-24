#ifndef FIRE_H
#define FIRE_H

#include "../PixelStrip.h"

namespace Fire {

// --- Effect-specific constants you can tweak ---
const int COOLING = 55;   // How fast the fire cools down. Less cooling = taller flames. (0-100)
const int SPARKING = 120; // Chance of new sparks. Higher value = more intense fire. (0-255)
const int MAX_LEDS = 300; // Must be at least as large as your LED_COUNT

// Internal state array for the heat of each pixel
static byte heat[MAX_LEDS];

// This helper function maps a heat temperature (0-255) to a fire-like color
RgbColor HeatColor(byte temperature) {
    byte t192 = round((temperature / 255.0) * 191);
    byte heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2; // scale to 0..252
    if( t192 > 0x80) { // 128
        return RgbColor(255, 255, heatramp);
    } else if( t192 > 0x40 ) { // 64
        return RgbColor(255, heatramp, 0);
    } else { // 0..63
        return RgbColor(heatramp, 0, 0);
    }
}

inline void start(PixelStrip::Segment* seg, uint32_t color1, uint32_t color2) {
    seg->setEffect(PixelStrip::Segment::SegmentEffect::FIRE);
    seg->active = true;
    seg->interval = (color1 > 0) ? color1 : 15; // Default to 15ms delay
    seg->lastUpdate = millis();
}

inline void update(PixelStrip::Segment* seg) {
    if (!seg->active) return;

    if (millis() - seg->lastUpdate < seg->interval) return;
    seg->lastUpdate = millis();

    int start = seg->startIndex();
    int end = seg->endIndex();
    int len = (end - start + 1);

    // Step 1. Cool down every cell a little
    for (int i = start; i <= end; i++) {
      heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / len) + 2));
    }
  
    // Step 2. Heat from each cell drifts 'up' and diffuses a little
    for (int k = end; k >= start + 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }
    
    // Step 3. Randomly ignite new 'sparks' of heat at the bottom
    if (random8() < SPARKING) {
      int y = start + random8(7);
      heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // Step 4. Map from heat cells to LED colors
    for (int j = start; j <= end; j++) {
      RgbColor color = HeatColor(heat[j]);
      seg->getParent().getStrip().SetPixelColor(j, color);
    }
}

}

#endif