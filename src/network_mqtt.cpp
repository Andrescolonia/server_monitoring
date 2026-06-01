#include "network_mqtt.h"

#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <string.h>

#include "actuators.h"
#include "config.h"
#include "control_modes.h"
#include "secrets.h"
#include "status_led.h"

namespace {

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

uint32_t lastReconnectAttemptMs = 0;

bool isPlaceholder(const char *value, const char *placeholder) {
  return value == nullptr || value[0] == '\0' || strcmp(value, placeholder) == 0;
}

bool hasNetworkCredentials() {
  return !isPlaceholder(WIFI_SSID, "TU_WIFI") &&
         !isPlaceholder(WIFI_PASSWORD, "TU_PASSWORD_WIFI") &&
         !isPlaceholder(MQTT_HOST, "TU_HOST_HIVEMQ") &&
         !isPlaceholder(MQTT_USER, "TU_USUARIO_HIVEMQ") &&
         !isPlaceholder(MQTT_PASSWORD, "TU_PASSWORD_HIVEMQ");
}

bool connectWifiBlocking() {
  if (!hasNetworkCredentials()) {
    Serial.println(F("WiFi/MQTT: credenciales pendientes en include/secrets.h"));
    return false;
  }

  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.print(F("WiFi: conectando a "));
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    updateStatusLed(RackStatus::Connecting);
    delay(250);
    Serial.print(F("."));
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("WiFi: no conectado"));
    return false;
  }

  Serial.print(F("WiFi: conectado, IP "));
  Serial.println(WiFi.localIP());
  return true;
}

void subscribeCommandTopics() {
  bool ok = true;
  ok &= mqttClient.subscribe(MQTT_TOPIC_COMMAND_MODE_SET);
  ok &= mqttClient.subscribe(MQTT_TOPIC_COMMAND_DOOR_UNLOCK);
  ok &= mqttClient.subscribe(MQTT_TOPIC_COMMAND_DOOR_LOCK);
  ok &= mqttClient.subscribe(MQTT_TOPIC_COMMAND_FAN_AUTO);
  ok &= mqttClient.subscribe(MQTT_TOPIC_COMMAND_FAN_MANUAL);
  if (!ok) {
    Serial.println(F("MQTT: error suscribiendo algun topico de comandos"));
    return;
  }

  Serial.println(F("MQTT: suscrito a topicos de comandos"));
}

bool connectMqttBlocking() {
  if (!hasNetworkCredentials()) {
    return false;
  }

  if (mqttClient.connected()) {
    return true;
  }

  if (!connectWifiBlocking()) {
    return false;
  }

  Serial.print(F("MQTT: conectando a "));
  Serial.print(MQTT_HOST);
  Serial.print(F(":"));
  Serial.println(MQTT_PORT);

  const uint32_t startMs = millis();
  while (!mqttClient.connected() && millis() - startMs < MQTT_CONNECT_TIMEOUT_MS) {
    updateStatusLed(RackStatus::Connecting);
    if (mqttClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASSWORD, MQTT_TOPIC_STATUS_LAST_WILL, 1, true, "offline_lost_signal_or_power")) {
      mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);
      mqttClient.publish(MQTT_TOPIC_STATUS_NETWORK, "online", true);
      mqttClient.publish(MQTT_TOPIC_STATUS_LAST_WILL, "online", true);
      subscribeCommandTopics();
      Serial.println(F("MQTT: conectado"));
      return true;
    }

    Serial.print(F("MQTT: fallo rc="));
    Serial.println(mqttClient.state());
    delay(1000);
  }

  Serial.println(F("MQTT: no conectado"));
  return false;
}

bool publishText(const char *topic, const char *payload, bool retained = true) {
  if (!mqttClient.connected()) {
    return false;
  }

  return mqttClient.publish(topic, payload, retained);
}

bool publishFloatValue(const char *topic, bool ok, float value, uint8_t decimals) {
  if (!ok || isnan(value)) {
    return publishText(topic, "null");
  }

  const String payload = String(value, static_cast<unsigned int>(decimals));
  return publishText(topic, payload.c_str());
}

void handleMqttCommand(char *topic, byte *payload, unsigned int length) {
  String message;
  message.reserve(length + 1);
  for (unsigned int index = 0; index < length; index++) {
    message += static_cast<char>(payload[index]);
  }
  message.trim();

  Serial.print(F("MQTT COMMAND: "));
  Serial.print(topic);
  Serial.print(F(" -> "));
  Serial.println(message);

  if (strcmp(topic, MQTT_TOPIC_COMMAND_MODE_SET) == 0) {
    if (setActiveControlModeFromText(message.c_str())) {
      publishText(MQTT_TOPIC_CONTROL_MODE, controlModeToText(getActiveControlMode()));
    } else {
      Serial.println(F("MQTT COMMAND: modo no valido"));
    }
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_COMMAND_DOOR_UNLOCK) == 0) {
    if (message == "unlock") {
      requestDoorUnlock();
      publishControlData(getActuatorState());
    }
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_COMMAND_DOOR_LOCK) == 0) {
    if (message == "lock") {
      requestDoorLock();
      publishControlData(getActuatorState());
    }
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_COMMAND_FAN_AUTO) == 0) {
    if (message == "auto") {
      setFanAuto();
      publishControlData(getActuatorState());
    }
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_COMMAND_FAN_MANUAL) == 0) {
    const int powerPct = message.toInt();
    setFanManual(static_cast<uint8_t>(constrain(powerPct, 0, 100)));
    publishControlData(getActuatorState());
    return;
  }
}

}  // namespace

void initNetworkMqtt() {
  if (MQTT_ALLOW_INSECURE_TLS) {
    wifiClient.setInsecure();
  }

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
  mqttClient.setCallback(handleMqttCommand);

  connectWifiBlocking();
  connectMqttBlocking();
}

void maintainNetworkMqtt() {
  if (!hasNetworkCredentials()) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
    mqttClient.loop();
    return;
  }

  const uint32_t now = millis();
  if (now - lastReconnectAttemptMs < NETWORK_RECONNECT_INTERVAL_MS) {
    return;
  }

  lastReconnectAttemptMs = now;
  connectMqttBlocking();
}

bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool isMqttConnected() {
  return mqttClient.connected();
}

bool publishRackData(const SensorReadings &readings, const RackStatusDetails &details) {
  if (!mqttClient.connected()) {
    return false;
  }

  bool ok = true;
  ok &= publishText(MQTT_TOPIC_STATUS_SYSTEM, rackStatusToText(details.status));
  ok &= publishText(MQTT_TOPIC_STATUS_NETWORK, isWifiConnected() && isMqttConnected() ? "online" : "offline");
  ok &= publishText(MQTT_TOPIC_STATUS_REASON, details.reason);
  ok &= publishText(MQTT_TOPIC_STATUS_TEMPERATURE, details.temperatureState);
  ok &= publishText(MQTT_TOPIC_STATUS_HUMIDITY, details.humidityState);
  ok &= publishText(MQTT_TOPIC_STATUS_POSITION, details.positionState);
  ok &= publishText(MQTT_TOPIC_STATUS_DOOR, details.doorState);
  ok &= publishFloatValue(MQTT_TOPIC_TEMPERATURE, readings.dht.ok, readings.dht.temperatureC, 1);
  ok &= publishFloatValue(MQTT_TOPIC_HUMIDITY, readings.dht.ok, readings.dht.humidityPct, 1);
  ok &= publishFloatValue(MQTT_TOPIC_MPU_ACCEL_X, readings.mpu.ok, readings.mpu.accelX, 3);
  ok &= publishFloatValue(MQTT_TOPIC_MPU_ACCEL_Y, readings.mpu.ok, readings.mpu.accelY, 3);
  ok &= publishFloatValue(MQTT_TOPIC_MPU_ACCEL_Z, readings.mpu.ok, readings.mpu.accelZ, 3);
  ok &= publishFloatValue(MQTT_TOPIC_MPU_TILT, readings.mpu.ok, readings.mpu.tiltDeg, 1);
  ok &= publishFloatValue(MQTT_TOPIC_MPU_VIBRATION, readings.mpu.ok, readings.mpu.vibrationG, 3);
  ok &= publishFloatValue(MQTT_TOPIC_MPU_PITCH, readings.mpu.ok, readings.mpu.pitchDeg, 1);
  ok &= publishFloatValue(MQTT_TOPIC_MPU_ROLL, readings.mpu.ok, readings.mpu.rollDeg, 1);
  ok &= publishText(MQTT_TOPIC_MPU_POSITION, readings.mpu.ok ? readings.mpu.position : "unknown");
  ok &= publishText(MQTT_TOPIC_DOOR_STATE, doorStateToText(readings.ultrasonic.doorState));

  return ok;
}

bool publishControlData(const ActuatorState &actuators) {
  if (!mqttClient.connected()) {
    return false;
  }

  const String fanPower = String(actuators.fanPowerPct);
  bool ok = true;
  ok &= publishText(MQTT_TOPIC_CONTROL_MODE, controlModeToText(actuators.controlMode));
  ok &= publishText(MQTT_TOPIC_ACTUATOR_FAN_POWER, fanPower.c_str());
  ok &= publishText(MQTT_TOPIC_ACTUATOR_FAN_MODE, fanModeToText(actuators.fanMode));
  ok &= publishText(MQTT_TOPIC_ACTUATOR_DOOR_LOCK, doorLockStateToText(actuators.doorLockState));
  ok &= publishText(MQTT_TOPIC_SECURITY_DOOR, doorSecurityStateToText(actuators.doorSecurityState));
  ok &= publishText(MQTT_TOPIC_SECURITY_ACCESS, accessEventToText(actuators.lastAccessEvent));

  return ok;
}
