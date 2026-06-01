# Bridge Python MQTT A InfluxDB

Esta fase conecta la arquitectura:

```text
ESP32 -> HiveMQ -> Python -> InfluxDB
```

## Carpeta

```text
bridge-python/
├── main.py
├── requirements.txt
├── .env.example
└── README.md
```

## Topicos Suscritos

El bridge se suscribe a:

```text
monitor-servidores/rack01/#
```

## Mapeo A InfluxDB

| Topico MQTT | Measurement | Field |
|---|---|---|
| `status/system` | `rack_status` | `system_status`, `status_code` |
| `status/network` | `rack_status` | `network_status`, `network_online` |
| `status/reason` | `rack_status` | `status_reason` |
| `status/last_will` | `rack_status` | `last_will`, `last_will_online` |
| `status/components/temperature` | `rack_components` | `component_state` |
| `status/components/humidity` | `rack_components` | `component_state` |
| `status/components/position` | `rack_components` | `component_state` |
| `status/components/door` | `rack_components` | `component_state` |
| `sensors/dht22/temperature_c` | `rack_environment` | `temperature_c` |
| `sensors/dht22/humidity_pct` | `rack_environment` | `humidity_pct` |
| `sensors/door/state` | `rack_door` | `door_state`, `door_open` |
| `sensors/mpu6050/accel_x` | `rack_motion` | `accel_x` |
| `sensors/mpu6050/accel_y` | `rack_motion` | `accel_y` |
| `sensors/mpu6050/accel_z` | `rack_motion` | `accel_z` |
| `sensors/mpu6050/tilt_deg` | `rack_motion` | `tilt_deg` |
| `sensors/mpu6050/vibration_level` | `rack_motion` | `vibration_level` |
| `sensors/mpu6050/pitch_deg` | `rack_motion` | `pitch_deg` |
| `sensors/mpu6050/roll_deg` | `rack_motion` | `roll_deg` |
| `sensors/mpu6050/position` | `rack_motion` | `position` |

## Mapeo Para Control

La fase de API REST y actuadores agrega topicos de modo, ventilador, bloqueo y seguridad. El bridge guarda estos valores para historicos y para que Grafana/API puedan consultarlos.

| Topico MQTT | Measurement | Field |
|---|---|---|
| `control/mode` | `rack_control` | `mode`, `mode_code` |
| `actuators/fan/power_pct` | `rack_actuators` | `fan_power_pct` |
| `actuators/fan/mode` | `rack_actuators` | `fan_mode`, `fan_mode_code`, `fan_auto` |
| `actuators/door_lock/state` | `rack_actuators` | `door_lock_state`, `door_lock_code`, `door_locked` |
| `security/door/state` | `rack_security` | `door_security_state`, `door_security_code`, `forced_open` |
| `security/access/last_event` | `rack_security` | `access_event`, `access_event_code`, `access_granted` |

Todos los puntos incluyen estos tags:

```text
device_id=rack01
location=maqueta
sensor=dht22/mpu6050/door/system/network/mode/fan/door_lock/access
```

## Estados Numericos

`status_code`:

| Estado | Codigo |
|---|---:|
| `connecting` | 0 |
| `offline` | 1 |
| `optimal` | 2 |
| `warning` | 3 |
| `critical` | 4 |
| `sensor_error` | 5 |

`door_open`:

| Estado | Valor |
|---|---:|
| `closed` | 0 |
| `open` | 1 |

`network_online`:

| Estado | Valor |
|---|---:|
| `offline` | 0 |
| `online` | 1 |

`last_will_online`:

| Estado | Valor |
|---|---:|
| `offline_lost_signal_or_power` | 0 |
| `online` | 1 |

`mode_code`:

| Modo | Valor |
|---|---:|
| `standard` | 0 |
| `energy_saving` | 1 |
| `performance` | 2 |

`fan_mode_code`:

| Modo | Valor |
|---|---:|
| `manual` | 0 |
| `auto` | 1 |

`door_lock_code`:

| Estado | Valor |
|---|---:|
| `locked` | 0 |
| `unlocked` | 1 |
| `locking` | 2 |
| `unlocking` | 3 |
| `lock_error` | 4 |

`door_security_code`:

| Estado | Valor |
|---|---:|
| `door_secure` | 0 |
| `door_unlocked_closed` | 1 |
| `door_open_authorized` | 2 |
| `door_open_forced` | 3 |
| `door_lock_error` | 4 |

`access_event_code`:

| Evento | Valor |
|---|---:|
| `none` | 0 |
| `granted` | 1 |
| `denied` | 2 |
| `timeout` | 3 |
| `forced_open` | 4 |
| `manual_lock` | 5 |

## Validacion De Fase 3

Con el ESP32 publicando y el bridge en ejecucion, probar comandos desde MQTT Explorer:

```text
monitor-servidores/rack01/commands/mode/set -> performance
monitor-servidores/rack01/commands/fan/manual -> 80
monitor-servidores/rack01/commands/door/unlock -> unlock
```

Luego confirmar en InfluxDB que existen puntos recientes en:

```text
rack_control
rack_actuators
rack_security
```

## Ejecucion

1. Crear bucket en InfluxDB:

```text
server_monitoring
```

2. Copiar:

```text
bridge-python/.env.example -> bridge-python/.env
```

3. Completar credenciales de HiveMQ e InfluxDB.
4. Instalar dependencias:

```bash
cd bridge-python
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

5. Ejecutar:

```bash
python main.py
```
