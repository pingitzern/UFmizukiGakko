#pragma once

#include <Arduino.h>

void uiBegin();
void uiSetState(const char* stateCode);
void uiSetTimers(uint16_t tServiceMinutes, uint16_t tFlushSeconds);
void uiRender(uint16_t minutes, uint16_t seconds);
