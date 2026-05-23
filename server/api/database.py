from sqlalchemy import text
from sqlalchemy.ext.asyncio import create_async_engine, async_sessionmaker, AsyncSession
from sqlalchemy.orm import DeclarativeBase
import os

DATABASE_URL = os.getenv("DATABASE_URL", "postgresql+asyncpg://dava:dava_password@localhost/dava_monitoring")

engine = create_async_engine(DATABASE_URL, echo=False)
AsyncSessionLocal = async_sessionmaker(engine, expire_on_commit=False)


class Base(DeclarativeBase):
    pass


async def get_db() -> AsyncSession:
    async with AsyncSessionLocal() as session:
        yield session


async def init_db():
    async with engine.begin() as conn:
        # Create PostgreSQL ENUM type safely — no error if already exists
        await conn.execute(text(
            "DO $$ BEGIN "
            "  CREATE TYPE lightmode AS ENUM ('auto', 'manual'); "
            "EXCEPTION WHEN duplicate_object THEN null; "
            "END $$"
        ))
        await conn.run_sync(Base.metadata.create_all)
        # Migrasi kolom is_deleted untuk database yang sudah ada
        await conn.execute(text(
            "ALTER TABLE devices ADD COLUMN IF NOT EXISTS is_deleted BOOLEAN NOT NULL DEFAULT FALSE"
        ))
        # Migrasi kolom power_status untuk database yang sudah ada
        await conn.execute(text(
            "ALTER TABLE devices ADD COLUMN IF NOT EXISTS power_status BOOLEAN"
        ))
