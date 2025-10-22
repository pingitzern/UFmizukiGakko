#pragma once

#include <Arduino.h>

class Relays;

namespace Fsm {

enum class State : uint8_t {
    INIT = 0,
    SERVICE,
    FLUSH_A,
    FLUSH_B,
    PAUSE
};

void begin(Relays* relays);
void update();

void start();
void stop();
void toggle();
void next();

void setServiceMinutes(uint16_t minutes);
void setFlushSeconds(uint16_t seconds);

uint16_t serviceMinutes();
uint16_t flushSeconds();

void getRemaining(uint16_t& minutes, uint16_t& seconds);
State state();
const char* stateLabel();

void enableStartupFlush(bool enable);

}  // namespace Fsm
