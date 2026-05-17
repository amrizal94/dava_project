import asyncio
import json
from fastapi import APIRouter
from fastapi.responses import StreamingResponse

from ..mqtt_handler import register_sse_queue, unregister_sse_queue

router = APIRouter(prefix="/api", tags=["sse"])


@router.get("/stream")
async def sse_stream():
    q: asyncio.Queue = asyncio.Queue()
    register_sse_queue(q)

    async def generator():
        try:
            yield "data: {\"event\": \"connected\"}\n\n"
            while True:
                try:
                    data = await asyncio.wait_for(q.get(), timeout=30)
                    yield f"data: {data}\n\n"
                except asyncio.TimeoutError:
                    yield ": keepalive\n\n"
        finally:
            unregister_sse_queue(q)

    return StreamingResponse(
        generator(),
        media_type="text/event-stream",
        headers={
            "Cache-Control": "no-cache",
            "X-Accel-Buffering": "no",
        },
    )
