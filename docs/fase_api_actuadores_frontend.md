# Fase Nueva: API REST, Frontend Y Actuadores

Esta fase amplia el proyecto desde monitoreo hacia monitoreo y control. El rack seguira midiendo temperatura, humedad, puerta, posicion y vibracion, pero ahora tambien tendra una capa de control para modos de operacion, ventilador y bloqueo interno de puerta.

Como el servo y el ventilador aun no estan instalados, la primera version debe simular ambos actuadores por consola serial del ESP32 y por respuestas de la API.

## Arquitectura Actualizada

```text
Sensores + actuadores simulados
ESP32
  | WiFi / MQTT
  v
HiveMQ Cloud
  | subscribe telemetry                 | publish commands
  v                                     ^
Bridge Python -> InfluxDB <------------- API REST
                    ^                       ^
                    | query                 | HTTP
                 Grafana                 Frontend Web
                                            ^
                                            |
                                         Usuario
```

## Nuevos Componentes

| Componente | Responsabilidad |
|---|---|
| ESP32 | Leer sensores, evaluar estado, simular servo/ventilador y recibir comandos MQTT |
| HiveMQ | Broker MQTT para telemetria y comandos |
| Bridge Python | Guardar telemetria y eventos en InfluxDB |
| API REST | Consultar estado, cambiar modo, solicitar bloqueo/desbloqueo y controlar ventilador |
| Frontend Web | Interfaz para ver el estado, cambiar modo y desbloquear puerta con usuario/contrasena |
| Grafana | Dashboard historico y tecnico |

## Objetivos De La Fase

1. Crear una API REST que consulte el estado general del rack.
2. Permitir cambiar el modo de operacion del rack.
3. Definir perfiles de umbrales predeterminados.
4. Simular un servo de bloqueo de puerta.
5. Simular un ventilador con potencia variable.
6. Agregar estados nuevos relacionados con bloqueo y acceso.
7. Preparar un frontend para consultar la API.
8. Permitir desbloqueo mediante usuario y contrasena.
9. Mantener Grafana como vista historica, sin reemplazarlo por el frontend.

## Modos De Operacion

Los modos modifican los valores optimos y tambien la respuesta del ventilador. Estos valores son iniciales para el prototipo y se pueden ajustar despues de pruebas.

| Modo | Nombre API | Temperatura optima maxima | Temperatura critica | Ventilador base | Uso |
|---|---|---:|---:|---:|---|
| Estandar | `standard` | 32 C | 35 C | 25% | Balance normal del proyecto |
| Ahorro de energia | `energy_saving` | 34 C | 38 C | 0% | Reduce uso del ventilador |
| Potencia | `performance` | 28 C | 33 C | 50% | Enfria antes y con mas fuerza |

La humedad tambien puede variar por modo:

| Modo | Humedad minima alerta | Humedad maxima alerta | Humedad minima critica | Humedad maxima critica |
|---|---:|---:|---:|---:|
| Estandar | 30% | 65% | 25% | 75% |
| Ahorro de energia | 25% | 70% | 20% | 80% |
| Potencia | 35% | 60% | 30% | 70% |

## Logica Del Ventilador

El ventilador trabajara en automatico segun el modo y el estado del rack.

| Condicion | Estandar | Ahorro de energia | Potencia |
|---|---:|---:|---:|
| Estado optimo | 25% | 0% | 50% |
| Temperatura en alerta | 60% | 40% | 80% |
| Temperatura critica | 100% | 80% | 100% |
| Error de sensor DHT | 80% | 60% | 100% |
| Rack offline | Mantener ultimo valor seguro | Mantener ultimo valor seguro | Mantener ultimo valor seguro |

Mientras no exista hardware fisico, el ESP32 debe imprimir algo como:

```text
ACTUADOR FAN: modo=standard, estado=warning, potencia=60%
```

## Bloqueo De Puerta

El servo se usara para trabar la puerta desde adentro. En esta fase se simula por consola.

Estados propuestos:

| Estado | Significado |
|---|---|
| `locked` | Puerta trabada |
| `unlocked` | Puerta liberada temporalmente |
| `locking` | Simulando movimiento del servo hacia bloqueo |
| `unlocking` | Simulando movimiento del servo hacia desbloqueo |
| `lock_error` | El bloqueo no pudo completarse |
| `simulated` | Estado usado mientras no exista servo fisico |

Estados de seguridad de puerta:

| Estado | Condicion |
|---|---|
| `door_secure` | Puerta cerrada y bloqueo activo |
| `door_unlocked_closed` | Puerta cerrada pero desbloqueada |
| `door_open_authorized` | Puerta abierta despues de un desbloqueo valido |
| `door_open_forced` | Puerta abierta sin desbloqueo autorizado |
| `door_lock_error` | Error de bloqueo o simulacion fallida |

Regla inicial:

1. La puerta inicia como `locked`.
2. Si el usuario autorizado solicita desbloqueo, pasa a `unlocking` y luego `unlocked`.
3. El desbloqueo dura una ventana corta, por ejemplo 10 segundos.
4. Si la puerta se abre dentro de esa ventana, el evento es autorizado.
5. Si la puerta se abre sin desbloqueo previo, se reporta `door_open_forced`.
6. Cuando la puerta vuelve a cerrar, el sistema intenta bloquear de nuevo.

## API REST Propuesta

La API puede crearse en una carpeta nueva:

```text
api-rest/
├── main.py
├── requirements.txt
├── .env.example
└── README.md
```

Stack recomendado para mantenerlo simple:

| Elemento | Uso |
|---|---|
| FastAPI | API REST |
| Uvicorn | Servidor local |
| paho-mqtt | Publicar comandos a HiveMQ |
| influxdb-client | Consultar estado e historicos |
| python-dotenv | Variables de entorno |

Endpoints iniciales:

| Metodo | Ruta | Uso |
|---|---|---|
| `GET` | `/api/rack/status` | Estado general actual del rack |
| `GET` | `/api/rack/modes` | Lista de modos disponibles y umbrales |
| `PUT` | `/api/rack/mode` | Cambiar modo de operacion |
| `GET` | `/api/actuators` | Estado actual de ventilador y bloqueo |
| `POST` | `/api/door/unlock` | Desbloquear puerta con usuario y contrasena |
| `POST` | `/api/door/lock` | Bloquear puerta manualmente |
| `POST` | `/api/fan/auto` | Volver el ventilador a modo automatico |
| `POST` | `/api/fan/manual` | Fijar potencia manual del ventilador |
| `GET` | `/api/history/environment` | Historico simple de temperatura y humedad |

Ejemplo de respuesta de `/api/rack/status`:

```json
{
  "device_id": "rack01",
  "mode": "standard",
  "status": "optimal",
  "reason": "all_conditions_optimal",
  "temperature_c": 27.4,
  "humidity_pct": 45.0,
  "door_state": "closed",
  "door_lock_state": "locked",
  "door_security_state": "door_secure",
  "fan_power_pct": 25,
  "fan_mode": "auto",
  "mpu_position": "vertical normal",
  "network": "online",
  "last_will": "online"
}
```

Ejemplo para cambiar modo:

```http
PUT /api/rack/mode
Content-Type: application/json

{
  "mode": "performance"
}
```

Ejemplo para desbloquear:

```http
POST /api/door/unlock
Content-Type: application/json

{
  "username": "DianaMena",
  "password": "RackSeguro2026!"
}
```

Para el prototipo universitario esta autenticacion puede iniciar con usuario y contrasena en variables de entorno. Para una version mas seria se debe usar hash de contrasena y token de sesion.

## Topicos MQTT Nuevos

La API publicara comandos y el ESP32 publicara estados de actuadores.

### Comandos Hacia El ESP32

| Topico | Payload |
|---|---|
| `monitor-servidores/rack01/commands/mode/set` | `standard`, `energy_saving`, `performance` |
| `monitor-servidores/rack01/commands/door/unlock` | `unlock` |
| `monitor-servidores/rack01/commands/door/lock` | `lock` |
| `monitor-servidores/rack01/commands/fan/auto` | `auto` |
| `monitor-servidores/rack01/commands/fan/manual` | `0` a `100` |

### Estados Publicados Por El ESP32

| Topico | Payload |
|---|---|
| `monitor-servidores/rack01/control/mode` | Modo activo |
| `monitor-servidores/rack01/actuators/fan/power_pct` | Potencia del ventilador |
| `monitor-servidores/rack01/actuators/fan/mode` | `auto` o `manual` |
| `monitor-servidores/rack01/actuators/door_lock/state` | `locked`, `unlocked`, `locking`, `unlocking`, `lock_error`, `simulated` |
| `monitor-servidores/rack01/security/door/state` | `door_secure`, `door_unlocked_closed`, `door_open_authorized`, `door_open_forced`, `door_lock_error` |
| `monitor-servidores/rack01/security/access/last_event` | `granted`, `denied`, `timeout`, `forced_open` |

## Cambios En InfluxDB

El bridge debe guardar nuevos estados para que Grafana y la API puedan consultar historicos.

| Measurement | Sensor/tag | Fields |
|---|---|---|
| `rack_control` | `mode` | `mode` |
| `rack_actuators` | `fan` | `fan_power_pct`, `fan_mode` |
| `rack_actuators` | `door_lock` | `door_lock_state` |
| `rack_security` | `door` | `door_security_state` |
| `rack_security` | `access` | `access_event` |

## Cambios En Firmware

Modulos propuestos:

| Archivo | Responsabilidad |
|---|---|
| `include/control_modes.h` / `src/control_modes.cpp` | Perfiles de umbrales por modo |
| `include/actuators.h` / `src/actuators.cpp` | Simulacion y luego control real de servo/ventilador |
| `include/security_state.h` / `src/security_state.cpp` | Estado de bloqueo, acceso autorizado y puerta forzada |
| `include/network_mqtt.h` / `src/network_mqtt.cpp` | Suscripcion a comandos MQTT |

Cambios de comportamiento:

1. El ESP32 debe guardar el modo activo.
2. La evaluacion del rack debe usar los umbrales del modo activo.
3. El ventilador debe calcular potencia segun modo y estado.
4. El servo se simula por Serial mientras no exista hardware.
5. Los comandos MQTT deben cambiar modo, bloquear, desbloquear y controlar ventilador.
6. Los estados de actuadores deben publicarse por MQTT.

## Frontend Propuesto

Pantallas iniciales:

| Vista | Contenido |
|---|---|
| Resumen | Estado general, motivo, modo activo, temperatura, humedad, puerta, ventilador |
| Control | Cambiar modo, bloquear/desbloquear puerta, ventilador auto/manual |
| Acceso | Formulario de usuario y contrasena para desbloquear |
| Historico | Graficas basicas consumidas desde API o enlace a Grafana |

El frontend no reemplaza Grafana. Grafana queda para analisis historico y el frontend queda para operacion directa del prototipo.

## Plan De Trabajo

### Paso 1: Documentar arquitectura y contratos

- Definir modos, endpoints, topicos MQTT y estados nuevos.
- Dejar claro que servo y ventilador estan simulados.
- Actualizar documentacion tecnica del proyecto.

Estado: completado.

### Paso 2: Firmware con simulacion

- Crear modulos de modos y actuadores.
- Agregar estados `door_lock_state`, `door_security_state`, `fan_power_pct` y `fan_mode`.
- Imprimir por Serial las acciones simuladas.
- Publicar estados nuevos por MQTT.
- Suscribirse a topicos de comandos.

Estado: completado en firmware y MQTT. Pendiente: prueba manual desde MQTT Explorer despues de cargar el binario actualizado.

### Paso 3: Bridge hacia InfluxDB

- Agregar handlers para los nuevos topicos.
- Crear measurements `rack_control`, `rack_actuators` y `rack_security`.
- Validar que los datos aparecen en InfluxDB.

Estado: implementado en el bridge. Pendiente: ejecutar el bridge con el ESP32 publicando y confirmar los puntos en InfluxDB.

### Paso 4: API REST

- Crear `api-rest/` con FastAPI.
- Conectar API a InfluxDB para lectura.
- Conectar API a HiveMQ para publicar comandos.
- Implementar endpoints de estado, modo, ventilador y puerta.
- Implementar credenciales simples para desbloqueo.

Estado: implementado. Pendiente: copiar `.env.example` como `.env`, completar credenciales y probar con `/docs`.

### Paso 5: Frontend

- Crear una interfaz web simple.
- Mostrar estado general y actuadores.
- Agregar control de modo.
- Agregar formulario de desbloqueo.
- Mostrar errores claros si API, MQTT o InfluxDB no responden.

Estado: implementado como app estatica en `frontend/`. Pendiente: prueba manual con API REST ejecutandose.

### Paso 6: Grafana

- Agregar fila `Control y seguridad`.
- Agregar panel `Modo activo`.
- Agregar panel `Potencia ventilador`.
- Agregar panel `Estado del bloqueo`.
- Agregar panel `Seguridad de puerta`.
- Agregar timeline `Eventos de acceso`.
- Agregar timeline `Timeline de cambios de modo`.

Estado: implementado en `docs/grafana_dashboard_rack_servidores.json`.

### Paso 7: Instalacion fisica

- Conectar servo con fuente adecuada.
- Conectar ventilador con transistor/MOSFET y fuente externa.
- Reemplazar simulacion por control real.
- Probar corriente, tierra comun y proteccion electrica.

## Reglas De Seguridad Del Prototipo

- El servo y el ventilador no deben alimentarse directamente desde un GPIO.
- El ESP32 solo debe enviar senales de control.
- Si se usa fuente externa para servo/ventilador, debe compartir GND con el ESP32.
- La contrasena no debe guardarse dentro del firmware.
- El desbloqueo debe tener timeout.
- Si la puerta se abre sin desbloqueo valido, debe quedar como evento de seguridad.
- Si el sensor DHT falla, el ventilador debe ir a una potencia segura.

## Criterios De Validacion

La fase se considera lista cuando:

1. Desde la API se consulta `/api/rack/status`.
2. Desde la API se cambia entre `standard`, `energy_saving` y `performance`.
3. El ESP32 muestra por Serial el cambio de modo.
4. El ESP32 muestra por Serial la potencia simulada del ventilador.
5. El endpoint `/api/door/unlock` acepta credenciales correctas y rechaza incorrectas.
6. El ESP32 muestra por Serial `unlocking`, `unlocked`, `locking` y `locked`.
7. Los nuevos estados se publican en MQTT.
8. El bridge guarda esos estados en InfluxDB.
9. Grafana o el frontend pueden mostrar los nuevos valores.
