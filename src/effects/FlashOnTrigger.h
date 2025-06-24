#ifndef FLASH_ON_TRIGGER_H
#define FLASH_ON_TRIGGER_H

#include "../PixelStrip.h" // Include the main PixelStrip header to get Segment info

namespace FlashOnTrigger {

    inline void start(PixelStrip::Segment* seg, uint32_t baseColor, bool useRainbow, unsigned long interval) {
        if (!seg) return;
        
        seg->setEffect(PixelStrip::Segment::SegmentEffect::FLASH_TRIGGER);
        seg->flashTriggerActive = true;
        seg->flashBaseColor = baseColor;
        seg->flashUseRainbow = useRainbow;
        seg->flashInterval = interval;
    }

    /**
     * @brief (CORRECTED) Updates the effect on each loop iteration.
     * @param seg The segment being updated.
     * @param isActive The current state of the trigger (e.g., true if a beat is detected).
     * @param triggerBrightness The brightness value (0-255) from the trigger.
     */
    inline void update(PixelStrip::Segment* seg, bool isActive, uint8_t triggerBrightness) {
        if (!seg || !seg->flashTriggerActive) {
            return;
        }

        PixelStrip& strip = seg->getParent();

        if (isActive) {
            uint32_t baseColor;

            if (seg->flashUseRainbow) {
                // Get a full-brightness rainbow color. Brightness will be applied next.
                uint16_t hue = (millis() * 25) % 65536;
                baseColor = strip.ColorHSV(hue, 255, 255);
            } else {
                // Use the configured base color for the segment.
                baseColor = seg->flashBaseColor;
            }

            // --- FIX for the "White Color" Bug ---
            // Manually deconstruct the base color into its R, G, B components.
            uint8_t r = (baseColor >> 16) & 0xFF;
            uint8_t g = (baseColor >> 8) & 0xFF;
            uint8_t b = baseColor & 0xFF;
            
            // Create the RgbColor object from the individual components for reliability.
            RgbColor finalColor(r, g, b);
            
            // --- FIX for the "Double Dimming" Bug ---
            // Apply the dynamic brightness from the audio trigger.
            finalColor.Dim(triggerBrightness);

            // Set the color directly on the underlying NeoPixelBus object.
            // This bypasses the segment's master brightness, which is what we want
            // so that the flash brightness is controlled ONLY by the audio trigger.
            for (uint16_t i = seg->startIndex(); i <= seg->endIndex(); ++i) {
                strip.getStrip().SetPixelColor(i, finalColor);
            }

        } else {
            // If the trigger is not active, turn the segment's pixels off directly.
            seg->allOff();
        }
    }

} // namespace FlashOnTrigger

#endif // FLASH_ON_TRIGGER_H