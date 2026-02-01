#ifndef OLED_H
#define OLED_H

#include <Arduino.h>

void oledInit();
void oledBoot();
void oledShowIP(String ip);
void oledShowStatus(bool door1, bool door2);

void oledShowResetInfo(uint8_t resetPin);
void oledUpdate();

void oledActiveResetInfo();
void oledCancelResetInfo();

void oledShowApMode(String apName);

void oledShowResetCountdown(int secondsLeft, int progressPercent);

#endif
