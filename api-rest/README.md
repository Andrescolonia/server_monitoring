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

## Ejecucion Con Docker

Construir la imagen desde la raiz del proyecto. Esta imagen incluye la API y el frontend estatico:

```bash
docker build -f api-rest/Dockerfile -t monitor-rack-api .
```

Ejecutar usando el `.env` local de la API:

```bash
docker run --rm --env-file api-rest/.env -p 8000:8000 monitor-rack-api
```

Si el puerto `8000` ya esta ocupado, usar otro puerto local:

```bash
docker run --rm --env-file api-rest/.env -p 8001:8000 monitor-rack-api
```

En cloud, configurar las variables de entorno como secretos del servicio. No copiar `.env` dentro de la imagen.

El frontend queda servido por la misma API:

```text
http://127.0.0.1:8000/
```

Para un frontend desplegado en Azure, agregar su URL en:

```text
API_CORS_ORIGINS=http://localhost:5173,http://127.0.0.1:5173,https://TU_FRONTEND.azurestaticapps.net
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
