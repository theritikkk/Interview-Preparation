"""rpc_base.py — the four gRPC call shapes, implemented over framing.py.

Client side: four functions, one per call shape, each taking/returning
raw serialized protobuf bytes (the service-specific client in client.py
wraps these with the actual message types from gen/telemetry_pb2.py — the
same layering a real generated gRPC stub would have, just hand-written
instead of generated).

Server side: a small Dispatcher that a service implementation registers
handlers with, declaring each method's call shape up front — mirroring
how a real generated gRPC "Servicer" base class routes each method to the
right calling convention under the hood.
"""

from __future__ import annotations

import socket
import threading
from dataclasses import dataclass
from enum import Enum, auto
from queue import Queue
from typing import Callable, Iterable, Iterator

from . import framing


class Kind(Enum):
    UNARY = auto()
    SERVER_STREAM = auto()
    CLIENT_STREAM = auto()
    BIDI_STREAM = auto()


# ----------------------------------------------------------------------
# Client side
# ----------------------------------------------------------------------

def unary_call(sock: socket.socket, method: str, request_bytes: bytes) -> bytes:
    framing.send_call_start(sock, method, request_bytes)
    frame_type, payload = framing.recv_frame(sock)
    if frame_type == framing.ERROR:
        raise framing.RpcError(payload.decode("utf-8"))
    if frame_type != framing.DATA:
        raise framing.RpcError(f"expected DATA, got {framing.frame_type_name(frame_type)}")
    response = payload
    frame_type, _ = framing.recv_frame(sock)  # expect the closing END_STREAM
    if frame_type == framing.ERROR:
        raise framing.RpcError("server reported an error after the response")
    return response


def server_streaming_call(sock: socket.socket, method: str, request_bytes: bytes) -> Iterator[bytes]:
    framing.send_call_start(sock, method, request_bytes)
    while True:
        frame_type, payload = framing.recv_frame(sock)
        if frame_type == framing.DATA:
            yield payload
        elif frame_type == framing.END_STREAM:
            return
        elif frame_type == framing.ERROR:
            raise framing.RpcError(payload.decode("utf-8"))
        else:
            raise framing.RpcError(f"unexpected frame {framing.frame_type_name(frame_type)}")


def client_streaming_call(sock: socket.socket, method: str, requests: Iterable[bytes]) -> bytes:
    framing.send_call_start(sock, method, b"")
    for req in requests:
        framing.send_frame(sock, framing.DATA, req)
    framing.send_frame(sock, framing.END_STREAM)

    frame_type, payload = framing.recv_frame(sock)
    if frame_type == framing.ERROR:
        raise framing.RpcError(payload.decode("utf-8"))
    if frame_type != framing.DATA:
        raise framing.RpcError(f"expected DATA, got {framing.frame_type_name(frame_type)}")
    response = payload
    framing.recv_frame(sock)  # closing END_STREAM
    return response


def bidi_streaming_call(sock: socket.socket, method: str, requests: Iterable[bytes]) -> Iterator[bytes]:
    framing.send_call_start(sock, method, b"")

    def writer():
        for req in requests:
            framing.send_frame(sock, framing.DATA, req)
        framing.send_frame(sock, framing.END_STREAM)

    writer_thread = threading.Thread(target=writer, daemon=True)
    writer_thread.start()

    while True:
        frame_type, payload = framing.recv_frame(sock)
        if frame_type == framing.DATA:
            yield payload
        elif frame_type == framing.END_STREAM:
            break
        elif frame_type == framing.ERROR:
            raise framing.RpcError(payload.decode("utf-8"))
        else:
            raise framing.RpcError(f"unexpected frame {framing.frame_type_name(frame_type)}")
    writer_thread.join(timeout=5)


# ----------------------------------------------------------------------
# Server side
# ----------------------------------------------------------------------

@dataclass
class MethodHandler:
    kind: Kind
    # UNARY:          fn(request_bytes) -> response_bytes
    # SERVER_STREAM:  fn(request_bytes) -> Iterator[bytes]
    # CLIENT_STREAM:  fn(Iterator[bytes]) -> response_bytes
    # BIDI_STREAM:    fn(Iterator[bytes]) -> Iterator[bytes]
    fn: Callable


class Dispatcher:
    """Routes an incoming connection's CALL_START to a registered handler
    and drives the right read/write pattern for that handler's Kind —
    the hand-rolled equivalent of a generated gRPC service base class."""

    def __init__(self):
        self._handlers: dict[str, MethodHandler] = {}

    def register(self, method: str, kind: Kind, fn: Callable) -> None:
        self._handlers[method] = MethodHandler(kind, fn)

    def handle_connection(self, sock: socket.socket, log=lambda msg: None) -> None:
        try:
            frame_type, payload = framing.recv_frame(sock)
        except framing.ConnectionClosed:
            return
        if frame_type != framing.CALL_START:
            framing.send_frame(sock, framing.ERROR, b"expected CALL_START")
            return

        method, initial = framing.parse_call_start(payload)
        handler = self._handlers.get(method)
        if handler is None:
            framing.send_frame(sock, framing.ERROR, f"unknown method {method!r}".encode())
            return

        log(f"dispatching {method} ({handler.kind.name})")
        try:
            if handler.kind is Kind.UNARY:
                response = handler.fn(initial)
                framing.send_frame(sock, framing.DATA, response)
                framing.send_frame(sock, framing.END_STREAM)

            elif handler.kind is Kind.SERVER_STREAM:
                for response in handler.fn(initial):
                    framing.send_frame(sock, framing.DATA, response)
                framing.send_frame(sock, framing.END_STREAM)

            elif handler.kind is Kind.CLIENT_STREAM:
                requests = list(_read_request_stream(sock))
                response = handler.fn(iter(requests))
                framing.send_frame(sock, framing.DATA, response)
                framing.send_frame(sock, framing.END_STREAM)

            elif handler.kind is Kind.BIDI_STREAM:
                request_queue: Queue = Queue()

                def reader():
                    for req in _read_request_stream(sock):
                        request_queue.put(req)
                    request_queue.put(None)  # sentinel: end of requests

                reader_thread = threading.Thread(target=reader, daemon=True)
                reader_thread.start()

                def request_iterator():
                    while True:
                        item = request_queue.get()
                        if item is None:
                            return
                        yield item

                for response in handler.fn(request_iterator()):
                    framing.send_frame(sock, framing.DATA, response)
                framing.send_frame(sock, framing.END_STREAM)
                reader_thread.join(timeout=5)

        except framing.ConnectionClosed:
            pass
        except Exception as e:  # noqa: BLE001 — this boundary intentionally catches everything
            try:
                framing.send_frame(sock, framing.ERROR, str(e).encode("utf-8"))
            except OSError:
                pass


def _read_request_stream(sock: socket.socket) -> Iterator[bytes]:
    while True:
        frame_type, payload = framing.recv_frame(sock)
        if frame_type == framing.DATA:
            yield payload
        elif frame_type == framing.END_STREAM:
            return
        else:
            raise framing.RpcError(f"unexpected frame {framing.frame_type_name(frame_type)} in request stream")
