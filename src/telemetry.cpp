#include "telemetry.h"

#include "config.h"

namespace {

void appendJsonFloat(String &json, const char *key, bool ok, float value, uint8_t decimals) {
  json += "\"";
  json += key;
  json += "\":";

  if (!ok || isnan(value)) {
    json += "null";
    return;
  }

  json += String(value, static_cast<unsigned int>(decimals));
}

}  // namespace

String buildTelemetryJson(const SensorReadings &readings, const RackStatusDetails &details) {
  return buildTelemetryJson(readings, details, getActuatorState());
}

String buildTelemetryJson(const SensorReadings &readings, const RackStatusDetails &details, const ActuatorState &actuators) {
  String json;
  json.reserve(760);

  json += "{";
  json += "\"device_id\":\"";
  json += DEVICE_ID;
  json += "\",";
  json += "\"control_mode\":\"";
  json += controlModeToText(actuators.controlMode);
  json += "\",";
  appendJsonFloat(json, "temperature_c", readings.dht.ok, readings.dht.temperatureC, 1);
  json += ",";
  appendJsonFloat(json, "humidity_pct", readings.dht.ok, readings.dht.humidityPct, 1);
  json += ",\"door_state\":\"";
  json += doorStateToText(readings.ultrasonic.doorState);
  json += "\"";
  json += ",";
  appendJsonFloat(json, "accel_x", readings.mpu.ok, readings.mpu.accelX, 3);
  json += ",";
  appendJsonFloat(json, "accel_y", readings.mpu.ok, readings.mpu.accelY, 3);
  json += ",";
  appendJsonFloat(json, "accel_z", readings.mpu.ok, readings.mpu.accelZ, 3);
  json += ",";
  appendJsonFloat(json, "tilt_deg", readings.mpu.ok, readings.mpu.tiltDeg, 1);
  json += ",";
  appendJsonFloat(json, "vibration_level", readings.mpu.ok, readings.mpu.vibrationG, 3);
  json += ",";
  appendJsonFloat(json, "pitch_deg", readings.mpu.ok, readings.mpu.pitchDeg, 1);
  json += ",";
  appendJsonFloat(json, "roll_deg", readings.mpu.ok, readings.mpu.rollDeg, 1);
  json += ",\"mpu_position\":";
  if (readings.mpu.ok) {
    json += "\"";
    json += readings.mpu.position;
    json += "\"";
  } else {
    json += "null";
  }
  json += ",\"status\":\"";
  json += rackStatusToText(details.status);
  json += "\",\"status_code\":";
  json += rackStatusToCode(details.status);
  json += ",\"status_reason\":\"";
  json += details.reason;
  json += "\",\"temperature_state\":\"";
  json += details.temperatureState;
  json += "\",\"humidity_state\":\"";
  json += details.humidityState;
  json += "\",\"position_state\":\"";
  json += details.positionState;
  json += "\",\"door_component_state\":\"";
  json += details.doorState;
  json += "\"";
  json += ",\"fan_mode\":\"";
  json += fanModeToText(actuators.fanMode);
  json += "\",\"fan_power_pct\":";
  json += actuators.fanPowerPct;
  json += ",\"door_lock_state\":\"";
  json += doorLockStateToText(actuators.doorLockState);
  json += "\",\"door_security_state\":\"";
  json += doorSecurityStateToText(actuators.doorSecurityState);
  json += "\",\"access_event\":\"";
  json += accessEventToText(actuators.lastAccessEvent);
  json += "\",\"actuators_simulated\":";
  json += actuators.simulated ? "true" : "false";
  json += ",\"uptime_ms\":";
  json += millis();
  json += "}";

  return json;
}

void printHumanDiagnostic(const SensorReadings &readings, const RackStatusDetails &details) {
  printHumanDiagnostic(readings, details, getActuatorState());
}

void printHumanDiagnostic(const SensorReadings &readings, const RackStatusDetails &details, const ActuatorState &actuators) {
  Serial.println(F("--------------------------------------------------"));
  Serial.print(F("Estado general: "));
  Serial.print(rackStatusToText(details.status));
  Serial.print(F(" | razon="));
  Serial.println(details.reason);
  Serial.print(F("Componentes: temp="));
  Serial.print(details.temperatureState);
  Serial.print(F(", humedad="));
  Serial.print(details.humidityState);
  Serial.print(F(", posicion="));
  Serial.print(details.positionState);
  Serial.print(F(", puerta="));
  Serial.println(details.doorState);
  printActuatorDiagnostic(actuators);

  Serial.print(F("DHT: "));
  if (readings.dht.ok) {
    Serial.print(readings.dht.temperatureC, 1);
    Serial.print(F(" C, "));
    Serial.print(readings.dht.humidityPct, 1);
    Serial.println(F(" %HR"));
  } else {
    Serial.println(F("error de lectura"));
  }

  Serial.print(F("Puerta: "));
  Serial.print(doorStateToText(readings.ultrasonic.doorState));
  Serial.print(F(" | Ultrasonico: "));
  if (readings.ultrasonic.ok) {
    Serial.print(readings.ultrasonic.distanceCm, 1);
    Serial.print(F(" cm, echo="));
    Serial.print(readings.ultrasonic.echoDurationUs);
    Serial.println(F(" us"));
  } else {
    Serial.println(F("sin eco o fuera de rango"));
  }

  Serial.print(F("MPU6050: "));
  if (readings.mpu.ok) {
    Serial.print(F("ax="));
    Serial.print(readings.mpu.accelX, 3);
    Serial.print(F("g, ay="));
    Serial.print(readings.mpu.accelY, 3);
    Serial.print(F("g, az="));
    Serial.print(readings.mpu.accelZ, 3);
    Serial.print(F("g, tilt="));
    Serial.print(readings.mpu.tiltDeg, 1);
    Serial.print(F(" deg, vibracion="));
    Serial.print(readings.mpu.vibrationG, 3);
    Serial.print(F("g, pitch="));
    Serial.print(readings.mpu.pitchDeg, 1);
    Serial.print(F(" deg, roll="));
    Serial.print(readings.mpu.rollDeg, 1);
    Serial.print(F(" deg, posicion="));
    Serial.println(readings.mpu.position);
  } else {
    Serial.println(F("error de lectura o sensor no detectado"));
  }
}
