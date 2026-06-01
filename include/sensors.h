#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <math.h>

struct DhtReading {
  bool ok = false;
  float temperatureC = NAN;
  float humidityPct = NAN;
};

enum class DoorState {
  Closed,
  Open,
};

struct UltrasonicReading {
  bool ok = false;
  float distanceCm = NAN;
  uint32_t echoDurationUs = 0;
  DoorState doorState = DoorState::Open;
};

struct MpuReading {
  bool ok = false;
  float accelX = NAN;
  float accelY = NAN;
  float accelZ = NAN;
  float tiltDeg = NAN;
  float vibrationG = NAN;
  float pitchDeg = NAN;
  float rollDeg = NAN;
  const char *position = "unknown";
};

struct SensorReadings {
  DhtReading dht;
  UltrasonicReading ultrasonic;
  MpuReading mpu;
};

void initSensors();
SensorReadings readSensors();
DhtReading readDhtSensor();
MpuReading readMpuSensor();
bool isMpuDetected();
const char *doorStateToText(DoorState state);

#endif
