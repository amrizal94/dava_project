import asyncio
import json
import logging
import os
from datetime import datetime, timezone

import paho.mqtt.client as mqtt
from sqlalchemy import select, update
from sqlalchemy.dialects.postgresql import insert

from .database import AsyncSessionLocal
from .models import Device, SensorReading, DeviceSettings, LightMode

logger = logging.getLogger(__name__)

MQTT_HOST = os.getenv("MQTT_HOST", "localhost")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_USERNAME = os.getenv("MQTT_USERNAME", "")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD", "")

_loop: asyncio.AbstractEventLoop | None = None
_sse_queues: list[asyncio.Queue] = []
_mqtt_client: mqtt.Client | None = None


def get_mqtt_client() -> mqtt.Client | None:
    return _mqtt_client


def register_sse_queue(q: asyncio.Queue):
    _sse_queues.append(q)


def unregister_sse_queue(q: asyncio.Queue):
    _sse_queues.discard(q) if hasattr(_sse_queues, "discard") else None
    if q in _sse_queues:
        _sse_queues.remove(q)


def _broadcast(event: dict):
    if not _loop:
        return
    data = json.dumps(event)
    for q in list(_sse_queues):
        _loop.call_soon_threadsafe(q.put_nowait, data)


async def _handle_register(mac: str, payload: dict):
    async with AsyncSessionLocal() as db:
        stmt = insert(Device).values(
            mac_address=mac,
            alias=payload.get("alias", ""),
            location=payload.get("location", ""),
            firmware_version=payload.get("firmware", ""),
            is_online=True,
            last_seen=datetime.now(timezone.utc),
        ).on_conflict_do_update(
            index_elements=["mac_address"],
            set_={
                "is_online": True,
                "last_seen": datetime.now(timezone.utc),
                "firmware_version": payload.get("firmware", ""),
            },
        )
        await db.execute(stmt)

        # Create default settings if not exist
        existing = await db.get(DeviceSettings, mac)
        if not existing:
            db.add(DeviceSettings(device_mac=mac))

        await db.commit()

    logger.info(f"Device registered: {mac}")
    _broadcast({"event": "device_online", "mac": mac})


async def _handle_telemetry(mac: str, payload: dict):
    async with AsyncSessionLocal() as db:
        # Upsert device online status
        await db.execute(
            update(Device)
            .where(Device.mac_address == mac)
            .values(is_online=True, last_seen=datetime.now(timezone.utc))
        )

        reading = SensorReading(
            device_mac=mac,
            temp_indoor=payload.get("temp_in"),
            temp_outdoor=payload.get("temp_out"),
            lux=payload.get("lux"),
            voltage=payload.get("voltage"),
            current=payload.get("current"),
            power=payload.get("power"),
            frequency=payload.get("frequency"),
            power_factor=payload.get("power_factor"),
            power_status=payload.get("power_status"),
        )
        db.add(reading)
        await db.commit()

    sse_data = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "temp_indoor":    payload.get("temp_in"),
        "temp_outdoor":   payload.get("temp_out"),
        "lux":            payload.get("lux"),
        "voltage":        payload.get("voltage"),
        "current":        payload.get("current"),
        "power":          payload.get("power"),
        "frequency":      payload.get("frequency"),
        "power_factor":   payload.get("power_factor"),
        "power_status":   payload.get("power_status"),
    }
    _broadcast({"event": "telemetry", "mac": mac, "data": sse_data})


async def _handle_status(mac: str, payload: dict):
    async with AsyncSessionLocal() as db:
        await db.execute(
            update(Device)
            .where(Device.mac_address == mac)
            .values(is_online=True, last_seen=datetime.now(timezone.utc))
        )
        await db.commit()

    _broadcast({"event": "status", "mac": mac})


async def _handle_ack_light(mac: str, payload: dict):
    _broadcast({"event": "light_ack", "mac": mac, "data": payload})


def _on_message(client, userdata, msg: mqtt.MQTTMessage):
    try:
        parts = msg.topic.split("/")
        # Expected: dava/{mac}/telemetry|status|register|ack/light
        if len(parts) < 3 or parts[0] != "dava":
            return

        mac = parts[1]
        action = parts[2]

        try:
            payload = json.loads(msg.payload.decode())
        except Exception:
            payload = {}

        if not _loop:
            return

        if action == "register":
            asyncio.run_coroutine_threadsafe(_handle_register(mac, payload), _loop)
        elif action == "telemetry":
            asyncio.run_coroutine_threadsafe(_handle_telemetry(mac, payload), _loop)
        elif action == "status":
            asyncio.run_coroutine_threadsafe(_handle_status(mac, payload), _loop)
        elif action == "ack" and len(parts) >= 4 and parts[3] == "light":
            asyncio.run_coroutine_threadsafe(_handle_ack_light(mac, payload), _loop)

    except Exception as e:
        logger.error(f"MQTT message error: {e}")


def _on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        client.subscribe("dava/+/register")
        client.subscribe("dava/+/telemetry")
        client.subscribe("dava/+/status")
        client.subscribe("dava/+/ack/light")
        logger.info("MQTT connected and subscribed")
    else:
        logger.error(f"MQTT connect failed rc={rc}")


def start_mqtt(loop: asyncio.AbstractEventLoop):
    global _loop, _mqtt_client

    _loop = loop
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = _on_connect
    client.on_message = _on_message

    if MQTT_USERNAME:
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

    try:
        client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
        client.loop_start()
        _mqtt_client = client
        logger.info(f"MQTT connecting to {MQTT_HOST}:{MQTT_PORT}")
    except Exception as e:
        logger.warning(f"MQTT not available: {e}")


def publish_light_command(mac: str, mode: str, brightness: int | None = None):
    if not _mqtt_client:
        return
    payload = {"mode": mode}
    if brightness is not None:
        payload["brightness"] = brightness
    _mqtt_client.publish(f"dava/{mac}/control/light", json.dumps(payload), qos=1)
