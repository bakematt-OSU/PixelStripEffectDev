#include "PixelStrip.h"
// #include "Sensors.h" // For mic input

// ============================================================
//                    PixelStrip CLASS
// ============================================================

/**
 * @brief PixelStrip constructor. Sets up the strip and optional sections.
 */
PixelStrip::PixelStrip(uint8_t pin, uint16_t ledCount, uint8_t brightness, uint8_t numSections)
    : strip(ledCount, pin, NEO_GRB + NEO_KHZ800)
{
  strip.setBrightness(brightness);

  // Add "all" segment (whole strip)
  segments_.push_back(new Segment(*this, 0, ledCount - 1, String("all"), 0));

  // Optional: subdivide into sections
  if (numSections > 0)
  {
    uint16_t per = ledCount / numSections;
    for (uint8_t s = 0; s < numSections; ++s)
    {
      uint16_t start = s * per;
      uint16_t end = (s == numSections - 1) ? (ledCount - 1) : (start + per - 1);
      segments_.push_back(new Segment(*this, start, end, String("seg") + String(s + 1), s + 1));
    }
  }
}

/**
 * @brief Add a custom section (segment) by index range.
 */
void PixelStrip::addSection(uint16_t start, uint16_t end, const String &name)
{
  uint8_t newId = segments_.size();
  segments_.push_back(new Segment(*this, start, end, name, newId));
}

// --- Basic strip controls ---
void PixelStrip::begin()
{
  strip.begin();
  strip.show();
}
void PixelStrip::show() { strip.show(); }
void PixelStrip::clear()
{
  strip.clear();
  strip.show();
}
uint32_t PixelStrip::Color(uint8_t r, uint8_t g, uint8_t b) { return strip.Color(r, g, b); }
void PixelStrip::setPixel(uint16_t i, uint32_t col) { strip.setPixelColor(i, col); }
void PixelStrip::clearPixel(uint16_t i) { strip.setPixelColor(i, 0); }

// ============================================================
//                    Segment CLASS
// ============================================================

/**
 * @brief Segment constructor. Holds state and config for one LED zone.
 */
PixelStrip::Segment::Segment(PixelStrip &p, uint16_t s, uint16_t e, const String &n, uint8_t i)
    : parent(p), startIdx(s), endIdx(e), name(n), id(i),
      brightness(p.strip.getBrightness()), effect(NONE), delayMs(0), lastUpdate(0),
      primaryColor(0), stepIndex(0), cycles(0), subState(0), starLen(0), loop(false),
      ax(0), ay(0), az(0), fireC1(0), fireC2(0), fireC3(0),
      intColors(nullptr), intCount(0), intPercent(0), flashDuration(0), delayMax(0),
      startBrightness(brightness), flashBrightness(brightness), rainbowMode(false),
      soundThreshold(0), lastSoundIntensity(0), lightningActive(false)
{
}

String PixelStrip::Segment::getName() const { return name; }
uint8_t PixelStrip::Segment::getId() const { return id; }
void PixelStrip::Segment::begin()
{
  effect = NONE;
  clear();
}
bool PixelStrip::Segment::isIdle() const { return effect == NONE; }
void PixelStrip::Segment::setBrightness(uint8_t b)
{
  brightness = b;
  parent.strip.setBrightness(b);
}
uint8_t PixelStrip::Segment::getBrightness() const { return brightness; }

// ============================================================
//            EFFECT STARTERS (public triggers)
// ============================================================

void PixelStrip::Segment::wipe(uint32_t c, unsigned long d, bool l)
{
  effect = WIPE;
  primaryColor = c;
  delayMs = d;
  loop = l;
  stepIndex = startIdx;
  lastUpdate = millis();
}

void PixelStrip::Segment::rainbow(unsigned long d, bool l)
{
  effect = RAINBOW;
  delayMs = d;
  loop = l;
  stepIndex = 0;
  lastUpdate = millis();
}

void PixelStrip::Segment::solidRainbow()
{
  effect = SOLID_RAINBOW;
  stepIndex = startIdx;
  lastUpdate = millis();
}

void PixelStrip::Segment::theaterChase(uint32_t c, unsigned long d, uint8_t spacing, bool l)
{
  effect = THEATER_CHASE;
  primaryColor = c;
  delayMs = d;
  cycles = spacing;
  loop = l;
  stepIndex = startIdx;
  lastUpdate = millis();
}

void PixelStrip::Segment::jump(uint32_t c, unsigned long d, uint8_t blockSize, bool l)
{
  effect = JUMP;
  primaryColor = c;
  delayMs = d;
  cycles = blockSize;
  loop = l;
  stepIndex = startIdx;
  lastUpdate = millis();
}

void PixelStrip::Segment::fire(unsigned long d, uint16_t sparks, bool l)
{
  effect = FIRE;
  delayMs = d;
  cycles = sparks;
  loop = l;
  lastUpdate = millis();
}

void PixelStrip::Segment::coloredFire(uint32_t c1, uint32_t c2, uint32_t c3, unsigned long d, uint16_t sparks, bool l)
{
  effect = COLORED_FIRE;
  fireC1 = c1;
  fireC2 = c2;
  fireC3 = c3;
  delayMs = d;
  cycles = sparks;
  loop = l;
  lastUpdate = millis();
}

void PixelStrip::Segment::breathe(uint32_t c, unsigned long d, uint8_t cyc, bool l)
{
  effect = BREATHE;
  primaryColor = c;
  delayMs = d;
  cycles = cyc;
  loop = l;
  stepIndex = startIdx;
  subState = 0;
  lastUpdate = millis();
}

void PixelStrip::Segment::flash(uint32_t c, unsigned long d, uint8_t cyc, bool l)
{
  effect = FLASH;
  primaryColor = c;
  delayMs = d;
  cycles = cyc;
  loop = l;
  subState = 0;
  lastUpdate = millis();
}

void PixelStrip::Segment::fallingStar(uint32_t c, unsigned long d, uint8_t len, bool l)
{
  effect = FALLING_STAR;
  primaryColor = c;
  delayMs = d;
  starLen = len;
  loop = l;
  stepIndex = startIdx;
  lastUpdate = millis();
}

void PixelStrip::Segment::intensityMeter(uint8_t pct, const uint32_t *cols, uint8_t cnt)
{
  effect = INTENSITY_METER;
  intPercent = pct;
  intColors = cols;
  intCount = cnt;
  lastUpdate = millis();
}

void PixelStrip::Segment::bounceWithAccel(float x, float y, float z, unsigned long d, bool l, uint32_t xColor, uint32_t yColor, uint32_t zColor)
{
  effect = BOUNCE_ACCEL;
  ax = x;
  ay = y;
  az = z;
  delayMs = d;
  loop = l;
  axisXColor = xColor;
  axisYColor = yColor;
  axisZColor = zColor;
  lastUpdate = millis();
}

void PixelStrip::Segment::tiltRainbow(float x, unsigned long d, bool l)
{
  effect = TILT_RAINBOW;
  ax = x;
  delayMs = d;
  loop = l;
  stepIndex = 0;
  lastUpdate = millis();
}

void PixelStrip::Segment::lightningStrike(uint32_t c, unsigned long speed, unsigned long flashMs, unsigned long maxDelay, bool rainbow, uint8_t startB, uint8_t flashB)
{
  effect = LIGHTNING_STRIKE;
  primaryColor = c;
  delayMs = speed;
  flashDuration = flashMs;
  delayMax = maxDelay;
  rainbowMode = rainbow;
  startBrightness = startB;
  flashBrightness = flashB;
  subState = 0;
  stepIndex = startIdx;
  lastUpdate = millis();
}

void PixelStrip::Segment::soundLightning(uint32_t c, unsigned long speed, unsigned long flashMs, unsigned long maxDelay, bool rainbow, uint8_t startB, uint8_t flashB, uint8_t threshold)
{
  effect = SOUND_LIGHTNING;
  primaryColor = c;
  delayMs = speed;
  flashDuration = flashMs;
  delayMax = maxDelay;
  rainbowMode = rainbow;
  startBrightness = startB;
  flashBrightness = flashB;
  soundThreshold = threshold;
  lastSoundIntensity = 0;
  lightningActive = false;
  subState = 0;
  stepIndex = startIdx;
  lastUpdate = millis();
}

uint8_t PixelStrip::Segment::getSoundIntensity() const {
  return lastSoundIntensity;
}

// ============================================================
//            MAIN EFFECT UPDATE ROUTER
// ============================================================

/**
 * @brief Called repeatedly to update the current effect.
 */
#include "WipeEffect.h"
#include "RainbowEffect.h"
#include "SolidRainbowEffect.h"
#include "TheaterChaseEffect.h"
#include "JumpEffect.h"
#include "FireEffect.h"
#include "ColoredFireEffect.h"
#include "BreatheEffect.h"
#include "FlashEffect.h"
#include "FallingStarEffect.h"
#include "IntensityMeterEffect.h"
#include "BounceAccelEffect.h"
#include "TiltRainbowEffect.h"
#include "LightningStrikeEffect.h"
#include "SoundLightningEffect.h"

void PixelStrip::Segment::update() {
  switch (effect)
  {
  case NONE:
    break;
  case WIPE:
    WipeEffect::run(this);
    break;
  case RAINBOW:
    RainbowEffect::run(this);
    break;
  case SOLIDRAINBOW:
    SolidRainbowEffect::run(this);
    break;
  case THEATERCHASE:
    TheaterChaseEffect::run(this);
    break;
  case JUMP:
    JumpEffect::run(this);
    break;
  case FIRE:
    FireEffect::run(this);
    break;
  case COLOREDFIRE:
    ColoredFireEffect::run(this);
    break;
  case BREATHE:
    BreatheEffect::run(this);
    break;
  case FLASH:
    FlashEffect::run(this);
    break;
  case FALLINGSTAR:
    FallingStarEffect::run(this);
    break;
  case INTENSITYMETER:
    IntensityMeterEffect::run(this);
    break;
  case BOUNCEACCEL:
    BounceAccelEffect::run(this);
    break;
  case TILTRAINBOW:
    TiltRainbowEffect::run(this);
    break;
  case LIGHTNINGSTRIKE:
    LightningStrikeEffect::run(this);
    break;
  case SOUNDLIGHTNING:
    SoundLightningEffect::run(this);
    break;
  }
}

// ============================================================
//                 EFFECT STEPPER FUNCTIONS
//    (One per effect, called by update())
// ============================================================

// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file
// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file
// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file

// Effect step moved to separate file

// ============================================================
//                    SEGMENT HELPERS
// ============================================================

/**
 * @brief Set all LEDs in this segment to a color.
 */
void PixelStrip::Segment::allOn(uint32_t color)
{
  for (uint16_t i = startIdx; i <= endIdx; ++i)
    parent.strip.setPixelColor(i, color);
  parent.strip.show();
}

/**
 * @brief Turn all LEDs in this segment OFF.
 */
void PixelStrip::Segment::allOff()
{
  for (uint16_t i = startIdx; i <= endIdx; ++i)
    parent.strip.setPixelColor(i, 0);
  parent.strip.show();
}
