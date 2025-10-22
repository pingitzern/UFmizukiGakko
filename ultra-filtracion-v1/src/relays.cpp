#include "relays.hpp"

#ifndef ACTIVE_LOW
#define ACTIVE_LOW 1
#endif

namespace {
constexpr bool kActiveLow = ACTIVE_LOW != 0;
}

void Relays::begin() {
    pinMode(RelayPins::PERM, OUTPUT);
    pinMode(RelayPins::WASH_A, OUTPUT);
    pinMode(RelayPins::WASH_B, OUTPUT);
    pinMode(RelayPins::FREE, OUTPUT);
    allSafe();
}

void Relays::allSafe() {
    permOff();
    washAOff();
    washBOff();
    write(RelayPins::FREE, false);
}

void Relays::permOn() { write(RelayPins::PERM, true); }
void Relays::permOff() { write(RelayPins::PERM, false); }

void Relays::washAOn() { write(RelayPins::WASH_A, true); }
void Relays::washAOff() { write(RelayPins::WASH_A, false); }

void Relays::washBOn() { write(RelayPins::WASH_B, true); }
void Relays::washBOff() { write(RelayPins::WASH_B, false); }

void Relays::write(uint8_t pin, bool on) {
    digitalWrite(pin, on ? (kActiveLow ? LOW : HIGH) : (kActiveLow ? HIGH : LOW));
}
