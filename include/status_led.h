#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "rack_status.h"

void initStatusLed();
void setRgb(bool red, bool green, bool blue);
void showStatusColor(RackStatus status);
void updateStatusLed(RackStatus status);
void runLedSelfTest();

#endif
