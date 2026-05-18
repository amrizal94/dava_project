from datetime import datetime, timezone
from typing import Annotated
from fastapi import APIRouter, Depends, HTTPException
from pydantic import BaseModel
from sqlalchemy import select, desc
from sqlalchemy.ext.asyncio import AsyncSession
from sqlalchemy.dialects.postgresql import insert

from ..database import get_db
from ..models import Device, DeviceSettings, SensorReading, LightMode
from ..mqtt_handler import publish_light_command

router = APIRouter(prefix="/api/devices", tags=["devices"])


class DeviceUpdate(BaseModel):
    alias: str | None = None
    location: str | None = None
    latitude: float | None = None
    longitude: float | None = None

    model_config = {"extra": "ignore"}


class LightCommand(BaseModel):
    mode: str  # "auto" or "manual"
    brightness: int | None = None  # 1-100, only for manual


@router.get("")
async def list_devices(db: AsyncSession = Depends(get_db)):
    result = await db.execute(select(Device).order_by(Device.created_at))
    devices = result.scalars().all()
    return [_device_dict(d) for d in devices]


@router.get("/{mac}")
async def get_device(mac: str, db: AsyncSession = Depends(get_db)):
    device = await db.get(Device, mac)
    if not device:
        raise HTTPException(404, "Device not found")
    return _device_dict(device)


@router.patch("/{mac}")
async def update_device(mac: str, body: DeviceUpdate, db: AsyncSession = Depends(get_db)):
    device = await db.get(Device, mac)
    if not device:
        raise HTTPException(404, "Device not found")
    if body.alias is not None:
        device.alias = body.alias
    if body.location is not None:
        device.location = body.location
    # latitude/longitude bisa diset ke null (hapus koordinat)
    if "latitude" in body.model_fields_set:
        device.latitude = body.latitude
    if "longitude" in body.model_fields_set:
        device.longitude = body.longitude
    await db.commit()
    return _device_dict(device)


@router.delete("/{mac}")
async def delete_device(mac: str, db: AsyncSession = Depends(get_db)):
    device = await db.get(Device, mac)
    if not device:
        raise HTTPException(404, "Device not found")
    await db.delete(device)
    await db.commit()
    return {"status": "deleted", "mac_address": mac}


@router.get("/{mac}/readings")
async def get_readings(mac: str, limit: int = 100, db: AsyncSession = Depends(get_db)):
    result = await db.execute(
        select(SensorReading)
        .where(SensorReading.device_mac == mac)
        .order_by(desc(SensorReading.timestamp))
        .limit(limit)
    )
    readings = result.scalars().all()
    return [_reading_dict(r) for r in reversed(readings)]


@router.get("/{mac}/latest")
async def get_latest(mac: str, db: AsyncSession = Depends(get_db)):
    result = await db.execute(
        select(SensorReading)
        .where(SensorReading.device_mac == mac)
        .order_by(desc(SensorReading.timestamp))
        .limit(1)
    )
    reading = result.scalar_one_or_none()
    if not reading:
        raise HTTPException(404, "No readings yet")
    return _reading_dict(reading)


@router.post("/{mac}/light")
async def control_light(mac: str, cmd: LightCommand, db: AsyncSession = Depends(get_db)):
    if cmd.mode not in ("auto", "manual"):
        raise HTTPException(400, "mode must be 'auto' or 'manual'")
    if cmd.mode == "manual" and (cmd.brightness is None or not (1 <= cmd.brightness <= 100)):
        raise HTTPException(400, "brightness must be 1-100 for manual mode")

    device = await db.get(Device, mac)
    if not device:
        raise HTTPException(404, "Device not found")

    settings = await db.get(DeviceSettings, mac)
    if not settings:
        settings = DeviceSettings(device_mac=mac)
        db.add(settings)

    settings.light_mode = LightMode(cmd.mode)
    if cmd.mode == "manual" and cmd.brightness is not None:
        settings.light_brightness = cmd.brightness
    settings.updated_at = datetime.now(timezone.utc)
    await db.commit()

    publish_light_command(mac, cmd.mode, cmd.brightness if cmd.mode == "manual" else None)
    return {"status": "sent", "mode": cmd.mode, "brightness": cmd.brightness}


@router.get("/{mac}/settings")
async def get_settings(mac: str, db: AsyncSession = Depends(get_db)):
    settings = await db.get(DeviceSettings, mac)
    if not settings:
        return {"light_mode": "auto", "light_brightness": 100}
    return {"light_mode": settings.light_mode.value, "light_brightness": settings.light_brightness}


def _device_dict(d: Device) -> dict:
    return {
        "mac_address": d.mac_address,
        "alias": d.alias or d.mac_address,
        "location": d.location,
        "is_online": d.is_online,
        "last_seen": d.last_seen.isoformat() if d.last_seen else None,
        "firmware_version": d.firmware_version,
        "created_at": d.created_at.isoformat(),
        "latitude": d.latitude,
        "longitude": d.longitude,
    }


def _reading_dict(r: SensorReading) -> dict:
    return {
        "id": r.id,
        "timestamp": r.timestamp.isoformat(),
        "temp_indoor": r.temp_indoor,
        "temp_outdoor": r.temp_outdoor,
        "lux": r.lux,
        "voltage": r.voltage,
        "current": r.current,
        "power": r.power,
        "frequency": r.frequency,
        "power_factor": r.power_factor,
        "power_status": r.power_status,
    }
