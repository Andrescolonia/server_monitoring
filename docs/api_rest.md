# API REST

La carpeta `api-rest/` contiene el servicio FastAPI para operar el prototipo desde HTTP.

La API cumple tres funciones:

1. Consultar los ultimos valores guardados en InfluxDB.
2. Publicar comandos MQTT hacia HiveMQ para que el ESP32 cambie modo, ventilador o bloqueo.
3. Validar usuario y contrasena antes de desbloquear la puerta.

## Archivos

```text
api-rest/
├── main.py
├── requirements.txt
├── .env.example
└── README.md
```

## Endpoints Implementados

| Metodo | Ruta | Uso |
|---|---|---|
| `GET` | `/api/health` | Verificar que la API esta viva |
| `GET` | `/api/rack/status` | Consultar estado general actual |
| `GET` | `/api/rack/modes` | Consultar modos disponibles y modo activo |
| `PUT` | `/api/rack/mode` | Cambiar modo del rack |
| `GET` | `/api/actuators` | Consultar ventilador, bloqueo y seguridad |
| `POST` | `/api/door/unlock` | Desbloquear puerta con usuario/contrasena |
| `POST` | `/api/door/lock` | Bloquear puerta |
| `POST` | `/api/fan/auto` | Poner ventilador en automatico |
| `POST` | `/api/fan/manual` | Fijar potencia manual del ventilador |

## Flujo De Datos

```text
Frontend -> API REST -> HiveMQ -> ESP32
Frontend -> API REST -> InfluxDB
```

La API no habla directamente con el ESP32. Los cambios se envian como comandos MQTT y el ESP32 responde publicando nuevos estados.

## Variables De Entorno

Crear:

```text
api-rest/.env
```

desde:

```text
api-rest/.env.example
```

Variables principales:

| Variable | Uso |
|---|---|
| `MQTT_HOST` / `MQTT_PORT` | Conexion a HiveMQ |
| `MQTT_USERNAME` / `MQTT_PASSWORD` | Credenciales MQTT |
| `INFLUXDB_URL` / `INFLUXDB_TOKEN` | Conexion a InfluxDB |
| `INFLUXDB_ORG` / `INFLUXDB_BUCKET` | Organizacion y bucket |
| `API_UNLOCK_USERNAME` / `API_UNLOCK_PASSWORD` | Credenciales para desbloqueo |
| `API_CORS_ORIGINS` | Origenes permitidos para el frontend |

## Ejecucion

```bash
cd api-rest
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
uvicorn main:app --reload --host 127.0.0.1 --port 8000
```

Documentacion interactiva:

```text
http://127.0.0.1:8000/docs
```

## Validacion

1. Ejecutar el bridge Python.
2. Ejecutar la API REST.
3. Confirmar `GET /api/rack/status`.
4. Probar `PUT /api/rack/mode` con `performance`.
5. Confirmar en MQTT Explorer que cambia `control/mode`.
6. Probar `POST /api/fan/manual` con `80`.
7. Probar `POST /api/door/unlock` con credenciales correctas.
8. Probar el mismo unlock con credenciales incorrectas y confirmar error `401`.

Credencial de prueba recomendada:

```text
API_UNLOCK_USERNAME=DianaMena
API_UNLOCK_PASSWORD=RackSeguro2026!
```
