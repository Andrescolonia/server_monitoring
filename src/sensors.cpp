#include "sensors.h"

#include <DHT.h>
#include <Wire.h>

#include "config.h"

namespace {

float lastAccelMagnitude = NAN;
bool mpuAvailable = false;
constexpr float MPU_POSITION_AXIS_THRESHOLD_G = 0.75F;
DHT dhtSensor(PIN_DHT, DHT_TYPE);

DhtReading readDht() {
  DhtReading reading;

  const float humidity = dhtSensor.readHumidity();
  const float temperature = dhtSensor.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    return reading;
  }

  reading.temperatureC = temperature;
  reading.humidityPct = humidity;
  reading.temperatureC += DHT_TEMPERATURE_OFFSET_C;
  reading.humidityPct = constrain(reading.humidityPct + DHT_HUMIDITY_OFFSET_PCT, 0.0F, 100.0F);
  reading.ok = true;
  return reading;
}

UltrasonicReading readUltrasonic() {
  UltrasonicReading reading;

  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

  reading.echoDurationUs = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (reading.echoDurationUs == 0) {
    return reading;
  }

  reading.distanceCm = (reading.echoDurationUs * SPEED_OF_SOUND_CM_PER_US) / 2.0F;
  reading.doorState = reading.distanceCm <= DOOR_CLOSED_MAX_CM ? DoorState::Closed : DoorState::Open;
  reading.ok = true;
  return reading;
}

void writeMpuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readMpuRegister(uint8_t reg) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return 0;
  }

  Wire.requestFrom(MPU6050_ADDRESS, static_cast<uint8_t>(1));
  if (Wire.available() < 1) {
    return 0;
  }

  return Wire.read();
}

bool initMpu6050() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  delay(100);

  const uint8_t whoAmI = readMpuRegister(0x75);
  if (whoAmI != 0x68 && whoAmI != 0x70) {
    return false;
  }

  writeMpuRegister(0x6B, 0x00);  // Wake up.
  writeMpuRegister(0x1C, 0x00);  // Accelerometer +/- 2g.
  delay(100);
  return true;
}

bool readMpuRawAccel(int16_t &rawX, int16_t &rawY, int16_t &rawZ) {
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t requestedBytes = 6;
  Wire.requestFrom(MPU6050_ADDRESS, requestedBytes);
  if (Wire.available() < requestedBytes) {
    return false;
  }

  rawX = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  rawY = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  rawZ = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  return true;
}

float calculateMpuPitchDeg(float accelX, float accelY, float accelZ) {
  return atan2(-accelX, sqrt((accelY * accelY) + (accelZ * accelZ))) * 180.0F / PI;
}

float calculateMpuRollDeg(float accelY, float accelZ) {
  return atan2(accelY, accelZ) * 180.0F / PI;
}

float calculateMpuTiltDeg(float accelX, float accelY, float accelZ) {
  float normalX = 0.0F;
  float normalY = 0.0F;
  float normalZ = 0.0F;

  if (MPU_NORMAL_AXIS == 'X') {
    normalX = MPU_NORMAL_SIGN;
  } else if (MPU_NORMAL_AXIS == 'Y') {
    normalY = MPU_NORMAL_SIGN;
  } else {
    normalZ = MPU_NORMAL_SIGN;
  }

  const float accelMagnitude = sqrt((accelX * accelX) + (accelY * accelY) + (accelZ * accelZ));
  if (accelMagnitude <= 0.0F) {
    return NAN;
  }

  const float dot = (accelX * normalX) + (accelY * normalY) + (accelZ * normalZ);
  const float normalizedDot = constrain(dot / accelMagnitude, -1.0F, 1.0F);
  return acos(normalizedDot) * 180.0F / PI;
}

const char *estimateMpuPosition(float accelX, float accelY, float accelZ) {
  const float absX = fabs(accelX);
  const float absY = fabs(accelY);
  const float absZ = fabs(accelZ);

  if (absZ >= absX && absZ >= absY && absZ >= MPU_POSITION_AXIS_THRESHOLD_G) {
    if (MPU_NORMAL_AXIS == 'Z') {
      return (accelZ * MPU_NORMAL_SIGN) >= 0.0F ? "vertical normal" : "vertical invertido";
    }

    return accelZ >= 0.0F ? "costado Z positivo" : "costado Z negativo";
  }

  if (absX >= absY && absX >= absZ && absX >= MPU_POSITION_AXIS_THRESHOLD_G) {
    if (MPU_NORMAL_AXIS == 'X') {
      return (accelX * MPU_NORMAL_SIGN) >= 0.0F ? "vertical normal" : "vertical invertido";
    }

    return accelX >= 0.0F ? "costado X negativo" : "costado X positivo";
  }

  if (absY >= absX && absY >= absZ && absY >= MPU_POSITION_AXIS_THRESHOLD_G) {
    if (MPU_NORMAL_AXIS == 'Y') {
      return (accelY * MPU_NORMAL_SIGN) >= 0.0F ? "vertical normal" : "vertical invertido";
    }

    return accelY >= 0.0F ? "costado Y positivo" : "costado Y negativo";
  }

  return "inclinado / transicion";
}

MpuReading readMpu() {
  MpuReading reading;
  int16_t rawX = 0;
  int16_t rawY = 0;
  int16_t rawZ = 0;

  if (!mpuAvailable || !readMpuRawAccel(rawX, rawY, rawZ)) {
    return reading;
  }

  reading.accelX = rawX / 16384.0F;
  reading.accelY = rawY / 16384.0F;
  reading.accelZ = rawZ / 16384.0F;

  reading.tiltDeg = calculateMpuTiltDeg(reading.accelX, reading.accelY, reading.accelZ);
  reading.pitchDeg = calculateMpuPitchDeg(reading.accelX, reading.accelY, reading.accelZ);
  reading.rollDeg = calculateMpuRollDeg(reading.accelY, reading.accelZ);
  reading.position = estimateMpuPosition(reading.accelX, reading.accelY, reading.accelZ);

  const float accelMagnitude = sqrt((reading.accelX * reading.accelX) +
                                    (reading.accelY * reading.accelY) +
                                    (reading.accelZ * reading.accelZ));
  reading.vibrationG = isnan(lastAccelMagnitude) ? 0.0F : fabs(accelMagnitude - lastAccelMagnitude);
  lastAccelMagnitude = accelMagnitude;
  reading.ok = true;
  return reading;
}

}  // namespace

void initSensors() {
  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  dhtSensor.begin();

  mpuAvailable = initMpu6050();
}

SensorReadings readSensors() {
  SensorReadings readings;
  readings.dht = readDht();
  readings.ultrasonic = readUltrasonic();
  readings.mpu = readMpu();
  return readings;
}

DhtReading readDhtSensor() {
  return readDht();
}

MpuReading readMpuSensor() {
  return readMpu();
}

bool isMpuDetected() {
  return mpuAvailable;
}

const char *doorStateToText(DoorState state) {
  switch (state) {
    case DoorState::Closed:
      return "closed";
    case DoorState::Open:
      return "open";
  }

  return "unknown";
}
