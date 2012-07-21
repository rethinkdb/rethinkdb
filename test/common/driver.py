import sys, os, time, socket, signal, subprocess, shutil, tempfile, random

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
    assert "resunder" in subprocess.check_output(["ps", "-A"])
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
        paths.append(os.path.join(os.environ["RETHINKDB"], subpath))
    for path in paths:
        if os.path.exists(path):
            return path
    raise RuntimeError("Can't find path %s.  Tried these paths: %s" % (subpath, paths))

def find_rethinkdb_executable(mode = "debug"):
    return find_subpath("build/%s/rethinkdb" % mode)

def get_namespace_host(port, processes):
    return 'localhost', port + random.choice(processes).port_offset

class Metacluster(object):
    """A `Metacluster` is a group of clusters. It's responsible for maintaining
    `resunder` blocks between different clusters. It's also a context manager
    that cleans up all the processes and deletes all the files. """

    def __init__(self):
        self.clusters = set()
        self.dbs_path = tempfile.mkdtemp()
        self.files_counter = 0
        self.closed = False
        try:
            self.base_port = int(os.environ["RETHINKDB_BASE_PORT"])
        except KeyError:
            self.base_port = random.randint(20000, 60000)

    def close(self):
        """Kills all processes and deletes all files. Also, makes the
        `Metacluster` object invalid. Call `close()` xor `__exit__()`, not
        both, because `__exit__()` calls `close()`. """
        assert not self.closed
        self.closed = True
        while self.clusters:
            iter(self.clusters).next().close()
        self.clusters = None
        shutil.rmtree(self.dbs_path)

    def __enter__(self):
        return self

    def __exit__(self, exc, etype, tb):
        self.close()

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

    def __init__(self, metacluster):
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
        nonzero exit code. Also makes the cluster object invalid, like
        `close()`. """
        try:
            while self.processes:
                iter(self.processes).next().check_and_stop()
        finally:
            self.close()

    def close(self):
        """Kills every process in the cluster (that is still running) and
        makes the `Cluster` object invalid."""
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

    def __init__(self, metacluster, machine_name = None, db_path = None, log_path = None, executable_path = None, command_prefix = []):
        assert isinstance(metacluster, Metacluster)
        assert not metacluster.closed
        assert machine_name is None or isinstance(machine_name, str)
        assert db_path is None or isinstance(db_path, str)

        if executable_path is None:
            executable_path = find_rethinkdb_executable("debug")
        assert os.access(executable_path, os.X_OK), "no such executable: %r" % executable_path

        self.id_number = metacluster.files_counter
        metacluster.files_counter += 1

        if db_path is None:
            self.db_path = os.path.join(metacluster.dbs_path, str(self.id_number))
        else:
            assert not os.path.exists(db_path)
            self.db_path = db_path

        if machine_name is None:
            self.machine_name = "node-%d" % self.id_number
        else:
            self.machine_name = machine_name

        create_args = command_prefix + [executable_path, "create",
            "--directory=" + self.db_path,
            "--name=" + self.machine_name]

        if log_path is None:
            log_path = "/dev/null"
        with open(log_path, "w") as log_file:
            subprocess.check_call(create_args, stdout = log_file, stderr = log_file)

class _Process(object):
    # Base class for Process & ProxyProcess. Do not instantiate directly.
    def __init__(self, cluster, options, log_path = None, executable_path = None, command_prefix = []):
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None
        assert all(hasattr(self, x) for x in
                   "cluster_port local_cluster_port http_port port_offset".split())

        if executable_path is None:
            executable_path = find_rethinkdb_executable("debug")
        assert os.access(executable_path, os.X_OK), "no such executable: %r" % executable_path

        for other_cluster in cluster.metacluster.clusters:
            if other_cluster is not cluster:
                other_cluster._block_process(self)

        try:
            self.args = command_prefix + [executable_path] + options
            for peer in cluster.processes:
                if peer is not self:
                    self.args.append("--join=" + socket.gethostname() + ":" + str(peer.cluster_port))

            self.log_path = log_path
            if self.log_path is None:
                self.log_file = sys.stdout
            else:
                self.log_file = open(self.log_path, "w")

            self.process = subprocess.Popen(self.args, stdout = self.log_file, stderr = self.log_file)

        except Exception, e:
            # `close()` won't be called because we haven't put ourself into
            #  `cluster.processes` yet, so we have to clean up manually
            for other_cluster in cluster.metacluster:
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
            grace_period = 15
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

    def __init__(self, cluster, files, log_path = None, executable_path = None, command_prefix = [], extra_options = []):
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None
        assert isinstance(files, Files)

        self.files = files

        self.cluster_port = cluster.metacluster.base_port + self.files.id_number * 2
        self.local_cluster_port = cluster.metacluster.base_port + self.files.id_number * 2 + 1
        self.http_port = self.cluster_port + 1000
        self.port_offset = self.files.id_number

        options = ["serve",
                   "--directory=" + self.files.db_path,
                   "--port=" + str(self.cluster_port),
                   "--port-offset=" + str(self.port_offset),
                   "--client-port=" + str(self.local_cluster_port)] + extra_options

        _Process.__init__(self, cluster, options,
            log_path=log_path, executable_path=executable_path, command_prefix=command_prefix)

class ProxyProcess(_Process):
    """A `ProxyProcess` object represents a running RethinkDB proxy. It cannot be
    restarted; stop it and then create a new one instead. """

    def __init__(self, cluster, logfile_path, log_path = None, executable_path = None, command_prefix = [], extra_options = []):
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None

        # We grab a value from the files counter even though we don't have files, just to ensure
        # uniqueness. TODO: rename it something more appropriate, like uid_counter.
        self.id_number = cluster.metacluster.files_counter
        cluster.metacluster.files_counter += 1

        self.logfile_path = logfile_path

        self.cluster_port = cluster.metacluster.base_port + self.id_number * 2
        self.local_cluster_port = cluster.metacluster.base_port + self.id_number * 2 + 1
        self.http_port = self.cluster_port + 1000
        self.port_offset = self.id_number

        options = ["proxy",
                   "--log-file=" + self.logfile_path,
                   "--port=" + str(self.cluster_port),
                   "--port-offset=" + str(self.port_offset),
                   "--client-port=" + str(self.local_cluster_port)] + extra_options

        _Process.__init__(self, cluster, options,
            log_path=log_path, executable_path=executable_path, command_prefix=command_prefix)

if __name__ == "__main__":
    with Metacluster() as mc:
        c = Cluster(mc)
        f = Files(mc)
        p = Process(c, f)
        time.sleep(3)
        p.check_and_stop()

