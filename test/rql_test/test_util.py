import random
import socket
from time import sleep
from subprocess import call, Popen

# Manages a cluster of RethinkDB servers
class RethinkDBTestServers(object):
    def __init__(self, num_servers=4, server_build='debug', use_default_port=False):
        assert num_servers >= 1
        self.num_servers = num_servers
        self.server_build = server_build
        self.use_default_port = use_default_port

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args):
        self.stop()

    def start(self):
        self.servers = [RethinkDBTestServer(self.server_build, self.use_default_port)
                            for i in xrange(0, self.num_servers)]

        cluster_port = self.servers[0].start()
        for server in self.servers[1:]:
            server.join(cluster_port)

    def stop(self):
        for server in self.servers:     
            server.stop()
        self.clear_data()

    def clear_data(self):
#        call(['rm', '-rf', 'run'])
        pass

    def restart(self):
        self.stop()
        self.start()

    def driver_port(self):
        return self.servers[0].cpp_port

# Manages starting and stopping an instance of the Rethindb server
class RethinkDBTestServer(object):
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
        if self.use_default_port:
            self.cpp_port = 28015
        else:
            self.cpp_port = self.find_available_port()
        cluster_port = self.find_available_port()
        executable, directory, log_out = self.create()

        self.cpp_server = Popen([executable, 'serve',
                                 '--driver-port', str(self.cpp_port),
                                 '--directory', directory,
                                 '--http-port', '0',
                                 '--cluster-port', str(cluster_port)],
                                stdout=log_out, stderr=log_out)
        sleep(0.2)

        # Create database 'test' which the tests assume but doesn't get created when we
        # start up rethinkdb like this
        call([executable, 'admin', '--join', 'localhost:%d' % cluster_port, 'create', 'database', 'test'],
            stdout=log_out, stderr=log_out)
        return cluster_port

    # Join a cluster headed by a server previously invoked with start
    def join(self, cluster_port):
        self.cpp_port = self.find_available_port()
        executable, directory, log_out = self.create()
        self.cpp_server = Popen([executable, 'serve',
                                 '--driver-port', str(self.cpp_port),
                                 '--directory', directory,
                                 '--http-port', '0',
                                 '--join', 'localhost:%d' % cluster_port],
                                stdout=log_out, stderr=log_out)
        sleep(0.2)

    def create(self):
        # Really I should use python's directory tools to ensure compatibility with platforms
        # that use an alternative path separator but I'm not going to.
        directory = 'run/server_%s/' % self.cpp_port
        rdbfile = directory+'rdb'
        call(['mkdir', '-p', directory])
        log_out = open(directory+'server-log.txt','a')
        executable = '../../build/%s/rethinkdb' % self.server_build
        call([executable, 'create', '--directory', rdbfile], stdout=log_out, stderr=log_out)
        return executable, rdbfile, log_out

    def stop(self):
        self.cpp_server.terminate()
        sleep(0.1)

    def restart(self):
        self.stop()
        self.start()

