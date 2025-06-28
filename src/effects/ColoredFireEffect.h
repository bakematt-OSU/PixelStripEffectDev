#ifndef COLOREDFIREEFFECT_H
#define COLOREDFIREEFFECT_H

#include "../PixelStrip.h"
#include <Arduino.h> // Required for the random() function

namespace ColoredFireEffect {

// --- Effect-specific constants ---
// NOTE: This static array is shared among effects that use it.
const int MAX_LEDS_FIRE = 300; 
static byte heat[MAX_LEDS_FIRE];

// --- Helper Functions ---

inline byte qadd8(byte a, byte b) {
    unsigned int sum = a + b;
    if (sum > 255) return 255;
    return static_cast<byte>(sum);
}

inline byte qsub8(byte a, byte b) {
    if (b > a) return 0;
    return a - b;
}

// Helper for linear interpolation of a single byte
inline byte lerp8(byte a, byte b, byte t) {
    return a + ((long)(b - a) * t) / 255;
}

/**
 * @brief Creates a color from a 3-color gradient based on heat.
 * @param heat The heat value (0-255).
 * @param c1 The coolest color (for heat near 0).
 * @param c2 The middle color (for heat near 128).
 * @param c3 The hottest color (for heat near 255).
 * @return RgbColor The interpolated color.
 */
inline RgbColor ThreeColorHeatColor(byte heat, RgbColor c1, RgbColor c2, RgbColor c3) {
    if (heat <= 127) {
        // Interpolate between color 1 and color 2
        byte t = heat * 2; // Scale 0-127 to 0-254
        return RgbColor(lerp8(c1.R, c2.R, t), lerp8(c1.G, c2.G, t), lerp8(c1.B, c2.B, t));
    } else {
        // Interpolate between color 2 and color 3
        byte t = (heat - 128) * 2; // Scale 128-255 to 0-254
        return RgbColor(lerp8(c2.R, c3.R, t), lerp8(c2.G, c3.G, t), lerp8(c2.B, c3.B, t));
    }
}


// --- Main Effect Functions (Implemented Inline) ---

/**
 * @brief Initializes the Colored Fire effect on a segment.
 * @param seg Pointer to the segment to run the effect on.
 * @param color1 Unused.
 * @param color2 Unused.
 */
inline void start(PixelStrip::Segment* seg, uint32_t color1, uint32_t color2) {
    seg->setEffect(PixelStrip::Segment::SegmentEffect::COLORED_FIRE);
    seg->active = true;
    seg->interval = 15;
}

/**
 * @brief Updates the animation frame for the Colored Fire effect.
 * @param seg Pointer to the segment being updated.
 */
inline void update(PixelStrip::Segment* seg) {
    if (!seg->active || (millis() - seg->lastUpdate < seg->interval)) {
        return;
    }
    seg->lastUpdate = millis();

    int start = seg->startIndex();
    int end = seg->endIndex();
    int len = (end - start + 1);

    // Step 1. Cool down every cell
    for (int i = start; i <= end; i++) {
        heat[i] = qsub8(heat[i], random(0, ((seg->fireCooling * 10) / len) + 2));
    }
  
    // Step 2. Heat drifts 'up' and diffuses
    for (int k = end; k >= start + 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }
    
    // Step 3. Randomly ignite new 'sparks' near the bottom
    if (random(255) < seg->fireSparking) {
        int y = start + random(7);
        heat[y] = qadd8(heat[y], random(160, 255));
    }

    // Step 4. Map from heat to our new 3-color gradient
    RgbColor c1((seg->fireColor1 >> 16) & 0xFF, (seg->fireColor1 >> 8) & 0xFF, seg->fireColor1 & 0xFF);
    RgbColor c2((seg->fireColor2 >> 16) & 0xFF, (seg->fireColor2 >> 8) & 0xFF, seg->fireColor2 & 0xFF);
    RgbColor c3((seg->fireColor3 >> 16) & 0xFF, (seg->fireColor3 >> 8) & 0xFF, seg->fireColor3 & 0xFF);
    for (int j = start; j <= end; j++) {
        RgbColor color = ThreeColorHeatColor(heat[j], c1, c2, c3);
        seg->getParent().getStrip().SetPixelColor(j, color);
    }
}

} // namespace ColoredFireEffect

#endif // COLOREDFIREEFFECT_H