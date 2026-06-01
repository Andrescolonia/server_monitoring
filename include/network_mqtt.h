#ifndef NETWORK_MQTT_H
#define NETWORK_MQTT_H

#include <Arduino.h>

#include "actuators.h"
#include "rack_status.h"
#include "sensors.h"

void initNetworkMqtt();
void maintainNetworkMqtt();
bool isWifiConnected();
bool isMqttConnected();
bool publishRackData(const SensorReadings &readings, const RackStatusDetails &details);
bool publishControlData(const ActuatorState &actuators);

#endif
