import grpc
import greeter_pb2
import greeter_pb2_grpc

def main():
    # Connect to the PROXY's port, not the real server's port directly --
    # so every byte passes through our logger on the way.
    channel = grpc.insecure_channel('127.0.0.1:50051')
    stub = greeter_pb2_grpc.GreeterStub(channel)

    print("[client] calling SayHello(name='World')")
    response = stub.SayHello(greeter_pb2.HelloRequest(name='World'))
    print(f"[client] response: {response.message!r}")

if __name__ == '__main__':
    main()