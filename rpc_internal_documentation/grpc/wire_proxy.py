"""
A dumb TCP pass-through proxy that logs every byte it relays, in each
direction, to a file -- purely for observability. It does NOT decode or
alter anything; it just sits between the real gRPC client and the real
gRPC server so we can see the exact bytes that cross the wire.
"""
import socket
import threading
import sys

LISTEN_PORT = 50051
UPSTREAM_HOST = '127.0.0.1'
UPSTREAM_PORT = 50052
LOGFILE = 'wire_capture.log'

log_lock = threading.Lock()

def log_bytes(direction, data):
    with log_lock:
        with open(LOGFILE, 'a') as f:
            f.write(f"\n=== {direction} ({len(data)} bytes) ===\n")
            for i in range(0, len(data), 16):
                chunk = data[i:i+16]
                hexpart = ' '.join(f'{b:02x}' for b in chunk)
                asciipart = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
                f.write(f"  {i:04x}: {hexpart:<48} {asciipart}\n")
        # ALSO append the raw bytes to a binary capture file, tagged with
        # direction, so we can parse robustly without re-parsing our own
        # hexdump text.
        with open('client_to_server.bin' if direction.startswith('CLIENT') else 'server_to_client.bin', 'ab') as fb:
            fb.write(data)

def pump(src, dst, direction):
    try:
        while True:
            data = src.recv(4096)
            if not data:
                break
            log_bytes(direction, data)
            dst.sendall(data)
    except OSError:
        pass
    finally:
        try: dst.shutdown(socket.SHUT_WR)
        except OSError: pass

def handle(client_sock):
    upstream = socket.create_connection((UPSTREAM_HOST, UPSTREAM_PORT))
    t1 = threading.Thread(target=pump, args=(client_sock, upstream, "CLIENT -> SERVER"))
    t2 = threading.Thread(target=pump, args=(upstream, client_sock, "SERVER -> CLIENT"))
    t1.start(); t2.start()
    t1.join(); t2.join()
    client_sock.close()
    upstream.close()

def main():
    open(LOGFILE, 'w').close()
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind(('127.0.0.1', LISTEN_PORT))
    listener.listen(5)
    print(f"[proxy] listening on 127.0.0.1:{LISTEN_PORT}, forwarding to "
          f"{UPSTREAM_HOST}:{UPSTREAM_PORT}, logging to {LOGFILE}")
    sys.stdout.flush()
    while True:
        client_sock, addr = listener.accept()
        threading.Thread(target=handle, args=(client_sock,)).start()

if __name__ == '__main__':
    main()