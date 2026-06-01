#ifndef RACK_STATUS_H
#define RACK_STATUS_H

#include <Arduino.h>

#include "sensors.h"

enum class DoorSecurityState;

enum class RackStatus {
  Connecting,
  Offline,
  Optimal,
  Warning,
  Critical,
  SensorError,
};

struct RackStatusDetails {
  RackStatus status = RackStatus::Connecting;
  const char *reason = "connecting";
  const char *temperatureState = "unknown";
  const char *humidityState = "unknown";
  const char *positionState = "unknown";
  const char *doorState = "unknown";
};

RackStatus evaluateRackStatus(const SensorReadings &readings, bool networkConnected);
RackStatus evaluateRackStatus(const SensorReadings &readings, bool networkConnected, DoorSecurityState doorSecurityState);
RackStatusDetails evaluateRackStatusDetails(const SensorReadings &readings, bool networkConnected);
RackStatusDetails evaluateRackStatusDetails(const SensorReadings &readings, bool networkConnected, DoorSecurityState doorSecurityState);
const char *rackStatusToText(RackStatus status);
int rackStatusToCode(RackStatus status);

#endif
