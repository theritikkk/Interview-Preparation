import json
import socket


def call(port, msg, timeout=2.0):
    s = socket.create_connection(("127.0.0.1", port), timeout=timeout)
    s.sendall((json.dumps(msg) + "\n").encode())
    data = b""
    while not data.endswith(b"\n"):
        chunk = s.recv(4096)
        if not chunk:
            break
        data += chunk
    s.close()
    return json.loads(data)


ACCOUNT, INVENTORY, SHIPPING = 8101, 8102, 8103
