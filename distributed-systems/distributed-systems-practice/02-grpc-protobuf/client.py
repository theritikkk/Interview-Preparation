#!/usr/bin/env python3
"""client.py — exercises all four FleetTelemetry RPCs against server.py.

Each call wraps the generic rpc_base helpers with the actual generated
message types, the same way a real generated gRPC client stub would (just
written by hand instead of by a code generator — see ANALYSIS.md).
"""

import socket
import sys
import time

sys.path.insert(0, "gen")
import telemetry_pb2 as pb  # noqa: E402

from rpc import rpc_base  # noqa: E402

HOST = "127.0.0.1"
PORT = 50551


def connect() -> socket.socket:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
    return sock


def demo_unary(device_id: str) -> None:
    print("\n=== 1. Unary: RegisterDevice ===")
    with connect() as sock:
        req = pb.DeviceInfo(device_id=device_id, model="sensor-x200", firmware_version=2.1)
        resp_bytes = rpc_base.unary_call(sock, "RegisterDevice", req.SerializeToString())
        ack = pb.RegisterAck()
        ack.ParseFromString(resp_bytes)
        print(f"  accepted={ack.accepted} region={ack.assigned_region!r} "
              f"server_time_ms={ack.server_time_ms}")


def demo_server_streaming(device_id: str) -> None:
    print("\n=== 2. Server streaming: StreamAlerts ===")
    with connect() as sock:
        req = pb.DeviceId(device_id=device_id)
        severity_name = {0: "INFO", 1: "WARNING", 2: "CRITICAL"}
        for raw in rpc_base.server_streaming_call(sock, "StreamAlerts", req.SerializeToString()):
            alert = pb.Alert()
            alert.ParseFromString(raw)
            print(f"  [{severity_name[alert.severity]}] {alert.message} (t={alert.timestamp_ms})")


def demo_client_streaming(device_id: str) -> None:
    print("\n=== 3. Client streaming: UploadReadings ===")
    readings = [22.1, 22.4, 23.0, 24.8, 23.9, 22.7]

    def request_gen():
        for v in readings:
            yield pb.SensorReading(
                device_id=device_id, sensor_name="temp_c", value=v,
                timestamp_ms=int(time.time() * 1000),
            ).SerializeToString()
            time.sleep(0.05)

    with connect() as sock:
        resp_bytes = rpc_base.client_streaming_call(sock, "UploadReadings", request_gen())
        summary = pb.ReadingsSummary()
        summary.ParseFromString(resp_bytes)
        print(f"  uploaded {len(readings)} readings -> server summary: "
              f"count={summary.count} min={summary.min_value} max={summary.max_value} "
              f"avg={summary.avg_value:.2f}")
        expected_avg = sum(readings) / len(readings)
        assert summary.count == len(readings), "server summary count mismatch"
        assert abs(summary.avg_value - expected_avg) < 1e-6, "server summary avg mismatch"
        print("  (verified: server's count/avg match what the client actually sent)")


def demo_bidi_streaming(device_id: str) -> None:
    print("\n=== 4. Bidirectional streaming: ControlChannel ===")
    commands = ["calibrate", "reboot"]

    def request_gen():
        for action in commands:
            print(f"  -> sending command: {action}")
            yield pb.Command(device_id=device_id, action=action).SerializeToString()
            time.sleep(0.15)

    with connect() as sock:
        for raw in rpc_base.bidi_streaming_call(sock, "ControlChannel", request_gen()):
            status = pb.Status()
            status.ParseFromString(raw)
            print(f"  <- status: {status.state} (t={status.timestamp_ms})")


def main() -> None:
    device_id = "dev-001"
    demo_unary(device_id)
    demo_server_streaming(device_id)
    demo_client_streaming(device_id)
    demo_bidi_streaming(device_id)
    print("\nAll four RPC shapes completed successfully.")


if __name__ == "__main__":
    main()
