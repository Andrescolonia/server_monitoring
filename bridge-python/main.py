import json
import os
import signal
import ssl
import sys
import time
from dataclasses import dataclass
from typing import Any, Callable

import paho.mqtt.client as mqtt
from dotenv import load_dotenv
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS


load_dotenv()


def env(name: str, default: str | None = None, required: bool = False) -> str:
    value = os.getenv(name, default)
    if required and not value:
        raise RuntimeError(f"Missing required environment variable: {name}")
    return value or ""


def env_bool(name: str, default: bool = False) -> bool:
    value = os.getenv(name)
    if value is None:
        return default
    return value.strip().lower() in {"1", "true", "yes", "y", "on"}


@dataclass(frozen=True)
class BridgeConfig:
    device_id: str
    location: str
    mqtt_host: str
    mqtt_port: int
    mqtt_username: str
    mqtt_password: str
    mqtt_topic: str
    mqtt_tls: bool
    mqtt_tls_insecure: bool
    influx_url: str
    influx_token: str
    influx_org: str
    influx_bucket: str


def load_config() -> BridgeConfig:
    device_id = env("DEVICE_ID", "rack01")
    return BridgeConfig(
        device_id=device_id,
        location=env("RACK_LOCATION", "maqueta"),
        mqtt_host=env("MQTT_HOST", required=True),
        mqtt_port=int(env("MQTT_PORT", "8883")),
        mqtt_username=env("MQTT_USERNAME", required=True),
        mqtt_password=env("MQTT_PASSWORD", required=True),
        mqtt_topic=env("MQTT_TOPIC", f"monitor-servidores/{device_id}/#"),
        mqtt_tls=env_bool("MQTT_TLS", True),
        mqtt_tls_insecure=env_bool("MQTT_TLS_INSECURE", False),
        influx_url=env("INFLUXDB_URL", required=True),
        influx_token=env("INFLUXDB_TOKEN", required=True),
        influx_org=env("INFLUXDB_ORG", required=True),
        influx_bucket=env("INFLUXDB_BUCKET", "server_monitoring"),
    )


def parse_float(payload: str) -> float | None:
    if payload.lower() == "null":
        return None
    return float(payload)


def status_code(status: str) -> int:
    return {
        "connecting": 0,
        "offline": 1,
        "optimal": 2,
        "warning": 3,
        "critical": 4,
        "sensor_error": 5,
    }.get(status, -1)


def network_code(status: str) -> int:
    return 1 if status == "online" else 0


def door_code(state: str) -> int:
    return 1 if state == "open" else 0


def control_mode_code(mode: str) -> int:
    return {
        "standard": 0,
        "energy_saving": 1,
        "performance": 2,
    }.get(mode, -1)


def fan_mode_code(mode: str) -> int:
    return {
        "manual": 0,
        "auto": 1,
    }.get(mode, -1)


def door_lock_code(state: str) -> int:
    return {
        "locked": 0,
        "unlocked": 1,
        "locking": 2,
        "unlocking": 3,
        "lock_error": 4,
    }.get(state, -1)


def door_security_code(state: str) -> int:
    return {
        "door_secure": 0,
        "door_unlocked_closed": 1,
        "door_open_authorized": 2,
        "door_open_forced": 3,
        "door_lock_error": 4,
    }.get(state, -1)


def access_event_code(event: str) -> int:
    return {
        "none": 0,
        "granted": 1,
        "denied": 2,
        "timeout": 3,
        "forced_open": 4,
        "manual_lock": 5,
    }.get(event, -1)


class InfluxWriter:
    def __init__(self, config: BridgeConfig) -> None:
        self.config = config
        self.client = InfluxDBClient(
            url=config.influx_url,
            token=config.influx_token,
            org=config.influx_org,
        )
        self.write_api = self.client.write_api(write_options=SYNCHRONOUS)

    def close(self) -> None:
        self.client.close()

    def write_point(self, point: Point) -> None:
        self.write_api.write(
            bucket=self.config.influx_bucket,
            org=self.config.influx_org,
            record=point,
        )

    def base_point(self, measurement: str, sensor: str) -> Point:
        return (
            Point(measurement)
            .tag("device_id", self.config.device_id)
            .tag("location", self.config.location)
            .tag("sensor", sensor)
        )


def write_numeric(
    writer: InfluxWriter,
    measurement: str,
    sensor: str,
    field: str,
    payload: str,
) -> None:
    value = parse_float(payload)
    if value is None:
        return

    point = (
        writer.base_point(measurement, sensor)
        .field(field, value)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_system_status(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_status", "system")
        .field("system_status", payload)
        .field("status_code", status_code(payload))
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_network_status(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_status", "network")
        .field("network_status", payload)
        .field("network_online", network_code(payload))
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_status_reason(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_status", "system")
        .field("status_reason", payload)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_last_will(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_status", "last_will")
        .field("last_will", payload)
        .field("last_will_online", 1 if payload == "online" else 0)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_component_state(writer: InfluxWriter, component: str, payload: str) -> None:
    point = (
        writer.base_point("rack_components", component)
        .field("component_state", payload)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_door_state(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_door", "door")
        .field("door_state", payload)
        .field("door_open", door_code(payload))
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_mpu_position(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_motion", "mpu6050")
        .field("position", payload)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_control_mode(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_control", "mode")
        .field("mode", payload)
        .field("mode_code", control_mode_code(payload))
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_fan_mode(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_actuators", "fan")
        .field("fan_mode", payload)
        .field("fan_mode_code", fan_mode_code(payload))
        .field("fan_auto", 1 if payload == "auto" else 0)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_door_lock_state(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_actuators", "door_lock")
        .field("door_lock_state", payload)
        .field("door_lock_code", door_lock_code(payload))
        .field("door_locked", 1 if payload == "locked" else 0)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_door_security_state(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_security", "door")
        .field("door_security_state", payload)
        .field("door_security_code", door_security_code(payload))
        .field("forced_open", 1 if payload == "door_open_forced" else 0)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


def write_access_event(writer: InfluxWriter, payload: str) -> None:
    point = (
        writer.base_point("rack_security", "access")
        .field("access_event", payload)
        .field("access_event_code", access_event_code(payload))
        .field("access_granted", 1 if payload == "granted" else 0)
        .time(time.time_ns(), WritePrecision.NS)
    )
    writer.write_point(point)


TopicHandler = Callable[[InfluxWriter, str], None]


def build_topic_handlers(device_id: str) -> dict[str, TopicHandler]:
    prefix = f"monitor-servidores/{device_id}"

    return {
        f"{prefix}/status/system": write_system_status,
        f"{prefix}/status/network": write_network_status,
        f"{prefix}/status/reason": write_status_reason,
        f"{prefix}/status/last_will": write_last_will,
        f"{prefix}/status/components/temperature": (
            lambda writer, payload: write_component_state(writer, "temperature", payload)
        ),
        f"{prefix}/status/components/humidity": (
            lambda writer, payload: write_component_state(writer, "humidity", payload)
        ),
        f"{prefix}/status/components/position": (
            lambda writer, payload: write_component_state(writer, "position", payload)
        ),
        f"{prefix}/status/components/door": (
            lambda writer, payload: write_component_state(writer, "door", payload)
        ),
        f"{prefix}/sensors/door/state": write_door_state,
        f"{prefix}/sensors/mpu6050/position": write_mpu_position,
        f"{prefix}/sensors/dht22/temperature_c": (
            lambda writer, payload: write_numeric(writer, "rack_environment", "dht22", "temperature_c", payload)
        ),
        f"{prefix}/sensors/dht22/humidity_pct": (
            lambda writer, payload: write_numeric(writer, "rack_environment", "dht22", "humidity_pct", payload)
        ),
        f"{prefix}/sensors/mpu6050/accel_x": (
            lambda writer, payload: write_numeric(writer, "rack_motion", "mpu6050", "accel_x", payload)
        ),
        f"{prefix}/sensors/mpu6050/accel_y": (
            lambda writer, payload: write_numeric(writer, "rack_motion", "mpu6050", "accel_y", payload)
        ),
        f"{prefix}/sensors/mpu6050/accel_z": (
            lambda writer, payload: write_numeric(writer, "rack_motion", "mpu6050", "accel_z", payload)
        ),
        f"{prefix}/sensors/mpu6050/tilt_deg": (
            lambda writer, payload: write_numeric(writer, "rack_motion", "mpu6050", "tilt_deg", payload)
        ),
        f"{prefix}/sensors/mpu6050/vibration_level": (
            lambda writer, payload: write_numeric(writer, "rack_motion", "mpu6050", "vibration_level", payload)
        ),
        f"{prefix}/sensors/mpu6050/pitch_deg": (
            lambda writer, payload: write_numeric(writer, "rack_motion", "mpu6050", "pitch_deg", payload)
        ),
        f"{prefix}/sensors/mpu6050/roll_deg": (
            lambda writer, payload: write_numeric(writer, "rack_motion", "mpu6050", "roll_deg", payload)
        ),
        f"{prefix}/control/mode": write_control_mode,
        f"{prefix}/actuators/fan/power_pct": (
            lambda writer, payload: write_numeric(writer, "rack_actuators", "fan", "fan_power_pct", payload)
        ),
        f"{prefix}/actuators/fan/mode": write_fan_mode,
        f"{prefix}/actuators/door_lock/state": write_door_lock_state,
        f"{prefix}/security/door/state": write_door_security_state,
        f"{prefix}/security/access/last_event": write_access_event,
    }


def mqtt_connect_succeeded(reason_code: Any) -> bool:
    is_failure = getattr(reason_code, "is_failure", None)
    if is_failure is not None:
        return not bool(is_failure)

    try:
        return int(reason_code) == 0
    except (TypeError, ValueError):
        return str(reason_code).lower() in {"0", "success"}


def create_mqtt_client(config: BridgeConfig, writer: InfluxWriter) -> mqtt.Client:
    try:
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=f"{config.device_id}-bridge")
    except AttributeError:
        client = mqtt.Client(client_id=f"{config.device_id}-bridge")

    handlers = build_topic_handlers(config.device_id)

    def on_connect(client: mqtt.Client, userdata: Any, flags: Any, reason_code: Any, properties: Any = None) -> None:
        if mqtt_connect_succeeded(reason_code):
            print(f"MQTT connected. Subscribing to {config.mqtt_topic}")
            client.subscribe(config.mqtt_topic)
        else:
            print(f"MQTT connection failed: {reason_code}")

    def on_message(client: mqtt.Client, userdata: Any, message: mqtt.MQTTMessage) -> None:
        topic = message.topic
        payload = message.payload.decode("utf-8", errors="replace").strip()
        handler = handlers.get(topic)

        if handler is None:
            print(f"Ignored topic {topic}: {payload}")
            return

        try:
            handler(writer, payload)
            print(json.dumps({"topic": topic, "payload": payload, "written": True}))
        except Exception as exc:
            print(json.dumps({"topic": topic, "payload": payload, "written": False, "error": str(exc)}))

    client.username_pw_set(config.mqtt_username, config.mqtt_password)
    client.on_connect = on_connect
    client.on_message = on_message

    if config.mqtt_tls:
        client.tls_set(cert_reqs=ssl.CERT_NONE if config.mqtt_tls_insecure else ssl.CERT_REQUIRED)
        client.tls_insecure_set(config.mqtt_tls_insecure)

    return client


def main() -> int:
    config = load_config()
    writer = InfluxWriter(config)
    client = create_mqtt_client(config, writer)
    stop = False

    def request_stop(signum: int, frame: Any) -> None:
        nonlocal stop
        stop = True

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)

    print(f"Connecting to MQTT {config.mqtt_host}:{config.mqtt_port}")
    client.connect(config.mqtt_host, config.mqtt_port, keepalive=60)
    client.loop_start()

    try:
        while not stop:
            time.sleep(0.25)
    finally:
        print("Stopping bridge...")
        client.loop_stop()
        client.disconnect()
        writer.close()

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"Bridge failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
