from fastapi import APIRouter, Depends, Request
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates
from sqlalchemy import select
from sqlalchemy.ext.asyncio import AsyncSession

from ..database import get_db
from ..models import Device, DeviceSettings

router = APIRouter(tags=["pages"])
templates = Jinja2Templates(directory="api/templates")


@router.get("/", response_class=HTMLResponse)
async def dashboard(request: Request, db: AsyncSession = Depends(get_db)):
    result = await db.execute(select(Device).order_by(Device.created_at))
    devices = result.scalars().all()
    return templates.TemplateResponse("dashboard.html", {"request": request, "devices": devices})


@router.get("/device/{mac}", response_class=HTMLResponse)
async def device_detail(mac: str, request: Request, db: AsyncSession = Depends(get_db)):
    device = await db.get(Device, mac)
    settings = await db.get(DeviceSettings, mac)
    return templates.TemplateResponse("device.html", {
        "request": request,
        "device": device,
        "settings": settings,
    })
