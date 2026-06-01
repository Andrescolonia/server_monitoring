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
