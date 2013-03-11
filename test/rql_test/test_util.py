import random
import socket
from time import sleep
from subprocess import call, Popen

# Manages starting and stopping instances of the Rethindb server
class RethinkDBTestServers:
    def __init__(self, server_build='debug', use_default_port=False):
        self.server_build = server_build
        self.use_default_port = use_default_port

    # Implement `with` methods to ensure proper lifetime management
    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args):
        self.stop()

    # Find a free port to bind to
    def find_available_port(self):
        max_loop = 10
        for i in xrange(max_loop):
            port = random.randint(1025, 65535)
            if self.port_available(port):
                return port
        raise Exception("""Wow, you must have won the lottery or something.
                           Ten random ports and they're all occupied""")

    # Test if a given port is free
    def port_available(self, port):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.bind(("127.0.0.1",port))
            s.close()
        except socket.error:
            return False
        return True

    def start(self): 
        log_out = open('build/server-log.txt','a')
        if self.use_default_port:
            self.cpp_port = 28015
        else:
            self.cpp_port = self.find_available_port()
        self.cpp_server = Popen(['../../build/%s/rethinkdb' % self.server_build,
                                 '--driver-port', str(self.cpp_port),
                                 '--http-port=0',
                                 '--cluster-port=0'],
                                stdout=log_out, stderr=log_out)

        # self.js_port = self.find_available_port()
        # self.js_server = Popen(['node', '../../rqljs/build/rqljs', str(self.js_port), "0"])

        sleep(0.2)

    def stop(self):
        self.cpp_server.terminate()
        # self.js_server.terminate()
        self.clear_data()
        sleep(0.1)

    def clear_data(self):
        # call(['rm', 'rethinkdb-data']) # JS server data
        call(['rm', '-rf', 'rethinkdb_data'])

    def restart(self):
        self.stop()
        self.start()
