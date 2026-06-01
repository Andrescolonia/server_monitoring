# MQTT Con HiveMQ

Esta fase conecta el ESP32 a WiFi y publica el estado del rack en HiveMQ Cloud usando MQTT sobre TLS.

## Archivo De Credenciales

Las credenciales se configuran en:

```text
include/secrets.h
```

El archivo esta en `.gitignore` para evitar guardar claves en el repositorio. Existe tambien una plantilla:

```text
include/secrets.example.h
```

Contenido esperado:

```cpp
constexpr const char *WIFI_SSID = "NOMBRE_WIFI";
constexpr const char *WIFI_PASSWORD = "PASSWORD_WIFI";

constexpr const char *MQTT_HOST = "xxxxxxxx.s1.eu.hivemq.cloud";
constexpr uint16_t MQTT_PORT = 8883;
constexpr const char *MQTT_USER = "USUARIO_HIVEMQ";
constexpr const char *MQTT_PASSWORD = "PASSWORD_HIVEMQ";
```

## Topicos MQTT

| Topico | Uso |
|---|---|
| `monitor-servidores/rack01/status/system` | Estado general: `connecting`, `offline`, `optimal`, `warning`, `critical`, `sensor_error` |
| `monitor-servidores/rack01/status/network` | Estado de red: `online` u `offline` |
| `monitor-servidores/rack01/status/reason` | Razon principal del estado actual |
| `monitor-servidores/rack01/status/last_will` | Last Will: `online` u `offline_lost_signal_or_power` |
| `monitor-servidores/rack01/status/components/temperature` | Estado de temperatura |
| `monitor-servidores/rack01/status/components/humidity` | Estado de humedad |
| `monitor-servidores/rack01/status/components/position` | Estado de posicion, inclinacion o vibracion |
| `monitor-servidores/rack01/status/components/door` | Estado logico de puerta |
| `monitor-servidores/rack01/sensors/dht22/temperature_c` | Temperatura en grados Celsius |
| `monitor-servidores/rack01/sensors/dht22/humidity_pct` | Humedad relativa |
| `monitor-servidores/rack01/sensors/mpu6050/accel_x` | Aceleracion X |
| `monitor-servidores/rack01/sensors/mpu6050/accel_y` | Aceleracion Y |
| `monitor-servidores/rack01/sensors/mpu6050/accel_z` | Aceleracion Z |
| `monitor-servidores/rack01/sensors/mpu6050/tilt_deg` | Inclinacion respecto a la orientacion normal |
| `monitor-servidores/rack01/sensors/mpu6050/vibration_level` | Nivel de vibracion |
| `monitor-servidores/rack01/sensors/mpu6050/pitch_deg` | Pitch calculado |
| `monitor-servidores/rack01/sensors/mpu6050/roll_deg` | Roll calculado |
| `monitor-servidores/rack01/sensors/mpu6050/position` | Posicion estimada |
| `monitor-servidores/rack01/sensors/door/state` | Estado de puerta: `closed` u `open` |

## Topicos De Control

El firmware ya escucha comandos MQTT y publica estados de actuadores simulados. La API REST usa estos mismos topicos para enviar comandos al ESP32.

| Topico | Direccion | Uso |
|---|---|---|
| `monitor-servidores/rack01/commands/mode/set` | API -> ESP32 | Cambiar modo: `standard`, `energy_saving`, `performance` |
| `monitor-servidores/rack01/commands/door/unlock` | API -> ESP32 | Solicitar desbloqueo de puerta |
| `monitor-servidores/rack01/commands/door/lock` | API -> ESP32 | Solicitar bloqueo de puerta |
| `monitor-servidores/rack01/commands/fan/auto` | API -> ESP32 | Dejar ventilador en automatico |
| `monitor-servidores/rack01/commands/fan/manual` | API -> ESP32 | Fijar potencia manual de `0` a `100` |
| `monitor-servidores/rack01/control/mode` | ESP32 -> MQTT | Modo activo |
| `monitor-servidores/rack01/actuators/fan/power_pct` | ESP32 -> MQTT | Potencia del ventilador |
| `monitor-servidores/rack01/actuators/fan/mode` | ESP32 -> MQTT | `auto` o `manual` |
| `monitor-servidores/rack01/actuators/door_lock/state` | ESP32 -> MQTT | Estado del bloqueo |
| `monitor-servidores/rack01/security/door/state` | ESP32 -> MQTT | Estado de seguridad de puerta |
| `monitor-servidores/rack01/security/access/last_event` | ESP32 -> MQTT | Ultimo evento de acceso |

El ultimo topico de acceso completo es:

```text
monitor-servidores/rack01/security/access/last_event
```

## Prueba De Comandos MQTT

Desde MQTT Explorer, publicar estos payloads para validar la fase 2:

| Topico | Payload esperado | Resultado esperado |
|---|---|---|
| `monitor-servidores/rack01/commands/mode/set` | `standard` | `control/mode = standard` |
| `monitor-servidores/rack01/commands/mode/set` | `energy_saving` | `control/mode = energy_saving` |
| `monitor-servidores/rack01/commands/mode/set` | `performance` | `control/mode = performance` |
| `monitor-servidores/rack01/commands/fan/manual` | `80` | `actuators/fan/mode = manual`, `actuators/fan/power_pct = 80` |
| `monitor-servidores/rack01/commands/fan/auto` | `auto` | `actuators/fan/mode = auto` |
| `monitor-servidores/rack01/commands/door/unlock` | `unlock` | `actuators/door_lock/state = unlocking` y luego `unlocked` |
| `monitor-servidores/rack01/commands/door/lock` | `lock` | `actuators/door_lock/state = locking` y luego `locked` |

El ESP32 tambien imprime cada comando recibido en Monitor Serial con el formato:

```text
MQTT COMMAND: topico -> payload
```

## Payload

El ESP32 publica cada medida como payload simple. Ejemplos:

```text
monitor-servidores/rack01/sensors/dht22/temperature_c -> 27.4
monitor-servidores/rack01/sensors/door/state -> closed
monitor-servidores/rack01/status/system -> optimal
monitor-servidores/rack01/status/reason -> all_conditions_optimal
monitor-servidores/rack01/status/components/temperature -> optimal
```

## Razon Del Estado

El topico `status/reason` explica por que el rack esta en el estado actual.

Ejemplos:

| Razon | Significado |
|---|---|
| `all_conditions_optimal` | Todo esta dentro del rango esperado |
| `network_disconnected` | No hay WiFi o MQTT |
| `door_unlocked_closed` | La puerta esta cerrada, pero el bloqueo esta liberado |
| `door_open_authorized` | La puerta esta abierta despues de un desbloqueo valido |
| `door_open_forced` | La puerta esta abierta sin autorizacion |
| `door_lock_error` | El bloqueo de puerta reporto error |
| `temperature_warning_high` | Temperatura sobre el rango optimo |
| `temperature_critical_high` | Temperatura critica |
| `warning_low` / `warning_high` | Humedad cerca del limite |
| `critical_low` / `critical_high` | Humedad fuera del limite critico |
| `tilt_warning` / `tilt_critical` | Inclinacion fuera del rango |
| `vibration_warning` / `vibration_critical` | Vibracion alta |
| `dht_sensor_error` / `mpu_sensor_error` | Error de lectura del sensor |

## Last Will

El ESP32 registra un Last Will en:

```text
monitor-servidores/rack01/status/last_will
```

Cuando se conecta correctamente publica:

```text
online
```

Si pierde energia, WiFi o senal sin poder cerrar la conexion MQTT, HiveMQ publicara automaticamente:

```text
offline_lost_signal_or_power
```

## Validacion

1. Configurar `include/secrets.h`.
2. Cargar el firmware al ESP32.
3. Abrir Monitor Serial a `115200`.
4. Verificar mensajes:

```text
WiFi: conectado, IP ...
MQTT: conectado
MQTT: topicos individuales publicados
```

5. En HiveMQ Web Client o cualquier cliente MQTT, suscribirse a:

```text
monitor-servidores/rack01/#
```

## Nota Sobre TLS

Para el prototipo se usa `WiFiClientSecure` con `setInsecure()`, lo que simplifica la conexion TLS con HiveMQ Cloud. Para una version final mas robusta se debe cargar el certificado raiz del broker y desactivar `MQTT_ALLOW_INSECURE_TLS`.
