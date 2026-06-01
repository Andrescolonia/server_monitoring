#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>

#include "control_modes.h"
#include "rack_status.h"
#include "sensors.h"

enum class FanMode {
  Auto,
  Manual,
};

enum class DoorLockState {
  Locked,
  Unlocked,
  Locking,
  Unlocking,
  LockError,
};

enum class DoorSecurityState {
  DoorSecure,
  DoorUnlockedClosed,
  DoorOpenAuthorized,
  DoorOpenForced,
  DoorLockError,
};

enum class AccessEvent {
  None,
  Granted,
  Denied,
  Timeout,
  ForcedOpen,
  ManualLock,
};

struct ActuatorState {
  ControlMode controlMode = ControlMode::Standard;
  FanMode fanMode = FanMode::Auto;
  uint8_t fanPowerPct = 0;
  DoorLockState doorLockState = DoorLockState::Locked;
  DoorSecurityState doorSecurityState = DoorSecurityState::DoorSecure;
  AccessEvent lastAccessEvent = AccessEvent::None;
  bool simulated = true;
};

void initActuators();
void updateDoorAccessState(const SensorReadings &readings);
void updateActuators(const SensorReadings &readings, const RackStatusDetails &details);
const ActuatorState &getActuatorState();
void setFanAuto();
void setFanManual(uint8_t powerPct);
void requestDoorUnlock();
void requestDoorLock();
void denyDoorAccess();
void printActuatorDiagnostic(const ActuatorState &state);
const char *fanModeToText(FanMode mode);
const char *doorLockStateToText(DoorLockState state);
const char *doorSecurityStateToText(DoorSecurityState state);
const char *accessEventToText(AccessEvent event);

#endif
