# Bridge MQTT A InfluxDB

Este script se conecta a HiveMQ, se suscribe a los topicos del ESP32 y escribe las mediciones en InfluxDB.

Tambien guarda los topicos de la fase de control:

```text
rack_control
rack_actuators
rack_security
```

Estos measurements incluyen modo activo, potencia del ventilador, estado de bloqueo, estado de seguridad de puerta y ultimo evento de acceso.

## Instalacion

```bash
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

## Configuracion

Copiar `.env.example` como `.env` y completar:

```text
MQTT_HOST
MQTT_USERNAME
MQTT_PASSWORD
INFLUXDB_URL
INFLUXDB_TOKEN
INFLUXDB_ORG
INFLUXDB_BUCKET
```

## Ejecucion

```bash
python main.py
```

El script imprime cada mensaje recibido y confirma si fue escrito en InfluxDB.

## Ejecucion Con Docker

Construir la imagen desde esta carpeta:

```bash
docker build -t monitor-rack-bridge .
```

Ejecutar usando el archivo `.env` local:

```bash
docker run --rm --env-file .env monitor-rack-bridge
```

El contenedor no expone puertos porque es un worker: se conecta a HiveMQ, escucha los topicos MQTT y escribe en InfluxDB.

## Nota Para Azure

En Azure Container Apps este servicio debe ejecutarse como proceso continuo, con minimo `1` replica. Si escala a cero, deja de escuchar MQTT.

Las variables de `.env` deben configurarse como secretos o variables de entorno del servicio, no copiarse dentro de la imagen.
