#include "actuators.h"

#include <string.h>

#include "config.h"

namespace {

ActuatorState state;
uint32_t doorTransitionStartedMs = 0;
uint32_t doorUnlockExpiresMs = 0;
uint8_t lastPrintedFanPowerPct = 255;
FanMode lastPrintedFanMode = FanMode::Auto;
constexpr uint32_t SERVO_PERIOD_US = 1000000UL / SERVO_PWM_FREQ_HZ;
constexpr uint32_t SERVO_MAX_DUTY = (1UL << SERVO_PWM_RESOLUTION_BITS) - 1;

bool unlockWindowActive(uint32_t now) {
  return doorUnlockExpiresMs != 0 && static_cast<int32_t>(doorUnlockExpiresMs - now) > 0;
}

uint16_t angleToPulseUs(uint8_t angleDeg) {
  const uint8_t safeAngle = constrain(angleDeg, 0, 180);
  return SERVO_MIN_PULSE_US +
         ((static_cast<uint32_t>(SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * safeAngle) / 180);
}

void writeDoorServoAngle(uint8_t angleDeg) {
  const uint16_t pulseUs = angleToPulseUs(angleDeg);
  const uint32_t duty = (static_cast<uint32_t>(pulseUs) * SERVO_MAX_DUTY) / SERVO_PERIOD_US;
  ledcWrite(SERVO_PWM_CHANNEL, duty);
}

uint8_t doorServoAngleForState(DoorLockState nextState) {
  switch (nextState) {
    case DoorLockState::Locked:
    case DoorLockState::Locking:
      return SERVO_LOCK_CLOSED_ANGLE;
    case DoorLockState::Unlocked:
    case DoorLockState::Unlocking:
      return SERVO_LOCK_OPEN_ANGLE;
    case DoorLockState::LockError:
      return SERVO_LOCK_CLOSED_ANGLE;
  }

  return SERVO_LOCK_CLOSED_ANGLE;
}

void printDoorServoState(DoorLockState currentState, uint8_t angleDeg) {
  Serial.print(F("ACTUADOR SERVO: estado="));
  Serial.print(doorLockStateToText(currentState));
  Serial.print(F(", angulo="));
  Serial.print(angleDeg);
  Serial.println(F(" deg"));
}

void setDoorLockState(DoorLockState nextState) {
  if (state.doorLockState == nextState) {
    return;
  }

  state.doorLockState = nextState;
  doorTransitionStartedMs = millis();
  const uint8_t angleDeg = doorServoAngleForState(state.doorLockState);
  writeDoorServoAngle(angleDeg);
  printDoorServoState(state.doorLockState, angleDeg);
}

void updateDoorTransition(uint32_t now) {
  if (state.doorLockState == DoorLockState::Unlocking &&
      now - doorTransitionStartedMs >= DOOR_LOCK_SIMULATION_MS) {
    state.doorLockState = DoorLockState::Unlocked;
    doorUnlockExpiresMs = now + DOOR_UNLOCK_WINDOW_MS;
    printDoorServoState(state.doorLockState, doorServoAngleForState(state.doorLockState));
    Serial.println(F("SEGURIDAD: ventana de acceso activa"));
  }

  if (state.doorLockState == DoorLockState::Locking &&
      now - doorTransitionStartedMs >= DOOR_LOCK_SIMULATION_MS) {
    state.doorLockState = DoorLockState::Locked;
    doorUnlockExpiresMs = 0;
    printDoorServoState(state.doorLockState, doorServoAngleForState(state.doorLockState));
  }
}

void updateDoorSecurity(const SensorReadings &readings, uint32_t now) {
  const bool doorOpen = readings.ultrasonic.doorState == DoorState::Open;
  const bool accessWindow = unlockWindowActive(now) ||
                            state.doorLockState == DoorLockState::Unlocked ||
                            state.doorLockState == DoorLockState::Unlocking;

  if (state.doorLockState == DoorLockState::LockError) {
    state.doorSecurityState = DoorSecurityState::DoorLockError;
    return;
  }

  if (doorOpen) {
    if (accessWindow) {
      state.doorSecurityState = DoorSecurityState::DoorOpenAuthorized;
    } else {
      state.doorSecurityState = DoorSecurityState::DoorOpenForced;
      state.lastAccessEvent = AccessEvent::ForcedOpen;
    }
    return;
  }

  if (state.doorLockState == DoorLockState::Locked) {
    state.doorSecurityState = DoorSecurityState::DoorSecure;
    return;
  }

  state.doorSecurityState = DoorSecurityState::DoorUnlockedClosed;
}

void updateAutoFan(const SensorReadings &readings, const RackStatusDetails &details) {
  if (state.fanMode != FanMode::Auto) {
    return;
  }

  const ControlModeProfile &profile = getActiveControlModeProfile();
  state.controlMode = profile.mode;

  if (details.status == RackStatus::Offline) {
    return;
  }

  if (!readings.dht.ok || strcmp(details.reason, "dht_sensor_error") == 0) {
    state.fanPowerPct = profile.fanSensorErrorPct;
    return;
  }

  if (strcmp(details.temperatureState, "critical_high") == 0) {
    state.fanPowerPct = profile.fanCriticalPct;
    return;
  }

  if (strcmp(details.temperatureState, "warning_high") == 0) {
    state.fanPowerPct = profile.fanWarningPct;
    return;
  }

  state.fanPowerPct = profile.fanBasePct;
}

void autoLockAfterTimeout(uint32_t now, const SensorReadings &readings) {
  if (state.doorLockState != DoorLockState::Unlocked) {
    return;
  }

  if (readings.ultrasonic.doorState == DoorState::Open) {
    return;
  }

  if (!unlockWindowActive(now)) {
    state.lastAccessEvent = AccessEvent::Timeout;
    setDoorLockState(DoorLockState::Locking);
  }
}

void printFanChangeIfNeeded() {
  if (state.fanPowerPct == lastPrintedFanPowerPct && state.fanMode == lastPrintedFanMode) {
    return;
  }

  lastPrintedFanPowerPct = state.fanPowerPct;
  lastPrintedFanMode = state.fanMode;

  Serial.print(F("ACTUADOR FAN: modo="));
  Serial.print(fanModeToText(state.fanMode));
  Serial.print(F(", perfil="));
  Serial.print(controlModeToText(state.controlMode));
  Serial.print(F(", potencia="));
  Serial.print(state.fanPowerPct);
  Serial.println(F("% (simulado)"));
}

}  // namespace

void initActuators() {
  state = ActuatorState();
  state.controlMode = getActiveControlMode();
  state.simulated = false;
  lastPrintedFanPowerPct = 255;
  lastPrintedFanMode = state.fanMode;

  ledcSetup(SERVO_PWM_CHANNEL, SERVO_PWM_FREQ_HZ, SERVO_PWM_RESOLUTION_BITS);
  ledcAttachPin(PIN_SERVO_LOCK, SERVO_PWM_CHANNEL);
  writeDoorServoAngle(SERVO_LOCK_CLOSED_ANGLE);

  Serial.println(F("Actuadores listos: servo de bloqueo fisico y ventilador simulado."));
  printDoorServoState(state.doorLockState, SERVO_LOCK_CLOSED_ANGLE);
}

void updateActuators(const SensorReadings &readings, const RackStatusDetails &details) {
  state.controlMode = getActiveControlMode();

  updateAutoFan(readings, details);
  printFanChangeIfNeeded();
}

void updateDoorAccessState(const SensorReadings &readings) {
  const uint32_t now = millis();

  updateDoorTransition(now);
  autoLockAfterTimeout(now, readings);
  updateDoorSecurity(readings, now);
}

const ActuatorState &getActuatorState() {
  return state;
}

void setFanAuto() {
  state.fanMode = FanMode::Auto;
  Serial.println(F("ACTUADOR FAN: modo automatico activado"));
}

void setFanManual(uint8_t powerPct) {
  state.fanMode = FanMode::Manual;
  state.fanPowerPct = constrain(powerPct, 0, 100);
  Serial.print(F("ACTUADOR FAN: potencia manual="));
  Serial.print(state.fanPowerPct);
  Serial.println(F("%"));
}

void requestDoorUnlock() {
  if (state.doorLockState == DoorLockState::Unlocked ||
      state.doorLockState == DoorLockState::Unlocking) {
    return;
  }

  state.lastAccessEvent = AccessEvent::Granted;
  setDoorLockState(DoorLockState::Unlocking);
}

void requestDoorLock() {
  state.lastAccessEvent = AccessEvent::ManualLock;
  doorUnlockExpiresMs = 0;
  if (state.doorLockState != DoorLockState::Locked &&
      state.doorLockState != DoorLockState::Locking) {
    setDoorLockState(DoorLockState::Locking);
  }
}

void denyDoorAccess() {
  state.lastAccessEvent = AccessEvent::Denied;
  Serial.println(F("SEGURIDAD: acceso denegado"));
}

void printActuatorDiagnostic(const ActuatorState &currentState) {
  Serial.print(F("Modo control: "));
  Serial.print(controlModeToText(currentState.controlMode));
  Serial.print(F(" | Fan: "));
  Serial.print(fanModeToText(currentState.fanMode));
  Serial.print(F(" "));
  Serial.print(currentState.fanPowerPct);
  Serial.print(F("% | Bloqueo: "));
  Serial.print(doorLockStateToText(currentState.doorLockState));
  Serial.print(F(" | Seguridad puerta: "));
  Serial.print(doorSecurityStateToText(currentState.doorSecurityState));
  Serial.print(F(" | Acceso: "));
  Serial.println(accessEventToText(currentState.lastAccessEvent));
}

const char *fanModeToText(FanMode mode) {
  switch (mode) {
    case FanMode::Auto:
      return "auto";
    case FanMode::Manual:
      return "manual";
  }

  return "unknown";
}

const char *doorLockStateToText(DoorLockState currentState) {
  switch (currentState) {
    case DoorLockState::Locked:
      return "locked";
    case DoorLockState::Unlocked:
      return "unlocked";
    case DoorLockState::Locking:
      return "locking";
    case DoorLockState::Unlocking:
      return "unlocking";
    case DoorLockState::LockError:
      return "lock_error";
  }

  return "unknown";
}

const char *doorSecurityStateToText(DoorSecurityState currentState) {
  switch (currentState) {
    case DoorSecurityState::DoorSecure:
      return "door_secure";
    case DoorSecurityState::DoorUnlockedClosed:
      return "door_unlocked_closed";
    case DoorSecurityState::DoorOpenAuthorized:
      return "door_open_authorized";
    case DoorSecurityState::DoorOpenForced:
      return "door_open_forced";
    case DoorSecurityState::DoorLockError:
      return "door_lock_error";
  }

  return "unknown";
}

const char *accessEventToText(AccessEvent event) {
  switch (event) {
    case AccessEvent::None:
      return "none";
    case AccessEvent::Granted:
      return "granted";
    case AccessEvent::Denied:
      return "denied";
    case AccessEvent::Timeout:
      return "timeout";
    case AccessEvent::ForcedOpen:
      return "forced_open";
    case AccessEvent::ManualLock:
      return "manual_lock";
  }

  return "unknown";
}
