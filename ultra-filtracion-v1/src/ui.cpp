#include "ui.hpp"

#include <cstring>
#include <cstdio>
#include <LiquidCrystal.h>

namespace {
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
char currentState[6] = "INIT";
uint16_t serviceMinutes = 60;
uint16_t flushSeconds = 60;
}

void uiBegin() {
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INIT 00:00");
    lcd.setCursor(0, 1);
    lcd.print("TS= 60m TF= 60s");
}

void uiSetState(const char* stateCode) {
    strncpy(currentState, stateCode, sizeof(currentState) - 1);
    currentState[sizeof(currentState) - 1] = '\0';
}

void uiSetTimers(uint16_t tServiceMinutes, uint16_t tFlushSeconds) {
    serviceMinutes = tServiceMinutes;
    flushSeconds = tFlushSeconds;
}

void uiRender(uint16_t minutes, uint16_t seconds) {
    char line1[17];
    char line2[17];
    snprintf(line1, sizeof(line1), "%-4s %02u:%02u", currentState, minutes, seconds);
    snprintf(line2, sizeof(line2), "TS=%3um TF=%3us", serviceMinutes, flushSeconds);

    lcd.setCursor(0, 0);
    lcd.print(line1);
    int len1 = strlen(line1);
    for (int i = len1; i < 16; ++i) {
        lcd.print(' ');
    }

    lcd.setCursor(0, 1);
    lcd.print(line2);
    int len2 = strlen(line2);
    for (int i = len2; i < 16; ++i) {
        lcd.print(' ');
    }
}
