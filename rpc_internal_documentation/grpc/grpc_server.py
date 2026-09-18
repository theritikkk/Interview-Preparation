import grpc
from concurrent import futures
import greeter_pb2
import greeter_pb2_grpc

class GreeterServicer(greeter_pb2_grpc.GreeterServicer):
    def SayHello(self, request, context):
        print(f"[server] SayHello called with name={request.name!r}")
        return greeter_pb2.HelloReply(message=f"Hello, {request.name}!")

def main():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=2))
    greeter_pb2_grpc.add_GreeterServicer_to_server(GreeterServicer(), server)
    server.add_insecure_port('127.0.0.1:50052')
    server.start()
    print("[server] listening on 127.0.0.1:50052 (insecure, real HTTP/2)")
    server.wait_for_termination()

if __name__ == '__main__':
    main()