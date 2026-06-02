import ssl
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import paho.mqtt.client as mqtt
from dotenv import load_dotenv
from fastapi import FastAPI, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from influxdb_client import InfluxDBClient
from pydantic import BaseModel, Field


load_dotenv()


APP_DIR = Path(__file__).resolve().parent
FRONTEND_DIR = APP_DIR / "frontend"
if not FRONTEND_DIR.exists():
    FRONTEND_DIR = APP_DIR.parent / "frontend"


def env(name: str, default: str | None = None, required: bool = False) -> str:
    import os

    value = os.getenv(name)
    if value is None or value == "":
        value = default
    if required and not value:
        raise RuntimeError(f"Missing required environment variable: {name}")
    return value or ""


def env_bool(name: str, default: bool = False) -> bool:
    value = env(name)
    if not value:
        return default
    return value.strip().lower() in {"1", "true", "yes", "y", "on"}


@dataclass(frozen=True)
class ApiConfig:
    device_id: str
    mqtt_host: str
    mqtt_port: int
    mqtt_username: str
    mqtt_password: str
    mqtt_tls: bool
    mqtt_tls_insecure: bool
    influx_url: str
    influx_token: str
    influx_org: str
    influx_bucket: str
    query_lookback: str
    unlock_username: str
    unlock_password: str
    cors_origins: list[str]


def load_config() -> ApiConfig:
    cors_origins = [
        origin.strip()
        for origin in env("API_CORS_ORIGINS", "http://localhost:5173,http://127.0.0.1:5173").split(",")
        if origin.strip()
    ]

    return ApiConfig(
        device_id=env("DEVICE_ID", "rack01"),
        mqtt_host=env("MQTT_HOST", required=True),
        mqtt_port=int(env("MQTT_PORT", "8883")),
        mqtt_username=env("MQTT_USERNAME", required=True),
        mqtt_password=env("MQTT_PASSWORD", required=True),
        mqtt_tls=env_bool("MQTT_TLS", True),
        mqtt_tls_insecure=env_bool("MQTT_TLS_INSECURE", False),
        influx_url=env("INFLUXDB_URL", required=True),
        influx_token=env("INFLUXDB_TOKEN", required=True),
        influx_org=env("INFLUXDB_ORG", required=True),
        influx_bucket=env("INFLUXDB_BUCKET", "server_monitoring"),
        query_lookback=env("INFLUXDB_QUERY_LOOKBACK", "-24h"),
        unlock_username=env("API_UNLOCK_USERNAME", "DianaMena"),
        unlock_password=env("API_UNLOCK_PASSWORD", "RackSeguro2026!"),
        cors_origins=cors_origins,
    )


class ModeRequest(BaseModel):
    mode: str = Field(..., examples=["standard"])


class FanManualRequest(BaseModel):
    power_pct: int = Field(..., ge=0, le=100, examples=[80])


class UnlockRequest(BaseModel):
    username: str = Field(..., examples=["DianaMena"])
    password: str = Field(..., examples=["RackSeguro2026!"])


class InfluxLatestReader:
    def __init__(self, config: ApiConfig) -> None:
        self.config = config
        self.client = InfluxDBClient(
            url=config.influx_url,
            token=config.influx_token,
            org=config.influx_org,
        )
        self.query_api = self.client.query_api()

    def close(self) -> None:
        self.client.close()

    def latest(self, measurement: str, field: str, sensor: str | None = None) -> dict[str, Any]:
        sensor_filter = f' and r.sensor == "{sensor}"' if sensor else ""
        flux = f'''
from(bucket: "{self.config.influx_bucket}")
  |> range(start: {self.config.query_lookback})
  |> filter(fn: (r) => r._measurement == "{measurement}" and r._field == "{field}" and r.device_id == "{self.config.device_id}"{sensor_filter})
  |> last()
'''
        try:
            tables = self.query_api.query(org=self.config.influx_org, query=flux)
        except Exception as exc:
            raise HTTPException(
                status_code=status.HTTP_502_BAD_GATEWAY,
                detail=f"InfluxDB query failed for {measurement}.{field}: {exc}",
            ) from exc

        for table in tables:
            for record in table.records:
                return {
                    "value": record.get_value(),
                    "time": record.get_time().isoformat() if record.get_time() else None,
                }
        return {"value": None, "time": None}


def mqtt_client(config: ApiConfig) -> mqtt.Client:
    try:
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=f"{config.device_id}-api-{int(time.time())}")
    except AttributeError:
        client = mqtt.Client(client_id=f"{config.device_id}-api-{int(time.time())}")

    client.username_pw_set(config.mqtt_username, config.mqtt_password)
    if config.mqtt_tls:
        client.tls_set(cert_reqs=ssl.CERT_NONE if config.mqtt_tls_insecure else ssl.CERT_REQUIRED)
        client.tls_insecure_set(config.mqtt_tls_insecure)
    return client


def publish_command(config: ApiConfig, topic_suffix: str, payload: str) -> dict[str, str]:
    topic = f"monitor-servidores/{config.device_id}/{topic_suffix}"
    client = mqtt_client(config)
    loop_started = False
    try:
        rc = client.connect(config.mqtt_host, config.mqtt_port, keepalive=30)
        if rc != mqtt.MQTT_ERR_SUCCESS:
            raise HTTPException(
                status_code=status.HTTP_502_BAD_GATEWAY,
                detail=f"MQTT connect failed with rc={rc}",
            )

        client.loop_start()
        loop_started = True
        result = client.publish(topic, payload, qos=1, retain=False)
        result.wait_for_publish(timeout=5)
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            raise HTTPException(
                status_code=status.HTTP_502_BAD_GATEWAY,
                detail=f"MQTT publish failed with rc={result.rc}",
            )
    finally:
        if loop_started:
            client.loop_stop()
        client.disconnect()

    return {"topic": topic, "payload": payload}


MODES = {
    "standard": {
        "label": "Estandar",
        "temperature": {"optimal_max_c": 32, "critical_c": 35},
        "humidity": {"warning_low": 30, "warning_high": 65, "critical_low": 25, "critical_high": 75},
        "fan": {"base_pct": 25, "warning_pct": 60, "critical_pct": 100, "sensor_error_pct": 80},
    },
    "energy_saving": {
        "label": "Ahorro de energia",
        "temperature": {"optimal_max_c": 34, "critical_c": 38},
        "humidity": {"warning_low": 25, "warning_high": 70, "critical_low": 20, "critical_high": 80},
        "fan": {"base_pct": 0, "warning_pct": 40, "critical_pct": 80, "sensor_error_pct": 60},
    },
    "performance": {
        "label": "Potencia",
        "temperature": {"optimal_max_c": 28, "critical_c": 33},
        "humidity": {"warning_low": 35, "warning_high": 60, "critical_low": 30, "critical_high": 70},
        "fan": {"base_pct": 50, "warning_pct": 80, "critical_pct": 100, "sensor_error_pct": 100},
    },
}


config = load_config()
reader = InfluxLatestReader(config)

app = FastAPI(
    title="API Monitoreo Rack De Servidores",
    version="1.0.0",
    description="API REST para consultar estado del rack y enviar comandos MQTT al ESP32.",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=config.cors_origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.on_event("shutdown")
def shutdown() -> None:
    reader.close()


def latest_value(measurement: str, field: str, sensor: str | None = None) -> Any:
    return reader.latest(measurement, field, sensor)["value"]


@app.get("/api/health")
def health() -> dict[str, str]:
    return {"status": "ok", "device_id": config.device_id}


@app.get("/api/rack/status")
def rack_status() -> dict[str, Any]:
    return {
        "device_id": config.device_id,
        "mode": latest_value("rack_control", "mode", "mode"),
        "status": latest_value("rack_status", "system_status", "system"),
        "reason": latest_value("rack_status", "status_reason", "system"),
        "temperature_c": latest_value("rack_environment", "temperature_c", "dht22"),
        "humidity_pct": latest_value("rack_environment", "humidity_pct", "dht22"),
        "door_state": latest_value("rack_door", "door_state", "door"),
        "door_lock_state": latest_value("rack_actuators", "door_lock_state", "door_lock"),
        "door_security_state": latest_value("rack_security", "door_security_state", "door"),
        "fan_power_pct": latest_value("rack_actuators", "fan_power_pct", "fan"),
        "fan_mode": latest_value("rack_actuators", "fan_mode", "fan"),
        "mpu_position": latest_value("rack_motion", "position", "mpu6050"),
        "network": latest_value("rack_status", "network_status", "network"),
        "last_will": latest_value("rack_status", "last_will", "last_will"),
        "access_event": latest_value("rack_security", "access_event", "access"),
    }


@app.get("/api/rack/modes")
def rack_modes() -> dict[str, Any]:
    return {
        "active_mode": latest_value("rack_control", "mode", "mode"),
        "modes": MODES,
    }


@app.put("/api/rack/mode")
def set_rack_mode(request: ModeRequest) -> dict[str, Any]:
    mode = request.mode.strip().lower()
    if mode not in MODES:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Invalid mode. Use one of: {', '.join(MODES)}",
        )

    command = publish_command(config, "commands/mode/set", mode)
    return {"accepted": True, "mode": mode, "command": command}


@app.get("/api/actuators")
def actuators() -> dict[str, Any]:
    return {
        "fan": {
            "mode": latest_value("rack_actuators", "fan_mode", "fan"),
            "power_pct": latest_value("rack_actuators", "fan_power_pct", "fan"),
            "auto": latest_value("rack_actuators", "fan_auto", "fan"),
        },
        "door_lock": {
            "state": latest_value("rack_actuators", "door_lock_state", "door_lock"),
            "locked": latest_value("rack_actuators", "door_locked", "door_lock"),
        },
        "security": {
            "door_state": latest_value("rack_security", "door_security_state", "door"),
            "access_event": latest_value("rack_security", "access_event", "access"),
            "forced_open": latest_value("rack_security", "forced_open", "door"),
        },
    }


@app.post("/api/door/unlock")
def unlock_door(request: UnlockRequest) -> dict[str, Any]:
    if request.username != config.unlock_username or request.password != config.unlock_password:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid username or password")

    command = publish_command(config, "commands/door/unlock", "unlock")
    return {"accepted": True, "door_lock_state": "unlocking", "command": command}


@app.post("/api/door/lock")
def lock_door() -> dict[str, Any]:
    command = publish_command(config, "commands/door/lock", "lock")
    return {"accepted": True, "door_lock_state": "locking", "command": command}


@app.post("/api/fan/auto")
def fan_auto() -> dict[str, Any]:
    command = publish_command(config, "commands/fan/auto", "auto")
    return {"accepted": True, "fan_mode": "auto", "command": command}


@app.post("/api/fan/manual")
def fan_manual(request: FanManualRequest) -> dict[str, Any]:
    payload = str(request.power_pct)
    command = publish_command(config, "commands/fan/manual", payload)
    return {"accepted": True, "fan_mode": "manual", "fan_power_pct": request.power_pct, "command": command}


if FRONTEND_DIR.exists():
    app.mount("/assets", StaticFiles(directory=FRONTEND_DIR), name="frontend-assets")


@app.get("/", include_in_schema=False)
def frontend_index() -> FileResponse:
    index_path = FRONTEND_DIR / "index.html"
    if not index_path.exists():
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Frontend not bundled")
    return FileResponse(index_path)


@app.get("/{path:path}", include_in_schema=False)
def frontend_fallback(path: str) -> FileResponse:
    if path.startswith("api/") or path == "docs" or path.startswith("openapi"):
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Not found")

    requested_path = FRONTEND_DIR / path
    if requested_path.is_file():
        return FileResponse(requested_path)

    index_path = FRONTEND_DIR / "index.html"
    if not index_path.exists():
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Frontend not bundled")
    return FileResponse(index_path)
