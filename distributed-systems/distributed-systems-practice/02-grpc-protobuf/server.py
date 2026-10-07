#!/usr/bin/env python3
"""server.py — FleetTelemetry server.

Implements all four RPCs declared in proto/telemetry.proto, using the
real generated message classes in gen/telemetry_pb2.py for serialization
and the hand-rolled dispatcher in rpc/rpc_base.py for the call/response
plumbing (see ANALYSIS.md for exactly where the line between "real
protobuf" and "hand-rolled transport" sits).
"""

import socket
import sys
import threading
import time

sys.path.insert(0, "gen")
import telemetry_pb2 as pb  # noqa: E402

from rpc.rpc_base import Dispatcher, Kind  # noqa: E402

HOST = "127.0.0.1"
PORT = 50551


def now_ms() -> int:
    return int(time.time() * 1000)


class FleetTelemetryServicer:
    """Holds whatever server-side state the handlers need. A real gRPC
    service implementation looks almost exactly like this — a plain class
    with one method per RPC — except the base class and the method
    signatures (context objects, etc.) would be generated for you."""

    def __init__(self):
        self._lock = threading.Lock()
        self._devices: dict[str, pb.DeviceInfo] = {}

    # ---- unary ----------------------------------------------------
    def register_device(self, request_bytes: bytes) -> bytes:
        req = pb.DeviceInfo()
        req.ParseFromString(request_bytes)
        with self._lock:
            self._devices[req.device_id] = req
            region = "us-east" if hash(req.device_id) % 2 == 0 else "eu-west"
        print(f"  [server] registered device={req.device_id!r} model={req.model!r} "
              f"fw={req.firmware_version} -> region={region}")
        ack = pb.RegisterAck(accepted=True, assigned_region=region, server_time_ms=now_ms())
        return ack.SerializeToString()

    # ---- server streaming ------------------------------------------
    def stream_alerts(self, request_bytes: bytes):
        req = pb.DeviceId()
        req.ParseFromString(request_bytes)
        print(f"  [server] streaming alerts for device={req.device_id!r}")
        scripted = [
            (pb.INFO, "heartbeat ok"),
            (pb.WARNING, "temperature rising"),
            (pb.CRITICAL, "temperature threshold exceeded"),
        ]
        for severity, message in scripted:
            time.sleep(0.2)
            alert = pb.Alert(
                device_id=req.device_id,
                severity=severity,
                message=message,
                timestamp_ms=now_ms(),
            )
            yield alert.SerializeToString()

    # ---- client streaming -------------------------------------------
    def upload_readings(self, request_iter) -> bytes:
        count = 0
        total = 0.0
        min_v = None
        max_v = None
        device_id = ""
        for raw in request_iter:
            reading = pb.SensorReading()
            reading.ParseFromString(raw)
            device_id = reading.device_id
            count += 1
            total += reading.value
            min_v = reading.value if min_v is None else min(min_v, reading.value)
            max_v = reading.value if max_v is None else max(max_v, reading.value)
        avg = (total / count) if count else 0.0
        print(f"  [server] received {count} readings from device={device_id!r}, "
              f"avg={avg:.2f} min={min_v} max={max_v}")
        summary = pb.ReadingsSummary(
            count=count,
            min_value=min_v or 0.0,
            max_value=max_v or 0.0,
            avg_value=avg,
        )
        return summary.SerializeToString()

    # ---- bidi streaming -----------------------------------------------
    def control_channel(self, request_iter):
        for raw in request_iter:
            cmd = pb.Command()
            cmd.ParseFromString(raw)
            print(f"  [server] command for device={cmd.device_id!r}: {cmd.action!r}")
            if cmd.action == "reboot":
                states = ["rebooting", "online"]
            elif cmd.action == "calibrate":
                states = ["calibrating", "online"]
            else:
                states = ["ack"]
            for state in states:
                time.sleep(0.1)
                status = pb.Status(device_id=cmd.device_id, state=state, timestamp_ms=now_ms())
                yield status.SerializeToString()


def build_dispatcher() -> Dispatcher:
    svc = FleetTelemetryServicer()
    d = Dispatcher()
    d.register("RegisterDevice", Kind.UNARY, svc.register_device)
    d.register("StreamAlerts", Kind.SERVER_STREAM, svc.stream_alerts)
    d.register("UploadReadings", Kind.CLIENT_STREAM, svc.upload_readings)
    d.register("ControlChannel", Kind.BIDI_STREAM, svc.control_channel)
    return d


def serve(host: str = HOST, port: int = PORT) -> None:
    dispatcher = build_dispatcher()
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind((host, port))
    listener.listen(16)
    print(f"[server] FleetTelemetry listening on {host}:{port}")

    try:
        while True:
            conn, addr = listener.accept()
            t = threading.Thread(
                target=_handle, args=(dispatcher, conn, addr), daemon=True
            )
            t.start()
    except KeyboardInterrupt:
        print("\n[server] shutting down")
    finally:
        listener.close()


def _handle(dispatcher: Dispatcher, conn: socket.socket, addr) -> None:
    try:
        dispatcher.handle_connection(conn, log=lambda m: print(f"  [server:{addr[1]}] {m}"))
    finally:
        conn.close()


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else PORT
    serve(port=port)
