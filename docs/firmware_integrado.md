# Firmware Integrado Del ESP32

Esta fase convierte el diagnostico inicial en un firmware organizado por modulos. El ESP32 lee sensores, publica por MQTT, evalua el estado del rack y simula los actuadores de la fase de control: ventilador y bloqueo interno de puerta.

## Flujo General

Cada `TELEMETRY_INTERVAL_MS` el ESP32 ejecuta:

1. Leer DHT11/DHT22.
2. Leer distancia con sensor ultrasonico HC-SR04.
3. Leer MPU6050.
4. Evaluar el estado general del rack.
5. Calcular el modo activo y los umbrales correspondientes.
6. Actualizar actuadores simulados: ventilador y bloqueo de puerta.
7. Actualizar el LED RGB.
8. Imprimir diagnostico humano por Serial.
9. Imprimir JSON de telemetria por Serial.
10. Publicar cada medida y estado de control en topicos MQTT si hay conexion.

## Archivos Del Firmware

| Archivo | Responsabilidad |
|---|---|
| `include/config.h` | Pines, identificador del dispositivo, intervalos y umbrales |
| `include/control_modes.h` / `src/control_modes.cpp` | Modos `standard`, `energy_saving` y `performance` |
| `include/actuators.h` / `src/actuators.cpp` | Simulacion de ventilador, servo de bloqueo y seguridad de puerta |
| `include/sensors.h` / `src/sensors.cpp` | Inicializacion y lectura de sensores, usando libreria DHT para temperatura/humedad |
| `include/rack_status.h` / `src/rack_status.cpp` | Evaluacion del estado general |
| `include/status_led.h` / `src/status_led.cpp` | Control del LED RGB |
| `include/telemetry.h` / `src/telemetry.cpp` | Construccion del JSON y salida de diagnostico |
| `include/network_mqtt.h` / `src/network_mqtt.cpp` | Conexion WiFi y publicacion MQTT por topicos |
| `src/main.cpp` | Orquestador principal |

## Estados Del Rack

| Estado | Codigo | LED RGB | Uso |
|---|---:|---|---|
| `connecting` | 0 | Azul titilando | Intentando conectar WiFi o MQTT |
| `offline` | 1 | Cian titilando | Sin internet, WiFi o MQTT |
| `optimal` | 2 | Verde titilando | Temperatura hasta 32 C, humedad OK, puerta cerrada y MPU normal |
| `warning` | 3 | Amarillo titilando | Algun valor se acerca al limite, la puerta esta desbloqueada o abierta con autorizacion |
| `critical` | 4 | Rojo titilando | Apertura no autorizada, error de bloqueo, temperatura critica, humedad extrema, inclinacion o vibracion fuerte |
| `sensor_error` | 5 | Morado titilando | Fallo de lectura del DHT o MPU6050 |

## Modos De Operacion

El modo activo cambia los umbrales de temperatura/humedad y la potencia automatica del ventilador.

| Modo | Payload MQTT | Temperatura optima maxima | Temperatura critica | Fan optimo | Fan alerta | Fan critico |
|---|---|---:|---:|---:|---:|---:|
| Estandar | `standard` | 32 C | 35 C | 25% | 60% | 100% |
| Ahorro de energia | `energy_saving` | 34 C | 38 C | 0% | 40% | 80% |
| Potencia | `performance` | 28 C | 33 C | 50% | 80% | 100% |

El modo inicia en `standard`. Se puede cambiar por MQTT publicando en:

```text
monitor-servidores/rack01/commands/mode/set
```

con uno de estos payloads:

```text
standard
energy_saving
performance
```

## Actuadores Simulados

Mientras no esten conectados el servo y el ventilador, el firmware los simula por Monitor Serial y publica sus estados por MQTT.

Estados del ventilador:

| Campo | Valores |
|---|---|
| `fan_mode` | `auto`, `manual` |
| `fan_power_pct` | `0` a `100` |

Estados del bloqueo:

| Campo | Valores |
|---|---|
| `door_lock_state` | `locked`, `unlocked`, `locking`, `unlocking`, `lock_error` |
| `door_security_state` | `door_secure`, `door_unlocked_closed`, `door_open_authorized`, `door_open_forced`, `door_lock_error` |
| `access_event` | `none`, `granted`, `denied`, `timeout`, `forced_open`, `manual_lock` |

Regla de seguridad de puerta:

| Estado fisico/control | Estado publicado | Efecto en estado general |
|---|---|---|
| Cerrada y bloqueada | `door_secure` | Puede ser `optimal` si los demas sensores estan bien |
| Cerrada y desbloqueada | `door_unlocked_closed` | `warning` |
| Abierta despues de desbloqueo valido | `door_open_authorized` | `warning` |
| Abierta sin desbloqueo valido | `door_open_forced` | `critical` |
| Error del bloqueo | `door_lock_error` | `critical` |

Comandos MQTT de prueba:

| Accion | Topico | Payload |
|---|---|---|
| Desbloquear puerta | `monitor-servidores/rack01/commands/door/unlock` | `unlock` |
| Bloquear puerta | `monitor-servidores/rack01/commands/door/lock` | `lock` |
| Ventilador automatico | `monitor-servidores/rack01/commands/fan/auto` | `auto` |
| Ventilador manual | `monitor-servidores/rack01/commands/fan/manual` | `0` a `100` |

## JSON De Telemetria

Ejemplo de salida por Monitor Serial:

```json
{
  "device_id": "rack01",
  "control_mode": "standard",
  "temperature_c": 27.4,
  "humidity_pct": 58.2,
  "door_state": "closed",
  "accel_x": 0.01,
  "accel_y": 0.98,
  "accel_z": 0.03,
  "tilt_deg": 2.5,
  "vibration_level": 0.04,
  "pitch_deg": -0.7,
  "roll_deg": -2.0,
  "mpu_position": "vertical normal",
  "status": "optimal",
  "status_code": 2,
  "status_reason": "all_conditions_optimal",
  "temperature_state": "optimal",
  "humidity_state": "optimal",
  "position_state": "optimal",
  "door_component_state": "closed",
  "fan_mode": "auto",
  "fan_power_pct": 25,
  "door_lock_state": "locked",
  "door_security_state": "door_secure",
  "access_event": "none",
  "actuators_simulated": true,
  "uptime_ms": 120000
}
```

Si un sensor falla, los campos dependientes de ese sensor se envian como `null` y el estado general pasa a `sensor_error`.

## Configuracion Importante

En `include/config.h` se pueden ajustar:

| Constante | Uso |
|---|---|
| `DEVICE_ID` | Identificador del rack, por ahora `rack01` |
| `DHT_TYPE` | `22` para DHT22 o `11` para DHT11 |
| `DHT_TEMPERATURE_OFFSET_C` / `DHT_HUMIDITY_OFFSET_PCT` | Correccion aplicada a las lecturas del DHT antes de publicar |
| `TELEMETRY_INTERVAL_MS` | Intervalo de lectura y salida JSON |
| `DOOR_LOCK_SIMULATION_MS` | Tiempo simulado del movimiento del servo |
| `DOOR_UNLOCK_WINDOW_MS` | Ventana temporal para apertura autorizada |
| `PIN_ULTRASONIC_TRIG` | Pin de disparo del HC-SR04, por ahora GPIO 32 |
| `PIN_ULTRASONIC_ECHO` | Pin de eco del HC-SR04, por ahora GPIO 34 |
| `ULTRASONIC_TIMEOUT_US` | Tiempo maximo de espera del eco |
| `DOOR_CLOSED_MAX_CM` | Distancia maxima para considerar puerta cerrada |
| `MPU_NORMAL_AXIS` / `MPU_NORMAL_SIGN` | Orientacion fisica del MPU6050 que se considera normal |
| `TEMP_OPTIMAL_MAX_C` / `TEMP_CRITICAL_C` | Temperatura maxima optima, 32 C, y umbral critico |
| `TILT_WARNING_DEG` / `TILT_CRITICAL_DEG` | Umbrales de inclinacion |
| `VIBRATION_WARNING_G` / `VIBRATION_CRITICAL_G` | Umbrales de vibracion |
| `LED_COMMON_ANODE` | Debe estar en `true` porque el LED trabaja con logica invertida |

## Explicacion Del Estado

Ademas de `status/system`, el firmware publica:

```text
monitor-servidores/rack01/status/reason
monitor-servidores/rack01/status/components/temperature
monitor-servidores/rack01/status/components/humidity
monitor-servidores/rack01/status/components/position
monitor-servidores/rack01/status/components/door
```

Esto permite saber si el estado general viene de temperatura, humedad, posicion, vibracion, puerta o conectividad.

La fase de control tambien publica:

```text
monitor-servidores/rack01/control/mode
monitor-servidores/rack01/actuators/fan/power_pct
monitor-servidores/rack01/actuators/fan/mode
monitor-servidores/rack01/actuators/door_lock/state
monitor-servidores/rack01/security/door/state
monitor-servidores/rack01/security/access/last_event
```

## Last Will

El ESP32 configura Last Will en:

```text
monitor-servidores/rack01/status/last_will
```

Valor normal:

```text
online
```

Si se queda sin energia o pierde conexion sin avisar:

```text
offline_lost_signal_or_power
```

## Fase De Control Y Actuadores

La primera parte de la fase de control ya esta implementada en firmware: modos de operacion, ventilador simulado, bloqueo simulado y topicos MQTT de comandos/estado. El bridge y la API REST tambien estan implementados; queda pendiente el frontend y actualizar Grafana para los nuevos datos.

Documento de referencia:

```text
docs/fase_api_actuadores_frontend.md
```

## Validacion

Para dar esta fase como completada:

1. Compilar el proyecto en PlatformIO.
2. Cargar el firmware al ESP32.
3. Abrir Monitor Serial a `115200`.
4. Confirmar que se imprime un JSON valido cada 5 segundos.
5. Confirmar que el LED titila cada 4 segundos con el color del estado.
6. Confirmar en MQTT Explorer que llegan los topicos individuales.
7. Publicar `performance` en `commands/mode/set` y confirmar cambio de modo en Serial.
8. Publicar `unlock` en `commands/door/unlock` y confirmar estados `unlocking` y `unlocked`.
9. Publicar `80` en `commands/fan/manual` y confirmar potencia manual simulada.
