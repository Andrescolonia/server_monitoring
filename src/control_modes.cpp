#include "control_modes.h"

#include <ctype.h>
#include <string.h>

#include "config.h"

namespace {

ControlMode activeMode = ControlMode::Standard;

constexpr RackThresholds STANDARD_THRESHOLDS = {
  TEMP_OPTIMAL_MAX_C,
  TEMP_CRITICAL_C,
  HUM_LOW_WARNING,
  HUM_HIGH_WARNING,
  HUM_LOW_CRITICAL,
  HUM_HIGH_CRITICAL,
  TILT_WARNING_DEG,
  TILT_CRITICAL_DEG,
  VIBRATION_WARNING_G,
  VIBRATION_CRITICAL_G,
};

constexpr RackThresholds ENERGY_SAVING_THRESHOLDS = {
  34.0F,
  38.0F,
  25.0F,
  70.0F,
  20.0F,
  80.0F,
  TILT_WARNING_DEG,
  TILT_CRITICAL_DEG,
  VIBRATION_WARNING_G,
  VIBRATION_CRITICAL_G,
};

constexpr RackThresholds PERFORMANCE_THRESHOLDS = {
  28.0F,
  33.0F,
  35.0F,
  60.0F,
  30.0F,
  70.0F,
  TILT_WARNING_DEG,
  TILT_CRITICAL_DEG,
  VIBRATION_WARNING_G,
  VIBRATION_CRITICAL_G,
};

constexpr ControlModeProfile STANDARD_PROFILE = {
  ControlMode::Standard,
  "standard",
  STANDARD_THRESHOLDS,
  25,
  60,
  100,
  80,
};

constexpr ControlModeProfile ENERGY_SAVING_PROFILE = {
  ControlMode::EnergySaving,
  "energy_saving",
  ENERGY_SAVING_THRESHOLDS,
  0,
  40,
  80,
  60,
};

constexpr ControlModeProfile PERFORMANCE_PROFILE = {
  ControlMode::Performance,
  "performance",
  PERFORMANCE_THRESHOLDS,
  50,
  80,
  100,
  100,
};

bool equalsIgnoreCase(const char *left, const char *right) {
  if (left == nullptr || right == nullptr) {
    return false;
  }

  while (*left != '\0' && *right != '\0') {
    if (tolower(*left) != tolower(*right)) {
      return false;
    }

    left++;
    right++;
  }

  return *left == '\0' && *right == '\0';
}

}  // namespace

ControlMode getActiveControlMode() {
  return activeMode;
}

const ControlModeProfile &getActiveControlModeProfile() {
  return getControlModeProfile(activeMode);
}

const ControlModeProfile &getControlModeProfile(ControlMode mode) {
  switch (mode) {
    case ControlMode::EnergySaving:
      return ENERGY_SAVING_PROFILE;
    case ControlMode::Performance:
      return PERFORMANCE_PROFILE;
    case ControlMode::Standard:
      return STANDARD_PROFILE;
  }

  return STANDARD_PROFILE;
}

const char *controlModeToText(ControlMode mode) {
  return getControlModeProfile(mode).name;
}

bool setActiveControlMode(ControlMode mode) {
  if (activeMode == mode) {
    return false;
  }

  activeMode = mode;
  Serial.print(F("CONTROL MODE: modo activo="));
  Serial.println(controlModeToText(activeMode));
  return true;
}

bool setActiveControlModeFromText(const char *text) {
  if (equalsIgnoreCase(text, "standard") || equalsIgnoreCase(text, "estandar")) {
    setActiveControlMode(ControlMode::Standard);
    return true;
  }

  if (equalsIgnoreCase(text, "energy_saving") || equalsIgnoreCase(text, "ahorro")) {
    setActiveControlMode(ControlMode::EnergySaving);
    return true;
  }

  if (equalsIgnoreCase(text, "performance") || equalsIgnoreCase(text, "potencia")) {
    setActiveControlMode(ControlMode::Performance);
    return true;
  }

  return false;
}
