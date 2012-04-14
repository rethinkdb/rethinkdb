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

def find_rethinkdb_executable(mode = "debug"):
    subpath = "build/%s/rethinkdb" % (mode)
    paths = [subpath, "../" + subpath, "../../" + subpath, "../../../" + subpath]
    for path in paths:
        if os.path.exists(path):
            return path
    raise RuntimeError("Can't find RethinkDB executable. Tried these paths: %s" % paths)

class Metacluster(object):
    """A `Metacluster` is a group of clusters. It's responsible for maintaining
    `resunder` blocks between different clusters. It's also a context manager
    that cleans up all the processes and deletes all the files. """

    def __init__(self, executable_path = find_rethinkdb_executable("debug")):
        assert os.access(executable_path, os.X_OK), "no such executable: %r" % executable_path

        self.clusters = set()
        self.dbs_dir = tempfile.mkdtemp()
        self.files_counter = 0
        self.executable_path = executable_path
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
        shutil.rmtree(self.dbs_dir)

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
        for process in processes:
            source._block_process(process)
        for process in processes:
            dest._unblock_process(process)
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

    def __init__(self, metacluster, name = None, port_offset = 0, log_path = "stdout"):
        assert isinstance(metacluster, Metacluster)
        assert not metacluster.closed
        assert name is None or isinstance(name, str)

        self.id_number = metacluster.files_counter
        metacluster.files_counter += 1
        self.db_dir = os.path.join(metacluster.dbs_dir, str(self.id_number))

        self.port_offset = self.id_number

        create_args = [metacluster.executable_path, "create", "--directory=" + self.db_dir, "--port-offset=" + str(self.port_offset)]
        if name is not None:
            create_args.append("--name=" + name)

        if log_path == "stdout":
            subprocess.check_call(create_args, stdout = sys.stdout, stderr = sys.stdout)
        else:
            with open(log_path, "w") as log_file:
                subprocess.check_call(create_args, stdout = log_file, stderr = subprocess.STDOUT)

class Process(object):
    """A `Process` object represents a running RethinkDB server. It cannot be
    restarted; stop it and then create a new one instead. """

    def __init__(self, cluster, files, log_path = "stdout"):
        assert isinstance(cluster, Cluster)
        assert cluster.metacluster is not None
        assert isinstance(files, Files)

        self.files = files

        self.cluster_port = cluster.metacluster.base_port + self.files.id_number * 2
        self.local_cluster_port = cluster.metacluster.base_port + self.files.id_number * 2 + 1
        self.http_port = self.cluster_port + 1000
        self.port_offset = self.files.port_offset

        for other_cluster in cluster.metacluster.clusters:
            if other_cluster is not cluster:
                other_cluster._block_process(self)

        try:
            self.args = [cluster.metacluster.executable_path, "serve",
                "--directory=" + self.files.db_dir,
                "--port=" + str(self.cluster_port),
                "--client-port=" + str(self.local_cluster_port)]
            for peer in cluster.processes:
                if peer is not self:
                    self.args.append("--join=" + socket.gethostname() + ":" + str(peer.cluster_port))

            self.log_path = log_path
            if self.log_path == "stdout":
                self.log_file = sys.stdout
            else:
                self.log_file = open(self.log_path, "w")

            self.process = subprocess.Popen(self.args, stdout = self.log_file, stderr = subprocess.STDOUT)

            self.cluster = cluster
            self.cluster.processes.add(self)
 
        except Exception, e:
            # `close()` won't be called because we haven't put ourself into
            # `cluster.processes` yet, so we have to clean up manually
            for other_cluster in cluster.metacluster.clusters:
                if other_cluster is not cluster:
                    other_cluster._unblock_process(self)
            raise

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

        self.log_file.close()

        for other_cluster in self.cluster.metacluster.clusters:
            if other_cluster is not self.cluster:
                other_cluster._unblock_process(self)
        
        self.cluster.processes.remove(self)
        self.cluster = None

if __name__ == "__main__":
    with Metacluster() as mc:
        c = Cluster(mc)
        f = Files(mc)
        p = Process(c, f)
        time.sleep(3)
        p.check_and_stop()

