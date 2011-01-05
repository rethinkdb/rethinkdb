import shlex, sys, traceback, os, shutil, socket, subprocess, time, signal, threading, random
from vcoptparse import *
from corrupter import *
from rdbstat import *

class TestFailure(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class StdoutAsLog(object):
    def __init__(self, name, test_dir):
        self.name = name
        self.test_dir = test_dir
    def __enter__(self):
        self.path = self.test_dir.make_file(self.name)
        print "Writing log file %r." % self.path
        self.f = file(self.path, "w")
        sys.stdout = self.f
    def __exit__(self, x, y, z):
        sys.stdout = sys.__stdout__
        self.f.close()

def make_option_parser():
    o = OptParser()
    o["auto"] = BoolFlag("--auto")
    o["interactive"] = BoolFlag("--interactive")
    o["database"] = ChoiceFlag("--database", ["rethinkdb", "memcached"], "rethinkdb")
    o["mclib"] = ChoiceFlag("--mclib", ["memcache", "pylibmc"], "pylibmc")
    o["protocol"] = ChoiceFlag("--protocol", ["text", "binary"], "text")
    o["valgrind"] = BoolFlag("--no-valgrind", invert = True)
    o["valgrind-tool"] = StringFlag("--valgrind-tool", "memcheck")
    o["mode"] = StringFlag("--mode", "debug")
    o["netrecord"] = BoolFlag("--no-netrecord", invert = True)
    o["fsck"] = BoolFlag("--no-fsck", invert = True)
    o["restart_server_prob"] = FloatFlag("--restart-server-prob", 0)
    o["corruption_p"] = FloatFlag("--corruption-p", 0)
    o["cores"] = IntFlag("--cores", 2)
    o["slices"] = IntFlag("--slices", 3)
    o["memory"] = IntFlag("--memory", 100)
    o["duration"] = IntFlag("--duration", 10)
    o["serve-flags"] = StringFlag("--serve-flags", "")
    o["stress"] = StringFlag("--stress", "")
    o["sigint-timeout"] = IntFlag("--sigint-timeout", 60)
    o["no-timeout"] = BoolFlag("--no-timeout", invert = False)
    o["ssds"] = AllArgsAfterFlag("--ssds", default = [])
    o["mem-cap"] = IntFlag("--mem-cap", None)
    o["garbage-range"] = MultiValueFlag("--garbage-range", [float_converter, float_converter], default = None)
    return o

# Choose a random port at which to start searching to reduce the probability of collisions
# between different instances of the tests
current_port_to_check = random.randint(5000, 20000)
def find_unused_port():
    global current_port_to_check
    while True:
        current_port_to_check += 1
        s = socket.socket()
        try: s.bind(("", current_port_to_check))
        except socket.error, err:
            if "Address already in use" in str(err): continue
            else: raise
        else: return current_port_to_check
        finally: s.close()

def cmd_join(x):
    """The opposite of shlex.split()."""
    y = []
    for e in x:
        if " " not in e:
            y.append(e)
        else:
            y.append("\"%s\"" % e)
    return " ".join(y)

def signal_with_number(number):
    """Converts a numeric signal ID to a string like "SIGINT"."""
    for name in dir(signal):
        if name.startswith("SIG") and getattr(signal, name) == number:
            return name
    return str(number)

def wait_with_timeout(process, timeout):
    if timeout is None:
        return process.wait()
    else:
        wait_interval = 0.05
        n_intervals = int(timeout / wait_interval) + 1
        while process.poll() is None:
            n_intervals -= 1
            if n_intervals == 0: break
            time.sleep(wait_interval)
        return process.poll()

def run_executable(command_line, output_path, test_dir, timeout=60, valgrind_tool=None):
    summary = os.path.basename(command_line[0])
    if len(command_line) > 4: summary += " " + cmd_join(command_line[1:4]) + " [...]"
    else: summary += " " + cmd_join(command_line[1:])
    print "Running %r;" % summary,
    sys.stdout.flush()
    
    output_path = test_dir.make_file(output_path)
    
    if not os.path.exists(command_line[0]):
        raise RuntimeError("Bad executable: %r does not exist." % command_line[0])
    
    valgrind_error_code = 100
    if valgrind_tool is not None:
        cmd_line = ["valgrind",
            "--tool=%s" % valgrind_tool,
            "--error-exitcode=%d" % valgrind_error_code]
        if valgrind_tool == "memcheck": cmd_line.append("--leak-check=full")
        if valgrind_tool == "drd": cmd_line.append("--read-var-info=yes")
        command_line = cmd_line + command_line
    
    with file(output_path, "w") as output_file:
        sub = subprocess.Popen(command_line, stdout = output_file, stderr = subprocess.STDOUT)
        start_time = time.time()
        try:
            res = wait_with_timeout(sub, timeout)
            total_time = time.time() - start_time
        finally:
            try: sub.terminate()
            except: pass
    
    if res is None:
        assert timeout is not None
        raise RuntimeError("%r took more than %d seconds. Output is at %r." % \
            (" ".join(command_line), timeout, output_path))
    elif valgrind_tool is not None and res == valgrind_error_code:
        raise RuntimeError("Valgrind reported errors in %r. See %r." % \
            (" ".join(command_line), output_path))
    elif res > 0:
        raise RuntimeError("%r exited with exit status %d." % \
            (" ".join(command_line), res))
    elif res < 0:
        raise RuntimeError("%r exited with signal %s." % \
            (" ".join(command_line), signal_with_number(-res)))
    else:
        print "done after %f seconds." % total_time

def run_with_timeout(obj, test_dir, args = (), kwargs = {}, timeout = None, name = "the test"):
    print "Running %s..." % name
    
    result_holder = [None, None]
    kwargs['test_dir'] = test_dir
    def run():
        try:
            result = obj(*args, **kwargs)
        except Exception, e:
            result_holder[0] = "exception"
            result_holder[1] = sys.exc_info()
        else:
            result_holder[0] = "ok"
    thr = threading.Thread(target=run)
    thr.start()
    thr.join(timeout)

    if thr.isAlive():
        raise RuntimeError("%s timed out, probably because the timeout (%s) was set too short or the server " \
            "wasn't replying to queries fast enough." % (name.capitalize(), timeout))
    elif result_holder[0] == "exception":
        raise RuntimeError("There was an error in %s:\n" % name + \
            "".join(traceback.format_exception(*result_holder[1])).rstrip())
    elif result_holder[0] == "ok":
        print "%s succeeded." % name.capitalize()
    else:
        assert False

class NetRecord(object):
    def __init__(self, target_port, test_dir, name = "net"):
        nrc_log_dir = test_dir.make_file("netrecord/%s" % name.replace(" ", "_"))
        os.mkdir(nrc_log_dir)
        nrc_log_root = os.path.join(nrc_log_dir, "conn")
        print "Logging network traffic to %s_*." % nrc_log_root
        
        self.port = find_unused_port()
        try:
            self.process = subprocess.Popen(
                ["netrecord", "localhost", str(target_port), str(self.port), nrc_log_root],
                stdout = file("/dev/null", "w"), stderr = subprocess.STDOUT)
        except OSError, e:
            if "No such file or directory" in str(e):
                raise RuntimeError("Install netrecord. (It's in rethinkdb/scripts/netrecord/.)")
            else:
                raise
    
    def stop(self):
        assert self.process
        self.process.send_signal(signal.SIGINT)
        self.process.wait()
        self.process = None
    
    def __del__(self):
        if getattr(self, "process"): self.stop()

num_stress_clients = 0

def stress_client(test_dir, port=8080, host="localhost", workload={"gets":1, "inserts":1}, duration="10000q"):
    global num_stress_clients
    num_stress_clients += 1
    
    executable_path = os.path.join(os.path.dirname(__file__), "../../bench/stress-client/stress")
    if not os.path.exists(executable_path):
        raise ValueError("Looked for stress client at %r, didn't find it." % executable_path)
    
    command_line = [executable_path,
        "-s", "%s:%d" % (host, port),
        "-d", duration,
        "-w", "%d/%d/%d/%d/%d/%d/%d" % (workload.get("deletes", 0), workload.get("updates", 0),
            workload.get("inserts", 0), workload.get("gets", 0), workload.get("appends", 0),
            workload.get("prepends", 0), workload.get("verifies", 0)),
        ]
    
    key_file = test_dir.make_file("stress_client/keys")
    command_line.extend(["-o", key_file])
    if os.path.exists(key_file): command_line.extend(["-i", key_file])
    
    run_executable(command_line, "stress_client/output", timeout=None, test_dir=test_dir)

def rdb_stats(port=8080, host="localhost"):
    sock = socket.socket()
    sock.connect((host, port))
    sock.send("stats\r\n")
    
    buffer = ""
    while "END\r\n" not in buffer:
        buffer += sock.recv(4096)
    
    stats = {}
    for line in buffer.split("\r\n"):
        parts = line.split()
        if parts == ["END"]:
            break
        elif parts:
            assert parts[0] == "STAT"
            name = parts[1]
            value = " ".join(parts[2:])
            stats[name] = value
    else:
        raise ValueError("Didn't get an END");
    
    sock.shutdown(socket.SHUT_RDWR)
    sock.close()
    return stats

def get_executable_path(opts, name):
    executable_path = os.path.join(os.path.dirname(__file__), "../../build")
    executable_path = os.path.join(executable_path, opts["mode"])
    if opts["valgrind"]: executable_path += "-valgrind"
    executable_path = os.path.join(executable_path, name)
            
    if not os.path.exists(executable_path):
        raise ValueError(name + " has not been built; it should be at %r." % executable_path)

    return executable_path

class DataFiles(object):
    def __init__(self, opts, test_dir):
        self.opts = opts
        self.test_dir = test_dir
        
        if self.opts["ssds"]:
            self.files = [ssd.replace(' ','') for ssd in self.opts["ssds"]]
        else:
            db_data_dir = test_dir.p("db_data")
            if not os.path.isdir(db_data_dir): os.mkdir(db_data_dir)
            db_data_path = os.path.join(db_data_dir, "data_file")
            tries = 0
            while os.path.exists(db_data_path):
                tries += 1
                db_data_path = os.path.join(db_data_dir, "data_file_%d" % tries)
            self.files = [db_data_path]
        
        run_executable([
            get_executable_path(opts, "rethinkdb"), "create", "--force",
            "-s", str(self.opts["slices"]),
            "-c", str(self.opts["cores"]),
            ] + self.rethinkdb_flags(),
            "creator_output",
            timeout = 30,
            valgrind_tool = self.opts["valgrind-tool"] if self.opts["valgrind"] else None,
            test_dir = self.test_dir
            )
    
    def rethinkdb_flags(self):
        flags = []
        for file in self.files:
            flags.append("-f")
            flags.append(file)
        return flags
    
    def fsck(self):
        run_executable(
            [get_executable_path(self.opts, "rethinkdb"), "fsck"] + self.rethinkdb_flags(),
            "fsck_output.txt",
            timeout = 2000, #TODO this should be based on the size of the data file 4.6Gb takes about 30 minutes
            valgrind_tool = self.opts["valgrind-tool"] if self.opts["valgrind"] else None
            test_dir = self.test_dir
            )

class TestDir(object):
    def __init__(self, dir_name = "output_from_test"):
        self.name = dir_name
        if os.path.exists(dir_name):
            assert os.path.isdir(dir_name)
            shutil.rmtree(dir_name)
        os.mkdir(dir_name)

    def p(self, *path_elements):
        return os.path.join(self.name, *path_elements)

    def make_file(self, name):
        dirname = self.p(os.path.dirname(name))
        if not os.path.isdir(dirname): os.makedirs(dirname)
        basename = os.path.basename(name)
        tries = 0
        while os.path.exists(os.path.join(dirname, basename)):
            tries += 1
            basename = os.path.basename(name) + (".%d" % tries)
        return os.path.join(dirname, basename)

class Server(object):
    # Server should not take more than %(server_start_time)d seconds to start up
    server_start_time = 60
    
    # If server does not respond to SIGINT, give SIGquit and then, after %(server_sigquit_time)
    # seconds, send SIGKILL 
    server_sigquit_time = 15
    
    wait_interval = 0.25
    
    valgrind_error_code = 100
    
    def __init__(self, opts, test_dir, name = "server", extra_flags = [], data_files = None):
        self.running = False
        self.opts = opts
        self.base_name = name
        self.extra_flags = extra_flags
        self.test_dir = test_dir
        
        if self.opts["database"] == "rethinkdb":
            self.executable_path = get_executable_path(self.opts, "rethinkdb")
            if data_files is not None:
                self.data_files = data_files
            else:
                self.data_files = DataFiles(self.opts, test_dir=test_dir)
        
        self.times_started = 0
    
    def start(self):
        self.times_started += 1
        if self.times_started == 1: self.name = self.base_name
        else: self.name = self.base_name + " %d" % self.times_started
        print "Starting %s." % self.name
        
        # Make a file to log what the server prints
        output_path = self.test_dir.p("%s_output.txt" % self.name.replace(" ","_"))
        print "Redirecting output to %r." % output_path
        server_output = file(output_path, "w")
        
        server_port = find_unused_port()
        
        if self.opts["database"] == "rethinkdb":
            command_line = [self.executable_path,
                "-p", str(server_port),
                "-c", str(self.opts["cores"]),
                "-m", str(self.opts["memory"]),
                ] + self.data_files.rethinkdb_flags() + \
                shlex.split(self.opts["serve-flags"]) + \
                self.extra_flags
        
        elif self.opts["database"] == "memcached":
            command_line = ["memcached", "-p", str(server_port), "-M"]
        
        # Are we using valgrind?
        if self.opts["valgrind"]:
            cmd_line = ["valgrind"]
            if self.opts["valgrind-tool"]:
                self.valgrind_tool = self.opts["valgrind-tool"]
            if self.opts["interactive"]:
                cmd_line += ["--db-attach=yes"]
            cmd_line += \
                ["--tool=%s" % self.valgrind_tool,
                 "--error-exitcode=%d" % self.valgrind_error_code]
            if self.valgrind_tool == "memcheck":
                cmd_line.append("--leak-check=full")
                cmd_line.append("--track-origins=yes")
            if self.valgrind_tool == "drd":
                cmd_line.append("--read-var-info=yes")
            command_line = cmd_line + command_line
        
        # Start the server
        try:
            if self.opts["interactive"]:
                self.server = subprocess.Popen(command_line)
            else:
                self.server = subprocess.Popen(command_line,
                                               stdout = server_output, stderr = subprocess.STDOUT)
        except Exception as e:
            print command_line
            raise e
            
        self.running = True
        
        # Start netrecord if necessary
        
        if self.opts["netrecord"]:
            self.nrc = NetRecord(target_port=server_port, test_dir=self.test_dir, name=self.name)
            self.port = self.nrc.port
        else:
            self.port = server_port
        
        # Wait for server to start up
        
        waited = 0
        while waited < self.server_start_time:
            self.verify()
            s = socket.socket()
            try: s.connect(("localhost", server_port))
            except Exception, e:
                if "Connection refused" in str(e):
                    time.sleep(0.01)
                    waited += 0.01
                    continue
                else: raise
            else: break
            finally: s.close()
        else:
            raise RuntimeError("%s took longer than %.2f seconds to start." % \
                (self.name.capitalize(), self.server_start_time))
        print "%s took %.2f seconds to start." % (self.name.capitalize(), waited)
        time.sleep(0.2)
        
        # Make sure nothing went wrong during startup
        
        self.verify()
        
    def verify(self):
        assert self.running
        
        if self.server.poll() != None:
            self.running = False
            if self.server.poll() == 0:
                raise RuntimeError("%s shut down without being asked to." % self.name.capitalize())
            elif self.opts["valgrind"] and self.server.poll() == self.valgrind_error_code:
                raise RuntimeError("Valgrind reported errors in %s." % self.name)
            elif self.server.poll() > 0:
                raise RuntimeError("%s terminated unexpectedly with exit code %d" % \
                    (self.name.capitalize(), self.server.poll()))
            else:
                raise RuntimeError("%s terminated unexpectedly with signal %s" % \
                    (self.name.capitalize(), signal_with_number(-self.server.poll())))
    
    def shutdown(self):
        # Make sure the server didn't shut down without being asked to
        self.verify()
        
        assert self.running
        self.running = False
        
        if self.opts["interactive"]:
            # We're in interactive mode, probably in GDB. Don't kill it.
            return True
        
        # Shut down the server
        self.server.send_signal(signal.SIGINT)
        
        if self.opts["netrecord"]:
            self.nrc.stop()
        
        # Wait for the server to quit
        
        dead = wait_with_timeout(self.server, self.opts["sigint-timeout"]) is not None
        
        if not dead and self.opts["valgrind"]:
            # Sometimes valgrind loses track of the first SIGINT, especially if it receives it
            # right as it is starting up. Send a second SIGINT to make sure the server gets it.
            # This is stupid and we shouldn't have to do this.
            self.server.send_signal(signal.SIGINT)
            
            dead = wait_with_timeout(self.server, self.opts["sigint-timeout"]) is not None
        
        if not dead:
            self.server.send_signal(signal.SIGQUIT)
            
            dead = wait_with_timeout(self.server, self.server_sigquit_time) is not None
            
            if not dead:
                if self.opts["interactive"]:
                    # We're in interactive mode, probably in GDB. Don't kill it.
                    return
                self.server.send_signal(signal.SIGKILL)
                raise RuntimeError("%s did not shut down %d seconds after getting SIGINT, and " \
                    "did not respond within %d seconds of SIGQUIT either." % \
                    (self.name.capitalize(), self.opts["sigint-timeout"], self.server_sigquit_time))
            else:
                raise RuntimeError("%s did not shut down %d seconds after getting SIGINT, " \
                    "but responded to SIGQUIT." % (self.name.capitalize(), self.opts["sigint-timeout"]))
        
        # Make sure the server didn't encounter any problems
        if self.server.poll() != 0:
            if self.opts["valgrind"] and self.server.poll() == self.valgrind_error_code:
                raise RuntimeError("Valgrind reported errors in %s." % self.name)
            elif self.server.poll() > 0:
                raise RuntimeError("%s shut down with exit code %d." % \
                    (self.name.capitalize(), self.server.poll()))
            else:
                raise RuntimeError("%s shut down with signal %s." % \
                    (self.name.capitalize(), signal_with_number(-self.server.poll())))
        
        print "%s shut down successfully." % self.name.capitalize()
        
        if self.opts["fsck"]: self.data_files.fsck()
        
    def kill(self):
        if self.opts["interactive"]:
            # We're in interactive mode, probably in GDB. Don't kill it.
            return True
        
        # Make sure the server didn't shut down without being asked to
        self.verify()
        
        assert self.running
        self.running = False
        
        # Kill the server rudely
        self.server.send_signal(signal.SIGKILL)
        
        if self.opts["netrecord"]:
            self.nrc.stop()
        
        print "Killed %s." % self.name
        
        if self.opts["fsck"]: self.data_files.fsck()
    
    def __del__(self):
        # Avoid leaking processes in case there is an error in the test before we call end().
        if hasattr(self, "server") and self.server.poll() == None:
            self.server.send_signal(signal.SIGKILL)

class MemcachedWrapperThatRestartsServer(object):
    def __init__(self, opts, server, internal_mc_maker, test_dir):
        self.opts = opts
        self.server = server
        self.internal_mc_maker = internal_mc_maker
        self.internal_mc = self.internal_mc_maker()
        self.test_dir = test_dir
    def __getattr__(self, name):
        if random.random() < self.opts["restart_server_prob"]:
            self.do_restart()
        return getattr(self.internal_mc, name)
    def do_restart(self):
        self.internal_mc.disconnect_all()
        print "Interrupting test to restart server..."
        
        try:
            self.server.shutdown()
        finally:
            snapshot_dir = self.test_dir.p("db_data", self.server.name.replace(" ", "_"))
            print "Storing a snapshot of server data files in %r." % snapshot_dir
            os.mkdir(snapshot_dir)
            for fn in os.listdir(self.test_dir.p("db_data")):
                path = self.test_dir.p("db_data", fn)
                if os.path.isfile(path):
                    corrupt(path, self.opts["corruption_p"])
                    shutil.copyfile(path, os.path.join(snapshot_dir, fn))
        
        self.server.start()
        
        print "Done restarting server; now resuming test."
        self.internal_mc = self.internal_mc_maker()

def connect_to_port(opts, port):
    if opts["mclib"] == "pylibmc":
        import pylibmc
        mc = pylibmc.Client(["localhost:%d" % port], binary = (opts["protocol"] == "binary"))
        mc.behaviors["poll timeout"] = 10   # Seconds (I think)
    
    else:
        assert opts["protocol"] == "text"   # python-memcache does not support the binary protocol
        import rethinkdb_memcache as memcache
        mc = memcache.Client(["localhost:%d" % port])
    
    return mc

def connect_to_server(opts, server, test_dir):
    if opts.get("restart_server_prob", 0) > 0:
        mc_maker = lambda: connect_to_port(opts, server.port)
        mc = MemcachedWrapperThatRestartsServer(opts, server, mc_maker, test_dir)
    
    else:
        mc = connect_to_port(opts, server.port)
    
    return mc

def adjust_timeout(opts, timeout):
    if opts["interactive"] or opts["no-timeout"]:
        return None
    else:
        return timeout + 15
#make a dictionary of stat limits
def start_stats(opts, port):
    if not (opts["mem-cap"] or opts["garbage-range"]):
        return None

    limits = {}
    if opts["mem-cap"]:
        limits["memory_virtual[bytes]"] = StatRange(0, opts["mem-cap"] * 1024 * 1024)
    if opts["garbage-range"]:
        limits["serializer_garbage_ratio"] = StatRange(opts["garbage-range"][0], opts["garbage-range"][1])

    if limits:
        stat_checker = RDBStat(('localhost', port), limits)
        stat_checker.start()
        return stat_checker
    else:
        return None

def auto_server_test_main(test_function, opts, timeout = 30, extra_flags = [], test_dir=TestDir()):
    """Drop-in main() for tests that test against one server that they do not start up or shut
    down, but that they may open multiple connections against."""
    
    if opts["restart_server_prob"]:
        raise ValueError("--restart-server-prob is invalid for this test.")
    
    if opts["auto"]:
        server = Server(opts, extra_flags=extra_flags, test_dir=test_dir)
        server.start()

        stat_checker = start_stats(opts, server.port)

        try:
            run_with_timeout(test_function, args=(opts, server.port), timeout = adjust_timeout(opts, timeout), test_dir=test_dir)
        except Exception, e:
            test_failure = e
        else:
            test_failure = None

        if stat_checker:
            stat_checker.stop()

        server.shutdown()

        if test_failure: raise test_failure
    else:
        port = int(os.environ.get("RUN_PORT", "11211"))
        print "Assuming user has started a server on port %d." % port
        
        if opts["netrecord"]:
            nrc = NetRecord(target_port=port, test_dir=test_dir)
            port = nrc.port

        stat_checker = start_stats(opts, port)

        run_with_timeout(test_function, args=(opts, port), timeout = adjust_timeout(opts, timeout), test_dir=test_dir)

        if stat_checker:
            stat_checker.stop()
        
        if opts["netrecord"]:
            nrc.stop()

def simple_test_main(test_function, opts, timeout = 30, extra_flags = [], test_dir=TestDir()):
    """Drop-in main() for tests that open a single connection against a single server that they
    do not shut down or start up again."""
    try:
        if opts["auto"]:
            server = Server(opts, extra_flags=extra_flags, test_dir=test_dir)
            server.start()

            stat_checker = start_stats(opts, server.port)

            mc = connect_to_server(opts, server, test_dir=test_dir)
            try:
                run_with_timeout(test_function, args=(opts, mc), timeout = adjust_timeout(opts, timeout), test_dir=test_dir)
            except Exception, e:
                test_failure = e
            else:
                test_failure = None
            mc.disconnect_all()

            if stat_checker:
                stat_checker.stop()

            if server.running: server.shutdown()

            if test_failure: raise test_failure

        else:
            if opts["restart_server_prob"]:
                raise ValueError("--restart-server-prob is invalid without --auto.")
            
            port = int(os.environ.get("RUN_PORT", "11211"))
            print "Assuming user has started a server on port %d." % port
            
            if opts["netrecord"]:
                nrc = NetRecord(target_port=port, test_dir=test_dir)
                port = nrc.port
            
            stat_checker = start_stats(opts, port)
            mc = connect_to_port(opts, port)
            run_with_timeout(test_function, args=(opts, mc), timeout = adjust_timeout(opts, timeout), test_dir=test_dir)
            mc.disconnect_all()
            if stat_checker:
                stat_checker.stop()
            
            if opts["netrecord"]:
                nrc.stop()

    except ValueError, e:
        # Handle ValueError explicitly to give nicer error output
        print ""
        print "[FAILURE]Value Error: %s[/FAILURE]" % e
        sys.exit(1)
    
    sys.exit(0)

