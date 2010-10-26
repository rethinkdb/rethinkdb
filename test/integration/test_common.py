import shlex, sys, traceback, os, shutil, socket, subprocess, time, signal, threading, random
from vcoptparse import *

test_dir = "output_from_test"
made_test_dir = False

def make_test_dir():
    global made_test_dir
    made_test_dir = True
    if os.path.exists(test_dir):
        assert os.path.isdir(test_dir)
        shutil.rmtree(test_dir)
    os.mkdir(test_dir)

class StdoutAsLog(object):
    def __init__(self, name):
        self.name = name
    def __enter__(self):
        assert made_test_dir
        self.path = os.path.join(test_dir, self.name)
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
    o["mode"] = ChoiceFlag("--mode", ["debug", "release"], "debug")
    o["netrecord"] = BoolFlag("--no-netrecord", invert = True)
    o["restart_server_prob"] = FloatFlag("--restart-server-prob", 0)
    o["cores"] = IntFlag("--cores", 2)
    o["slices"] = IntFlag("--slices", 3)
    o["duration"] = IntFlag("--duration", 10)
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
    wait_interval = 0.05
    n_intervals = int(timeout / wait_interval) + 1
    while process.poll() is None:
        n_intervals -= 1
        if n_intervals == 0: break
        time.sleep(wait_interval)
    return process.poll()

def run_and_report(obj, args = (), kwargs = {}, timeout = None, name = "the test"):
    
    print "Running %s..." % name
    
    result_holder = [None, None]
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
        print "ERROR: %s timed out, probably because the timeout (%s) was set too short or the server " \
            "wasn't replying to queries fast enough." % (name.capitalize(), timeout)
        return False
    elif result_holder[0] == "exception":
        print "ERROR: There was an error in %s:" % name
        print "".join(traceback.format_exception(*result_holder[1])).rstrip()
        return False
    elif result_holder[0] == "ok":
        print "%s succeeded." % name.capitalize()
        return True
    else:
        assert False

class Server(object):
    
    # Special exit code that we pass to Valgrind to indicate an error
    valgrind_error_code = 100
    
    # Server should shut down within %(server_quit_time)d seconds of SIGINT
    server_sigint_time = 3
    
    # If server does not respond to SIGINT, give SIGTERM and then, after %(server_sigterm_time)
    # seconds, send SIGKILL 
    server_sigterm_time = 3
    
    wait_interval = 0.25
    
    def __init__(self, opts, name = "server", extra_flags = []):
        
        assert made_test_dir
        
        self.running = False
        self.opts = opts
        self.base_name = name
        self.extra_flags = extra_flags
        
        self.times_started = 0
    
    def start(self):
        
        self.times_started += 1
        if self.times_started == 1: self.name = self.base_name
        else: self.name = self.base_name + " %d" % self.times_started
        print "Starting %s." % self.name
        
        # Make a file to log what the server prints
        output_path = os.path.join(test_dir, "%s_output.txt" % self.name.replace(" ","_"))
        print "Redirecting output to %r." % output_path
        server_output = file(output_path, "w")
        
        server_port = find_unused_port()
        
        if self.opts["database"] == "rethinkdb":
            
            # Make a directory to hold server data files
            db_data_dir = os.path.join(test_dir, "db_data")
            if not os.path.isdir(db_data_dir): os.mkdir(db_data_dir)
            
            executable_path = os.path.join(os.path.dirname(__file__), "../../build")
            executable_path = os.path.join(executable_path, self.opts["mode"])
            if self.opts["valgrind"]: executable_path += "-valgrind"
            executable_path = os.path.join(executable_path, "rethinkdb")
            
            if not os.path.exists(executable_path):
                raise ValueError("rethinkdb has not been built; it should be at %r." %
                    executable_path);
            
            command_line = [executable_path,
                "-p", str(server_port),
                "-c", str(self.opts["cores"]),
                "-s", str(self.opts["slices"]),
                ] + self.extra_flags
            
            if self.times_started == 1:
                command_line.extend(["--create", "-f", os.path.join(db_data_dir, "data_file")])
            else:
                command_line.extend(["-f", os.path.join(db_data_dir, "data_file")])
        
        elif self.opts["database"] == "memcached":
            
            command_line = ["memcached", "-p", str(server_port), "-M"]
        
        # Are we using valgrind?
        if self.opts["valgrind"]:
            cmd_line = ["valgrind"]
            if self.opts["interactive"]:
                cmd_line += ["--db-attach=yes"]
            cmd_line += \
                ["--leak-check=full",
                 "--error-exitcode=%d" % self.valgrind_error_code]
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
            
            nrc_log_dir = os.path.join(test_dir, "network_logs")
            if not os.path.isdir(nrc_log_dir): os.mkdir(nrc_log_dir)
            nrc_log_root = os.path.join(nrc_log_dir, "%s_conn" % self.name.replace(" ","_"))
            print "Logging network traffic to %s_*." % nrc_log_root
            
            nrc_port = find_unused_port()
            try:
                self.nrc = subprocess.Popen(
                    ["netrecord", "localhost", str(server_port), str(nrc_port), nrc_log_root],
                    stdout = file("/dev/null", "w"), stderr = subprocess.STDOUT)
            except OSError, e:
                if "No such file or directory" in str(e):
                    raise ValueError("We don't have netrecord.")
                else:
                    raise
            
            self.port = nrc_port
            
        else:
            self.port = server_port
        
        # Wait for server to start up
        
        waited = 0
        limit = 5
        while waited < limit:
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
            print "%s took longer than %.2f seconds to start." % (self.name.capitalize(), limit)
            return False
        print "%s took %.2f seconds to start." % (self.name.capitalize(), waited)
        time.sleep(0.2)
        
        # Make sure nothing went wrong during startup
        
        return self.verify()
        
    def verify(self):
        
        assert self.running
        
        if self.server.poll() != None:
            self.running = False
            if self.server.poll() == 0:
                print "ERROR: %s shut down without being asked to." % self.name.capitalize()
            elif self.opts["valgrind"] and self.server.poll() == self.valgrind_error_code:
                print "ERROR: Valgrind reported errors in %s." % self.name
            elif self.server.poll() > 0:
                print "ERROR: %s terminated unexpectedly with exit code %d" % \
                    (self.name.capitalize(), self.server.poll())
            else:
                print "ERROR: %s terminated unexpectedly with signal %s" % \
                    (self.name.capitalize(), signal_with_number(-self.server.poll()))
            return False
        
        return True
    
    def shutdown(self):
        
        # Make sure the server didn't shut down without being asked to
        if not self.verify():
            return False
        
        assert self.running
        self.running = False
        
        if self.opts["interactive"]:
            # We're in interactive mode, probably in GDB. Don't kill it.
            return True
        
        # Shut down the server
        self.server.send_signal(signal.SIGINT)
        
        if self.opts["netrecord"]:
            self.nrc.send_signal(signal.SIGINT)
        
        # Wait for the server to quit
        
        dead = wait_with_timeout(self.server, self.server_sigint_time) is not None
        
        if not dead and self.opts["valgrind"]:
            
            # Sometimes valgrind loses track of the first SIGINT, especially if it receives it
            # right as it is starting up. Send a second SIGINT to make sure the server gets it.
            # This is stupid and we shouldn't have to do this.
            self.server.send_signal(signal.SIGINT)
            
            dead = wait_with_timeout(self.server, self.server_sigint_time) is not None
        
        if not dead:
            
            self.server.send_signal(signal.SIGQUIT)
            
            dead = wait_with_timeout(self.server, self.server_sigterm_time) is not None
            
            if not dead:
                if self.opts["interactive"]:
                    # We're in interactive mode, probably in GDB. Don't kill it.
                    return
                self.server.send_signal(signal.SIGKILL)
                print "ERROR: %s did not shut down %d seconds after getting SIGINT, and " \
                    "did not respond within %d seconds of SIGQUIT either." % \
                    (self.name.capitalize(), self.server_sigint_time, self.server_sigterm_time)
                return False
            else:
                print "ERROR: %s did not shut down %d seconds after getting SIGINT, " \
                    "but responded to SIGQUIT." % (self.name.capitalize(), self.server_sigint_time)
                return False
        
        # Make sure the server didn't encounter any problems
        if self.server.poll() != 0:
            if self.opts["valgrind"] and self.server.poll() == self.valgrind_error_code:
                print "ERROR: Valgrind reported errors in %s." % self.name
            elif self.server.poll() > 0:
                print "ERROR: %s shut down with exit code %d." % \
                    (self.name.capitalize(), self.server.poll())
            else:
                print "ERROR: %s shut down with signal %s." % \
                    (self.name.capitalize(), signal_with_number(-self.server.poll()))
            return False
        
        print "%s shut down successfully." % self.name.capitalize()
        return True
        
    def kill(self):
        
        if self.opts["interactive"]:
            # We're in interactive mode, probably in GDB. Don't kill it.
            return True
        
        # Make sure the server didn't shut down without being asked to
        if not self.verify():
            return False
        
        assert self.running
        self.running = False
        
        # Kill the server rudely
        self.server.send_signal(signal.SIGKILL)
        
        if self.opts["netrecord"]:
            self.nrc.send_signal(signal.SIGINT)
        
        print "Killed %s." % self.name
        return True
    
    def __del__(self):
        
        # Avoid leaking processes in case there is an error in the test before we call end().
        if hasattr(self, "server") and self.server.poll() == None:
            self.server.send_signal(signal.SIGKILL)
        
        if hasattr(self, "nrc") and self.nrc.poll() == None:
            self.nrc.send_signal(signal.SIGINT)

class MemcachedWrapperThatRestartsServer(object):
    def __init__(self, opts, server, internal_mc_maker):
        self.opts = opts
        self.server = server
        self.internal_mc_maker = internal_mc_maker
        self.internal_mc = self.internal_mc_maker()
    def __getattr__(self, name):
        if random.random() < self.opts["restart_server_prob"]:
            self.do_restart()
        return getattr(self.internal_mc, name)
    def do_restart(self):
        
        self.internal_mc.disconnect_all()
        print "Interrupting test to restart server..."
        
        shutdown_ok = self.server.shutdown()
        
        snapshot_dir = os.path.join(test_dir, "db_data", self.server.name.replace(" ", "_"))
        print "Storing a snapshot of server data files in %r." % snapshot_dir
        os.mkdir(snapshot_dir)
        for fn in os.listdir(os.path.join(test_dir, "db_data")):
            path = os.path.join(test_dir, "db_data", fn)
            if os.path.isfile(path):
                shutil.copyfile(path, os.path.join(snapshot_dir, fn))
        
        assert shutdown_ok
        
        assert self.server.start()
        
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

def connect_to_server(opts, server):
    
    if opts.get("restart_server_prob", 0) > 0:
        mc_maker = lambda: connect_to_port(opts, server.port)
        mc = MemcachedWrapperThatRestartsServer(opts, server, mc_maker)
    
    else:
        mc = connect_to_port(opts, server.port)
    
    return mc

def adjust_timeout(opts, timeout):
    if opts["interactive"]:
        return None
    else:
        return timeout

def auto_server_test_main(test_function, opts, timeout = 30, extra_flags = []):
    """Drop-in main() for tests that test against one server that they do not start up or shut
    down, but that they may open multiple connections against."""
    
    if opts["restart_server_prob"]:
        raise ValueError("--restart-server-prob is invalid for this test.")
    
    if opts["auto"]:
        
        make_test_dir()
        
        server = Server(opts, extra_flags = extra_flags)
        if not server.start(): sys.exit(1)
        test_ok = run_and_report(test_function, (opts, server.port), timeout = adjust_timeout(opts, timeout))
        server_ok = server.shutdown()
        if not (test_ok and server_ok): sys.exit(1)
    
    else:
        
        port = int(os.environ.get("RUN_PORT", "11211"))
        print "Assuming user has started a server on port %d." % port
        if not run_and_report(test_function, (opts, port), timeout=timeout): sys.exit(1)
    
    sys.exit(0)

def simple_test_main(test_function, opts, timeout = 30, extra_flags = []):
    """Drop-in main() for tests that open a single connection against a single server that they
    do not shut down or start up again."""
    if opts["auto"]:
        
        make_test_dir()
        
        server = Server(opts, extra_flags = extra_flags)
        if not server.start(): sys.exit(1)
        mc = connect_to_server(opts, server)
        test_ok = run_and_report(test_function, (opts, mc), timeout = adjust_timeout(opts, timeout))
        mc.disconnect_all()
        server_ok = server.shutdown()
        if not (test_ok and server_ok): sys.exit(1)
    
    else:
        
        if opts["restart_server_prob"]:
            raise ValueError("--restart-server-prob is invalid without --auto.")
        
        port = int(os.environ.get("RUN_PORT", "11211"))
        print "Assuming user has started a server on port %d." % port
        mc = connect_to_port(opts, port)
        ok = run_and_report(test_function, (opts, mc), timeout = timeout)
        mc.disconnect_all()
        if not ok: sys.exit(1)
    
    sys.exit(0)
    
