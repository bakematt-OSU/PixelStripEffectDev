#ifndef PIXELSTRIP_H
#define PIXELSTRIP_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <vector>

/**
 * @brief PixelStrip manages an addressable LED strip and its logical segments/effects.
 *
 * Use this class to divide a NeoPixel strip into logical sections and run effects.
 */
class PixelStrip
{
public:
  /**
   * @brief Represents a contiguous run of pixels, used for zone effects.
   *
   * Each Segment can run its own effect independently.
   */
  class Segment
  {
  public:
    /**
     * @brief Construct a Segment.
     * @param parent Reference to parent PixelStrip.
     * @param startIdx First pixel index (inclusive).
     * @param endIdx Last pixel index (inclusive).
     * @param name Name of this segment.
     * @param id Unique segment ID.
     */
    Segment(PixelStrip &parent, uint16_t startIdx, uint16_t endIdx, const String &name, uint8_t id);

    // --- Getters ---
    /**
     * @brief Get the starting pixel index of the segment.
     */
    uint16_t startIndex() const { return startIdx; }

    /**
     * @brief Get the ending pixel index of the segment.
     */
    uint16_t endIndex() const { return endIdx; }



    /**
     * @brief Get the segment name.
     * @return The name as a String.
     */
    String getName() const;

    /**
     * @brief Get the segment's ID.
     * @return ID value.
     */
    uint8_t getId() const;


    // GETTER FOR LAST SOUND INTENSITY
    uint8_t getSoundIntensity() const;

    // --- Lifecycle & helpers ---

    /**
     * @brief Initialize segment (clear, set idle).
     */
    void begin();

    /**
     * @brief Advance the currently running effect one step/frame.
     */
    void update();

    /**
     * @brief True if no effect is running.
     * @return True if idle.
     */
    bool isIdle() const;

    /**
     * @brief Set all pixels in this segment ON to a color.
     * @param color 24-bit RGB color.
     */
    void allOn(uint32_t color);

    /**
     * @brief Set all pixels in this segment OFF.
     */
    void allOff();

    /**
     * @brief Alias for allOff().
     */
    inline void clear() { allOff(); }

    /**
     * @brief Set brightness for this segment.
     * @param b Brightness (0–255).
     */
    void setBrightness(uint8_t b);

    /**
     * @brief Get segment's last-set brightness.
     * @return Brightness value.
     */
    uint8_t getBrightness() const;

    // --- Effects (starters) ---

    /**
     * @brief Start a wipe (chase) effect.
     * @param c Color (RGB packed).
     * @param d Delay (ms) between steps.
     * @param loop If true, effect repeats.
     */
    void wipe(uint32_t c, unsigned long d, bool loop = false);

    /**
     * @brief Start a moving rainbow effect.
     * @param d Delay (ms) between frames.
     * @param loop If true, effect repeats.
     */
    void rainbow(unsigned long d, bool loop = false);

    /**
     * @brief Fill segment with a static rainbow.
     */
    void solidRainbow();

    /**
     * @brief Start a theater chase effect.
     * @param c Color (RGB packed).
     * @param d Delay (ms) between steps.
     * @param spacing Distance between lit pixels.
     * @param loop If true, effect repeats.
     */
    void theaterChase(uint32_t c, unsigned long d, uint8_t spacing, bool loop = false);

    /**
     * @brief Start a jump block effect.
     * @param c Color (RGB packed).
     * @param d Delay (ms) between jumps.
     * @param blockSize Number of pixels in each block.
     * @param loop If true, effect repeats.
     */
    void jump(uint32_t c, unsigned long d, uint8_t blockSize, bool loop = false);

    /**
     * @brief Start a fire (sparkle) effect.
     * @param d Delay (ms) between frames.
     * @param sparks Number of sparks per frame.
     * @param loop If true, effect repeats.
     */
    void fire(unsigned long d, uint16_t sparks, bool loop = false);

    /**
     * @brief Start a colored fire effect with up to 3 colors.
     * @param c1 First color.
     * @param c2 Second color.
     * @param c3 Third color.
     * @param d Delay (ms).
     * @param sparks Number of sparks per frame.
     * @param loop If true, effect repeats.
     */
    void coloredFire(uint32_t c1, uint32_t c2, uint32_t c3,
                     unsigned long d, uint16_t sparks, bool loop = false);

    /**
     * @brief Start a breathing brightness effect.
     * @param c Color.
     * @param d Delay (ms) per step.
     * @param cycles Number of breaths before stopping.
     * @param loop If true, effect repeats.
     */
    void breathe(uint32_t c, unsigned long d, uint8_t cycles, bool loop = false);

    /**
     * @brief Start a flashing effect.
     * @param c Color.
     * @param d Delay (ms).
     * @param cycles Number of flashes before stopping.
     * @param loop If true, effect repeats.
     */
    void flash(uint32_t c, unsigned long d, uint8_t cycles, bool loop = false);

    /**
     * @brief Start a falling star (comet) effect.
     * @param c Color.
     * @param d Delay (ms).
     * @param len Tail length.
     * @param loop If true, effect repeats.
     */
    void fallingStar(uint32_t c, unsigned long d, uint8_t len, bool loop = false);

    /**
     * @brief Start an intensity meter (bar graph) effect.
     * @param pct Intensity percentage.
     * @param cols Array of colors for each meter step.
     * @param cnt Number of colors (steps).
     */
    void intensityMeter(uint8_t pct, const uint32_t *cols, uint8_t cnt);

    /**
     * @brief Start a bouncing pixel with acceleration control.
     * @param x X acceleration.
     * @param y Y acceleration.
     * @param z Z acceleration.
     * @param d Delay (ms) between moves.
     * @param loop If true, effect repeats.
     * @param xColor Pixel color for X.
     * @param yColor Pixel color for Y.
     * @param zColor Pixel color for Z.
     */
    void bounceWithAccel(float x, float y, float z, unsigned long d, bool loop = false, uint32_t xColor = 0, uint32_t yColor = 0, uint32_t zColor = 0);

    /**
     * @brief Start a tilt rainbow effect.
     * @param x Tilt (usually -1.0 to +1.0).
     * @param d Delay (ms).
     * @param loop If true, effect repeats.
     */
    void tiltRainbow(float x, unsigned long d, bool loop = true);

    /**
     * @brief Start a lightning strike animation.
     * @param c Color.
     * @param speed Delay (ms) for bolt growth.
     * @param flashMs Duration of full-segment flash (ms).
     * @param maxDelay Wait before next strike (ms).
     * @param rainbow If true, uses rainbow coloring.
     * @param startB Starting brightness.
     * @param flashB Flash brightness.
     */
    void lightningStrike(uint32_t c, unsigned long speed, unsigned long flashMs, unsigned long maxDelay,
                         bool rainbow = false, uint8_t startB = 50, uint8_t flashB = 255);

    /**
     * @brief Start a sound-reactive lightning strike.
     * @param c Color.
     * @param speed Delay (ms) for bolt growth.
     * @param flashMs Duration of flash (ms).
     * @param maxDelay Wait before next possible trigger (ms).
     * @param rainbow If true, uses rainbow coloring.
     * @param startB Starting brightness.
     * @param flashB Flash brightness.
     * @param threshold Minimum sound level to trigger.
     */
    void soundLightning(uint32_t c, unsigned long speed, unsigned long flashMs, unsigned long maxDelay,
                        bool rainbow = false, uint8_t startB = 50, uint8_t flashB = 255, uint8_t threshold = 20);

    /**
     * @brief Set the current sound intensity (for sound-reactive effects).
     * @param intensity Sound intensity (0-255).
     */
    void setSoundIntensity(uint8_t intensity);

  private:
    // --- Internal state ---

    PixelStrip &parent;        ///< Reference to parent strip
    uint16_t startIdx, endIdx; ///< Start and end pixel indexes
    String name;               ///< Segment name
    uint8_t id;                ///< Segment ID
    uint8_t brightness;        ///< Segment brightness (0-255)

    /// Enumerated effect types.
    enum Effect
    {
      NONE,
      WIPE,
      RAINBOW,
      SOLID_RAINBOW,
      THEATER_CHASE,
      JUMP,
      FIRE,
      COLORED_FIRE,
      BREATHE,
      FLASH,
      FALLING_STAR,
      INTENSITY_METER,
      BOUNCE_ACCEL,
      TILT_RAINBOW,
      LIGHTNING_STRIKE,
      SOUND_LIGHTNING
    } effect; ///< Currently active effect

    // --- Effect configuration and state variables ---
    unsigned long delayMs, lastUpdate;
    uint32_t primaryColor;
    uint32_t stepIndex;
    uint8_t cycles, subState, starLen;
    bool loop;

    // For bounce/tilt
    float ax, ay, az;
    uint32_t axisXColor = 0, axisYColor = 0, axisZColor = 0;

    // For coloredFire
    uint32_t fireC1, fireC2, fireC3;

    // For intensityMeter
    const uint32_t *intColors = nullptr;
    uint8_t intCount, intPercent;
    uint16_t intensityValue = 0;

    // For flash/lightning
    unsigned long flashDuration, delayMax;
    uint8_t startBrightness, flashBrightness;
    bool rainbowMode;

    // For sound lightning
    uint8_t soundThreshold = 20;    ///< Minimum mic peak to trigger (0–255)
    uint8_t lastSoundIntensity = 0; ///< Last measured intensity (0–255)
    bool lightningActive = false;   ///< Is the lightning currently running?

    // Breath
    bool active = true;

    // --- Per-effect step functions (private) ---
    void wipeStep();
    void rainbowStep();
    void solidRainbowStep();
    void theaterChaseStep();
    void jumpStep();
    void fireStep();
    void coloredFireStep();
    void breatheStep();
    void flashStep();
    void fallingStarStep();
    void intensityMeterStep();
    void bounceAccelStep();
    void tiltRainbowStep();
    void lightningStrikeStep();
    void soundLightningStep();
  };

  // --- PixelStrip class (root) ---

  /**
   * @brief Construct a PixelStrip.
   * @param pin Data pin.
   * @param ledCount Number of LEDs.
   * @param brightness Initial brightness (default 50).
   * @param numSections Number of segments/slices (in addition to "all").
   */
  PixelStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness = 50, uint8_t numSections = 0);

  /**
   * @brief Must call in Arduino setup() to initialize.
   */
  void begin();

  /**
   * @brief Push pixel buffer to the physical LEDs.
   */
  void show();

  /**
   * @brief Turn all pixels off.
   */
  void clear();

  /**
   * @brief Create a 24-bit packed RGB color.
   * @param r Red (0-255).
   * @param g Green (0-255).
   * @param b Blue (0-255).
   * @return Packed color.
   */
  uint32_t Color(uint8_t r, uint8_t g, uint8_t b);

  /**
   * @brief Set color of an individual pixel (does not show).
   * @param idx Pixel index.
   * @param color 24-bit RGB value.
   */
  void setPixel(uint16_t idx, uint32_t color);

  /**
   * @brief Turn off an individual pixel (does not show).
   * @param idx Pixel index.
   */
  void clearPixel(uint16_t idx);

  /**
   * @brief Get all defined segments (including "all" and slices).
   * @return Vector of Segment*.
   */
  const std::vector<Segment *> &getSegments() const { return segments_; }

  /**
   * @brief Add a custom section by index range and name.
   * @param start Starting pixel index.
   * @param end Ending pixel index.
   * @param name Name for this section.
   */
  void addSection(uint16_t start, uint16_t end, const String &name);

private:
  Adafruit_NeoPixel strip;          ///< The NeoPixel driver object
  std::vector<Segment *> segments_; ///< All segments ("all", slices, custom)
};

#endif // PIXELSTRIP_H
