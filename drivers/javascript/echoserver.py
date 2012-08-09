import SocketServer

class EchoHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        print "Recv'd"
        self.request.sendall("bob")

server = SocketServer.TCPServer(('localhost', 11211), EchoHandler)
server.serve_forever()
