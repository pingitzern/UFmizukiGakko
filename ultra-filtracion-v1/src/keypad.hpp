#pragma once

#include <Arduino.h>

namespace Keypad {

enum Key : uint8_t {
    NONE = 0,
    RIGHT,
    UP,
    DOWN,
    LEFT,
    SELECT
};

Key readKey();
bool readChord();

}  // namespace Keypad
