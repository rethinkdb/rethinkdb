# Copyright 2010-2015 RethinkDB, all rights reserved.

"""Manage a cluster of RethinkDB nodes, including simulating netsplits.
This is designed to simulate normal operations, so does not include support
for things like `--join`ing to an invalid port."""

import atexit, copy, datetime, os, random, re, shutil, signal, socket, string, subprocess, sys, tempfile, time, traceback, warnings

import utils, resunder

try:
    import thread
except ImportError:
    import _thread as thread
try:
    xrange
except NameError:
    xrange = range
try:
    unicode
except NameError:
    unicode = str

# == resunder support

class Resunder(object):
    
    blockedPaths = set()
    
    @classmethod
    def unblockAll(cls):
        for source, dest in cls.blockedPaths:
            cls.unblock_path(source, dest)
    
    @classmethod
    def send(cls, operation, source, dest):
        # - fail on MacOS
        if os.uname()[0] == 'Darwin':
            raise Exception('Resunder does not currently run on MacOS, see https://github.com/rethinkdb/rethinkdb/issues/3476')
        
        # - check that resunder is running
        # ToDo: fix this to use the pid file and flock
        if "resunder" not in subprocess.check_output(["ps", "-A", "-www", "-o", "command"]):
            raise Exception('Resunder is not running, please start it as root: `sudo %s/resunder.py start`' % os.path.realpath(os.path.dirname(__file__)))
        
        # - send to resunder
        # ToDo: re-write resunder.py so we can send multiple messages across the same socket
        resunderSocket = socket.create_connection(("localhost", resunder.resunderPort))
        resunderSocket.sendall("%s %s %s\n" % (str(operation), str(source), str(dest)))
        resunderSocket.close()
    
    @classmethod
    def block_path(cls, source_port, dest_port):
        cls.blockedPaths.add(tuple([source_port, dest_port]))
        cls.send('block', source_port, dest_port)
    
    @classmethod
    def unblock_path(cls, source_port, dest_port):
        ports = tuple([source_port, dest_port])
        if ports in cls.blockedPaths:
            cls.blockedPaths.remove(ports)
        cls.send('unblock', source_port, dest_port)
atexit.register(Resunder.unblockAll)

# == cleanup

runningServers = []
def endRunningServers():
    for server in copy.copy(runningServers):
        try:
            server.check_and_stop()
        except Exception as e:
            sys.stderr.write('Got error while shutting down server at exit: %s\n%s\n' % (str(e), traceback.format_exc()))
            sys.stderr.flush()
atexit.register(endRunningServers)

# ==

class Metacluster(object):
    """A `Metacluster` is a group of clusters. It's responsible for maintaining
    `resunder` blocks between different clusters. It's also a context manager
    that cleans up all the processes and deletes all the files. """
    
    __unique_id_counters = None
    _had_multiple_clusters = False
    
    def __init__(self, output_folder=None):
        self.clusters = set()
        self.__unique_id_counters = { 'server':0, 'proxy':0 }
        
        if output_folder is None:
            self.dbs_path = tempfile.mkdtemp()
            utils.cleanupPathAtExit(self.dbs_path)
        else:
            if not os.path.exists(output_folder):
                os.makedirs(output_folder)
            
            if os.path.isdir(output_folder):
                self.dbs_path = os.path.realpath(output_folder)
            else:
                raise ValueError('bad value for output_folder: %s' % str(output_folder))
    
    def stop(self):
        '''Stops all servers in all cluters'''
        for cluster in self.clusters:
            cluster.check_and_stop()
    def close(self):
        self.stop()

    def __enter__(self):
        return self

    def __exit__(self, exc, etype, tb):
        self.close()

    def get_new_unique_id(self, server_type='server'):
        if server_type not in self.__unique_id_counters:
            self.__unique_id_counters[server_type] = 0
        returnValue = self.__unique_id_counters[server_type]
        self.__unique_id_counters[server_type] += 1
        return returnValue
    
    def move_processes(self, source, dest, processes):
        """Moves a group of `Process`es from one `Cluster` to another. To split
        a cluster, create an empty cluster and use `move_processes()` to move
        some processes from the original one to the empty one; to join two
        clusters, move all the processes from one into the other. Note that
        this does not tell the servers to connect to each other; unless the
        incoming servers were connected to the existing servers before, or
        unless you start a new server in the destination cluster to bring the
        two groups of processes together, they may remain unconnected. """
        assert isinstance(source, Cluster)
        assert source.metacluster is self
        assert isinstance(dest, Cluster)
        assert dest.metacluster is self
        for process in processes:
            # assert sanity
            assert isinstance(process, Process)
            assert process.cluster is source
            assert process in process.cluster.processes
            
            # move the process
            process.cluster = dest
            source.processes.remove(process)
            dest.processes.append(process)
            
            # update the routes
            process.update_routing()

class Cluster(object):
    """A group of `Processes` that are all connected to each other (ideally, anyway; see the note in `move_processes`). """
    
    metacluster = None
    processes = None
    output_folder = None
    
    def __init__(self, metacluster=None, initial_servers=0, output_folder=None, console_output=True, executable_path=None, server_tags=None, command_prefix=None, extra_options=None, wait_until_ready=True):
        
        # -- input validation
        
        # - metacluster
        if metacluster is None:
            metacluster = Metacluster(output_folder=output_folder)
        
        assert isinstance(metacluster, Metacluster)
        self.metacluster = metacluster
        self.metacluster.clusters.add(self)
        if len(self.metacluster.clusters) > 1:
            self.metacluster._had_multiple_clusters = True
        
        # - output_folder
        if output_folder is None:
            output_folder = self.metacluster.dbs_path
        else:
            output_folder = str(output_folder)
            if os.path.exists(output_folder):
                assert os.path.isdir(output_folder), 'The output_folder was something other than a directory: %s' % output_folder
            else:
                assert os.path.isdir(os.path.dirname(output_folder)), 'The enclosing directory did not exist or was not a directory: %s' % output_folder
                os.mkdir(output_folder)
        self.output_folder = os.path.realpath(output_folder)
        
        # - initial_servers
        if isinstance(initial_servers, int):
            if initial_servers < 0:
                raise ValueError('the initial_servers input must be 0 or more, got: %d' % initial_servers)
            initial_servers = [None for _ in range(initial_servers)]
        elif hasattr(initial_servers, '__iter__'):
            for i in range(len(initial_servers)):
                initial_servers[i] = str(initial_servers[i])
        else:
            raise ValueError('the initial_servers input must be a number or a list of names/paths, got: %r' % initial_servers)
        
        # -- start servers
        
        self.processes = []
        if initial_servers:
            # start the first server, waiting until it is ready
            p = Process(cluster=self, name=initial_servers[0], console_output=console_output, executable_path=executable_path, command_prefix=command_prefix, extra_options=extra_options, wait_until_ready=True)
            assert p in self.processes
            
            # start others in parallel
            for name in initial_servers[1:]:
                # The constructor will insert itself into `self.processes`
                p = Process(cluster=self, name=name, console_output=console_output, executable_path=executable_path, command_prefix=command_prefix, extra_options=extra_options, wait_until_ready=False)
                assert p in self.processes
        
        # -- wait for all servers to be ready if asked
        
        if wait_until_ready:
            self.wait_until_ready()
    
    def __enter__(self):
        self.wait_until_ready()
        return self
    
    def __exit__(self, type, value, traceback):
        self.check_and_stop()
    
    @property
    def running(self):
        if not self.processes:
            return False
        for process in self.processes:
            if not process.running:
                return False
        return True
    
    def check(self):
        """Throws an exception if any of the processes in the cluster has stopped or crashed. """
        for proc in self.processes:
            proc.check()
    
    def wait_until_ready(self, timeout=30):
        for server in self.processes:
            server.wait_until_ready(timeout=timeout)
        # ToDo: try all of them in parallel to handle the timeout correctly
    
    def check_and_stop(self):
        '''Check that all servers are running as expected, then stop them all. Throws an error on unexpected exit codes'''
        try:
            for server in self.processes:
                server.check_and_stop()
        finally:
            for server in self.processes:
                server.stop()
    
    def update_routing(self, target=None):
        '''Update the routing tables that block servers from one cluster from seeing those from another'''
        
        # - short-circut if there is only a single cluster
        if len(self.metacluster.clusters) < 2 and self.metacluster._had_multiple_clusters is False:
            return
        
        # -
        servers = None
        if target is None:
            servers = self.processes
        elif hasattr(target, '__iter__'):
            servers = target
        else:
            assert isinstance(target, Process)
            servers = [target]
        
        # - update blocking for all servers
        allServers = sum([cluster.processes for cluster in self.metacluster.clusters], [])
        for server in servers:
            if not all([server._cluster_port, server._local_cluster_port]):
                continue # not much we can do here
                
            for otherServer in allServers:
                if not otherServer.ready or otherServer is server:
                    continue # nothing to do here
                
                if server.cluster is not otherServer.cluster and server.ready: # active server, block from server outside cluster
                    # outgoing paths
                    Resunder.block_path(server.cluster_port,            otherServer.local_cluster_port)
                    Resunder.block_path(server.cluster_port,            otherServer.cluster_port)
                    Resunder.block_path(server.local_cluster_port,      otherServer.cluster_port)
                    Resunder.block_path(server.local_cluster_port,      otherServer.local_cluster_port)
                    # incoming paths
                    Resunder.block_path(otherServer.cluster_port,       server.local_cluster_port)
                    Resunder.block_path(otherServer.cluster_port,       server.cluster_port)
                    Resunder.block_path(otherServer.local_cluster_port, server.cluster_port)
                    Resunder.block_path(otherServer.local_cluster_port, server.local_cluster_port)
                else:
                    # outgoing paths
                    Resunder.unblock_path(server.cluster_port,            otherServer.local_cluster_port)
                    Resunder.unblock_path(server.cluster_port,            otherServer.cluster_port)
                    Resunder.unblock_path(server.local_cluster_port,      otherServer.cluster_port)
                    Resunder.unblock_path(server.local_cluster_port,      otherServer.local_cluster_port)
                    # incoming paths
                    Resunder.unblock_path(otherServer.cluster_port,       server.local_cluster_port)
                    Resunder.unblock_path(otherServer.cluster_port,       server.cluster_port)
                    Resunder.unblock_path(otherServer.local_cluster_port, server.cluster_port)
                    Resunder.unblock_path(otherServer.local_cluster_port, server.local_cluster_port)
        
    def __getitem__(self, pos):
        if isinstance(pos, slice):
            return [self.processes[x] for x in xrange(*pos.indices(len(self.processes)))]
        elif isinstance(pos, int):
            if not (-1 * len(self.processes) <= pos < len(self.processes) ):
                raise IndexError('This cluster only has %d servers, so index %s is invalid' % (len(self.processes), str(pos)))
            return self.processes[pos]
        elif isinstance(pos, (str, unicode)):
            for server in self.processes:
                if pos == server.name:
                    return server
            else:
                raise KeyError('This cluster does not have a server named: %s' % pos) 
        else:
            raise TypeError("Invalid argument type: %s" % repr(pos))
    
    def __iter__(self):
        return iter(self.processes)
    
    def __len__(self):
        if not self.processes:
            return 0
        return len(self.processes)

class Process(object):
    '''A running RethinkDB server'''
    
    startupTimeout = 30
    
    server_type = 'server'
    cluster = None
    
    data_path = None
    logfile_path = None
    _console_file = None
    _console_file_name = 'console.txt'
    _console_file_path = None
    
    host = 'localhost'
    _cluster_port = None
    _driver_port = None
    _http_port = None
    _local_cluster_port = None # force the outgoing cluster connections to a single port to allow for blocking
    _ready_line = False
    _existing_log_len = 0
    returncode = None
    killed = False # True is self.kill() was used
    
    _name = None # cache for actual name 
    _desired_name = None # name to try and run with
    _uuid = None
    
    process = None
    pid = None
    
    args = None # complete command-line invocation
    command_prefix = None
    executable_path = None
    options = None
    tags = None
    
    logfilePortRegex = re.compile('Listening for (?P<type>intracluster|client driver|administrative HTTP) connections on port (?P<port>\d+)$')
    logfileServerIDRegex = re.compile('Our server ID is (?P<uuid>\w{8}-\w{4}-\w{4}-\w{4}-\w{12})$')
    logfileReadyRegex = re.compile('(Server|Proxy) ready(, "(?P<name>\w+)" (?P<uuid>\w{8}-\w{4}-\w{4}-\w{4}-\w{12}))?$')
    
    @staticmethod
    def genPath(name, path, extraLetters=3):
        '''generate a non-existing name with a bit of randomness at the end'''
        newPath = path # should always exist
        while os.path.exists(newPath):
            newPath = '%s_%s' % (os.path.join(path, name), ''.join(random.choice(string.ascii_lowercase) for i in range(extraLetters)))
        os.mkdir(newPath)
        return newPath
    
    def subclass_init(self):
        '''Complete init for this subclass.'''
        
        # -- modify options
        
        # add --cache-size if it is not specified
        if not '--cache-size' in self.options and not any([x.startswith('--cache-size=') for x in self.options]):
            self.options += ['--cache-size', '512']
        
        # add --directory
        for i, option in enumerate(self.options):
            if option == '--directory' or option.startswith('--directory='):
                raise ValueError('The --directory should not be provided in extra_options')
        self.options += ['--directory', self.data_path]
        
        # add 'serve' before the rest of the options
        self.options.insert(0, 'serve')
        
        # -- create the data folder if it does not already exists
        
        # get --server-tag entries
        server_tags = []
        for tag in self.server_tags:
            server_tags += ['--server-tag', tag]
        
        if not os.path.exists(os.path.join(self.data_path, 'metadata')):
            self._console_file.write("Creating data directory at %s (%s)\n" % (self.data_path, datetime.datetime.now().isoformat()))
            self._console_file.flush()
            command = self.command_prefix + [self.executable_path] + ['create', '--server-name', self._desired_name, '--directory', self.data_path, '--log-file', str(self.logfile_path)] + server_tags
            subprocess.check_call(command, stdout=self._console_file, stderr=subprocess.STDOUT)
    
    def __init__(self, cluster=None, name=None, console_output=None, executable_path=None, server_tags=None, command_prefix=None, extra_options=None, wait_until_ready=True):
        global runningServers
        
        # -- validate/default input
        
        # - cluster
        if cluster is None:
            cluster = Cluster()
        assert isinstance(cluster, Cluster), 'cluster must be a Cluster or None, got: %r' % cluster
        self.cluster = cluster
        self.cluster.processes.append(self)
        
        # - name/data_path
        assert isinstance(name, (str, unicode, None.__class__)), 'name was not an expected value: %r' % name
        if name is None:
            # default name and data_path
            name = '%s_%s' % (self.server_type, self.cluster.metacluster.get_new_unique_id(server_type=self.server_type))
            data_path = self.genPath(name, self.cluster.output_folder)
        elif name == '.':
            # random name in the current directory
            name = '%s_%s' % (self.server_type, self.cluster.metacluster.get_new_unique_id(server_type=self.server_type))
            data_path = self.genPath(name, os.path.realpath('.'))
        elif os.sep in name: # if it looks like a path, it is one, possibly relative
            data_path = os.path.abspath(os.path.join(self.cluster.output_folder, name)) # note: an absolute path survives this
            name = os.path.basename(name)
            if os.path.exists(data_path):
                assert os.path.isdir(data_path), 'The data_path was something other than a directory: %s' % data_path
                if os.path.isfile(os.path.join(data_path, 'log_file')):
                    # existing data folder, record server log file path
                    self.logfile_path = os.path.join(data_path, 'log_file')
                elif os.path.isfile(os.path.join(data_path, 'log_file.txt')):
                    # existing data folder, record server log file path
                    self.logfile_path = os.path.join(data_path, 'log_file.txt')
                else:
                    # folder for holding multiple server data folders
                    data_path = self.genPath(name, data_path)
            else:
                # specified path, use it exactly
                assert os.path.isdir(os.path.dirname(data_path)), 'The enclosing directory did not exist or was not a directory: %s' % data_path
        else:
            # just a name
            data_path = self.genPath(name, self.cluster.output_folder)
        self.data_path = data_path
        self._desired_name = name.replace('-', '_')
        
        # - console_output/console_file - can be: path, file-object, True/False
        if console_output is None:
            # default to stdout
            self._console_file_path = None
            self._console_file = sys.stdout
        elif console_output is False:
            # throw away file
            self._console_file_path = None
            self._console_file = tempfile.NamedTemporaryFile(mode='w+')
        elif console_output is True:
            # keep it with the data, but create it in the encosing folder until we are running
            self._console_file_path = os.path.join(self.data_path, self._console_file_name)
            self._console_file = tempfile.NamedTemporaryFile(mode='w+', dir=os.path.dirname(self.data_path), delete=False)
        elif hasattr(console_output, 'write'):
            # file-like object:
            self._console_file_path = None
            self._console_file = console_output
        else:
            # a path
            assert isinstance(console_output, (str, unicode)), 'Unknown console_output: %r' % console_output
            console_output = os.path.realpath(console_output)
            assert os.path.isdir(os.path.dirname(console_output)), 'console_output parent directory was not a folder: %r' % console_output
            assert os.path.isfile(console_output) or not os.path.exists(console_output), 'console_output location was not useable: %r' % console_output
            self._console_file_path = console_output
            self._console_file = open(console_output, 'a')
        
        # - server_tags
        if server_tags is None:
            self.server_tags = []
        elif hasattr(server_tags, '__iter__') and not hasattr(server_tags, 'capitalize'):
            self.server_tags = [str(x) for x in server_tags]
        else:
            self.server_tags = [str(server_tags)]
        
        # - executable_path
        if executable_path is None:
            executable_path = utils.find_rethinkdb_executable()
        assert os.access(executable_path, os.X_OK), "no such executable: %r" % executable_path
        self.executable_path = executable_path
        
        # - command_prefix
        if command_prefix is None:
            self.command_prefix = []
        elif not hasattr(command_prefix, '__iter__'):
            raise ValueError('command_prefix must be an array of command line options, got: %r' % command_prefix)
        else:
            self.command_prefix = command_prefix
        
        # - extra_options
        options = []
        if extra_options is None:
            pass
        elif not hasattr(extra_options, '__iter__'):
            raise ValueError('extra_options must be an array of command line options, got: %r' % extra_options)
        else:
            options = [str(x) for x in extra_options]
        
        # -- store values
        
        if not self.logfile_path:
            self.logfile_path = os.path.join(self.data_path, "log_file.txt")
        
        # -- set defaults
        
        if not '--bind' in options and not any([x.startswith('--bind=') for x in options]):
            options += ['--bind', 'all']
        
        if not '--cluster-port' in options and not any([x.startswith('--cluster-port=') for x in options]):
            options += ['--cluster-port', '0']
        
        if not '--driver-port' in options and not any([x.startswith('--driver-port=') for x in options]):
            options += ['--driver-port', '0']
        
        if not '--http-port' in options and not any([x.startswith('--http-port=') for x in options]):
            options += ['--http-port', '0']
        
        for i in range(len(options)):
            if options[i] == '--log-file':
                assert len(options) > i + 1, '--log-file specified in options without a path'
                self.logfile_path = os.path.realpath(options[i+1])
                break
            elif options[i].startswith('--log-file='):
                self.logfile_path = os.path.realpath(options[i][len('--log-file='):].strip('\'"'))
                break
        else:
            options += ['--log-file', str(self.logfile_path)]
        
        if not '--no-update-check' in options:
            options += ['--no-update-check'] # supress update checks/reporting in
        
        
        self.options = options
        
        # - subclass modifications
        
        self.subclass_init()
        
        # - save the args
        
        self.args = self.command_prefix + [self.executable_path] + self.options
        
        # -- start the server process
        
        self.start(wait_until_ready=wait_until_ready)
    
    def start(self, wait_until_ready=True):
        '''Start up the server'''
        global runningServers
        
        assert not self.running, 'Trying to start a server where running = %r' % self.running
        self.returncode = None
        self.killed = False
        
        # -- copy the options
        
        options = copy.copy(self.args)
        
        # -- set the local_cluster_port
        
        if not '--client-port' in options and not any([x.startswith('--client-port=') for x in options]):
            # allows resunder to know what port to block
            options += ['--client-port', str(self.local_cluster_port)]
        
        # -- set to join the cluster
        
        for peer in self.cluster.processes:
            if peer != self and peer.ready:
                options += ["--join", peer.host + ":" + str(peer.cluster_port)]
                break
        
        # -- get the length of an exiting log file
        
        if os.path.isfile(self.logfile_path):
            self._existing_log_len = os.path.getsize(self.logfile_path)
        
        # -- start the process
        
        try:
            self._console_file.write("Launching at %s:\n\t%s\n" % (datetime.datetime.now().isoformat(), " ".join(options)))
            self._console_file.flush()
            self.process = subprocess.Popen(options, stdout=self._console_file, stderr=subprocess.STDOUT, preexec_fn=os.setpgrp)
            
            if not self in runningServers:
                runningServers.append(self)
            
            # - start thread to tail output for needed info
            thread.start_new_thread(self.read_ports_from_log, ())

        except Exception:
            self.stop()
            raise
        
        finally:
            # - move console file if necessary
            if self._console_file_path and not os.path.exists(self._console_file_path):
                os.rename(self._console_file.name, self._console_file_path)
        
        # -- wait until ready (if requested)
        if wait_until_ready:
            self.wait_until_ready()
    
    def __enter__(self):
        self.wait_until_ready()
        return self
    
    def __exit__(self, type, value, traceback):
        # ToDo: handle non-normal exits
        self.stop()
    
    def __wait_for_value(self, valueName):
        
        deadline = time.time() + self.startupTimeout
        while deadline > time.time():
            value = getattr(self, str(valueName))
            if value is not None:
                return value
            self.check()
            time.sleep(.1)
        else:
            raise RuntimeError('Timed out waiting for %s value' % valueName.lstrip('_').replace('_', ' '))
    
    @property
    def cluster_port(self):
        return self.__wait_for_value('_cluster_port')
    
    @property
    def driver_port(self):
        return self.__wait_for_value('_driver_port')
        
    @property
    def http_port(self):
        return self.__wait_for_value('_http_port')
    
    @property
    def local_cluster_port(self):
        if self._local_cluster_port is None:
            self._local_cluster_port = utils.get_avalible_port()
        return self._local_cluster_port
    
    @property
    def name(self):
        return self.__wait_for_value('_name')

    @property
    def uuid(self):
        return self.__wait_for_value('_uuid')
    
    @property
    def ready(self):
        if not self.running:
            return False
        if not self._ready_line:
            return False
        if not all([self._cluster_port, self._driver_port, self._http_port, self._ready_line]):
            return False
        return True
    
    @property
    def running(self):
        '''Check that the process is running'''
        if self.process is None:
            return False
        if self.returncode is not None:
            return False
        elif self.process.poll() is not None:
            self.returncode = self.process.poll()
            return False
        return True
    
    @property
    def pid(self):
        '''Get the pid of the process'''
        assert self.running
        return self.process.pid
    
    def wait_until_ready(self, timeout=30):
        deadline = time.time() + timeout
        while deadline > time.time():
            self.check()
            if self.ready:
                return
            else:
                time.sleep(0.05)
        else:
            raise RuntimeError("Timed out after waiting %d seconds for startup." % timeout)
    
    def read_ports_from_log(self, timeout=30):
        deadline = time.time() + timeout
        
        # -- wait for the log file to appear
        
        while time.time() < deadline:
            self.check()
            if os.path.isfile(self.logfile_path):
                break
            else:
                time.sleep(0.1)
        else:
            raise RuntimeError("Timed out after %d seconds waiting for the log file to appear at: %s" % (timeout, self.logfile_path))
        
        # -- monitor the logfile for the given lines
        
        logLines = utils.nonblocking_readline(self.logfile_path, self._existing_log_len)
        
        # - look for the data lines
        
        while time.time() < deadline:
            
            # - bail out if we have everything
            
            self.check()
            if self.ready:
                return
            
            # - get a new line or sleep
            
            logLine = next(logLines)
            if logLine is None:
                time.sleep(0.05)
                self.check()
                continue
            
            # - see if it matches one we are looking for
            
            try:
                parsedLine = self.logfilePortRegex.search(logLine)
            except Exception as e:
                warnings.warn('Got unexpected logLine: %s' % repr(logLine))
            if parsedLine:
                if parsedLine.group('type') == 'intracluster':
                    self._cluster_port = int(parsedLine.group('port'))
                elif parsedLine.group('type') == 'client driver':
                    self._driver_port = int(parsedLine.group('port'))
                else:
                    self._http_port = int(parsedLine.group('port'))
                continue
            
            parsedLine = self.logfileServerIDRegex.search(logLine)
            if parsedLine:
                self._uuid = parsedLine.group('uuid')
            
            parsedLine = self.logfileReadyRegex.search(logLine)
            if parsedLine:
                self._ready_line = True
                self._name = parsedLine.group('name')
                self._uuid = parsedLine.group('uuid')
        else:
            raise RuntimeError("Timeout while trying to read ports from log file")
        
        # -- piggyback on this to setup Resunder blocking
        
        self.update_routing()
    
    def check(self):
        """Throws an exception if the process has crashed or stopped. """
        if not self.running:
            raise RuntimeError("%s %s stopped unexpectedly with return code: %r" % (self.server_type.capitalize(), self._name, self.returncode))

    def check_and_stop(self):
        '''Check if the server is running normally, then shut it down.'''
        global runningServers
        
        try:
            if self.running:
                self.check()
                self.stop()
            assert not self.running
            if self.killed is False and self.returncode is not 0:
                raise RuntimeError('%s %s stopped unexpectedly with return code: %r' % (self.server_type.capitalize(), self._name, self.returncode))
        finally:
            if self in runningServers:
                runningServers.remove(self)
            self.stop()
    
    def kill(self):
        '''Suddenly terminate the process ungracefully'''
        
        assert self.process is not None
        assert self.check() is None, 'When asked to kill a process it was already stopped!'
        
        utils.kill_process_group(self, timeout=0)
        self.returncode = self.process.wait()
        self.processs = None
        self.killed = True
        self.stop()
    
    def stop(self):
        '''Gracefully shut down the server'''
        global runningServers
        
        if self.running:
            if self.process.poll() is None:
                utils.kill_process_group(self, timeout=20)
                self.returncode = self.process.poll()
                if self.returncode is None:
                    try:
                        self.returncode = self.process.wait()
                    except OSError:
                        sys.stderr.write('The subprocess module lost the connection to the %s %s, assuming it closed cleanly (2)\n' % (self.server_type, self.name))
                        sys.stderr.flush()
                        self.returncode = 0
                assert self.returncode is not None, '%s %s failed to exit!' % (self.server_type.capitalize(), self.name)
        if self.process:
            try:
                self.returncode = self.process.wait()
            except OSError:
                sys.stderr.write('The subprocess module lost the connection to the %s %s, assuming it closed cleanly (3)\n' % (self.server_type, self.name))
                sys.stderr.flush()
                self.returncode = 0
        
        if self in runningServers:
            runningServers.remove(self)
        
        # - reset Resunder blocking for these ports
        
        self.update_routing()
        
        # - clean out running variables
        
        self.process = None
        self._cluster_port = None
        self._driver_port = None
        self._http_port = None
        self._ready_line = False
        self._log_maker = None
    def close(self):
        self.stop()
    
    @property
    def console_output(self):
        if self._console_file:
            try:
                self._console_file.flush()
            except Exception: pass
        if self._console_file_path and os.path.isfile(self._console_file_path):
            try:
                with open(self._console_file_path, 'r') as outputFile:
                    return(outputFile.read())
            except Exception as e:
                print('Error: unable to read server console output!')
        else:
            return ''
    
    def update_routing(self):
        self.cluster.update_routing(self)

class ProxyProcess(Process):
    '''A running RethinkDB proxy'''
    server_type = 'proxy'
    
    def subclass_init(self):
        
        # remove --cache-size
        for option in self.options[:]:
            if option == '--cache-size':
                position = options.index(option)
                assert len(serve_options) > position, 'Had a --cache-size marker without the spec' 
                self.options.pop(position)
                self.options.pop(position)
                break # we can only handle one
            elif option.startswith('--cache-size='):
                self.options.remove(option)
                break # we can only handle one
        
        # prepend 'proxy'
        self.options.insert(0, 'proxy')

# == main

if __name__ == "__main__":
    with Process() as p:
        time.sleep(3)
        p.check_and_stop()
