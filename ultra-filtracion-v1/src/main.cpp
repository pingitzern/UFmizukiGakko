#include <Arduino.h>

#include "fsm.hpp"
#include "keypad.hpp"
#include "relays.hpp"
#include "ui.hpp"

namespace {
Relays relays;
constexpr uint16_t kServiceStepMinutes = 5;
constexpr uint16_t kFlushStepSeconds = 10;
}

void adjustService(int16_t delta) {
    uint16_t current = Fsm::serviceMinutes();
    int32_t updated = static_cast<int32_t>(current) + delta;
    if (updated < 0) {
        updated = 0;
    }
    Fsm::setServiceMinutes(static_cast<uint16_t>(updated));
}

void adjustFlush(int16_t delta) {
    uint16_t current = Fsm::flushSeconds();
    int32_t updated = static_cast<int32_t>(current) + delta;
    if (updated < 0) {
        updated = 0;
    }
    Fsm::setFlushSeconds(static_cast<uint16_t>(updated));
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("[System] Booting"));

    relays.begin();
    uiBegin();

    Fsm::begin(&relays);
    Fsm::enableStartupFlush(false);
    uiSetTimers(Fsm::serviceMinutes(), Fsm::flushSeconds());
    uiSetState(Fsm::stateLabel());
    Serial.println(F("[System] Ready"));
}

void loop() {
    using namespace Keypad;
    Key key = readKey();

    if (key != Key::NONE) {
        switch (key) {
            case Key::SELECT:
                Fsm::toggle();
                break;
            case Key::UP:
                adjustService(kServiceStepMinutes);
                break;
            case Key::DOWN:
                adjustService(-static_cast<int16_t>(kServiceStepMinutes));
                break;
            case Key::RIGHT:
                adjustFlush(kFlushStepSeconds);
                break;
            case Key::LEFT:
                adjustFlush(-static_cast<int16_t>(kFlushStepSeconds));
                break;
            case Key::NONE:
            default:
                break;
        }
    }

    if (readChord()) {
        Serial.println(F("[Keypad] NEXT"));
        Fsm::next();
    }

    Fsm::update();

    uint16_t minutes = 0;
    uint16_t seconds = 0;
    Fsm::getRemaining(minutes, seconds);
    uiRender(minutes, seconds);
}
