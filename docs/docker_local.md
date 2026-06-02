# Docker Local

Este proyecto puede ejecutarse localmente con Docker usando dos servicios:

| Servicio | Imagen | Uso |
|---|---|---|
| `api` | `monitor-rack-api` | API REST + frontend web |
| `bridge` | `monitor-rack-bridge` | Worker MQTT hacia InfluxDB |

## Requisitos

- Docker Desktop activo.
- `api-rest/.env` completo.
- `bridge-python/.env` completo.
- ESP32 publicando en HiveMQ.

El `docker-compose.yml` monta los archivos `.env` como `/app/.env` dentro de cada contenedor. Esto evita que Docker Compose interprete caracteres especiales como `$` dentro de tokens o contrasenas.

## Construir Imagenes

Desde la raiz del proyecto:

```bash
docker compose build
```

Si Docker Desktop esta abierto pero Compose dice que el daemon no responde, seleccionar el contexto de Docker Desktop:

```bash
docker context use desktop-linux
```

## Levantar Todo

```bash
docker compose up
```

Abrir:

```text
http://127.0.0.1:8001
```

API docs:

```text
http://127.0.0.1:8001/docs
```

## Levantar En Segundo Plano

```bash
docker compose up -d
```

Ver logs:

```bash
docker compose logs -f
```

Detener:

```bash
docker compose down
```

## Nota Importante

No ejecutes al mismo tiempo el bridge local con `python main.py` y el bridge de Docker, porque ambos escucharian los mismos topicos MQTT y escribirian datos duplicados en InfluxDB.

Si solo quieres probar API + frontend:

```bash
docker compose up api
```

Si solo quieres probar el bridge:

```bash
docker compose up bridge
```
