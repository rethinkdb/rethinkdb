#!/usr/bin/env python

import atexit, shutil
import random
import socket
import os
import sys
from time import sleep
from subprocess import call, check_call, Popen, PIPE, STDOUT

runningServers = []
def shutdown_servers():
    global runningServers
    for server in runningServers:
        try:
            server.stop()
        except: pass
atexit.register(shutdown_servers)

# Connection to /dev/null to redirect unwanted subprocess output to
null = open('/dev/null', 'w')

# Manages a cluster of RethinkDB servers
class RethinkDBTestServers(object):
    
    group_data_dir = None
    servers = None
    
    def __init__(self, num_servers=4, server_build_dir=None, use_default_port=False, cache_size=1024, group_data_dir ='./run'):
        assert num_servers >= 1
        self.num_servers = num_servers
        self.server_build_dir = server_build_dir
        self.use_default_port = use_default_port
        self.cache_size = cache_size
        self. group_data_dir = os.path.realpath(group_data_dir) 

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *args):
        self.stop()

    def start(self):
        self.servers = [RethinkDBTestServer(self.server_build_dir, self.use_default_port, self.cache_size, self.group_data_dir)
                            for i in xrange(0, self.num_servers)]

        cluster_port = self.servers[0].start()
        for server in self.servers[1:]:
            server.join(cluster_port)

    def stop(self):
        if self.servers is not None:
            for server in reversed(self.servers):
                server.stop()
        self.servers = None
        self.clear_data()

    def clear_data(self):
        pass # TODO: figure out when to clear the data

    def restart(self, clear_data=False):
        self.stop()
        if clear_data == True:
            self.clear_data()
        self.start()

    def driver_port(self):
        return self.servers[0].driver_port

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
    
    group_data_dir = None
    server_data_dir = None
    
    driver_port = None
    cluster_port = None
    log_file = None
    rdbfile_path = None
    
    def __init__(self, server_build_dir=None, use_default_port=False, cache_size=1024, group_data_dir='./run'):
        self.server_build_dir = server_build_dir
        self.use_default_port = use_default_port
        self.cache_size = cache_size
        self.group_data_dir = group_data_dir

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
        raise Exception("Wow, you must have won the lottery or something: Ten random ports and they're all occupied")

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
            self.driver_port = 28015
        else:
            self.driver_port = self.find_available_port()
        self.cluster_port = self.find_available_port()
        self.create()

        self.cpp_server = Popen([self.executable, 'serve',
                                 '--driver-port', str(self.driver_port),
                                 '--directory', self.rdbfile_path,
                                 '--http-port', '0',
                                 '--cache-size', str(self.cache_size),
                                 '--cluster-port', str(self.cluster_port)],
                                stdout=self.log_file, stderr=self.log_file)
        runningServers.append(self)
        sleep(2)

        return self.cluster_port

    # Join a cluster headed by a server previously invoked with start
    def join(self, cluster_port):
        self.driver_port = self.find_available_port()
        self.cluster_port = self.find_available_port()
        self.create()
        
        self.cpp_server = Popen([self.executable, 'serve',
                                 '--driver-port', str(self.driver_port),
                                 '--cluster-port', str(self.cluster_port),
                                 '--directory', self.rdbfile_path,
                                 '--http-port', '0',
                                 '--join', 'localhost:%d' % cluster_port],
                                stdout=self.log_file, stderr=self.log_file)
        runningServers.append(self)
        sleep(2)

    def create(self):
        self.server_data_dir = os.path.join(self.group_data_dir, 'server_%s' % self.driver_port)
        self.rdbfile_path = os.path.join(self.server_data_dir, 'rdb')
        
        if os.path.exists(self.server_data_dir): # we need a clean data directory TODO: evaluate moving this to a tempfile folder
            # TODO: log that we are cleaning off this directory
            if os.path.isdir(self.server_data_dir) and not os.path.islink(self.server_data_dir):    
                shutil.rmtree(self.server_data_dir)
            else:
                os.unlink(self.server_data_dir)
        os.makedirs(self.server_data_dir)
        
        self.log_file = open(os.path.join(self.server_data_dir, 'server-log.txt'), 'a+')
        
        self.executable = os.path.join(self.server_build_dir or os.getenv('RETHINKDB_BUILD_DIR') or '../../build/debug', 'rethinkdb')
        check_call([self.executable, 'create', '--directory', self.rdbfile_path], stdout=self.log_file, stderr=STDOUT)

    def stop(self):
        logFilePath = self.log_file.name
        code = self.cpp_server.poll()
        if code == None:
            self.cpp_server.terminate()
            code = self.cpp_server.wait()
        self.log_file.close()
        if code != 0:
            logOutput = ''
            try:
                logOutput = open(logFilePath).read()
            except: pass
            
            raise Exception("Error: rethinkdb process %d failed with error code %d\n%s" % (self.cpp_server.pid, code, logOutput))
        self.cpp_server = None
        self.log_file = None
        runningServers.append(self)
        sleep(0.1)

    def restart(self):
        self.stop()
        self.start()

    def alive(self):
        return self.cpp_server.poll() == None

def shard_table(port, build, table_name):
    rtn_sum = 0
    rtn_sum += call([build, 'admin', '--join', 'localhost:%d' % port, 'split', 'shard', table_name,
					'Nc040800000000000\2333'], stdout=null, stderr=null)
    rtn_sum += call([build, 'admin', '--join', 'localhost:%d' % port, 'split', 'shard', table_name,
					'Nc048800000000000\2349'], stdout=null, stderr=null)
    rtn_sum += call([build, 'admin', '--join', 'localhost:%d' % port, 'split', 'shard', table_name,
					'Nc04f000000000000\2362'], stdout=null, stderr=null)
    sleep(3.0)
    return rtn_sum

def set_auth(port, build, auth_key):
    if len(auth_key) != 0:
        rtn_value = call([build, 'admin', '--join', 'localhost:%d' % port, 'set', 'auth', str(auth_key)], stdout=null, stderr=null)
    else:
        rtn_value = call([build, 'admin', '--join', 'localhost:%d' % port, 'unset', 'auth'], stdout=null, stderr=null)
    sleep(1.0)
    return rtn_value
