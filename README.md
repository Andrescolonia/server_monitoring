# Sistema De Monitoreo Y Control De Rack Con ESP32

Proyecto universitario de monitoreo y control para un rack de servidores a escala. El prototipo usa un ESP32, sensores ambientales y de movimiento, comunicacion MQTT, persistencia en InfluxDB, visualizacion en Grafana, una API REST y un frontend web operativo.

La maqueta fisica esta planteada como una caja de aproximadamente `16 cm` de ancho, `10 cm` de profundidad y `32 cm` de alto.

## Objetivo

Construir un sistema distribuido que permita:

- Medir temperatura y humedad dentro del rack.
- Detectar apertura o cierre de puerta.
- Detectar orientacion, inclinacion y vibracion del rack.
- Indicar estado general mediante LED RGB.
- Publicar telemetria en MQTT usando HiveMQ Cloud.
- Persistir datos historicos en InfluxDB.
- Consultar y operar el sistema desde una API REST.
- Visualizar el estado desde un frontend web y Grafana.
- Controlar estados de operacion como modo, ventilador y bloqueo de puerta.

## Estado Actual

| Area | Estado |
|---|---|
| Firmware ESP32 | Implementado con sensores, MQTT, estados, LED, comandos y servo de bloqueo |
| Sensores | DHT22, HC-SR04 y MPU6050 integrados |
| LED RGB | Integrado, logica invertida, titileo por estado |
| MQTT HiveMQ | Integrado con topicos de sensores, estado, control y seguridad |
| Bridge Python | Implementado, suscribe MQTT y escribe en InfluxDB |
| InfluxDB | Integrado mediante bucket `server_monitoring` |
| Grafana | Dashboard JSON disponible en `docs/` |
| API REST | Implementada con FastAPI |
| Frontend | Implementado como interfaz web estatica servida por la API |
| Docker | API/frontend y bridge dockerizados |
| Azure | Preparado para Container Apps y Container Registry |
| Servo puerta | Integrado por firmware en `GPIO14`, requiere calibracion mecanica fina |
| Ventilador | Logica de control implementada; etapa fisica pendiente con transistor/MOSFET |

## Arquitectura General

```text
Capa de dispositivos
ESP32
  |-- DHT22
  |-- HC-SR04
  |-- MPU6050
  |-- LED RGB
  |-- Servo bloqueo puerta
  |-- Ventilador 5V pendiente via transistor/MOSFET

Capa de red y mensajeria
ESP32 -> WiFi -> HiveMQ Cloud MQTT TLS

Capa de aplicacion
API REST FastAPI -> HiveMQ commands
Frontend web -> API REST

Capa de gestion de datos
Bridge Python -> InfluxDB
Grafana -> InfluxDB

Capa de operacion
Usuario -> Frontend / Grafana / API docs
```

Flujo principal:

```text
ESP32 -> HiveMQ -> Bridge Python -> InfluxDB -> Grafana
Frontend -> API REST -> HiveMQ -> ESP32
Frontend -> API REST -> InfluxDB
```

La API no se comunica directamente con el ESP32. Todas las ordenes operativas se envian por MQTT y el ESP32 responde publicando nuevos estados.

## Hardware

### Componentes

| Componente | Uso |
|---|---|
| ESP32 | Controlador principal, WiFi y MQTT |
| DHT22 | Temperatura y humedad |
| HC-SR04 | Estado de puerta mediante distancia |
| MPU6050 | Orientacion, inclinacion y vibracion |
| LED RGB | Indicador local de estado |
| Servo 5V | Bloqueo interno de puerta |
| Ventilador 5V | Refrigeracion controlada, pendiente de etapa fisica |

### Pines Del ESP32

| Elemento | Pin ESP32 | Nota |
|---|---:|---|
| DHT22 DATA | `GPIO4` | Alimentacion a `3V3` |
| HC-SR04 TRIG | `GPIO32` | Salida ESP32 |
| HC-SR04 ECHO | `GPIO34` | Entrada con divisor de voltaje desde 5V a 3.3V |
| MPU6050 SDA | `GPIO21` | I2C |
| MPU6050 SCL | `GPIO22` | I2C |
| LED rojo | `GPIO25` | Con resistencia |
| LED verde | `GPIO26` | Con resistencia |
| LED azul | `GPIO27` | Con resistencia |
| Servo bloqueo | `GPIO14` | Solo senal PWM; alimentacion externa 5V |
| Ventilador | `GPIO33` propuesto | Requiere MOSFET/transistor, no conectar directo |

Todas las tierras deben estar unidas:

```text
GND ESP32
GND sensores
GND fuente externa servo
GND fuente externa ventilador
```

### HC-SR04 Y Divisor De Voltaje

El pin `ECHO` del HC-SR04 entrega aproximadamente `5V`. El ESP32 no tolera 5V en sus entradas, por lo que se usa divisor de voltaje.

```text
HC-SR04 ECHO
    |
   R1 = 1k
    |
    +---- GPIO34 ESP32
    |
   R2 = 2k
    |
   GND
```

### Servo De Puerta

Conexion:

```text
Servo VCC    -> fuente externa 5V
Servo GND    -> GND comun
Servo signal -> GPIO14 ESP32
```

Configuracion actual:

```cpp
SERVO_LOCK_CLOSED_ANGLE = 180
SERVO_LOCK_OPEN_ANGLE = 90
```

Si el servo vibra o queda forzado, el ajuste correcto debe hacerse primero en la mecanica de la traba y luego en los grados de cierre. No debe quedar empujando contra un tope duro por tiempo prolongado.

### Ventilador

El ventilador no debe conectarse directo a un GPIO. La conexion recomendada es mediante MOSFET canal N logico.

```text
ESP32 GPIO33 -> resistencia -> Gate MOSFET
Source MOSFET -> GND comun
Drain MOSFET  -> negativo ventilador
positivo ventilador -> 5V externo
```

## Firmware ESP32

El firmware esta basado en PlatformIO con Arduino Framework.

```text
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

Dependencias principales:

| Libreria | Uso |
|---|---|
| `PubSubClient` | Cliente MQTT |
| `DHT sensor library` | Lectura DHT11/DHT22 |
| `Adafruit Unified Sensor` | Dependencia de DHT |
| `WiFi` / `WiFiClientSecure` | Red y TLS |
| `Wire` | I2C para MPU6050 |

### Modulos Del Firmware

| Archivo | Responsabilidad |
|---|---|
| `include/config.h` | Pines, topicos, umbrales, modos de prueba e intervalos |
| `src/main.cpp` | Orquestador principal |
| `src/sensors.cpp` | Lectura DHT22, HC-SR04 y MPU6050 |
| `src/rack_status.cpp` | Evaluacion del estado general |
| `src/status_led.cpp` | Control del LED RGB |
| `src/network_mqtt.cpp` | WiFi, MQTT, Last Will, publicacion y comandos |
| `src/telemetry.cpp` | JSON de diagnostico |
| `src/control_modes.cpp` | Perfiles `standard`, `energy_saving`, `performance` |
| `src/actuators.cpp` | Servo de puerta, ventilador logico y seguridad de acceso |

### Modos De Prueba

En `include/config.h` existen banderas para probar hardware de forma aislada:

```cpp
ENABLE_LED_GRADIENT_TEST
ENABLE_ULTRASONIC_TEST
ENABLE_DHT_TEST
ENABLE_MPU_TEST
ENABLE_SERVO_TEST
```

Para ejecutar el firmware completo, todas deben estar en `0`.

### Calibraciones Actuales

| Parametro | Valor actual | Uso |
|---|---:|---|
| `DHT_TEMPERATURE_OFFSET_C` | `-5.0` | Correccion por lectura alta del DHT |
| `DHT_HUMIDITY_OFFSET_PCT` | `-20.0` | Correccion por lectura alta de humedad |
| `DOOR_CLOSED_MAX_CM` | `8.0` | Distancia maxima para puerta cerrada |
| `MPU_NORMAL_AXIS` | `Y` | Eje normal del rack |
| `MPU_NORMAL_SIGN` | `1` | Sentido normal del eje |
| `TEMP_OPTIMAL_MAX_C` | `32.0` | Temperatura maxima optima |
| `TEMP_CRITICAL_C` | `35.0` | Temperatura critica |
| `LED_BLINK_INTERVAL_MS` | `4000` | Titileo apagado/color |

## Estados Del Rack

| Estado | Codigo | LED | Uso |
|---|---:|---|---|
| `connecting` | 0 | Azul | Conectando WiFi o MQTT |
| `offline` | 1 | Cian | Sin red o sin MQTT |
| `optimal` | 2 | Verde | Condiciones normales |
| `warning` | 3 | Amarillo | Condicion fuera de rango moderada o apertura autorizada |
| `critical` | 4 | Rojo | Condicion critica o apertura forzada |
| `sensor_error` | 5 | Morado | Error de sensor |

El LED RGB usa logica invertida:

```cpp
LED_COMMON_ANODE = true
```

## Modos De Control

| Modo | Payload | Descripcion |
|---|---|---|
| Estandar | `standard` | Balance entre temperatura y consumo |
| Ahorro | `energy_saving` | Permite temperaturas mas altas y menor ventilacion |
| Potencia | `performance` | Umbrales mas estrictos y mayor ventilacion |

Los modos se cambian por MQTT o desde la API REST.

## MQTT

Raiz de topicos:

```text
monitor-servidores/rack01
```

### Sensores Y Estado

| Topico relativo | Uso |
|---|---|
| `status/system` | Estado general |
| `status/network` | Estado de red |
| `status/reason` | Motivo principal del estado |
| `status/last_will` | Last Will MQTT |
| `status/components/temperature` | Estado de temperatura |
| `status/components/humidity` | Estado de humedad |
| `status/components/position` | Estado de orientacion/vibracion |
| `status/components/door` | Estado logico de puerta |
| `sensors/dht22/temperature_c` | Temperatura |
| `sensors/dht22/humidity_pct` | Humedad |
| `sensors/door/state` | Puerta `open` o `closed` |
| `sensors/mpu6050/position` | Posicion estimada |
| `sensors/mpu6050/tilt_deg` | Inclinacion |
| `sensors/mpu6050/vibration_level` | Vibracion |

### Comandos

| Topico relativo | Direccion | Payload |
|---|---|---|
| `commands/mode/set` | API -> ESP32 | `standard`, `energy_saving`, `performance` |
| `commands/door/unlock` | API -> ESP32 | `unlock` |
| `commands/door/lock` | API -> ESP32 | `lock` |
| `commands/fan/auto` | API -> ESP32 | `auto` |
| `commands/fan/manual` | API -> ESP32 | `0` a `100` |

### Control Y Seguridad

| Topico relativo | Uso |
|---|---|
| `control/mode` | Modo activo |
| `actuators/fan/power_pct` | Potencia logica del ventilador |
| `actuators/fan/mode` | `auto` o `manual` |
| `actuators/door_lock/state` | `locked`, `unlocked`, `locking`, `unlocking`, `lock_error` |
| `security/door/state` | Seguridad de puerta |
| `security/access/last_event` | Ultimo evento de acceso |

## Bridge Python

El bridge se encuentra en `bridge-python/`.

Responsabilidad:

```text
HiveMQ MQTT -> Bridge Python -> InfluxDB
```

Se suscribe a:

```text
monitor-servidores/rack01/#
```

Measurements principales:

| Measurement | Datos |
|---|---|
| `rack_status` | Estado general, red, motivo y Last Will |
| `rack_components` | Estado por componente |
| `rack_environment` | Temperatura y humedad |
| `rack_door` | Estado fisico de puerta |
| `rack_motion` | MPU6050, inclinacion y vibracion |
| `rack_control` | Modo activo |
| `rack_actuators` | Ventilador y bloqueo |
| `rack_security` | Seguridad de puerta y eventos de acceso |

Todos los puntos incluyen tags como `device_id` y `location`.

## API REST

La API esta en `api-rest/` y usa FastAPI.

Funciones:

- Consultar el ultimo estado desde InfluxDB.
- Publicar comandos MQTT hacia HiveMQ.
- Validar credenciales antes de desbloquear puerta.
- Servir el frontend web cuando se despliega como contenedor.

Endpoints principales:

| Metodo | Ruta | Uso |
|---|---|---|
| `GET` | `/api/health` | Estado de API |
| `GET` | `/api/rack/status` | Estado general actual |
| `GET` | `/api/rack/modes` | Modos disponibles y modo activo |
| `PUT` | `/api/rack/mode` | Cambiar modo |
| `GET` | `/api/actuators` | Estado de actuadores |
| `POST` | `/api/door/unlock` | Desbloquear con usuario y contrasena |
| `POST` | `/api/door/lock` | Bloquear puerta |
| `POST` | `/api/fan/auto` | Ventilador automatico |
| `POST` | `/api/fan/manual` | Ventilador manual |

Documentacion interactiva local:

```text
http://127.0.0.1:8000/docs
```

## Frontend

El frontend esta en `frontend/` y es una interfaz web estatica.

Vistas principales:

- Resumen general del rack.
- Modo activo y cambio de modo.
- Estado fisico de puerta y bloqueo.
- Formulario de desbloqueo con usuario y contrasena.
- Boton para cerrar puerta.
- Estado y control del ventilador.
- Acceso a Grafana o documentacion.

En despliegue Docker/Azure, el frontend se sirve desde la misma API.

## Grafana

El dashboard JSON esta en:

```text
docs/grafana_dashboard_rack_servidores.json
```

Paneles esperados:

- Estado general.
- Motivo del estado.
- Temperatura y humedad.
- Estado de puerta.
- Modo activo.
- Potencia de ventilador.
- Estado del bloqueo.
- Eventos de acceso.
- Timeline de cambios de modo.

## Docker

Servicios definidos en `docker-compose.yml`:

| Servicio | Funcion |
|---|---|
| `api` | API REST + frontend |
| `bridge` | Worker MQTT hacia InfluxDB |

Construir:

```bash
docker compose build
```

Levantar:

```bash
docker compose up -d
```

Logs:

```bash
docker compose logs -f
```

Detener:

```bash
docker compose down
```

La API local queda expuesta normalmente en:

```text
http://127.0.0.1:8001
```

## Despliegue En Azure

La arquitectura esta preparada para ejecutarse en Azure Container Apps.

| Container App | Ingress | Replica minima | Funcion |
|---|---|---:|---|
| API | Externo | 0 o 1 | API REST y frontend |
| Bridge | Deshabilitado | 1 | Worker MQTT siempre escuchando |

Recomendaciones:

- Usar Azure Container Registry para imagenes.
- Configurar variables sensibles como secretos de Container Apps.
- Mapear cada secreto a una variable de entorno.
- El bridge debe mantener minimo `1` replica para no dejar de escuchar MQTT.
- La API necesita URL publica si el frontend se consume desde navegador.

## Variables De Entorno

No se deben escribir credenciales reales en este README.

### API REST

| Variable | Uso |
|---|---|
| `DEVICE_ID` | Identificador del rack |
| `MQTT_HOST` | Broker HiveMQ |
| `MQTT_PORT` | Puerto MQTT TLS, normalmente `8883` |
| `MQTT_USERNAME` | Usuario MQTT de la API |
| `MQTT_PASSWORD` | Contrasena MQTT de la API |
| `MQTT_TLS` | Activar TLS |
| `MQTT_TLS_INSECURE` | Permitir TLS sin validar certificado para prototipo |
| `INFLUXDB_URL` | URL de InfluxDB |
| `INFLUXDB_TOKEN` | Token de InfluxDB |
| `INFLUXDB_ORG` | Organizacion InfluxDB |
| `INFLUXDB_BUCKET` | Bucket, por ejemplo `server_monitoring` |
| `INFLUXDB_QUERY_LOOKBACK` | Ventana de consulta, por ejemplo `-24h` |
| `API_UNLOCK_USERNAME` | Usuario autorizado para desbloquear |
| `API_UNLOCK_PASSWORD` | Contrasena autorizada para desbloquear |
| `API_CORS_ORIGINS` | Origenes permitidos |

### Bridge Python

| Variable | Uso |
|---|---|
| `DEVICE_ID` | Identificador del rack |
| `RACK_LOCATION` | Ubicacion o etiqueta del rack |
| `MQTT_HOST` | Broker HiveMQ |
| `MQTT_PORT` | Puerto MQTT TLS |
| `MQTT_USERNAME` | Usuario MQTT del bridge |
| `MQTT_PASSWORD` | Contrasena MQTT del bridge |
| `MQTT_TOPIC` | Topico a suscribir, normalmente `monitor-servidores/rack01/#` |
| `MQTT_TLS` | Activar TLS |
| `MQTT_TLS_INSECURE` | Permitir TLS sin validar certificado para prototipo |
| `INFLUXDB_URL` | URL de InfluxDB |
| `INFLUXDB_TOKEN` | Token de InfluxDB |
| `INFLUXDB_ORG` | Organizacion InfluxDB |
| `INFLUXDB_BUCKET` | Bucket destino |

### Firmware ESP32

Las credenciales WiFi/MQTT del ESP32 deben estar fuera del control de versiones.

Archivo esperado:

```text
include/secrets.h
```

Debe contener valores para:

```cpp
WIFI_SSID
WIFI_PASSWORD
MQTT_HOST
MQTT_PORT
MQTT_USER
MQTT_PASSWORD
```

## Politica De Secretos

No se deben versionar:

- `include/secrets.h`
- `api-rest/.env`
- `bridge-python/.env`
- tokens de InfluxDB
- usuarios y contrasenas de HiveMQ
- credenciales de desbloqueo de puerta
- URLs privadas internas o tokens de Azure

Usar archivos `.env.example`, secretos de Azure o variables de entorno con placeholders.

## Ejecucion Local

### Firmware

Compilar:

```bash
platformio run
```

Cargar al ESP32:

```bash
platformio run -t upload
```

Monitor serial:

```bash
platformio device monitor
```

### Bridge

```bash
cd bridge-python
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

### API

```bash
cd api-rest
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
uvicorn main:app --reload --host 127.0.0.1 --port 8000
```

### Frontend Sin Docker

```bash
cd frontend
python -m http.server 5173
```

Abrir:

```text
http://127.0.0.1:5173
```

## Estructura Del Proyecto

```text
.
|-- api-rest/          # FastAPI, Dockerfile y servicio HTTP
|-- bridge-python/     # Bridge MQTT -> InfluxDB
|-- docs/              # Documentacion, diagramas y dashboard Grafana
|-- frontend/          # Interfaz web estatica
|-- include/           # Headers del firmware ESP32
|-- lib/               # Librerias locales opcionales de PlatformIO
|-- src/               # Codigo fuente del firmware
|-- test/              # Carpeta de pruebas PlatformIO
|-- docker-compose.yml # Orquestacion local API + bridge
|-- platformio.ini     # Configuracion PlatformIO
`-- README.md          # Documento principal del proyecto
```

## Checklist De Validacion

### Hardware

- ESP32 enciende correctamente.
- Todos los GND estan unidos.
- DHT22 reporta temperatura y humedad.
- HC-SR04 cambia entre `closed` y `open`.
- MPU6050 reporta `vertical normal` cuando el rack esta en posicion normal.
- LED RGB muestra los colores de estado.
- Servo responde a comandos `lock` y `unlock`.
- Ventilador no esta conectado directo a GPIO.

### MQTT

- ESP32 conecta a WiFi.
- ESP32 conecta a HiveMQ.
- MQTT Explorer ve mensajes en `monitor-servidores/rack01/#`.
- Last Will publica `online` al conectar.
- Los comandos MQTT cambian estados publicados.

### Datos

- Bridge esta suscrito a HiveMQ.
- InfluxDB recibe datos recientes.
- Measurements de control existen: `rack_control`, `rack_actuators`, `rack_security`.
- Grafana consulta el bucket correcto.

### API Y Frontend

- `GET /api/health` responde correctamente.
- `GET /api/rack/status` retorna datos recientes.
- El frontend muestra estado general, modo, puerta y ventilador.
- Cambio de modo desde frontend llega al ESP32.
- Desbloqueo con credenciales validas publica `commands/door/unlock`.
- Cierre de puerta publica `commands/door/lock`.

## Problemas Comunes

| Sintoma | Causa probable | Revision |
|---|---|---|
| ESP32 se reinicia al mover servo | Fuente insuficiente o GND no comun | Usar fuente 5V externa y unir GND |
| Servo vibra en cerrado | Tope mecanico o angulo demasiado extremo | Ajustar traba o bajar grados de cierre |
| HC-SR04 no mide | ECHO sin divisor o mala tierra | Revisar divisor y GND |
| Frontend muestra datos vacios | API no consulta Influx o falta variable | Revisar logs API e `INFLUXDB_QUERY_LOOKBACK` |
| Bridge no escribe datos | No hay mensajes MQTT o credenciales incorrectas | Revisar logs bridge y topico |
| Grafana muestra `No data` | Field o measurement incorrecto | Revisar query Flux y bucket |
| API no desbloquea | Credenciales de API invalidas | Revisar variables `API_UNLOCK_*` |

## Documentacion Complementaria

| Documento | Uso |
|---|---|
| `docs/conexiones.md` | Cableado del prototipo |
| `docs/pruebas_hardware.md` | Pruebas individuales de sensores |
| `docs/mqtt_hivemq.md` | Topicos MQTT y validacion |
| `docs/influxdb_bridge.md` | Bridge y mapeo InfluxDB |
| `docs/api_rest.md` | API REST y endpoints |
| `docs/frontend.md` | Interfaz web |
| `docs/docker_local.md` | Ejecucion con Docker |
| `docs/arquitectura_monitor_rack.drawio` | Diagrama editable en diagrams.net |
| `docs/grafana_dashboard_rack_servidores.json` | Dashboard importable |

## Notas De Presentacion

Para una demostracion ordenada:

1. Encender ESP32 y mostrar monitor serial.
2. Confirmar telemetria en MQTT Explorer.
3. Mostrar InfluxDB recibiendo datos.
4. Abrir frontend y explicar estado general.
5. Cambiar modo desde frontend.
6. Probar desbloqueo y cierre de puerta.
7. Mostrar Grafana con historicos.
8. Explicar Last Will desconectando el ESP32 o simulando perdida de red.

## Pendientes Tecnicos

- Ajustar mecanicamente el servo para evitar vibracion en estado cerrado.
- Implementar etapa fisica del ventilador con MOSFET.
- Reemplazar `MQTT_ALLOW_INSECURE_TLS` por certificado raiz en una version final.
- Actualizar documentacion secundaria que aun menciona actuadores simulados.
- Validar dashboard de Grafana con datos reales de larga duracion.
- Revisar consumo total de la fuente externa con servo y ventilador conectados.
