import random
import socket
import os
import sys
from time import sleep
from subprocess import call, Popen, PIPE

# Manages a cluster of RethinkDB servers
class RethinkDBTestServers(object):
    def __init__(self, num_servers=4, server_build_dir=None, use_default_port=False):
        assert num_servers >= 1
        self.num_servers = num_servers
        self.server_build_dir = server_build_dir
        self.use_default_port = use_default_port

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args):
        self.stop()

    def start(self):
        self.servers = [RethinkDBTestServer(self.server_build_dir, self.use_default_port)
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

    def cluster_port(self):
        return self.servers[0].cluster_port

    def executable(self):
        return self.servers[0].executable

    def alive(self):
        for s in self.servers:
            if not s.alive():
                return False
        return True

# Manages starting and stopping an instance of the Rethindb server
class RethinkDBTestServer(object):
    def __init__(self, server_build_dir=None, use_default_port=False):
        self.server_build_dir = server_build_dir
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
        self.cluster_port = self.find_available_port()
        directory, log_out = self.create()

        self.cpp_server = Popen([self.executable, 'serve',
                                 '--driver-port', str(self.cpp_port),
                                 '--directory', directory,
                                 '--http-port', '0',
                                 '--cluster-port', str(self.cluster_port)],
                                stdout=log_out, stderr=log_out)
        sleep(0.2)

        # Create database 'test' which the tests assume but doesn't get created when we
        # start up rethinkdb like this
        returncode = call(
            [self.executable, 'admin', '--join', 'localhost:%d' % self.cluster_port, 'create', 'database', 'test'],
            stdout=log_out, stderr=log_out)
        if returncode != 0:
            sys.stderr.write(open(self.log_file).read())
            raise Exception("Cannot create database test: rethinkdb admin process exited with code %d" % returncode)
        return self.cluster_port

    # Join a cluster headed by a server previously invoked with start
    def join(self, cluster_port):
        self.cpp_port = self.find_available_port()
        self.cluster_port = self.find_available_port()
        directory, log_out = self.create()
        self.cpp_server = Popen([self.executable, 'serve',
                                 '--driver-port', str(self.cpp_port),
                                 '--cluster-port', str(self.cluster_port),
                                 '--directory', directory,
                                 '--http-port', '0',
                                 '--join', 'localhost:%d' % cluster_port],
                                stdout=log_out, stderr=log_out)
        sleep(0.2)

    def create(self):
        directory = 'run/server_%s/' % self.cpp_port
        rdbfile = directory+'rdb'
        call(['mkdir', '-p', directory])
        self.log_file = directory+'server-log.txt'
        log_out = open(self.log_file, 'a')
        self.executable = os.path.join(self.server_build_dir or os.getenv('RETHINKDB_BUILD_DIR') or '../../build/debug', 'rethinkdb')
        call([self.executable, 'create', '--directory', rdbfile], stdout=log_out, stderr=log_out)
        return rdbfile, log_out

    def stop(self):
        code = self.cpp_server.poll()
        if code != None:
            if code != 0:
                sys.stderr.write(open(self.log_file).read())
                raise Exception("Error: rethinkdb process %d failed with error code %d\n" % (self.cpp_server.pid, code))
        else:
            self.cpp_server.terminate()
        sleep(0.1)

    def resstart(self):
        self.stop()
        self.start()

    def alive(self):
        return self.cpp_server.poll() == None

def shard_table(port, build, table_name):
    rtn_sum = 0
    rtn_sum += call([build, 'admin', '--join', 'localhost:%d' % port, 'split', 'shard', table_name,
					'Nc040800000000000\2333'], stdout=PIPE, stderr=PIPE)
    rtn_sum += call([build, 'admin', '--join', 'localhost:%d' % port, 'split', 'shard', table_name,
					'Nc048800000000000\2349'], stdout=PIPE, stderr=PIPE)
    rtn_sum += call([build, 'admin', '--join', 'localhost:%d' % port, 'split', 'shard', table_name,
					'Nc04f000000000000\2362'], stdout=PIPE, stderr=PIPE)
    sleep(3.0)
    return rtn_sum
