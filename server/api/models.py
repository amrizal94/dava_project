from datetime import datetime, timezone
from sqlalchemy import String, Float, Boolean, Integer, DateTime, ForeignKey, Enum, UniqueConstraint
from sqlalchemy.orm import Mapped, mapped_column, relationship
import enum

from .database import Base


class LightMode(str, enum.Enum):
    auto = "auto"
    manual = "manual"


class Device(Base):
    __tablename__ = "devices"

    mac_address: Mapped[str] = mapped_column(String(17), primary_key=True)
    alias: Mapped[str] = mapped_column(String(64), default="")
    location: Mapped[str] = mapped_column(String(128), default="")
    is_online: Mapped[bool] = mapped_column(Boolean, default=False)
    last_seen: Mapped[datetime | None] = mapped_column(DateTime(timezone=True), nullable=True)
    firmware_version: Mapped[str] = mapped_column(String(32), default="")
    latitude: Mapped[float | None] = mapped_column(Float, nullable=True)
    longitude: Mapped[float | None] = mapped_column(Float, nullable=True)
    power_status: Mapped[bool | None] = mapped_column(Boolean, nullable=True)
    is_deleted: Mapped[bool] = mapped_column(Boolean, default=False)
    created_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=lambda: datetime.now(timezone.utc))

    readings: Mapped[list["SensorReading"]] = relationship(back_populates="device", cascade="all, delete-orphan")
    settings: Mapped["DeviceSettings | None"] = relationship(back_populates="device", uselist=False, cascade="all, delete-orphan")


class SensorReading(Base):
    __tablename__ = "sensor_readings"

    id: Mapped[int] = mapped_column(Integer, primary_key=True, autoincrement=True)
    device_mac: Mapped[str] = mapped_column(ForeignKey("devices.mac_address", ondelete="CASCADE"))
    timestamp: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=lambda: datetime.now(timezone.utc))

    temp_indoor: Mapped[float | None] = mapped_column(Float, nullable=True)
    temp_outdoor: Mapped[float | None] = mapped_column(Float, nullable=True)
    lux: Mapped[float | None] = mapped_column(Float, nullable=True)

    voltage: Mapped[float | None] = mapped_column(Float, nullable=True)
    current: Mapped[float | None] = mapped_column(Float, nullable=True)
    power: Mapped[float | None] = mapped_column(Float, nullable=True)
    frequency: Mapped[float | None] = mapped_column(Float, nullable=True)
    power_factor: Mapped[float | None] = mapped_column(Float, nullable=True)
    power_status: Mapped[bool | None] = mapped_column(Boolean, nullable=True)

    device: Mapped["Device"] = relationship(back_populates="readings")


class DeviceSettings(Base):
    __tablename__ = "device_settings"

    device_mac: Mapped[str] = mapped_column(ForeignKey("devices.mac_address", ondelete="CASCADE"), primary_key=True)
    light_mode: Mapped[LightMode] = mapped_column(Enum(LightMode, name='lightmode', create_type=False), default=LightMode.auto)
    light_brightness: Mapped[int] = mapped_column(Integer, default=100)
    updated_at: Mapped[datetime] = mapped_column(DateTime(timezone=True), default=lambda: datetime.now(timezone.utc), onupdate=lambda: datetime.now(timezone.utc))

    device: Mapped["Device"] = relationship(back_populates="settings")
