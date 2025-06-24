#ifndef EFFECTS_H
#define EFFECTS_H

// This file should ONLY define the master list of effects.
// It should not include any other files.
// Format: X(ENUM_NAME, CppNamespaceName)
#define EFFECT_LIST(X) \
    X(RAINBOW, RainbowChase) \
    X(SOLID, SolidColor) \
    X(FLASH_TRIGGER, FlashOnTrigger) \
    X(RAINBOW_CYCLE, RainbowCycle) \
    X(THEATER_CHASE, TheaterChase)\
    X(FIRE, Fire) \
    // * When you create a new effect, add its X macro line here. *

#endif // EFFECTS_H