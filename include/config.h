#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Serial
constexpr uint32_t SERIAL_BAUD = 115200;

// Identificacion del prototipo.
constexpr const char *DEVICE_ID = "rack01";

// MQTT / HiveMQ.
constexpr const char *MQTT_TOPIC_STATUS = "monitor-servidores/rack01/status";
constexpr const char *MQTT_TOPIC_STATUS_SYSTEM = "monitor-servidores/rack01/status/system";
constexpr const char *MQTT_TOPIC_STATUS_NETWORK = "monitor-servidores/rack01/status/network";
constexpr const char *MQTT_TOPIC_STATUS_REASON = "monitor-servidores/rack01/status/reason";
constexpr const char *MQTT_TOPIC_STATUS_LAST_WILL = "monitor-servidores/rack01/status/last_will";
constexpr const char *MQTT_TOPIC_STATUS_TEMPERATURE = "monitor-servidores/rack01/status/components/temperature";
constexpr const char *MQTT_TOPIC_STATUS_HUMIDITY = "monitor-servidores/rack01/status/components/humidity";
constexpr const char *MQTT_TOPIC_STATUS_POSITION = "monitor-servidores/rack01/status/components/position";
constexpr const char *MQTT_TOPIC_STATUS_DOOR = "monitor-servidores/rack01/status/components/door";
constexpr const char *MQTT_TOPIC_TEMPERATURE = "monitor-servidores/rack01/sensors/dht22/temperature_c";
constexpr const char *MQTT_TOPIC_HUMIDITY = "monitor-servidores/rack01/sensors/dht22/humidity_pct";
constexpr const char *MQTT_TOPIC_MPU_ACCEL_X = "monitor-servidores/rack01/sensors/mpu6050/accel_x";
constexpr const char *MQTT_TOPIC_MPU_ACCEL_Y = "monitor-servidores/rack01/sensors/mpu6050/accel_y";
constexpr const char *MQTT_TOPIC_MPU_ACCEL_Z = "monitor-servidores/rack01/sensors/mpu6050/accel_z";
constexpr const char *MQTT_TOPIC_MPU_TILT = "monitor-servidores/rack01/sensors/mpu6050/tilt_deg";
constexpr const char *MQTT_TOPIC_MPU_VIBRATION = "monitor-servidores/rack01/sensors/mpu6050/vibration_level";
constexpr const char *MQTT_TOPIC_MPU_PITCH = "monitor-servidores/rack01/sensors/mpu6050/pitch_deg";
constexpr const char *MQTT_TOPIC_MPU_ROLL = "monitor-servidores/rack01/sensors/mpu6050/roll_deg";
constexpr const char *MQTT_TOPIC_MPU_POSITION = "monitor-servidores/rack01/sensors/mpu6050/position";
constexpr const char *MQTT_TOPIC_DOOR_STATE = "monitor-servidores/rack01/sensors/door/state";
constexpr const char *MQTT_TOPIC_COMMAND_MODE_SET = "monitor-servidores/rack01/commands/mode/set";
constexpr const char *MQTT_TOPIC_COMMAND_DOOR_UNLOCK = "monitor-servidores/rack01/commands/door/unlock";
constexpr const char *MQTT_TOPIC_COMMAND_DOOR_LOCK = "monitor-servidores/rack01/commands/door/lock";
constexpr const char *MQTT_TOPIC_COMMAND_FAN_AUTO = "monitor-servidores/rack01/commands/fan/auto";
constexpr const char *MQTT_TOPIC_COMMAND_FAN_MANUAL = "monitor-servidores/rack01/commands/fan/manual";
constexpr const char *MQTT_TOPIC_CONTROL_MODE = "monitor-servidores/rack01/control/mode";
constexpr const char *MQTT_TOPIC_ACTUATOR_FAN_POWER = "monitor-servidores/rack01/actuators/fan/power_pct";
constexpr const char *MQTT_TOPIC_ACTUATOR_FAN_MODE = "monitor-servidores/rack01/actuators/fan/mode";
constexpr const char *MQTT_TOPIC_ACTUATOR_DOOR_LOCK = "monitor-servidores/rack01/actuators/door_lock/state";
constexpr const char *MQTT_TOPIC_SECURITY_DOOR = "monitor-servidores/rack01/security/door/state";
constexpr const char *MQTT_TOPIC_SECURITY_ACCESS = "monitor-servidores/rack01/security/access/last_event";
constexpr uint16_t MQTT_BUFFER_SIZE = 700;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t MQTT_CONNECT_TIMEOUT_MS = 10000;
constexpr uint32_t NETWORK_RECONNECT_INTERVAL_MS = 5000;
constexpr bool MQTT_ALLOW_INSECURE_TLS = true;

// Modos de prueba temporales. Dejar todos en 0 para volver al firmware integrado.
#define ENABLE_LED_GRADIENT_TEST 0
#define ENABLE_ULTRASONIC_TEST 0
#define ENABLE_DHT_TEST 0
#define ENABLE_MPU_TEST 0
#define ENABLE_SERVO_TEST 0

// Sensor DHT. Cambiar a 11 si el sensor instalado es DHT11.
constexpr uint8_t DHT_TYPE = 22;
constexpr uint8_t PIN_DHT = 4;
constexpr float DHT_TEMPERATURE_OFFSET_C = -5.0F;
constexpr float DHT_HUMIDITY_OFFSET_PCT = -20.0F;

// Sensor ultrasonico tipo HC-SR04.
// El pin ECHO debe pasar por divisor de voltaje porque el modulo entrega 5V.
constexpr uint8_t PIN_ULTRASONIC_TRIG = 32;
constexpr uint8_t PIN_ULTRASONIC_ECHO = 34;
constexpr uint32_t ULTRASONIC_TIMEOUT_US = 25000;
constexpr float SPEED_OF_SOUND_CM_PER_US = 0.0343F;
constexpr float DOOR_CLOSED_MAX_CM = 8.0F;

// MPU6050 por I2C.
constexpr uint8_t PIN_I2C_SDA = 21;
constexpr uint8_t PIN_I2C_SCL = 22;
constexpr uint8_t MPU6050_ADDRESS = 0x68;
constexpr char MPU_NORMAL_AXIS = 'Y';
constexpr int8_t MPU_NORMAL_SIGN = 1;

// LED RGB con logica invertida: comun a 3V3, cada color se activa con LOW.
constexpr uint8_t PIN_LED_R = 25;
constexpr uint8_t PIN_LED_G = 26;
constexpr uint8_t PIN_LED_B = 27;
constexpr bool LED_COMMON_ANODE = true;

// Servo para bloqueo interno de puerta.
// Alimentar el servo con fuente externa de 5V y unir GND de la fuente con GND del ESP32.
constexpr uint8_t PIN_SERVO_LOCK = 14;
constexpr uint8_t SERVO_PWM_CHANNEL = 4;
constexpr uint32_t SERVO_PWM_FREQ_HZ = 50;
constexpr uint8_t SERVO_PWM_RESOLUTION_BITS = 16;
constexpr uint16_t SERVO_MIN_PULSE_US = 500;
constexpr uint16_t SERVO_MAX_PULSE_US = 2400;
constexpr uint8_t SERVO_LOCK_CLOSED_ANGLE = 180;
constexpr uint8_t SERVO_LOCK_OPEN_ANGLE = 90;
constexpr uint32_t SERVO_LOCK_TEST_STEP_MS = 3000;

// Intervalos.
constexpr uint32_t TELEMETRY_INTERVAL_MS = 5000;
constexpr uint32_t LED_TEST_STEP_MS = 500;
constexpr uint32_t LED_BLINK_INTERVAL_MS = 4000;

// Umbrales iniciales del prototipo.
constexpr float TEMP_OPTIMAL_MAX_C = 32.0F;
constexpr float TEMP_CRITICAL_C = 35.0F;
constexpr float HUM_LOW_WARNING = 30.0F;
constexpr float HUM_HIGH_WARNING = 65.0F;
constexpr float HUM_LOW_CRITICAL = 25.0F;
constexpr float HUM_HIGH_CRITICAL = 75.0F;
constexpr float TILT_WARNING_DEG = 8.0F;
constexpr float TILT_CRITICAL_DEG = 15.0F;
constexpr float VIBRATION_WARNING_G = 0.12F;
constexpr float VIBRATION_CRITICAL_G = 0.25F;

// Actuadores para la fase API REST y frontend.
constexpr uint32_t DOOR_LOCK_SIMULATION_MS = 1200;
constexpr uint32_t DOOR_UNLOCK_WINDOW_MS = 10000;

#endif
