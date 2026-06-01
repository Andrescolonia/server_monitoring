# API REST Del Rack

Esta API consulta los ultimos datos en InfluxDB y publica comandos MQTT hacia el ESP32 por HiveMQ.

## Instalacion

```bash
cd api-rest
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

## Configuracion

Copiar `.env.example` como `.env` y completar credenciales:

```text
MQTT_HOST
MQTT_USERNAME
MQTT_PASSWORD
INFLUXDB_URL
INFLUXDB_TOKEN
INFLUXDB_ORG
INFLUXDB_BUCKET
API_UNLOCK_USERNAME
API_UNLOCK_PASSWORD
```

Puede usar el mismo usuario MQTT del bridge o uno dedicado para la API.

## Ejecucion

```bash
uvicorn main:app --reload --host 127.0.0.1 --port 8000
```

Documentacion interactiva:

```text
http://127.0.0.1:8000/docs
```

## Endpoints

| Metodo | Ruta | Uso |
|---|---|---|
| `GET` | `/api/health` | Salud basica de la API |
| `GET` | `/api/rack/status` | Ultimo estado general del rack |
| `GET` | `/api/rack/modes` | Modos disponibles y umbrales |
| `PUT` | `/api/rack/mode` | Cambiar modo de operacion |
| `GET` | `/api/actuators` | Estado del ventilador, bloqueo y seguridad |
| `POST` | `/api/door/unlock` | Desbloquear puerta con usuario y contrasena |
| `POST` | `/api/door/lock` | Bloquear puerta |
| `POST` | `/api/fan/auto` | Volver ventilador a automatico |
| `POST` | `/api/fan/manual` | Fijar potencia manual |

## Ejemplos

Cambiar modo:

```bash
curl -X PUT http://127.0.0.1:8000/api/rack/mode ^
  -H "Content-Type: application/json" ^
  -d "{\"mode\":\"performance\"}"
```

Desbloquear puerta:

```bash
curl -X POST http://127.0.0.1:8000/api/door/unlock ^
  -H "Content-Type: application/json" ^
  -d "{\"username\":\"DianaMena\",\"password\":\"RackSeguro2026!\"}"
```

Ventilador manual:

```bash
curl -X POST http://127.0.0.1:8000/api/fan/manual ^
  -H "Content-Type: application/json" ^
  -d "{\"power_pct\":80}"
```
