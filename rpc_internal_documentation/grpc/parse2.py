FRAME_TYPES = {0:'DATA',1:'HEADERS',2:'PRIORITY',3:'RST_STREAM',4:'SETTINGS',
               5:'PUSH_PROMISE',6:'PING',7:'GOAWAY',8:'WINDOW_UPDATE',9:'CONTINUATION'}

def parse(label, data):
    print(f"\n========== {label} : {len(data)} bytes total ==========")
    i = 0
    preface = b'PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n'
    if data[:len(preface)] == preface:
        print(f"[HTTP/2 CLIENT CONNECTION PREFACE]: {preface!r}")
        i = len(preface)
    while i + 9 <= len(data):
        length = int.from_bytes(data[i:i+3], 'big')
        ftype = data[i+3]
        flags = data[i+4]
        stream_id = int.from_bytes(data[i+5:i+9], 'big') & 0x7fffffff
        payload = data[i+9:i+9+length]
        tname = FRAME_TYPES.get(ftype, f'UNKNOWN(0x{ftype:02x})')
        print(f"FRAME  len={length:<5} type={tname:<13} flags=0x{flags:02x}  stream_id={stream_id}")
        if tname == 'SETTINGS' and length % 6 == 0 and length > 0:
            NAMES = {1:'HEADER_TABLE_SIZE',2:'ENABLE_PUSH',3:'MAX_CONCURRENT_STREAMS',
                     4:'INITIAL_WINDOW_SIZE',5:'MAX_FRAME_SIZE',6:'MAX_HEADER_LIST_SIZE'}
            for j in range(0, length, 6):
                ident = int.from_bytes(payload[j:j+2], 'big')
                val = int.from_bytes(payload[j+2:j+6], 'big')
                print(f"       setting {NAMES.get(ident, ident)} = {val}")
        if tname == 'WINDOW_UPDATE' and length == 4:
            inc = int.from_bytes(payload, 'big') & 0x7fffffff
            print(f"       window size increment = {inc}")
        if tname == 'PING':
            print(f"       opaque payload = {payload.hex()}")
        if tname == 'DATA' and length >= 5:
            comp = payload[0]
            msglen = int.from_bytes(payload[1:5], 'big')
            proto_bytes = payload[5:5+msglen]
            print(f"       gRPC frame: compressed_flag={comp} message_length={msglen}")
            print(f"       protobuf payload (hex) = {proto_bytes.hex()}")
            if len(proto_bytes) >= 2:
                tag = proto_bytes[0]
                field_num = tag >> 3
                wire_type = tag & 0x7
                flen = proto_bytes[1]
                fval = proto_bytes[2:2+flen]
                print(f"       decoded protobuf: field={field_num} wire_type={wire_type} "
                      f"len={flen} value={fval!r}")
        if tname == 'HEADERS':
            print(f"       HPACK-compressed header block (hex) = {payload.hex()}")
        i += 9 + length
    if i != len(data):
        print(f"[!] {len(data)-i} trailing bytes not parsed as a full frame")

with open('client_to_server.bin', 'rb') as f:
    c2s = f.read()
with open('server_to_client.bin', 'rb') as f:
    s2c = f.read()

parse("CLIENT -> SERVER (all frames, concatenated)", c2s)
parse("SERVER -> CLIENT (all frames, concatenated)", s2c)