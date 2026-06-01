#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

#include "actuators.h"
#include "rack_status.h"
#include "sensors.h"

String buildTelemetryJson(const SensorReadings &readings, const RackStatusDetails &details);
String buildTelemetryJson(const SensorReadings &readings, const RackStatusDetails &details, const ActuatorState &actuators);
void printHumanDiagnostic(const SensorReadings &readings, const RackStatusDetails &details);
void printHumanDiagnostic(const SensorReadings &readings, const RackStatusDetails &details, const ActuatorState &actuators);

#endif
