# Copyright 2010-2015 RethinkDB, all rights reserved.

"""`driver.py` is a module for starting groups of RethinkDB cluster nodes and
connecting them to each other. It also supports netsplits.

It does not support administering a cluster, either through the HTTP interface
or using `rethinkdb admin`. It is meant to be used with other modules that
administer the cluster which it starts.

`driver.py` is designed to use the RethinkDB command line interface, not to
test it; if you want to do strange things like tell RethinkDB to `--join` an
invalid port, or delete the files out from under a running RethinkDB process,
or so on, you should start a RethinkDB process manually using some other
module. """

from __future__ import print_function

import atexit, copy, os, random, re, shutil, signal, socket, subprocess, sys, tempfile, time, warnings

import utils

try:
    import thread
except ImportError:
    import _thread as thread
try:
    xrange
except NameError:
    xrange = range

def block_path(source_port, dest_port):
    # `-A` means list all processes. `-www` prevents `ps` from truncating the output at
    # some column width. `-o command` means that the output format should be to print the
    # command being run.
    if "resunder" not in subprocess.check_output(["ps", "-A", "-www", "-o", "command"]):
        sys.stderr.write('\nPlease start resunder process in test/common/resunder.py (as root)\n\n')
        assert False, 'Resunder is not running, please start it from test/common/resunder.py (as root)'
    conn = socket.create_connection(("localhost", 46594))
    conn.sendall("block %s %s\n" % (str(source_port), str(dest_port)))
    # TODO: Wait for ack?
    conn.close()

def unblock_path(source_port, dest_port):
    assert "resunder" in subprocess.check_output(["ps", "-A", "-www", "-o", "command"])
    conn = socket.create_connection(("localhost", 46594))
    conn.sendall("unblock %s %s\n" % (str(source_port), str(dest_port)))
    conn.close()

runningServers = []
def endRunningServers():
    for server in copy.copy(runningServers):
        try:
            server.check_and_stop()
        except Exception as e:
            sys.stderr.write('Got error while shutting down server at exit: %s\n' % str(e))
atexit.register(endRunningServers)

def get_table_host(processes):
    return ("localhost", random.choice(processes).driver_port)

class Metacluster(object):
    """A `Metacluster` is a group of clusters. It's responsible for maintaining
    `resunder` blocks between different clusters. It's also a context manager
    that cleans up all the processes and deletes all the files. """
    
    __unique_id_counter = None
    
    def __init__(self, output_folder=None):
        self.clusters = set()
        self.__unique_id_counter = 0
        self.closed = False
        
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
    
    def close(self):
        """Kills all processes and deletes all files. Also, makes the
        `Metacluster` object invalid. Call `close()` xor `__exit__()`, not
        both, because `__exit__()` calls `close()`. """
        assert not self.closed
        self.closed = True
        while self.clusters:
            iter(self.clusters).next().check_and_stop()
        self.clusters = None
        shutil.rmtree(self.dbs_path)

    def __enter__(self):
        return self

    def __exit__(self, exc, etype, tb):
        self.close()

    def get_new_unique_id(self):
        returnValue = self.__unique_id_counter
        self.__unique_id_counter += 1
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
            assert isinstance(process, Process)
            assert process.cluster is source
            process.cluster = None
            source.processes.remove(process)
        try:
            for process in processes:
                source._block_process(process)
            for process in processes:
                dest._unblock_process(process)
        except Exception:
            for process in processes:
                process.close()
            raise
        for process in processes:
            process.cluster = dest
            dest.processes.add(process)

class Cluster(object):
    """A `Cluster` represents a group of `Processes` that are all connected to each other (ideally, anyway; see the note in `move_processes`). """
    
    metacluster = None
    processes = None
    
    def __init__(self, metacluster=None, initial_servers=0, output_folder=None, console_output=True, executable_path=None, command_prefix=None, extra_options=None, wait_until_ready=True):
        
        # -- input validation
        
        # - initial_servers
        
        if isinstance(initial_servers, int):
            if initial_servers < 0:
                raise ValueError('the initial_servers input must be 0 or more, got: %d' % initial_servers)
            initial_servers = [None for _ in range(initial_servers)]
        elif hasattr(initial_servers, '__iter__'):
            replacement = []
            for entry in initial_servers:
                if not isinstance(entry, Files):
                    # not a files object, then a name
                    entry = str(entry)
                replacement.append(entry)
            initial_servers = replacement
        else:
            raise ValueError('the initial_servers input must be a number or a list of names or files objects, got: %s' % str(initial_servers))
        
        # - metacluster
        
        if metacluster is not None and output_folder is not None:
            raise NotImplementedError('supplying a metacluster and an output_folder does not currently work')
        elif metacluster is None:
            metacluster = Metacluster(output_folder=output_folder)
        
        assert isinstance(metacluster, Metacluster)
        assert not metacluster.closed
        
        # -- set variables
        
        self.metacluster = metacluster
        self.metacluster.clusters.add(self)
        self.processes = set()
        
        # -- start servers
        
        for nameOrFiles in initial_servers:
            self.processes.add(Process(cluster=self, files=nameOrFiles, console_output=console_output, executable_path=executable_path, command_prefix=command_prefix, extra_options=extra_options))
        
        # -- wait for servers
        
        if wait_until_ready:
            self.wait_until_ready()
    
    def __enter__(self):
        self.wait_until_ready()
        return self
    
    def __exit__(self, type, value, traceback):
        self.check_and_stop()
    
    def check(self):
        """Throws an exception if any of the processes in the cluster has stopped or crashed. """
        for proc in self.processes:
            proc.check()
    
    def wait_until_ready(self, timeout=30):
        for server in self.processes:
            server.wait_until_started_up(timeout=timeout)
        # ToDo: try all of them in parallel to handle the timeout correctly
    
    def check_and_stop(self):
        """First checks that each process in the cluster is still running, then
        stops them by sending SIGINT. Throws an exception if any exit with a
        nonzero exit code. Also makes the cluster object invalid """
        try:
            while self.processes:
                iter(self.processes).next().check_and_stop()
        finally:
            while self.processes:
                iter(self.processes).next().close()
            if self.metacluster is not None:
                self.metacluster.clusters.remove(self)
                self.metacluster = None
    
    def _block_process(self, process):
        assert process not in self.processes
        for other_process in self.processes:
            block_path(process.cluster_port, other_process.local_cluster_port)
            block_path(other_process.local_cluster_port, process.cluster_port)
            block_path(process.local_cluster_port, other_process.cluster_port)
            block_path(other_process.cluster_port, process.local_cluster_port)

    def _unblock_process(self, process):
        assert process not in self.processes
        for other_process in self.processes:
            unblock_path(process.cluster_port, other_process.local_cluster_port)
            unblock_path(other_process.local_cluster_port, process.cluster_port)
            unblock_path(process.local_cluster_port, other_process.cluster_port)
            unblock_path(other_process.cluster_port, process.local_cluster_port)
    
    def __getitem__(self, pos):
        if isinstance(pos, slice):
            items = list(self.processes)
            return [items[x] for x in xrange(*pos.indices(len(items)))]
        elif isinstance(pos, int):
            if not (-1 * len(self.processes) <= pos < len(self.processes) ):
                raise IndexError('This cluster only has %d servers, so index %s is invalid' % (len(self.processes), str(pos)))
            return list(self.processes)[pos]
        else:
            raise TypeError("Invalid argument type: %s" % repr(pos))
    
    def __iter__(self):
        return iter(self.processes)

class Files(object):
    """A `Files` object is a RethinkDB data directory. Each `Process` needs a
    `Files`. To "restart" a server, create a `Files`, create a `Process`, stop
    the process, and then start a new `Process` on the same `Files`. """

    db_path = None
    server_name = None
    metacluster = None
    
    def __init__(self, metacluster=None, server_name=None, server_tags=None, db_path=None, db_containter=None, console_output=None, executable_path=None, command_prefix=None):
        
        # -- input validation/defaulting
        
        if metacluster is None:
            metacluster = Metacluster() # ToDo: make this be the "default" Metacluster
        assert isinstance(metacluster, Metacluster)
        assert not metacluster.closed
        self.metacluster = metacluster
        
        self.id_number = self.metacluster.get_new_unique_id()
        
        if server_name is None:
            self.server_name = "node_%d" % self.id_number
        else:
            self.server_name = str(server_name)
        
        if db_path is None:
            folderName = server_name + '_data' if server_name else 'db-%d' % self.id_number
            if db_containter is None:
                self.db_path = os.path.join(self.metacluster.dbs_path, folderName)
            else:
                assert os.path.isdir(db_containter)
                self.db_path = os.path.join(db_containter, folderName)
        else:
            db_containter = db_containter or '.'
            self.db_path = os.path.join(db_containter, str(db_path))
        assert not os.path.exists(self.db_path), 'The path given for the new files already existed: %s' % self.db_path
        
        moveConsole = False
        if console_output is None:
            print("Redirecting console_output to /dev/null.")
            console_output = "/dev/null"
        elif console_output is True:
            console_output = tempfile.NamedTemporaryFile('w+', dir=self.metacluster.dbs_path)
            moveConsole = True
        elif console_output is False:
            console_output = tempfile.NamedTemporaryFile('w+')
        
        if executable_path is None:
            executable_path = utils.find_rethinkdb_executable()
        assert os.access(executable_path, os.X_OK), "no such executable: %r" % executable_path
        self.executable_path = executable_path
        
        if command_prefix is None:
            command_prefix = []
        
        if server_tags is None:
            server_tags = []
                
        # -- create the directory

        create_args = command_prefix + [
            self.executable_path, "create",
            "--directory", self.db_path,
            "--server-name", self.server_name]
        
        for tag in server_tags:
            create_args += ["--server-tag", tag]
        
        if hasattr(console_output, 'write'):
            subprocess.check_call(create_args, stdout=console_output, stderr=console_output)
        else:
            with open(console_output, "a") as console_file:
                subprocess.check_call(create_args, stdout=console_file, stderr=console_file)

class _Process(object):
    # Base class for Process & ProxyProcess. Do not instantiate directly.
    
    startupTimeout = 30
    
    running = False
    ready = False
    
    cluster = None
    executable_path = None
    
    files = None
    logfile_path = None
    console_file = None
    _close_console_output = False
    
    host = 'localhost'
    _cluster_port = None
    _driver_port = None
    _http_port = None
    local_cluster_port = None
    
    _name = None
    _uuid = None
    
    process_group_id = None
    
    logfilePortRegex = re.compile('Listening for (?P<type>intracluster|client driver|administrative HTTP) connections on port (?P<port>\d+)$')
    logfileServerIDRegex = re.compile('Our server ID is (?P<uuid>\w{8}-\w{4}-\w{4}-\w{4}-\w{12})$')
    logfileReadyRegex = re.compile('(Server|Proxy) ready(, "(?P<name>\w+)" (?P<uuid>\w{8}-\w{4}-\w{4}-\w{4}-\w{12}))?$')
    
    def __init__(self, cluster, options, console_output=None, executable_path=None, command_prefix=None):
        global runningServers
        
        # - validate input
        
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None
        
        if command_prefix is None:
            command_prefix = []
        
        if options is None:
            options = []
        elif not hasattr(options, 'index'):
            raise ValueError('options must be an array of command line options, got: %s' % str(options))
        
        if executable_path is None:
            executable_path = utils.find_rethinkdb_executable()
        assert os.access(executable_path, os.X_OK), "no such executable: %r" % executable_path
        self.executable_path = executable_path
        
        for other_cluster in cluster.metacluster.clusters:
            if other_cluster is not cluster:
                other_cluster._block_process(self)
        
        if self.console_file is None:
            if console_output is None:
                self.console_file = sys.stdout
            elif console_output in (False, True):
                self._close_console_output = True
                if console_output is False or self.files is None:
                    self.console_file = tempfile.NamedTemporaryFile(mode='w+')
                else:
                    self.console_file = open(os.path.join(self.files.db_path, 'console.txt'), 'w+')
            elif hasattr(console_output, 'write'):
                self.console_file = console_output
            else:
                self._close_console_output = True
                self.console_file = open(console_output, "a")
        
        # - add to the cluster
        
        self.cluster = cluster
        self.cluster.processes.add(self)
        
        # - set defaults
        
        if not '--bind' in options:
            options += ['--bind', 'all']
        
        if not '--cluster-port' in options:
            options += ['--cluster-port', '0']
        
        if not '--driver-port' in options:
            options += ['--driver-port', '0']
        
        if not '--http-port' in options:
            options += ['--http-port', '0']
        
        if not '--client-port' in options: # allows resunder to know what port to block
            self.local_cluster_port = utils.get_avalible_port()
            options += ['--client-port', str(self.local_cluster_port)]
        
        # - set to join the cluster
        
        for peer in cluster.processes:
            if peer != self:
                options += ["--join", peer.host + ":" + str(peer.cluster_port)]
                break
        
        # -
        
        try:
            self.args = command_prefix + [self.executable_path] + options

            if os.path.exists(self.logfile_path):
                os.unlink(self.logfile_path)
            
            self.console_file.write("Launching:\n%s\n" % str(self.args))
            
            self.process = subprocess.Popen(self.args, stdout=self.console_file, stderr=self.console_file, preexec_fn=os.setpgrp)
            
            runningServers.append(self)
            self.process_group_id = self.process.pid
            self.running = True
            
            # - start thread to tail output for needed info
            
            thread.start_new_thread(self.read_ports_from_log, ())

        except Exception:
            # `close()` won't be called because we haven't put ourself into
            #  `cluster.processes` yet, so we have to clean up manually
            for other_cluster in cluster.metacluster.clusters:
                if other_cluster is not cluster:
                    other_cluster._unblock_process(self)
            if self.cluster and self in self.cluster.processes:
                self.cluster.processes.remove(self)
            raise
    
    def __enter__(self):
        self.wait_until_started_up()
        return self
    
    def __exit__(self, type, value, traceback):
        # ToDo: handle non-normal exits
        self.close()
    
    def __wait_for_value(self, valueName):
        
        deadline = time.time() + self.startupTimeout
        while deadline > time.time():
            value = getattr(self, '_%s' % valueName)
            if value is not None:
                return value
            time.sleep(.1)
        else:
            raise RuntimeError('Timed out waiting for %s value' % valueName.replace('_', ' '))
    
    @property
    def cluster_port(self):
        return self.__wait_for_value('cluster_port')
    
    @property
    def driver_port(self):
        return self.__wait_for_value('driver_port')
        
    @property
    def http_port(self):
        return self.__wait_for_value('http_port')
    
    @property
    def name(self):
        return self.__wait_for_value('name')

    @property
    def uuid(self):
        return self.__wait_for_value('uuid')
        
    def wait_until_started_up(self, timeout=30):
        deadline = time.time() + timeout
        while deadline > time.time():
            if not self.ready:
                self.check()
                time.sleep(0.05)
            else:
                self.check()
                return
        else:
            raise RuntimeError("Timed out after waiting %d seconds for startup." % timeout)
    
    def read_ports_from_log(self, timeout=30):
        deadline = time.time() + timeout
        
        # - wait for the log file to appear
        
        while time.time() < deadline:
            #self.check()
            if os.path.isfile(self.logfile_path):
                break
            else:
                time.sleep(0.1)
        else:
            raise RuntimeError("Timed out after %d seconds waiting for the log file to appear at: %s" % (timeout, self.logfile_path))
        
        # - monitor the logfile for the given lines
        
        logLines = utils.nonblocking_readline(self.logfile_path)
        
        while time.time() < deadline:
            
            # - bail out if we have everything
            
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
                self.ready = True
                self._name = parsedLine.group('name')
                self._uuid = parsedLine.group('uuid')
        else:
            raise RuntimeError("Timeout while trying to read cluster port from log file")
    
    def check(self):
        """Throws an exception if the process has crashed or stopped. """
        assert self.process is not None
        if self.process.poll() is not None:
            raise RuntimeError("Process %s stopped unexpectedly with return code %d" % (self._name, self.process.poll()))

    def check_and_stop(self):
        """Asserts that the process is still running, and then shuts it down by
        sending `SIGINT`. Throws an exception if the exit code is nonzero. Also
        invalidates the `Process` object like `close()`. """
        
        global runningServers
        
        try:
            if self.running is True:
                assert self.process is not None
                self.check()
                self.process.send_signal(signal.SIGINT)
                start_time = time.time()
                grace_period = 300
                while time.time() < start_time + grace_period:
                    if self.process.poll() is not None:
                        break
                    time.sleep(1)
                else:
                    raise RuntimeError("Process %s failed to stop within %d seconds after SIGINT" % (self._name, grace_period))
                if self.process.poll() != 0:
                    raise RuntimeError("Process %s stopped unexpectedly with return code %d after SIGINT" % (self._name, self.process.poll()))
        finally:
            if self in runningServers:
                runningServers.remove(self)
            self.close()
    
    def kill(self):
        '''Suddenly terminate the process ungracefully'''
        
        assert self.process is not None
        assert self.check() is None, 'When asked to kill a process it was already stopped!'
        
        utils.kill_process_group(self.process_group_id, shutdown_grace=0)
        self.running = False
        self.close()
    
    def close(self):
        """Gracefully terminates the process (if possible), removes it from the cluster, and invalidates the `Process` object."""
        
        global runningServers
        
        if self.running is True:
            if self.process.poll() is None:
                utils.kill_process_group(self.process_group_id)
            
            if self in runningServers:
                runningServers.remove(self)
            
            if self._close_console_output:
                self.console_file.close()
        
        self.process = None
        self.running = False
        
        # `self.cluster` might be `None` if we crash in the middle of `move_processes()`
        if self.cluster is not None:
            for other_cluster in self.cluster.metacluster.clusters:
                if other_cluster is not self.cluster:
                    other_cluster._unblock_process(self)

            self.cluster.processes.remove(self)
            self.cluster = None
    
class Process(_Process):
    """A `Process` object represents a running RethinkDB server. It cannot be
    restarted; stop it and then create a new one instead. """

    def __init__(self, cluster=None, files=None, output_folder=None, console_output=None, executable_path=None, server_tags=None, command_prefix=None, extra_options=None, wait_until_ready=False):
        
        assert isinstance(cluster, (Cluster, None.__class__)), 'cluster must be a Cluster or None, got: %s' % repr(cluster)
        assert isinstance(files, (Files, str, None.__class__)), 'files must be a Files, string (name), or None, got: %s' % repr(cluster)
        assert output_folder is None or os.path.isdir(output_folder)
        
        # -- validate bad combinations
        
        if isinstance(files, Files):
            if output_folder is not None:
                raise ValueError('output_folder can not be provided alongside files')
            if server_tags is not None:
                raise ValueError('server_tags can not be provided alongside files')
        
        # -- validate/default cluster
        
        if cluster is None:
            if isinstance(files, Files):
                cluster = Cluster(metacluster=files.metacluster)
            else:
                cluster = Cluster(output_folder=output_folder)
        
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None
        
        # -- validate/default console_output/console_file
        
        moveConsoleFile = False
        if console_output is None:
            self.console_file = sys.stdout
        elif isinstance(console_output, bool):
            self._close_console_output = True
            if console_output is False:
                self.console_file = tempfile.NamedTemporaryFile(mode='w+')
            elif self.files is None:
                outerDir = cluster.metacluster.dbs_path
                if output_folder:
                    outerDir = output_folder
                self.console_file = tempfile.NamedTemporaryFile(mode='w+', dir=outerDir, delete=False)
                moveConsoleFile = True
            else:
                self.console_file = open(os.path.join(self.files.db_path, 'console.txt'), 'w+')
        elif hasattr(console_output, 'write'):
            self.console_file = console_output
        else:
            self._close_console_output = True
            self.console_file = open(console_output, "a")
        
        # -- default files
        
        if isinstance(files, (str, None.__class__)):
            self.console_file.write('========== Start Create Console ===========\n')
            self.console_file.flush()
            files = Files(metacluster=cluster.metacluster, server_name=files, server_tags=server_tags, db_containter=output_folder, console_output=self.console_file, executable_path=executable_path, command_prefix=command_prefix)
            self.console_file.write('=========== End Create Console ============\n\n')
            self.console_file.flush()
            if moveConsoleFile:
                os.rename(self.console_file.name, os.path.join(files.db_path, 'console.txt'))
            os.rename(os.path.join(files.db_path, 'log_file'), os.path.join(files.db_path, 'create_log_file'))
        assert isinstance(files, Files)
        
        # -- default command_prefix
        
        if command_prefix is None:
            command_prefix = []
        
        # -- default extra_options
        
        if extra_options is None:
            extra_options = ['--cache-size', '512']
        else:
            extra_options = copy.copy(extra_options)
            if not '--cache-size' in extra_options:
                extra_options += ['--cache-size', '512']
        
        # -- store values
        
        self.files = files
        self.logfile_path = os.path.join(files.db_path, "log_file")
        
        # -- run command
        
        options = ["serve", "--directory", self.files.db_path] + extra_options
        super(Process, self).__init__(cluster, options, executable_path=executable_path, command_prefix=command_prefix)
        
        # -- wait until ready (if requested)
        
        if wait_until_ready:
            self.wait_until_started_up()

class ProxyProcess(_Process):
    """A `ProxyProcess` object represents a running RethinkDB proxy. It cannot be
    restarted; stop it and then create a new one instead. """

    def __init__(self, cluster, logfile_path, console_output=None, executable_path=None, command_prefix=None, extra_options=None):
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None

        if command_prefix is None:
            command_prefix = []
        if extra_options is None:
            extra_options = []
        
        self.logfile_path = logfile_path

        options = ["proxy", "--log-file", self.logfile_path] + extra_options

        _Process.__init__(self, cluster, options, console_output=console_output, executable_path=executable_path, command_prefix=command_prefix)

if __name__ == "__main__":
    with Metacluster() as mc:
        c = Cluster(mc)
        f = Files(mc)
        p = Process(c, f)
        time.sleep(3)
        p.check_and_stop()
