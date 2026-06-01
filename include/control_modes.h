#ifndef CONTROL_MODES_H
#define CONTROL_MODES_H

#include <Arduino.h>

enum class ControlMode {
  Standard,
  EnergySaving,
  Performance,
};

struct RackThresholds {
  float tempOptimalMaxC;
  float tempCriticalC;
  float humLowWarning;
  float humHighWarning;
  float humLowCritical;
  float humHighCritical;
  float tiltWarningDeg;
  float tiltCriticalDeg;
  float vibrationWarningG;
  float vibrationCriticalG;
};

struct ControlModeProfile {
  ControlMode mode;
  const char *name;
  RackThresholds thresholds;
  uint8_t fanBasePct;
  uint8_t fanWarningPct;
  uint8_t fanCriticalPct;
  uint8_t fanSensorErrorPct;
};

ControlMode getActiveControlMode();
const ControlModeProfile &getActiveControlModeProfile();
const ControlModeProfile &getControlModeProfile(ControlMode mode);
const char *controlModeToText(ControlMode mode);
bool setActiveControlMode(ControlMode mode);
bool setActiveControlModeFromText(const char *text);

#endif
