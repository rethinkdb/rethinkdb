# Copyright 2010-2014 RethinkDB, all rights reserved.
import atexit, os, random, re, shutil, signal, socket, subprocess, sys, tempfile, time

import utils

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

def block_path(source_port, dest_port):
    if not ("resunder" in subprocess.check_output(["ps", "-A"])):
        print >> sys.stderr, '\nPlease start resunder process in test/common/resunder.py (as root)\n'
        assert False
    conn = socket.create_connection(("localhost", 46594))
    conn.sendall("block %s %s\n" % (str(source_port), str(dest_port)))
    # TODO: Wait for ack?
    conn.close()

def unblock_path(source_port, dest_port):
    assert "resunder" in subprocess.check_output(["ps", "-A"])
    conn = socket.create_connection(("localhost", 46594))
    conn.sendall("unblock %s %s\n" % (str(source_port), str(dest_port)))
    conn.close()

def find_subpath(subpath):
    paths = [subpath, "../" + subpath, "../../" + subpath, "../../../" + subpath]
    if "RETHINKDB" in os.environ:
        paths = [os.path.join(os.environ["RETHINKDB"], subpath)]
    for path in paths:
        if os.path.exists(path):
            return path
    raise RuntimeError("Can't find path %s.  Tried these paths: %s" % (subpath, paths))

def find_rethinkdb_executable(mode = ""):
    if mode == "":
        build_dir = os.getenv('RETHINKDB_BUILD_DIR')
        if build_dir:
            return os.path.join(build_dir, 'rethinkdb')
        else:
            mode = 'debug'
    return find_subpath("build/%s/rethinkdb" % mode)

def cleanupMetaclusterFolder(path):
    if os.path.isdir(str(path)):
        try:
            shutil.rmtree(path)
        except Exception, e:
            print('Warning: unable to cleanup Metacluster folder: %s - got error: %s' % (str(path), str(e)))

def get_table_host(processes):
    return ("localhost", random.choice(processes).driver_port)

class Metacluster(object):
    """A `Metacluster` is a group of clusters. It's responsible for maintaining
    `resunder` blocks between different clusters. It's also a context manager
    that cleans up all the processes and deletes all the files. """

    __unique_id_counter = None

    def __init__(self):
        self.clusters = set()
        self.dbs_path = tempfile.mkdtemp()
        atexit.register(cleanupMetaclusterFolder, self.dbs_path)
        self.__unique_id_counter = 0
        self.closed = False
        try:
            self.port_offset = int(os.environ["RETHINKDB_PORT_OFFSET"])
        except KeyError:
            self.port_offset = random.randint(0, 1800) * 10

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
    """A `Cluster` represents a group of `Processes` that are all connected to
    each other (ideally, anyway; see the note in `move_processes`). """

    def __init__(self, metacluster=None):

        if metacluster is None:
            metacluster = Metacluster()
        assert isinstance(metacluster, Metacluster)
        assert not metacluster.closed

        self.metacluster = metacluster
        self.metacluster.clusters.add(self)
        self.processes = set()

    def check(self):
        """Throws an exception if any of the processes in the cluster has
        stopped or crashed. """
        for proc in self.processes:
            proc.check()

    def check_and_stop(self):
        """First checks that each process in the cluster is still running, then
        stops them by sending SIGINT. Throws an exception if any exit with a
        nonzero exit code. Also makes the cluster object invalid """
        try:
            while self.processes:
                iter(self.processes).next().check_and_stop()
        finally:
            assert self.metacluster is not None
            while self.processes:
                iter(self.processes).next().close()
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

class Files(object):
    """A `Files` object is a RethinkDB data directory. Each `Process` needs a
    `Files`. To "restart" a server, create a `Files`, create a `Process`, stop
    the process, and then start a new `Process` on the same `Files`. """

    db_path = None
    machine_name = None
    
    def __init__(self, metacluster, machine_name = None, db_path = None, log_path = None, executable_path = None, command_prefix=None):
        assert isinstance(metacluster, Metacluster)
        assert not metacluster.closed
        assert machine_name is None or isinstance(machine_name, str)
        assert db_path is None or isinstance(db_path, str)

        if command_prefix is None:
            command_prefix = []

        if executable_path is None:
            executable_path = find_rethinkdb_executable()
        assert os.access(executable_path, os.X_OK), "no such executable: %r" % executable_path

        self.id_number = metacluster.get_new_unique_id()

        if db_path is None:
            self.db_path = os.path.join(metacluster.dbs_path, str(self.id_number))
        else:
            assert not os.path.exists(db_path)
            self.db_path = db_path

        if machine_name is None:
            self.machine_name = "node_%d" % self.id_number
        else:
            self.machine_name = machine_name

        create_args = command_prefix + [
            executable_path, "create",
            "--directory", self.db_path,
            "--machine-name", self.machine_name]

        if log_path is None:
            print "setting log_path to /dev/null."
            log_path = "/dev/null"
        with open(log_path, "a") as log_file:
            subprocess.check_call(create_args, stdout = log_file, stderr = log_file)

class _Process(object):
    # Base class for Process & ProxyProcess. Do not instantiate directly.

    cluster = None
    executable_path = None

    port_offset = None
    logfile_path = None

    host = 'localhost'
    cluster_port = None
    driver_port = None
    http_port = None
    
    def __init__(self, cluster, options, log_path = None, executable_path = None, command_prefix=None):
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None
        assert all(hasattr(self, x) for x in
                   "local_cluster_port port_offset logfile_path".split())

        if command_prefix is None:
            command_prefix = []

        if executable_path is None:
            executable_path = find_rethinkdb_executable()
        assert os.access(executable_path, os.X_OK), "no such executable: %r" % executable_path

        for other_cluster in cluster.metacluster.clusters:
            if other_cluster is not cluster:
                other_cluster._block_process(self)

        try:
            self.args = command_prefix + [executable_path] + options + ["--bind", "all"]
            for peer in cluster.processes:
                if peer is not self:
                    # TODO(OSX) Why did we ever use socket.gethostname() and not localhost?
                    # self.args.append("--join",  socket.gethostname() + ":" + str(peer.cluster_port))
                    self.args.append("--join")
                    self.args.append("localhost" + ":" + str(peer.cluster_port))

            self.log_path = log_path
            if self.log_path is None:
                self.log_file = sys.stdout
            else:
                self.log_file = open(self.log_path, "a")

            if os.path.exists(self.logfile_path):
                os.unlink(self.logfile_path)

            print "Launching:"
            print(self.args)
            self.process = subprocess.Popen(self.args, stdout = self.log_file, stderr = self.log_file)

            self.read_ports_from_log()

        except Exception, e:
            # `close()` won't be called because we haven't put ourself into
            #  `cluster.processes` yet, so we have to clean up manually
            for other_cluster in cluster.metacluster.clusters:
                if other_cluster is not cluster:
                    other_cluster._unblock_process(self)
            raise

        else:
            self.cluster = cluster
            self.cluster.processes.add(self)

    def wait_until_started_up(self, timeout = 30):
        time_limit = time.time() + timeout
        while time.time() < time_limit:
            self.check()
            s = socket.socket()
            try:
                s.connect(("localhost", self.http_port))
            except socket.error, e:
                time.sleep(1)
            else:
                break
            finally:
                s.close()
        else:
            raise RuntimeError("Process was not responding to HTTP traffic within %d seconds." % timeout)

    def read_ports_from_log(self, timeout = 30):
        time_limit = time.time() + timeout
        while time.time() < time_limit:
            self.check()
            try:
                log = open(self.logfile_path, 'r').read()
                cluster_ports = re.findall("(?<=Listening for intracluster connections on port )([0-9]+)", log)
                http_ports = re.findall("(?<=Listening for administrative HTTP connections on port )([0-9]+)", log)
                driver_ports = re.findall("(?<=Listening for client driver connections on port )([0-9]+)", log)
                if cluster_ports == [] or http_ports == []:
                    time.sleep(1)
                else:
                    self.cluster_port = int(cluster_ports[-1])
                    self.http_port = int(http_ports[-1])
                    self.driver_port = int(driver_ports[-1])
                    break
            except IOError, e:
                time.sleep(1)

        else:
            raise RuntimeError("Timeout while trying to read cluster port from log file")

    def check(self):
        """Throws an exception if the process has crashed or stopped. """
        assert self.process is not None
        if self.process.poll() is not None:
            raise RuntimeError("Process stopped unexpectedly with return code %d" % self.process.poll())

    def check_and_stop(self):
        """Asserts that the process is still running, and then shuts it down by
        sending `SIGINT`. Throws an exception if the exit code is nonzero. Also
        invalidates the `Process` object like `close()`. """
        assert self.process is not None
        try:
            self.check()
            self.process.send_signal(signal.SIGINT)
            start_time = time.time()
            grace_period = 300
            while time.time() < start_time + grace_period:
                if self.process.poll() is not None:
                    break
                time.sleep(1)
            else:
                raise RuntimeError("Process failed to stop within %d seconds after SIGINT" % grace_period)
            if self.process.poll() != 0:
                raise RuntimeError("Process stopped unexpectedly with return code %d after SIGINT" % self.process.poll())
        finally:
            self.close()

    def close(self):
        """Kills the process, removes it from the cluster, and invalidates the
        `Process` object. """
        if self.process.poll() is None:
            try:
                self.process.send_signal(signal.SIGKILL)
            except OSError:
                pass
        self.process = None

        if self.log_path is not None:
            self.log_file.close()

        # `self.cluster` might be `None` if we crash in the middle of
        # `move_processes()`.
        if self.cluster is not None:
            for other_cluster in self.cluster.metacluster.clusters:
                if other_cluster is not self.cluster:
                    other_cluster._unblock_process(self)

            self.cluster.processes.remove(self)
            self.cluster = None

class Process(_Process):
    """A `Process` object represents a running RethinkDB server. It cannot be
    restarted; stop it and then create a new one instead. """

    def __init__(self, cluster=None, files=None, log_path=None, executable_path=None, command_prefix=None, extra_options=None):

        if cluster is None:
            cluster = Cluster()
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None

        if files is None:
            files = Files(metacluster=cluster.metacluster, log_path=log_path, executable_path=executable_path, command_prefix=command_prefix)
        assert isinstance(files, Files)

        if command_prefix is None:
            command_prefix = []
        if extra_options is None:
            extra_options = []

        self.files = files
        self.logfile_path = os.path.join(files.db_path, "log_file")

        self.port_offset = cluster.metacluster.port_offset + self.files.id_number
        self.local_cluster_port = 29015 + self.port_offset

        options = ["serve",
                   "--directory", self.files.db_path,
                   "--port-offset", str(self.port_offset),
                   "--client-port", str(self.local_cluster_port),
                   "--cluster-port", "0",
                   "--driver-port", "0",
                   "--http-port", "0"
                   ] + extra_options

        _Process.__init__(
            self, cluster, options,
            log_path=log_path, executable_path=executable_path, command_prefix=command_prefix)

class ProxyProcess(_Process):
    """A `ProxyProcess` object represents a running RethinkDB proxy. It cannot be
    restarted; stop it and then create a new one instead. """

    def __init__(self, cluster, logfile_path, log_path = None, executable_path = None, command_prefix=None, extra_options=None):
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None

        if command_prefix is None:
            command_prefix = []
        if extra_options is None:
            extra_options = []

        # We grab a value from the files counter even though we don't have files, just to ensure
        # uniqueness. TODO: rename it something more appropriate, like uid_counter.
        self.id_number = cluster.metacluster.get_new_unique_id()

        self.logfile_path = logfile_path

        self.port_offset = cluster.metacluster.port_offset + self.id_number
        self.local_cluster_port = 28015 + self.port_offset

        options = ["proxy",
                   "--log-file", self.logfile_path,
                   "--port-offset", str(self.port_offset),
                   "--client-port", str(self.local_cluster_port)
                   ] + extra_options

        _Process.__init__(
            self, cluster, options,
            log_path=log_path, executable_path=executable_path, command_prefix=command_prefix)

if __name__ == "__main__":
    with Metacluster() as mc:
        c = Cluster(mc)
        f = Files(mc)
        p = Process(c, f)
        time.sleep(3)
        p.check_and_stop()
