import subprocess, os, time, string, signal
from vcoptparse import *

def run(command_line, host, port, timeout):
    start_time = time.time()
    end_time = start_time + timeout
    print "Running %r..." % command_line

    # Set up environment
    new_environ = os.environ.copy()
    new_environ["HOST"] = host
    new_environ["PORT"] = str(port)

    proc = subprocess.Popen(command_line, shell = True, env = new_environ, preexec_fn = lambda: os.setpgid(0, 0))

    try:
        while time.time() < end_time:
            result = proc.poll()
            if result is None:
                time.sleep(1)
            elif result == 0:
                print "Done (%d seconds)" % (time.time() - start_time)
                return
            else:
                print "Failed (%d seconds)" % (time.time() - start_time)
                raise RuntimeError("workload '%s' failed with error code %d" % (command_line, result))
        print "Timed out (%d seconds)" % (time.time() - start_time)
    finally:
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except OSError:
            pass
    raise RuntimeError("workload timed out before completion")

class ContinuousWorkload(object):
    def __init__(self, command_line, host, port):
        self.command_line = command_line
        self.host = host
        self.port = port
        self.running = False

    def __enter__(self):
        return self

    def start(self):
        assert not self.running
        print "Starting %r..." % self.command_line

        # Set up environment
        new_environ = os.environ.copy()
        new_environ["HOST"] = self.host
        new_environ["PORT"] = str(self.port)

        self.proc = subprocess.Popen(self.command_line, shell = True, env = new_environ, preexec_fn = lambda: os.setpgid(0, 0))

        self.running = True

        self.check()

    def check(self):
        assert self.running
        result = self.proc.poll()
        if result is not None:
            self.running = False
            raise RuntimeError("workload '%s' stopped prematurely with error code %d" % (self.command_line, result))

    def stop(self):
        self.check()
        print "Stopping %r..." % self.command_line
        self.proc.send_signal(signal.SIGTERM)
        shutdown_grace_period = 10   # seconds
        end_time = time.time() + shutdown_grace_period
        while time.time() < end_time:
            result = self.proc.poll()
            if result is None:
                time.sleep(1)
            elif result == 0 or result == -signal.SIGTERM:
                print "OK"
                self.running = False
                break
            else:
                self.running = False
                raise RuntimeError("workload '%s' failed when interrupted with error code %d" % (self.command_line, result))
        else:
            raise RuntimeError("workload '%s' failed to terminate within %d seconds of SIGTERM" % (self.command_line, shutdown_grace_period))

    def __exit__(self, exc = None, ty = None, tb = None):
        if self.running:
            try:
                os.killpg(self.proc.pid, signal.SIGTERM)
            except OSError:
                pass

# A lot of scenarios work either with a two-phase split workload or a continuous
# workload. This code factors out the details of parsing and handling that. The
# syntax for invoking such scenarios is as follows:
#     scenario.py --split-workload 'workload1.py $HOST:$PORT' 'workload2.py $HOST:$PORT' [--timeout timeout]
#     scenario.py --continuous-workload 'workload.py $HOST:$PORT'

def prepare_option_parser_for_split_or_continuous_workload(op):
    op["workload-type"] = ChoiceFlags(["--split-workload", "--continuous-workload"])
    op["workloads"] = ManyPositionalArgs()
    op["timeout"] = IntFlag("--timeout", 60)

class SplitOrContinuousWorkload(object):
    def __init__(self, opts, host, port):
        self.type = opts["workload-type"]
        assert self.type in ["split-workload", "continuous-workload"]
        if self.type == "continuous-workload":
            if len(opts["workloads"]) < 1:
                raise ValueError("You must specify the workload on the command line.")
            if len(opts["workloads"]) > 1:
                raise ValueError("If you specify --continuous-workload, you should only specify one workload on the command line.")
            self.workload, = opts["workloads"]
        else:
            if len(opts["workloads"]) != 2:
                raise ValueError("If you specify --split-workload, you should specify two workloads on the command line.")
            self.workload1, self.workload2 = opts["workloads"]
        self.timeout = opts["timeout"]
        self.host, self.port = host, port
    def __enter__(self):
        if self.type == "continuous-workload":
            self.continuous_workload = ContinuousWorkload(self.workload, self.host, self.port)
            self.continuous_workload.__enter__()
        return self
    def step1(self):
        if self.type == "continuous-workload":
            self.continuous_workload.start()
            duration = 10
            print "Letting %r run for %d seconds..." % (self.workload, duration)
            time.sleep(duration)
            self.continuous_workload.check()
        else:
            run(self.workload1, self.host, self.port, self.timeout)
    def check(self):
        if self.type == "continuous-workload":
            self.continuous_workload.check()
    def step2(self):
        if self.type == "continuous-workload":
            self.continuous_workload.check()
            duration = 10
            print "Letting %r run for %d seconds..." % (self.workload, duration)
            time.sleep(duration)
            self.continuous_workload.stop()
        else:
            run(self.workload2, self.host, self.port, self.timeout)
    def __exit__(self, exc = None, ty = None, tb = None):
        if self.type == "continuous-workload":
            self.continuous_workload.__exit__()
