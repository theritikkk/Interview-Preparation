"""framing.py — minimal length-prefixed framing over a TCP socket.

This is the hand-rolled stand-in for gRPC's own HTTP/2-based transport
(stream multiplexing, flow control, trailers, etc.) — see ANALYSIS.md for
exactly why that transport isn't available in this sandbox and what this
gives up by not having it. Everything this module carries in its payload
(once a frame reaches the application layer) is a REAL Protocol Buffers
serialized message produced by generated code in gen/telemetry_pb2.py —
only the "how do these bytes get from one process to another, and how do
we know where one logical call ends and the next begins" part is
hand-written here.

Wire format, one frame:

    [4 bytes: big-endian uint32 length of everything that follows]
    [1 byte : frame type]
    [N bytes: payload, meaning depends on frame type]

Frame types:
    CALL_START (1) — first frame of every call.
        payload = [2 bytes: method name length][method name, utf-8]
                  [remaining bytes: initial request message, serialized
                   protobuf — non-empty for unary/server-streaming calls,
                   which carry their one request right here; empty for
                   client-streaming/bidi calls, whose requests instead
                   arrive as a sequence of DATA frames]
    DATA (2)       — payload = one serialized protobuf message, exactly as
                     produced by `<Message>.SerializeToString()`.
    END_STREAM (3) — payload = empty. Marks "no more DATA frames from me
                     on this call", independently for each direction.
    ERROR (4)      — payload = UTF-8 error text. Stands in for a gRPC
                     Status — this project only needs "it failed, here's
                     why", not the full status-code taxonomy.
"""

import socket
import struct

CALL_START = 1
DATA = 2
END_STREAM = 3
ERROR = 4

_TYPE_NAMES = {CALL_START: "CALL_START", DATA: "DATA", END_STREAM: "END_STREAM", ERROR: "ERROR"}


class ConnectionClosed(Exception):
    """Raised when the peer closes the connection mid-frame."""


class RpcError(Exception):
    """Raised on the client when the server sends an ERROR frame."""


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    chunks = []
    remaining = n
    while remaining > 0:
        chunk = sock.recv(remaining)
        if not chunk:
            raise ConnectionClosed(f"peer closed with {remaining} bytes still expected")
        chunks.append(chunk)
        remaining -= len(chunk)
    return b"".join(chunks)


def send_frame(sock: socket.socket, frame_type: int, payload: bytes = b"") -> None:
    header = struct.pack(">IB", len(payload) + 1, frame_type)
    sock.sendall(header + payload)


def recv_frame(sock: socket.socket) -> tuple[int, bytes]:
    """Returns (frame_type, payload). Raises ConnectionClosed at a clean
    frame boundary (no partial frame in flight) if the peer hangs up."""
    header = sock.recv(5)
    if not header:
        raise ConnectionClosed("peer closed before sending a frame")
    if len(header) < 5:
        header += _recv_exact(sock, 5 - len(header))
    total_len, frame_type = struct.unpack(">IB", header)
    payload_len = total_len - 1
    payload = _recv_exact(sock, payload_len) if payload_len > 0 else b""
    return frame_type, payload


def send_call_start(sock: socket.socket, method: str, initial_payload: bytes = b"") -> None:
    name_bytes = method.encode("utf-8")
    payload = struct.pack(">H", len(name_bytes)) + name_bytes + initial_payload
    send_frame(sock, CALL_START, payload)


def parse_call_start(payload: bytes) -> tuple[str, bytes]:
    (name_len,) = struct.unpack(">H", payload[:2])
    method = payload[2 : 2 + name_len].decode("utf-8")
    initial = payload[2 + name_len :]
    return method, initial


def frame_type_name(t: int) -> str:
    return _TYPE_NAMES.get(t, f"UNKNOWN({t})")
