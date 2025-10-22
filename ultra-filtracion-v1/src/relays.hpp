#pragma once

#include <Arduino.h>

class Relays {
public:
    void begin();
    void allSafe();

    void permOn();
    void permOff();

    void washAOn();
    void washAOff();

    void washBOn();
    void washBOff();

private:
    void write(uint8_t pin, bool on);
};

namespace RelayPins {
    constexpr uint8_t PERM = 2;
    constexpr uint8_t WASH_A = 3;
    constexpr uint8_t WASH_B = 11;
    constexpr uint8_t FREE = 12;
}
