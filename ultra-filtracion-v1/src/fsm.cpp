#include <Arduino.h>
#include "fsm.hpp"

#include "relays.hpp"
#include "ui.hpp"

#ifndef ACTIVE_LOW
#define ACTIVE_LOW 1
#endif

namespace {
constexpr uint32_t kMillisPerMinute = 60000UL;
constexpr uint32_t kMillisPerSecond = 1000UL;
constexpr uint32_t kSettleMs = 2000UL;
constexpr uint32_t kStartupFlushTotalMs = 30000UL;
constexpr uint32_t kStartupFlushStepMs = 5000UL;

Relays* relaysPtr = nullptr;
Fsm::State currentState = Fsm::State::INIT;
uint32_t stateStartMs = 0;
uint32_t stateDurationMs = 0;
uint32_t settleStartMs = 0;
bool flushASettled = false;
bool flushAActive = false;
uint32_t flushAActiveStartMs = 0;
bool flushBPostSettle = false;
bool running = false;
bool startupFlushEnabled = false;
bool startupFlushDone = false;
bool startupMode = false;
uint32_t startupRemainingMs = 0;

struct Settings {
    uint16_t serviceMinutes = 60;
    uint16_t flushSeconds = 60;
} settings;

const char* stateCodes[] = {"INIT", "SERV", "FL_A", "FL_B", "PAUS"};

uint32_t clampServiceMinutes(uint16_t minutes) {
    uint16_t value = minutes;
    if (value < 20) value = 20;
    if (value > 120) value = 120;
    return static_cast<uint32_t>(value);
}

uint32_t clampFlushSeconds(uint16_t seconds) {
    uint16_t value = seconds;
    if (value < 20) value = 20;
    if (value > 120) value = 120;
    return static_cast<uint32_t>(value);
}

const char* labelForState(Fsm::State state) {
    return stateCodes[static_cast<uint8_t>(state)];
}

void logStateChange(Fsm::State state) {
    Serial.print(F("[FSM] -> "));
    Serial.println(labelForState(state));
}

void transitionTo(Fsm::State state) {
    currentState = state;
    stateStartMs = millis();
    flushASettled = false;
    flushBPostSettle = false;

    switch (state) {
        case Fsm::State::INIT:
            stateDurationMs = 0;
            if (relaysPtr) {
                relaysPtr->allSafe();
            }
            break;
        case Fsm::State::PAUSE:
            stateDurationMs = 0;
            if (relaysPtr) {
                relaysPtr->allSafe();
            }
            running = false;
            break;
        case Fsm::State::SERVICE:
            stateDurationMs = settings.serviceMinutes * kMillisPerMinute;
            if (relaysPtr) {
                relaysPtr->allSafe();
            }
            break;
        case Fsm::State::FLUSH_A: {
            uint32_t duration = startupMode ? kStartupFlushStepMs : settings.flushSeconds * kMillisPerSecond;
            if (startupMode && startupRemainingMs < duration) {
                duration = startupRemainingMs;
            }
            stateDurationMs = duration;
            if (relaysPtr) {
                relaysPtr->permOn();
                relaysPtr->washAOff();
                relaysPtr->washBOff();
            }
            settleStartMs = stateStartMs;
            flushAActive = false;
            flushAActiveStartMs = stateStartMs;
            break;
        }
        case Fsm::State::FLUSH_B: {
            uint32_t duration = startupMode ? kStartupFlushStepMs : settings.flushSeconds * kMillisPerSecond;
            if (startupMode && startupRemainingMs < duration) {
                duration = startupRemainingMs;
            }
            stateDurationMs = duration;
            flushAActive = false;
            if (relaysPtr) {
                relaysPtr->permOn();
                relaysPtr->washAOff();
                relaysPtr->washBOn();
            }
            break;
        }
    }

    uiSetState(labelForState(state));
    logStateChange(state);
}

void finishStartupCycle() {
    startupMode = false;
    startupFlushDone = true;
    startupRemainingMs = 0;
}

}  // namespace

namespace Fsm {

void begin(Relays* relays) {
    relaysPtr = relays;
    if (relaysPtr) {
        relaysPtr->allSafe();
    }
    running = false;
    startupMode = false;
    startupFlushDone = false;
    startupRemainingMs = 0;
    settings.serviceMinutes = static_cast<uint16_t>(clampServiceMinutes(60));
    settings.flushSeconds = static_cast<uint16_t>(clampFlushSeconds(60));
    uiSetTimers(settings.serviceMinutes, settings.flushSeconds);
    uiSetState(labelForState(currentState));
    stateStartMs = millis();
}

void update() {
    uint32_t now = millis();
    switch (currentState) {
        case State::SERVICE: {
            if (running && stateDurationMs > 0 && (now - stateStartMs) >= stateDurationMs) {
                transitionTo(State::FLUSH_A);
            }
            break;
        }
        case State::FLUSH_A: {
            if (!flushASettled && (now - settleStartMs) >= kSettleMs) {
                if (relaysPtr) {
                    relaysPtr->washAOn();
                }
                flushASettled = true;
                flushAActive = true;
                flushAActiveStartMs = now;
            }

            if (flushASettled && stateDurationMs > 0) {
                uint32_t flushElapsed = now - flushAActiveStartMs;
                if (flushElapsed < stateDurationMs) {
                    break;
                }
                if (relaysPtr) {
                    relaysPtr->washAOff();
                }
                if (startupMode) {
                    if (startupRemainingMs > stateDurationMs) {
                        startupRemainingMs -= stateDurationMs;
                    } else {
                        startupRemainingMs = 0;
                    }
                }
                flushAActive = false;
                transitionTo(State::FLUSH_B);
            }
            break;
        }
        case State::FLUSH_B: {
            if (!flushBPostSettle) {
                if (stateDurationMs > 0 && (now - stateStartMs) >= stateDurationMs) {
                    if (relaysPtr) {
                        relaysPtr->washBOff();
                    }
                    if (startupMode) {
                        if (startupRemainingMs > stateDurationMs) {
                            startupRemainingMs -= stateDurationMs;
                        } else {
                            startupRemainingMs = 0;
                        }
                    }
                    flushBPostSettle = true;
                    settleStartMs = now;
                }
            } else {
                if ((now - settleStartMs) >= kSettleMs) {
                    if (relaysPtr) {
                        relaysPtr->permOff();
                    }
                    if (startupMode) {
                        if (startupRemainingMs > 0) {
                            transitionTo(State::FLUSH_A);
                            return;
                        }
                        finishStartupCycle();
                    }
                    if (running) {
                        transitionTo(State::SERVICE);
                    } else {
                        transitionTo(State::PAUSE);
                    }
                }
            }
            break;
        }
        case State::PAUSE:
        case State::INIT:
        default:
            break;
    }
}

void start() {
    if (running) {
        return;
    }
    running = true;
    Serial.println(F("[FSM] START"));
    if ((currentState == State::INIT || currentState == State::PAUSE)) {
        if (startupFlushEnabled && !startupFlushDone) {
            startupMode = true;
            startupRemainingMs = kStartupFlushTotalMs;
            transitionTo(State::FLUSH_A);
        } else {
            transitionTo(State::SERVICE);
        }
    }
}

void stop() {
    running = false;
    startupMode = false;
    Serial.println(F("[FSM] STOP"));
    transitionTo(State::PAUSE);
}

void toggle() {
    if (running && currentState != State::PAUSE) {
        stop();
    } else {
        start();
    }
}

void next() {
    uint32_t now = millis();
    startupMode = false;
    startupRemainingMs = 0;
    switch (currentState) {
        case State::INIT:
        case State::PAUSE:
            transitionTo(State::SERVICE);
            running = true;
            break;
        case State::SERVICE:
            stateStartMs = now - stateDurationMs;
            update();
            break;
        case State::FLUSH_A:
            if (!flushASettled && relaysPtr) {
                relaysPtr->washAOn();
                flushASettled = true;
            }
            flushAActive = true;
            flushAActiveStartMs = (stateDurationMs > 0 && now > stateDurationMs) ? now - stateDurationMs : now;
            stateStartMs = now - stateDurationMs;
            update();
            break;
        case State::FLUSH_B:
            flushBPostSettle = true;
            if (relaysPtr) {
                relaysPtr->washBOff();
            }
            settleStartMs = (now > kSettleMs) ? now - kSettleMs : now;
            update();
            break;
    }
}

void setServiceMinutes(uint16_t minutes) {
    uint16_t clamped = static_cast<uint16_t>(clampServiceMinutes(minutes));
    if (settings.serviceMinutes == clamped) {
        return;
    }
    settings.serviceMinutes = clamped;
    uiSetTimers(settings.serviceMinutes, settings.flushSeconds);
    Serial.print(F("[FSM] T_SERVICIO="));
    Serial.print(settings.serviceMinutes);
    Serial.println(F("m"));
    if (currentState == State::SERVICE) {
        stateDurationMs = settings.serviceMinutes * kMillisPerMinute;
        stateStartMs = millis();
    }
}

void setFlushSeconds(uint16_t seconds) {
    uint16_t clamped = static_cast<uint16_t>(clampFlushSeconds(seconds));
    if (settings.flushSeconds == clamped) {
        return;
    }
    settings.flushSeconds = clamped;
    uiSetTimers(settings.serviceMinutes, settings.flushSeconds);
    Serial.print(F("[FSM] T_FLUSH="));
    Serial.print(settings.flushSeconds);
    Serial.println(F("s"));
    if (currentState == State::FLUSH_A || (currentState == State::FLUSH_B && !flushBPostSettle)) {
        stateDurationMs = settings.flushSeconds * kMillisPerSecond;
        stateStartMs = millis();
    }
}

uint16_t serviceMinutes() {
    return settings.serviceMinutes;
}

uint16_t flushSeconds() {
    return settings.flushSeconds;
}

void getRemaining(uint16_t& minutes, uint16_t& seconds) {
    uint32_t now = millis();
    uint32_t remainingMs = 0;

    switch (currentState) {
        case State::FLUSH_B:
            if (flushBPostSettle) {
                uint32_t elapsedSettle = now - settleStartMs;
                if (elapsedSettle < kSettleMs) {
                    remainingMs = kSettleMs - elapsedSettle;
                }
                break;
            }
            // fall through
        case State::SERVICE: {
            if (stateDurationMs > 0) {
                uint32_t elapsed = now - stateStartMs;
                if (elapsed < stateDurationMs) {
                    remainingMs = stateDurationMs - elapsed;
                }
            }
            break;
        }
        case State::FLUSH_A: {
            if (stateDurationMs > 0) {
                if (flushAActive) {
                    uint32_t elapsedFlush = now - flushAActiveStartMs;
                    if (elapsedFlush < stateDurationMs) {
                        remainingMs = stateDurationMs - elapsedFlush;
                    }
                } else {
                    remainingMs = stateDurationMs;
                }
            }
            break;
        }
        case State::INIT:
        case State::PAUSE:
        default:
            remainingMs = 0;
            break;
    }

    uint32_t totalSeconds = (remainingMs + 999UL) / kMillisPerSecond;
    minutes = static_cast<uint16_t>(totalSeconds / 60UL);
    seconds = static_cast<uint16_t>(totalSeconds % 60UL);
}

State state() {
    return currentState;
}

const char* stateLabel() {
    return labelForState(currentState);
}

void enableStartupFlush(bool enable) {
    startupFlushEnabled = enable;
    if (!enable) {
        startupFlushDone = false;
    }
}

}  // namespace Fsm
