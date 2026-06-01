#include "rack_status.h"

#include <string.h>

#include "actuators.h"
#include "config.h"
#include "control_modes.h"

namespace {

const char *evaluateTemperatureState(const DhtReading &dht, const RackThresholds &thresholds) {
  if (!dht.ok) {
    return "sensor_error";
  }
  if (dht.temperatureC >= thresholds.tempCriticalC) {
    return "critical_high";
  }
  if (dht.temperatureC > thresholds.tempOptimalMaxC) {
    return "warning_high";
  }
  return "optimal";
}

const char *evaluateHumidityState(const DhtReading &dht, const RackThresholds &thresholds) {
  if (!dht.ok) {
    return "sensor_error";
  }
  if (dht.humidityPct <= thresholds.humLowCritical) {
    return "critical_low";
  }
  if (dht.humidityPct >= thresholds.humHighCritical) {
    return "critical_high";
  }
  if (dht.humidityPct <= thresholds.humLowWarning) {
    return "warning_low";
  }
  if (dht.humidityPct >= thresholds.humHighWarning) {
    return "warning_high";
  }
  return "optimal";
}

const char *evaluatePositionState(const MpuReading &mpu, const RackThresholds &thresholds) {
  if (!mpu.ok) {
    return "sensor_error";
  }
  if (mpu.tiltDeg >= thresholds.tiltCriticalDeg) {
    return "tilt_critical";
  }
  if (mpu.vibrationG >= thresholds.vibrationCriticalG) {
    return "vibration_critical";
  }
  if (mpu.tiltDeg >= thresholds.tiltWarningDeg) {
    return "tilt_warning";
  }
  if (mpu.vibrationG >= thresholds.vibrationWarningG) {
    return "vibration_warning";
  }
  if (strcmp(mpu.position, "vertical normal") != 0) {
    return "not_vertical";
  }
  return "optimal";
}

DoorSecurityState inferDoorSecurityState(const UltrasonicReading &ultrasonic) {
  return ultrasonic.doorState == DoorState::Open
           ? DoorSecurityState::DoorOpenForced
           : DoorSecurityState::DoorSecure;
}

const char *evaluateDoorComponentState(DoorSecurityState doorSecurityState) {
  switch (doorSecurityState) {
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

bool isDoorCritical(DoorSecurityState doorSecurityState) {
  return doorSecurityState == DoorSecurityState::DoorOpenForced ||
         doorSecurityState == DoorSecurityState::DoorLockError;
}

bool isDoorWarning(DoorSecurityState doorSecurityState) {
  return doorSecurityState == DoorSecurityState::DoorUnlockedClosed ||
         doorSecurityState == DoorSecurityState::DoorOpenAuthorized;
}

const char *doorReason(DoorSecurityState doorSecurityState) {
  switch (doorSecurityState) {
    case DoorSecurityState::DoorUnlockedClosed:
      return "door_unlocked_closed";
    case DoorSecurityState::DoorOpenAuthorized:
      return "door_open_authorized";
    case DoorSecurityState::DoorOpenForced:
      return "door_open_forced";
    case DoorSecurityState::DoorLockError:
      return "door_lock_error";
    case DoorSecurityState::DoorSecure:
      return "all_conditions_optimal";
  }

  return "unknown";
}

}  // namespace

RackStatusDetails evaluateRackStatusDetails(const SensorReadings &readings, bool networkConnected) {
  return evaluateRackStatusDetails(readings, networkConnected, inferDoorSecurityState(readings.ultrasonic));
}

RackStatusDetails evaluateRackStatusDetails(
  const SensorReadings &readings,
  bool networkConnected,
  DoorSecurityState doorSecurityState
) {
  const RackThresholds &thresholds = getActiveControlModeProfile().thresholds;

  RackStatusDetails details;
  details.temperatureState = evaluateTemperatureState(readings.dht, thresholds);
  details.humidityState = evaluateHumidityState(readings.dht, thresholds);
  details.positionState = evaluatePositionState(readings.mpu, thresholds);
  details.doorState = evaluateDoorComponentState(doorSecurityState);

  if (!networkConnected) {
    details.status = RackStatus::Offline;
    details.reason = "network_disconnected";
    return details;
  }

  if (!readings.dht.ok || !readings.mpu.ok) {
    details.status = RackStatus::SensorError;
    details.reason = !readings.dht.ok ? "dht_sensor_error" : "mpu_sensor_error";
    return details;
  }

  const bool doorCritical = isDoorCritical(doorSecurityState);
  const bool critical = doorCritical ||
                        readings.dht.temperatureC >= thresholds.tempCriticalC ||
                        readings.dht.humidityPct <= thresholds.humLowCritical ||
                        readings.dht.humidityPct >= thresholds.humHighCritical ||
                        readings.mpu.tiltDeg >= thresholds.tiltCriticalDeg ||
                        readings.mpu.vibrationG >= thresholds.vibrationCriticalG;

  if (critical) {
    details.status = RackStatus::Critical;
    if (doorCritical) {
      details.reason = doorReason(doorSecurityState);
    } else if (strcmp(details.temperatureState, "critical_high") == 0) {
      details.reason = "temperature_critical_high";
    } else if (strncmp(details.humidityState, "critical", 8) == 0) {
      details.reason = details.humidityState;
    } else if (strcmp(details.positionState, "tilt_critical") == 0) {
      details.reason = "tilt_critical";
    } else {
      details.reason = "vibration_critical";
    }
    return details;
  }

  const bool doorWarning = isDoorWarning(doorSecurityState);
  const bool warning = doorWarning ||
                       readings.dht.temperatureC > thresholds.tempOptimalMaxC ||
                       readings.dht.humidityPct <= thresholds.humLowWarning ||
                       readings.dht.humidityPct >= thresholds.humHighWarning ||
                       readings.mpu.tiltDeg >= thresholds.tiltWarningDeg ||
                       readings.mpu.vibrationG >= thresholds.vibrationWarningG;

  if (warning) {
    details.status = RackStatus::Warning;
    if (doorWarning) {
      details.reason = doorReason(doorSecurityState);
    } else if (strcmp(details.temperatureState, "warning_high") == 0) {
      details.reason = "temperature_warning_high";
    } else if (strncmp(details.humidityState, "warning", 7) == 0) {
      details.reason = details.humidityState;
    } else if (strcmp(details.positionState, "tilt_warning") == 0) {
      details.reason = "tilt_warning";
    } else {
      details.reason = "vibration_warning";
    }
    return details;
  }

  details.status = RackStatus::Optimal;
  details.reason = "all_conditions_optimal";
  return details;
}

RackStatus evaluateRackStatus(const SensorReadings &readings, bool networkConnected) {
  return evaluateRackStatusDetails(readings, networkConnected).status;
}

RackStatus evaluateRackStatus(const SensorReadings &readings, bool networkConnected, DoorSecurityState doorSecurityState) {
  return evaluateRackStatusDetails(readings, networkConnected, doorSecurityState).status;
}

const char *rackStatusToText(RackStatus status) {
  switch (status) {
    case RackStatus::Connecting:
      return "connecting";
    case RackStatus::Offline:
      return "offline";
    case RackStatus::Optimal:
      return "optimal";
    case RackStatus::Warning:
      return "warning";
    case RackStatus::Critical:
      return "critical";
    case RackStatus::SensorError:
      return "sensor_error";
  }

  return "unknown";
}

int rackStatusToCode(RackStatus status) {
  switch (status) {
    case RackStatus::Connecting:
      return 0;
    case RackStatus::Offline:
      return 1;
    case RackStatus::Optimal:
      return 2;
    case RackStatus::Warning:
      return 3;
    case RackStatus::Critical:
      return 4;
    case RackStatus::SensorError:
      return 5;
  }

  return -1;
}
