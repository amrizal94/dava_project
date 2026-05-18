import asyncio
import logging
import os
from contextlib import asynccontextmanager

from dotenv import load_dotenv
load_dotenv()

from fastapi import FastAPI
from fastapi.staticfiles import StaticFiles

from .database import init_db
from .mqtt_handler import start_mqtt
from .routers import pages, api, devices

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

OFFLINE_THRESHOLD = int(os.getenv("OFFLINE_THRESHOLD_MINUTES", "5"))


async def _mark_offline_loop():
    """Mark devices as offline if they haven't sent data."""
    from datetime import datetime, timezone, timedelta
    from sqlalchemy import select, update
    from .database import AsyncSessionLocal
    from .models import Device
    from .mqtt_handler import _broadcast

    while True:
        await asyncio.sleep(30)
        try:
            threshold = datetime.now(timezone.utc) - timedelta(minutes=OFFLINE_THRESHOLD)
            async with AsyncSessionLocal() as db:
                # Find devices that are online but haven't been seen recently
                result = await db.execute(
                    select(Device.mac_address)
                    .where(Device.is_online == True, Device.last_seen < threshold)
                )
                stale_macs = result.scalars().all()

                if stale_macs:
                    await db.execute(
                        update(Device)
                        .where(Device.mac_address.in_(stale_macs))
                        .values(is_online=False)
                    )
                    await db.commit()
                    for mac in stale_macs:
                        _broadcast({"event": "device_offline", "mac": mac})
                        logger.info(f"Device marked offline: {mac}")
        except Exception as e:
            logger.error(f"Offline check error: {e}")


@asynccontextmanager
async def lifespan(app: FastAPI):
    await init_db()
    loop = asyncio.get_event_loop()
    start_mqtt(loop)
    asyncio.create_task(_mark_offline_loop())
    yield


app = FastAPI(title="Dava Monitoring", lifespan=lifespan)

app.mount("/static", StaticFiles(directory="api/static"), name="static")

app.include_router(pages.router)
app.include_router(api.router)
app.include_router(devices.router)
