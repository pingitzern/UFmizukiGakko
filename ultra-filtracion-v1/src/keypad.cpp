#include <Arduino.h>
#include "keypad.hpp"

namespace {
constexpr uint8_t kKeypadPin = A0;
constexpr uint32_t kDebounceMs = 60;
constexpr uint32_t kChordWindowMs = 750;
constexpr int kHysteresis = 30;

Keypad::Key mapAnalogToKey(int value, Keypad::Key lastStable) {
    // Thresholds with hysteresis compensation
    if (value < 50 + (lastStable == Keypad::Key::RIGHT ? kHysteresis : 0)) {
        return Keypad::Key::RIGHT;
    }
    if (value < 150 + (lastStable == Keypad::Key::UP ? kHysteresis : 0)) {
        return Keypad::Key::UP;
    }
    if (value < 350 + (lastStable == Keypad::Key::DOWN ? kHysteresis : 0)) {
        return Keypad::Key::DOWN;
    }
    if (value < 550 + (lastStable == Keypad::Key::LEFT ? kHysteresis : 0)) {
        return Keypad::Key::LEFT;
    }
    if (value < 800 + (lastStable == Keypad::Key::SELECT ? kHysteresis : 0)) {
        return Keypad::Key::SELECT;
    }
    return Keypad::Key::NONE;
}

Keypad::Key lastRawKey = Keypad::Key::NONE;
Keypad::Key stableKey = Keypad::Key::NONE;
Keypad::Key lastReportedKey = Keypad::Key::NONE;
uint32_t lastChangeMs = 0;
uint32_t lastPressMs[static_cast<uint8_t>(Keypad::Key::SELECT) + 1] = {0};
uint32_t lastChordMs = 0;

}  // namespace

namespace Keypad {

Key readKey() {
    uint32_t now = millis();
    int reading = analogRead(kKeypadPin);
    Key raw = mapAnalogToKey(reading, stableKey);

    if (raw != lastRawKey) {
        lastRawKey = raw;
        lastChangeMs = now;
    }

    if ((now - lastChangeMs) >= kDebounceMs) {
        stableKey = raw;
    }

    Key event = Key::NONE;
    if (stableKey != lastReportedKey) {
        if (stableKey != Key::NONE) {
            event = stableKey;
            lastPressMs[static_cast<uint8_t>(event)] = now;
        }
        lastReportedKey = stableKey;
    } else if (stableKey == Key::NONE && lastReportedKey != Key::NONE) {
        lastReportedKey = Key::NONE;
    }

    return event;
}

bool readChord() {
    uint32_t rightPress = lastPressMs[static_cast<uint8_t>(Key::RIGHT)];
    uint32_t selectPress = lastPressMs[static_cast<uint8_t>(Key::SELECT)];
    if (rightPress == 0 || selectPress == 0) {
        return false;
    }
    uint32_t now = millis();
    uint32_t diff = rightPress > selectPress ? rightPress - selectPress : selectPress - rightPress;
    if (diff <= kChordWindowMs && (now - lastChordMs) > kChordWindowMs) {
        lastChordMs = now;
        lastPressMs[static_cast<uint8_t>(Key::RIGHT)] = 0;
        lastPressMs[static_cast<uint8_t>(Key::SELECT)] = 0;
        return true;
    }
    return false;
}

}  // namespace Keypad
