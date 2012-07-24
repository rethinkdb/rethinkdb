import subprocess, os, time, string, signal
from vcoptparse import *

class Ports(object):
    def __init__(self, host, http_port, memcached_port):
        self.host = host
        self.http_port = http_port
        self.memcached_port = memcached_port

def run(command_line, ports, timeout):
    start_time = time.time()
    end_time = start_time + timeout
    print "Running %r..." % command_line

    # Set up environment
    new_environ = os.environ.copy()
    new_environ["HOST"] = ports.host
    new_environ["HTTP_PORT"] = str(ports.http_port)
    new_environ["MC_PORT"] = new_environ["PORT"] = str(ports.memcached_port)

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
    def __init__(self, command_line, ports):
        self.command_line = command_line
        self.ports = ports
        self.running = False

    def __enter__(self):
        return self

    def start(self):
        assert not self.running
        print "Starting %r..." % self.command_line

        # Set up environment
        new_environ = os.environ.copy()
        new_environ["HOST"] = self.ports.host
        new_environ["HTTP_PORT"] = str(self.ports.http_port)
        new_environ["MC_PORT"] = new_environ["PORT"] = str(self.ports.memcached_port)

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
        os.killpg(self.proc.pid, signal.SIGINT)
        shutdown_grace_period = 10   # seconds
        end_time = time.time() + shutdown_grace_period
        while time.time() < end_time:
            result = self.proc.poll()
            if result is None:
                time.sleep(1)
            elif result == 0 or result == -signal.SIGINT:
                print "OK"
                self.running = False
                break
            else:
                self.running = False
                raise RuntimeError("workload '%s' failed when interrupted with error code %d" % (self.command_line, result))
        else:
            raise RuntimeError("workload '%s' failed to terminate within %d seconds of SIGINT" % (self.command_line, shutdown_grace_period))

    def __exit__(self, exc = None, ty = None, tb = None):
        if self.running:
            try:
                os.killpg(self.proc.pid, signal.SIGTERM)
            except OSError:
                pass

def prepare_option_parser_for_split_or_continuous_workload(op, allow_between = False):
    op["workload-during"] = ValueFlag("--workload-during", converter = str, default = [], combiner = append_combiner)
    op["extra-before"] = IntFlag("--extra-before", 10)
    op["extra-after"] = IntFlag("--extra-after", 10)
    op["workload-before"] = StringFlag("--workload-before", None)
    op["timeout-before"] = IntFlag("--timeout-before", 600)
    if allow_between:
        op["workload-between"] = StringFlag("--workload-between", None)
        op["timeout-between"] = IntFlag("--timeout-between", 600)
    op["workload-after"] = StringFlag("--workload-after", None)
    op["timeout-after"] = IntFlag("--timeout-after", 600)

class SplitOrContinuousWorkload(object):
    def __init__(self, opts, ports):
        self.opts, self.ports = opts, ports
    def __enter__(self):
        self.continuous_workloads = []
        for cl in self.opts["workload-during"]:
            cwl = ContinuousWorkload(cl, self.ports)
            cwl.__enter__()
            self.continuous_workloads.append(cwl)
        return self
    def run_before(self):
        if self.opts["workload-before"] is not None:
            run(self.opts["workload-before"], self.ports, self.opts["timeout-before"])
        if self.opts["workload-during"]:
            for cwl in self.continuous_workloads:
                cwl.start()
            if self.opts["extra-before"] != 0:
                print "Letting %s run for %d seconds..." % \
                    (" and ".join(repr(x) for x in self.opts["workload-during"]), self.opts["extra-before"])
                for i in xrange(self.opts["extra-before"]):
                    time.sleep(1)
                    self.check()
    def check(self):
        for cwl in self.continuous_workloads:
            cwl.check()
    def run_between(self):
        self.check()
        assert "workload-between" in self.opts, "pass allow_between=True to prepare_option_parser_for_split_or_continuous_workload()"
        if self.opts["workload-between"] is not None:
            run(self.opts["workload-between"], self.ports, self.opts["timeout-between"])
    def run_after(self):
        if self.opts["workload-during"]:
            if self.opts["extra-after"] != 0:
                print "Letting %s run for %d seconds..." % \
                    (" and ".join(repr(x) for x in self.opts["workload-during"]), self.opts["extra-after"])
                for i in xrange(self.opts["extra-after"]):
                    self.check()
                    time.sleep(1)
            for cwl in self.continuous_workloads:
                cwl.stop()
        if self.opts["workload-after"] is not None:
            run(self.opts["workload-after"], self.ports, self.opts["timeout-after"])
    def __exit__(self, exc = None, ty = None, tb = None):
        if self.opts["workload-during"] is not None:
            for cwl in self.continuous_workloads:
                cwl.__exit__()
