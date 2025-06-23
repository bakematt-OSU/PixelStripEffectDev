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
void PixelStrip::Segment::update()
{
  switch (effect)
  {
  case NONE:
    break;
  case WIPE:
    wipeStep();
    break;
  case RAINBOW:
    rainbowStep();
    break;
  case SOLID_RAINBOW:
    solidRainbowStep();
    break;
  case THEATER_CHASE:
    theaterChaseStep();
    break;
  case JUMP:
    jumpStep();
    break;
  case FIRE:
    fireStep();
    break;
  case COLORED_FIRE:
    coloredFireStep();
    break;
  case BREATHE:
    breatheStep();
    break;
  case FLASH:
    flashStep();
    break;
  case FALLING_STAR:
    fallingStarStep();
    break;
  case INTENSITY_METER:
    intensityMeterStep();
    break;
  case BOUNCE_ACCEL:
    bounceAccelStep();
    break;
  case TILT_RAINBOW:
    tiltRainbowStep();
    break;
  case LIGHTNING_STRIKE:
    lightningStrikeStep();
    break;
  case SOUND_LIGHTNING:
    soundLightningStep();
    break;
  }
}

// ============================================================
//                 EFFECT STEPPER FUNCTIONS
//    (One per effect, called by update())
// ============================================================

// ------- Wipe ---------
void PixelStrip::Segment::wipeStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  parent.strip.setPixelColor(stepIndex++, primaryColor);
  parent.strip.show();
  if (stepIndex > endIdx)
  {
    if (loop)
      stepIndex = startIdx;
    else
      effect = NONE;
  }
}

// ------- Rainbow (animated) ---------
void PixelStrip::Segment::rainbowStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  for (uint16_t i = startIdx; i <= endIdx; ++i)
  {
    uint32_t col = parent.strip.ColorHSV(((stepIndex + i - startIdx) * 65536UL) / (endIdx - startIdx + 1));
    parent.strip.setPixelColor(i, col);
  }
  parent.strip.show();
  if (++stepIndex >= (endIdx - startIdx + 1))
  {
    stepIndex = 0;
    if (!loop)
      effect = NONE;
  }
}

// ------- Solid Rainbow (static) ---------
void PixelStrip::Segment::solidRainbowStep()
{
  for (uint16_t i = startIdx; i <= endIdx; ++i)
  {
    uint32_t color = parent.strip.ColorHSV(((i - startIdx) * 65536UL) / (endIdx - startIdx + 1));
    parent.strip.setPixelColor(i, color);
  }
  parent.strip.show();
  effect = NONE;
}

// ------- Theater Chase ---------
void PixelStrip::Segment::theaterChaseStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  for (uint16_t i = startIdx; i <= endIdx; ++i)
    parent.strip.setPixelColor(i, 0);
  for (uint16_t i = startIdx + (stepIndex % cycles); i <= endIdx; i += cycles)
  {
    parent.strip.setPixelColor(i, primaryColor);
  }
  parent.strip.show();
  stepIndex++;
  if (!loop && stepIndex >= cycles)
    effect = NONE;
}

// ------- Jump ---------
void PixelStrip::Segment::jumpStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  for (uint16_t i = startIdx; i <= endIdx; ++i)
    parent.strip.setPixelColor(i, 0);
  uint16_t blockStart = startIdx + (stepIndex * cycles);
  uint16_t blockEnd = blockStart + cycles - 1;
  for (uint16_t i = blockStart; i <= blockEnd && i <= endIdx; ++i)
    parent.strip.setPixelColor(i, primaryColor);
  parent.strip.show();
  stepIndex++;
  if (blockEnd >= endIdx)
  {
    if (loop)
      stepIndex = 0;
    else
      effect = NONE;
  }
}

// ------- Fire ---------
void PixelStrip::Segment::fireStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  for (uint16_t i = startIdx; i <= endIdx; ++i)
    parent.strip.setPixelColor(i, 0);
  for (uint8_t s = 0; s < cycles; ++s)
  {
    uint16_t idx = startIdx + random(endIdx - startIdx + 1);
    uint32_t color = parent.strip.Color(255, random(80, 160), 0); // Orange/yellow
    parent.strip.setPixelColor(idx, color);
  }
  parent.strip.show();
  if (!loop)
  {
    static uint16_t frame = 0;
    frame++;
    if (frame > 30)
    {
      frame = 0;
      effect = NONE;
    }
  }
}

// ------- Colored Fire ---------
void PixelStrip::Segment::coloredFireStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  for (uint16_t i = startIdx; i <= endIdx; ++i)
    parent.strip.setPixelColor(i, 0);
  for (uint8_t s = 0; s < cycles; ++s)
  {
    uint16_t idx = startIdx + random(endIdx - startIdx + 1);
    uint8_t which = random(3);
    uint32_t color = (which == 0) ? fireC1 : (which == 1) ? fireC2
                                                          : fireC3;
    parent.strip.setPixelColor(idx, color);
  }
  parent.strip.show();
  if (!loop)
  {
    static uint16_t frame = 0;
    frame++;
    if (frame > 30)
    {
      frame = 0;
      effect = NONE;
    }
  }
}

// ------- Breathe ---------
void PixelStrip::Segment::breatheStep()
{
  unsigned long now = millis();
  if (now - lastUpdate < delayMs)
    return;
  lastUpdate = now;

  // Store static state per segment instance
  static int breathValue = 10;
  static bool increasing = true;

  const uint8_t maxBrightness = 255;
  const uint8_t minBrightness = 10;

  // Adjust brightness
  if (increasing)
  {
    breathValue += 5;
    if (breathValue >= maxBrightness)
    {
      breathValue = maxBrightness;
      increasing = false;
    }
  }
  else
  {
    breathValue -= 5;
    if (breathValue <= minBrightness)
    {
      breathValue = minBrightness;
      increasing = true;

      // Optionally stop after one full cycle
      if (!loop)
      {
        active = false;
        return;
      }
    }
  }

  // Apply scaled color
  uint8_t r = ((primaryColor >> 16) & 0xFF) * breathValue / 255;
  uint8_t g = ((primaryColor >> 8) & 0xFF) * breathValue / 255;
  uint8_t b = (primaryColor & 0xFF) * breathValue / 255;

  for (uint16_t i = startIdx; i <= endIdx; i++)
  {
    parent.strip.setPixelColor(i, parent.strip.Color(r, g, b));
  }

  parent.strip.show();
}
// ------- Flash ---------
void PixelStrip::Segment::flashStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  if (subState == 0)
  {
    for (uint16_t i = startIdx; i <= endIdx; ++i)
      parent.strip.setPixelColor(i, primaryColor);
    parent.strip.show();
    subState = 1;
  }
  else
  {
    for (uint16_t i = startIdx; i <= endIdx; ++i)
      parent.strip.setPixelColor(i, 0);
    parent.strip.show();
    subState = 0;
    if (!loop)
    {
      static uint8_t flashes = 0;
      flashes++;
      if (flashes >= cycles)
      {
        flashes = 0;
        effect = NONE;
      }
    }
  }
}

// ------- Falling Star ---------
void PixelStrip::Segment::fallingStarStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  for (uint16_t i = startIdx; i <= endIdx; ++i)
    parent.strip.setPixelColor(i, 0);
  for (int16_t t = 0; t < starLen; ++t)
  {
    int16_t pos = stepIndex - t;
    if (pos < (int16_t)startIdx)
      break;
    float fade = 1.0f - (float)t / starLen;
    uint8_t r = (uint8_t)(((primaryColor >> 16) & 0xFF) * fade);
    uint8_t g = (uint8_t)(((primaryColor >> 8) & 0xFF) * fade);
    uint8_t b = (uint8_t)((primaryColor & 0xFF) * fade);
    parent.strip.setPixelColor(pos, parent.strip.Color(r, g, b));
  }
  parent.strip.show();
  stepIndex++;
  if (stepIndex > endIdx)
  {
    if (loop)
      stepIndex = startIdx;
    else
      effect = NONE;
  }
}

// ------- Intensity Meter ---------
// extern Microphone mic; // External mic instance

void PixelStrip::Segment::intensityMeterStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();

  // Use lastSoundIntensity (0–255) instead of mic.readPeak()
  uint8_t stepsLit = 0;
  if (lastSoundIntensity > 0)
  {
    stepsLit = map(lastSoundIntensity, 0, 255, 0, intCount);
    if (stepsLit > intCount)
      stepsLit = intCount;
  }

  for (uint16_t i = startIdx; i <= endIdx; ++i)
  {
    uint8_t barIdx = (i - startIdx) * intCount / (endIdx - startIdx + 1);
    if (barIdx < stepsLit)
      parent.strip.setPixelColor(i, intColors[barIdx]);
    else
      parent.strip.setPixelColor(i, 0);
  }

  parent.strip.show();
}
// ------- Bounce with Accel ---------
void PixelStrip::Segment::bounceAccelStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  static int8_t direction = 1;
  float absX = abs(ax), absY = abs(ay), absZ = abs(az);
  float dominant = absX;
  uint32_t color = axisXColor;
  if (absY > dominant)
  {
    dominant = absY;
    color = axisYColor;
  }
  if (absZ > dominant)
  {
    dominant = absZ;
    color = axisZColor;
  }
  uint8_t jumpAmount = 1 + (uint8_t)(dominant * 4.0f);
  if (jumpAmount > (endIdx - startIdx + 1) / 2)
    jumpAmount = (endIdx - startIdx + 1) / 2;
  unsigned long dynamicDelay = delayMs;
  if (dominant > 0.2f)
    dynamicDelay = max(10UL, (unsigned long)(delayMs / (1 + dominant * 5)));
  static unsigned long lastMove = 0;
  if (millis() - lastMove < dynamicDelay)
    return;
  lastMove = millis();
  for (uint16_t i = startIdx; i <= endIdx; ++i)
    parent.strip.setPixelColor(i, 0);
  parent.strip.setPixelColor(stepIndex, color);
  parent.strip.show();
  int16_t nextIndex = stepIndex + direction * jumpAmount;
  if (nextIndex > endIdx)
  {
    stepIndex = endIdx;
    direction = -1;
  }
  else if (nextIndex < (int16_t)startIdx)
  {
    stepIndex = startIdx;
    direction = 1;
    if (!loop)
      effect = NONE;
  }
  else
  {
    stepIndex = nextIndex;
  }
}

// ------- Tilt Rainbow ---------
void PixelStrip::Segment::tiltRainbowStep()
{
  if (millis() - lastUpdate < delayMs)
    return;
  lastUpdate = millis();
  float norm = constrain(ax, -1.0f, 1.0f);
  int16_t offset = (int16_t)((norm + 1.0f) * (endIdx - startIdx) / 2.0f);
  for (uint16_t i = startIdx; i <= endIdx; ++i)
  {
    uint16_t rainbowPos = (i - startIdx + offset) % (endIdx - startIdx + 1);
    uint32_t color = parent.strip.ColorHSV((rainbowPos * 65536UL) / (endIdx - startIdx + 1));
    parent.strip.setPixelColor(i, color);
  }
  parent.strip.show();
  if (!loop)
    effect = NONE;
}

// ------- Lightning Strike ---------
void PixelStrip::Segment::lightningStrikeStep() {
  unsigned long now = millis();

  switch (subState) {
    case 0:  // STRIKE HEAD MOVEMENT
      if (now - lastUpdate < delayMs) return;
      lastUpdate = now;

      // Clear previous LEDs in segment
      for (uint16_t i = startIdx; i <= endIdx; ++i) {
        parent.strip.setPixelColor(i, 0);
      }

      // Draw the moving "strike head"
      if (stepIndex <= endIdx) {
        uint32_t col = primaryColor;  // fallback to flat color
        parent.strip.setPixelColor(stepIndex++, col);
        parent.strip.show();
      } else {
        // Finished moving head — time for flash
        subState = 1;
        lastUpdate = now;
        parent.strip.setBrightness(flashBrightness);

        // Manually fill segment range with flash color
        for (uint16_t i = startIdx; i <= endIdx; ++i) {
          parent.strip.setPixelColor(i, primaryColor);
        }
        parent.strip.show();
      }
      break;

    case 1:  // FLASH HOLD
      if (now - lastUpdate < flashDuration) return;

      // End flash: dim down
      parent.strip.setBrightness(startBrightness);
      parent.strip.show();

      subState = 2;
      lastUpdate = now;
      break;

    case 2:  // POST-FLASH COOLDOWN
      if (now - lastUpdate < delayMax) return;

      subState = 0;
      stepIndex = startIdx;
      active = false;  // end of strike
      break;
  }
}


// ------- Sound Lightning ---------
void PixelStrip::Segment::setSoundIntensity(uint8_t intensity) { lastSoundIntensity = intensity; }

void PixelStrip::Segment::soundLightningStep()
{
  unsigned long now = millis();
  static uint8_t flickerCount = 0;
  static uint16_t flickerMask = 0;

  if (!lightningActive)
  {
    if (lastSoundIntensity > soundThreshold)
    {
      lightningActive = true;
      uint8_t mappedBrightness = map(lastSoundIntensity, soundThreshold, 255, 100, flashBrightness);
      if (mappedBrightness > 255)
        mappedBrightness = 255;
      flashBrightness = mappedBrightness;
      flashDuration = map(lastSoundIntensity, soundThreshold, 255, 50, 300);
      if (flashDuration > 300)
        flashDuration = 300;
      subState = 0;
      stepIndex = startIdx;
      flickerCount = 0;
      lastUpdate = now;
    }
    else
      return;
  }
  switch (subState)
  {
  case 0: // Grow
    if (now - lastUpdate < delayMs)
      return;
    lastUpdate = now;
    for (uint16_t i = startIdx; i <= stepIndex && i <= endIdx; ++i)
    {
      uint32_t col = rainbowMode
                         ? parent.strip.ColorHSV(((i - startIdx) * 65536UL) / (endIdx - startIdx + 1))
                         : primaryColor;
      parent.strip.setPixelColor(i, col);
    }
    for (uint16_t i = stepIndex + 1; i <= endIdx; ++i)
      parent.strip.setPixelColor(i, 0);
    parent.strip.show();
    stepIndex++;
    if (stepIndex > endIdx)
    {
      subState = 1;
      flickerCount = 0;
      lastUpdate = now;
    }
    break;
  case 1: // Flicker
    if (now - lastUpdate < delayMs / 2)
      return;
    lastUpdate = now;
    flickerMask = 0;
    for (uint16_t i = startIdx; i <= endIdx; ++i)
    {
      if (random(100) < 70)
        flickerMask |= (1 << (i - startIdx));
    }
    for (uint16_t i = startIdx; i <= endIdx; ++i)
    {
      if (flickerMask & (1 << (i - startIdx)))
      {
        uint32_t col = rainbowMode
                           ? parent.strip.ColorHSV(((i - startIdx) * 65536UL) / (endIdx - startIdx + 1))
                           : primaryColor;
        parent.strip.setPixelColor(i, col);
      }
      else
        parent.strip.setPixelColor(i, 0);
    }
    parent.strip.show();
    flickerCount++;
    if (flickerCount >= 4)
    {
      subState = 2;
      lastUpdate = now;
    }
    break;
  case 2: // Flash
    if (now - lastUpdate < flashDuration)
      return;
    lastUpdate = now;
    for (uint16_t i = startIdx; i <= endIdx; ++i)
    {
      uint32_t col = rainbowMode
                         ? parent.strip.ColorHSV(((i - startIdx) * 65536UL) / (endIdx - startIdx + 1))
                         : primaryColor;
      parent.strip.setPixelColor(i, col);
    }
    setBrightness(flashBrightness);
    parent.strip.show();
    setBrightness(startBrightness);
    subState = 3;
    lastUpdate = now;
    break;
  case 3: // All off
    if (now - lastUpdate < delayMax)
      return;
    lastUpdate = now;
    allOff();
    lightningActive = false;
    break;
  }
}

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
