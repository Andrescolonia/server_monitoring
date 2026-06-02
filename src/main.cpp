#include <Arduino.h>

#include "config.h"

#if ENABLE_SERVO_TEST

constexpr uint32_t SERVO_PERIOD_US = 1000000UL / SERVO_PWM_FREQ_HZ;
constexpr uint32_t SERVO_MAX_DUTY = (1UL << SERVO_PWM_RESOLUTION_BITS) - 1;

uint32_t lastServoTestMs = 0;
bool servoTestClosed = true;

uint16_t angleToPulseUs(uint8_t angleDeg) {
  const uint8_t safeAngle = constrain(angleDeg, 0, 180);
  return SERVO_MIN_PULSE_US +
         ((static_cast<uint32_t>(SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * safeAngle) / 180);
}

void writeServoState(const char *state, uint8_t angleDeg) {
  const uint16_t pulseUs = angleToPulseUs(angleDeg);
  const uint32_t duty = (static_cast<uint32_t>(pulseUs) * SERVO_MAX_DUTY) / SERVO_PERIOD_US;

  ledcWrite(SERVO_PWM_CHANNEL, duty);

  Serial.print(F("Servo bloqueo -> estado: "));
  Serial.print(state);
  Serial.print(F(" | angulo: "));
  Serial.print(angleDeg);
  Serial.print(F(" deg | pulso: "));
  Serial.print(pulseUs);
  Serial.println(F(" us"));
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  ledcSetup(SERVO_PWM_CHANNEL, SERVO_PWM_FREQ_HZ, SERVO_PWM_RESOLUTION_BITS);
  ledcAttachPin(PIN_SERVO_LOCK, SERVO_PWM_CHANNEL);

  Serial.println(F("Prueba servo de bloqueo"));
  Serial.println(F("Senal: GPIO 14 | VCC servo: 5V externo | GND servo: GND comun"));
  Serial.println(F("Alternando entre CERRADO y ABIERTO cada 3 segundos."));
  Serial.println(F("CERRADO: brazo horizontal como en la foto."));
  Serial.println(F("ABIERTO: brazo hacia arriba. Si queda al lado contrario, cambiar SERVO_LOCK_OPEN_ANGLE a 180."));

  writeServoState("cerrado", SERVO_LOCK_CLOSED_ANGLE);
}

void loop() {
  const uint32_t now = millis();
  if (now - lastServoTestMs < SERVO_LOCK_TEST_STEP_MS) {
    return;
  }

  lastServoTestMs = now;
  servoTestClosed = !servoTestClosed;

  if (servoTestClosed) {
    writeServoState("cerrado", SERVO_LOCK_CLOSED_ANGLE);
    return;
  }

  writeServoState("abierto", SERVO_LOCK_OPEN_ANGLE);
}

#elif ENABLE_MPU_TEST

#include "sensors.h"

constexpr uint32_t MPU_TEST_INTERVAL_MS = 2000;
constexpr float MPU_POSITION_AXIS_THRESHOLD_G = 0.75F;

uint32_t lastMpuTestMs = 0;

float calculatePitchDeg(const MpuReading &reading) {
  return atan2(-reading.accelX, sqrt((reading.accelY * reading.accelY) +
                                     (reading.accelZ * reading.accelZ))) *
         180.0F / PI;
}

float calculateRollDeg(const MpuReading &reading) {
  return atan2(reading.accelY, reading.accelZ) * 180.0F / PI;
}

const char *estimatePosition(const MpuReading &reading) {
  const float absX = fabs(reading.accelX);
  const float absY = fabs(reading.accelY);
  const float absZ = fabs(reading.accelZ);

  if (absZ >= absX && absZ >= absY && absZ >= MPU_POSITION_AXIS_THRESHOLD_G) {
    if (MPU_NORMAL_AXIS == 'Z') {
      return (reading.accelZ * MPU_NORMAL_SIGN) >= 0.0F ? "vertical normal" : "vertical invertido";
    }

    return reading.accelZ >= 0.0F ? "costado Z positivo" : "costado Z negativo";
  }

  if (absX >= absY && absX >= absZ && absX >= MPU_POSITION_AXIS_THRESHOLD_G) {
    if (MPU_NORMAL_AXIS == 'X') {
      return (reading.accelX * MPU_NORMAL_SIGN) >= 0.0F ? "vertical normal" : "vertical invertido";
    }

    return reading.accelX >= 0.0F ? "costado X negativo" : "costado X positivo";
  }

  if (absY >= absX && absY >= absZ && absY >= MPU_POSITION_AXIS_THRESHOLD_G) {
    if (MPU_NORMAL_AXIS == 'Y') {
      return (reading.accelY * MPU_NORMAL_SIGN) >= 0.0F ? "vertical normal" : "vertical invertido";
    }

    return reading.accelY >= 0.0F ? "costado Y positivo" : "costado Y negativo";
  }

  return "inclinado / transicion";
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  initSensors();

  Serial.println(F("Prueba MPU6050"));
  Serial.println(F("SDA: GPIO 21 | SCL: GPIO 22 | VCC: 3V3 | GND: GND"));
  Serial.print(F("MPU6050: "));
  Serial.println(isMpuDetected() ? F("detectado") : F("no detectado"));
  Serial.println(F("Leyendo aceleracion, inclinacion y vibracion cada 2 segundos..."));
  Serial.println(F("La posicion se estima segun el eje dominante X/Y/Z."));
}

void loop() {
  const uint32_t now = millis();
  if (now - lastMpuTestMs < MPU_TEST_INTERVAL_MS) {
    return;
  }

  lastMpuTestMs = now;

  const MpuReading reading = readMpuSensor();
  if (!reading.ok) {
    Serial.println(F("Error leyendo MPU6050"));
    return;
  }

  Serial.print(F("Accel X: "));
  Serial.print(reading.accelX, 3);
  Serial.print(F(" g | Y: "));
  Serial.print(reading.accelY, 3);
  Serial.print(F(" g | Z: "));
  Serial.print(reading.accelZ, 3);
  Serial.print(F(" g | Inclinacion: "));
  Serial.print(reading.tiltDeg, 1);
  Serial.print(F(" deg | Vibracion: "));
  Serial.print(reading.vibrationG, 3);
  Serial.print(F(" g | Pitch: "));
  Serial.print(calculatePitchDeg(reading), 1);
  Serial.print(F(" deg | Roll: "));
  Serial.print(calculateRollDeg(reading), 1);
  Serial.print(F(" deg | Posicion: "));
  Serial.println(estimatePosition(reading));
}

#elif ENABLE_DHT_TEST

#include "sensors.h"

constexpr uint32_t DHT_TEST_INTERVAL_MS = 2000;

uint32_t lastDhtTestMs = 0;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  pinMode(PIN_DHT, INPUT_PULLUP);

  Serial.println(F("Prueba DHT22"));
  Serial.println(F("DATA: GPIO 4 | VCC: 3V3 | GND: GND"));
  Serial.println(F("Leyendo temperatura y humedad cada 2 segundos..."));
}

void loop() {
  const uint32_t now = millis();
  if (now - lastDhtTestMs < DHT_TEST_INTERVAL_MS) {
    return;
  }

  lastDhtTestMs = now;

  const DhtReading reading = readDhtSensor();
  if (!reading.ok) {
    Serial.println(F("Error leyendo DHT22"));
    return;
  }

  Serial.print(F("Temperatura: "));
  Serial.print(reading.temperatureC, 1);
  Serial.print(F(" C | Humedad: "));
  Serial.print(reading.humidityPct, 1);
  Serial.println(F(" %"));
}

#elif ENABLE_ULTRASONIC_TEST

constexpr uint32_t ULTRASONIC_TEST_INTERVAL_MS = 2000;

uint32_t lastUltrasonicTestMs = 0;

float readDistanceCm(uint32_t &echoDurationUs) {
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

  echoDurationUs = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (echoDurationUs == 0) {
    return NAN;
  }

  return (echoDurationUs * SPEED_OF_SOUND_CM_PER_US) / 2.0F;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

  Serial.println(F("Prueba HC-SR04"));
  Serial.println(F("TRIG: GPIO 32 | ECHO: GPIO 34 con divisor de voltaje"));
}

void loop() {
  const uint32_t now = millis();
  if (now - lastUltrasonicTestMs < ULTRASONIC_TEST_INTERVAL_MS) {
    return;
  }

  lastUltrasonicTestMs = now;

  uint32_t echoDurationUs = 0;
  const float distanceCm = readDistanceCm(echoDurationUs);

  if (isnan(distanceCm)) {
    Serial.println(F("Sin eco o fuera de rango"));
    return;
  }

  Serial.print(F("Distancia: "));
  Serial.print(distanceCm, 1);
  Serial.print(F(" cm | Eco: "));
  Serial.print(echoDurationUs);
  Serial.println(F(" us"));
}

#elif ENABLE_LED_GRADIENT_TEST

constexpr uint8_t LED_PWM_CHANNEL_R = 0;
constexpr uint8_t LED_PWM_CHANNEL_G = 1;
constexpr uint8_t LED_PWM_CHANNEL_B = 2;
constexpr uint32_t LED_PWM_FREQ_HZ = 5000;
constexpr uint8_t LED_PWM_RESOLUTION_BITS = 8;
constexpr uint16_t GRADIENT_STEPS = 255;
constexpr uint16_t GRADIENT_STEP_DELAY_MS = 10;

uint8_t outputLevel(uint8_t value) {
  return LED_COMMON_ANODE ? 255 - value : value;
}

uint8_t blendColor(uint8_t from, uint8_t to, uint16_t step) {
  const int32_t delta = static_cast<int32_t>(to) - static_cast<int32_t>(from);
  return static_cast<uint8_t>(from + ((delta * step) / GRADIENT_STEPS));
}

void writeRgb(uint8_t red, uint8_t green, uint8_t blue) {
  ledcWrite(LED_PWM_CHANNEL_R, outputLevel(red));
  ledcWrite(LED_PWM_CHANNEL_G, outputLevel(green));
  ledcWrite(LED_PWM_CHANNEL_B, outputLevel(blue));
}

void fadeBetween(uint8_t startR, uint8_t startG, uint8_t startB,
                 uint8_t endR, uint8_t endG, uint8_t endB) {
  for (uint16_t step = 0; step <= GRADIENT_STEPS; step++) {
    writeRgb(
      blendColor(startR, endR, step),
      blendColor(startG, endG, step),
      blendColor(startB, endB, step)
    );
    delay(GRADIENT_STEP_DELAY_MS);
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  ledcSetup(LED_PWM_CHANNEL_R, LED_PWM_FREQ_HZ, LED_PWM_RESOLUTION_BITS);
  ledcSetup(LED_PWM_CHANNEL_G, LED_PWM_FREQ_HZ, LED_PWM_RESOLUTION_BITS);
  ledcSetup(LED_PWM_CHANNEL_B, LED_PWM_FREQ_HZ, LED_PWM_RESOLUTION_BITS);

  ledcAttachPin(PIN_LED_R, LED_PWM_CHANNEL_R);
  ledcAttachPin(PIN_LED_G, LED_PWM_CHANNEL_G);
  ledcAttachPin(PIN_LED_B, LED_PWM_CHANNEL_B);

  Serial.println(F("Prueba LED RGB - gradiente rojo, verde y azul"));
}

void loop() {
  fadeBetween(255, 0, 0, 0, 255, 0);
  fadeBetween(0, 255, 0, 0, 0, 255);
  fadeBetween(0, 0, 255, 255, 0, 0);
}

#else

#include "actuators.h"
#include "control_modes.h"
#include "rack_status.h"
#include "network_mqtt.h"
#include "sensors.h"
#include "status_led.h"
#include "telemetry.h"

uint32_t lastSensorReadMs = 0;
RackStatus lastRackStatus = RackStatus::Connecting;

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  Serial.println();
  Serial.println(F("Monitor de servidores - firmware integrado"));
  Serial.print(F("Dispositivo: "));
  Serial.println(DEVICE_ID);

  initStatusLed();
  runLedSelfTest();
  updateStatusLed(RackStatus::Connecting);

  initSensors();
  initActuators();
  initNetworkMqtt();

  Serial.print(F("MPU6050: "));
  Serial.println(isMpuDetected() ? F("detectado") : F("no detectado"));
  Serial.print(F("Modo inicial: "));
  Serial.println(controlModeToText(getActiveControlMode()));
  Serial.println(F("Telemetria lista por Monitor Serial y topicos MQTT individuales."));
}

void loop() {
  maintainNetworkMqtt();
  updateStatusLed(isWifiConnected() && isMqttConnected() ? lastRackStatus : RackStatus::Offline);

  const uint32_t now = millis();
  if (now - lastSensorReadMs < TELEMETRY_INTERVAL_MS) {
    return;
  }

  lastSensorReadMs = now;

  const SensorReadings readings = readSensors();
  const bool networkConnected = isWifiConnected() && isMqttConnected();

  updateDoorAccessState(readings);
  const ActuatorState &doorActuators = getActuatorState();
  const RackStatusDetails details = evaluateRackStatusDetails(
    readings,
    networkConnected,
    doorActuators.doorSecurityState
  );
  const RackStatus status = details.status;
  lastRackStatus = status;
  updateActuators(readings, details);
  const ActuatorState &actuators = getActuatorState();

  updateStatusLed(status);
  printHumanDiagnostic(readings, details, actuators);

  const String payload = buildTelemetryJson(readings, details, actuators);
  Serial.println(payload);

  const bool telemetryPublished = publishRackData(readings, details);
  const bool controlPublished = publishControlData(actuators);
  if (telemetryPublished && controlPublished) {
    Serial.println(F("MQTT: topicos de sensores y control publicados"));
  } else {
    Serial.println(F("MQTT: topicos de sensores o control no publicados"));
  }
}

#endif
