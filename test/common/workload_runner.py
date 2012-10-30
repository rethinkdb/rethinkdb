# Copyright 2010-2012 RethinkDB, all rights reserved.
import subprocess, os, time, string, signal
from vcoptparse import *

# TODO: Merge this file into `scenario_common.py`?

class MemcachedPorts(object):
    def __init__(self, host, http_port, memcached_port):
        self.host = host
        self.http_port = http_port
        self.memcached_port = memcached_port
    def add_to_environ(self, env):
        env["HOST"] = self.host
        env["HTTP_PORT"] = str(self.http_port)
        env["PORT"] = str(self.memcached_port)

class RDBPorts(object):
    def __init__(self, host, http_port, rdb_port, db_name, table_name):
        self.host = host
        self.http_port = http_port
        self.rdb_port = rdb_port
        self.db_name = db_name
        self.table_name = table_name
    def add_to_environ(self, env):
        env["HOST"] = self.host
        env["HTTP_PORT"] = str(self.http_port)
        env["PORT"] = str(self.rdb_port)
        env["DB_NAME"] = self.db_name
        env["TABLE_NAME"] = self.table_name

def run(protocol, command_line, ports, timeout):
    assert protocol in ["rdb", "memcached"]
    if protocol == "memcached":
        assert isinstance(ports, MemcachedPorts)
    else:
        assert isinstance(ports, RDBPorts)

    start_time = time.time()
    end_time = start_time + timeout
    print "Running %s workload %r..." % (protocol, command_line)

    # Set up environment
    new_environ = os.environ.copy()
    ports.add_to_environ(new_environ)

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
    def __init__(self, command_line, protocol, ports):
        assert protocol in ["rdb", "memcached"]
        if protocol == "memcached":
            assert isinstance(ports, MemcachedPorts)
        else:
            assert isinstance(ports, RDBPorts)
        self.command_line = command_line
        self.protocol = protocol
        self.ports = ports
        self.running = False

    def __enter__(self):
        return self

    def start(self):
        assert not self.running
        print "Starting %s workload %r..." % (self.protocol, self.command_line)

        # Set up environment
        new_environ = os.environ.copy()
        self.ports.add_to_environ(new_environ)

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
    # `--workload-during` specifies one or more workloads that will be run
    # continuously while other stuff is happening. `--extra-*` specifies how
    # many seconds to sit while the workloads are running.
    op["workload-during"] = ValueFlag("--workload-during", converter = str, default = [], combiner = append_combiner)
    op["extra-before"] = IntFlag("--extra-before", 10)
    op["extra-between"] = IntFlag("--extra-between", 10)
    op["extra-after"] = IntFlag("--extra-after", 10)

    # `--workload-*` specifies a workload to run at some point in the scenario.
    # `--timeout-*` specifies the timeout to enforce for `--workload-*`.
    op["workload-before"] = StringFlag("--workload-before", None)
    op["timeout-before"] = IntFlag("--timeout-before", 600)
    if allow_between:
        op["workload-between"] = StringFlag("--workload-between", None)
        op["timeout-between"] = IntFlag("--timeout-between", 600)
    op["workload-after"] = StringFlag("--workload-after", None)
    op["timeout-after"] = IntFlag("--timeout-after", 600)

class SplitOrContinuousWorkload(object):
    def __init__(self, opts, protocol, ports):
        self.opts, self.protocol, self.ports = opts, protocol, ports
    def __enter__(self):
        self.continuous_workloads = []
        for cl in self.opts["workload-during"]:
            cwl = ContinuousWorkload(cl, self.protocol, self.ports)
            cwl.__enter__()
            self.continuous_workloads.append(cwl)
        return self
    def _spin_continuous_workloads(self, seconds):
        assert self.opts["workload-during"]
        if seconds != 0:
            print "Letting %s run for %d seconds..." % \
                (" and ".join(repr(x) for x in self.opts["workload-during"]), seconds)
            for i in xrange(seconds):
                time.sleep(1)
                self.check()
    def run_before(self):
        if self.opts["workload-before"] is not None:
            run(self.protocol, self.opts["workload-before"], self.ports, self.opts["timeout-before"])
        if self.opts["workload-during"]:
            for cwl in self.continuous_workloads:
                cwl.start()
            self._spin_continuous_workloads(self.opts["extra-before"])
    def check(self):
        for cwl in self.continuous_workloads:
            cwl.check()
    def run_between(self):
        self.check()
        assert "workload-between" in self.opts, "pass allow_between=True to prepare_option_parser_for_split_or_continuous_workload()"
        if self.opts["workload-between"] is not None:
            run(self.protocol, self.opts["workload-between"], self.ports, self.opts["timeout-between"])
        if self.opts["workload-during"]:
            self._spin_continuous_workloads(self.opts["extra-between"])
    def run_after(self):
        if self.opts["workload-during"]:
            self._spin_continuous_workloads(self.opts["extra-after"])
            for cwl in self.continuous_workloads:
                cwl.stop()
        if self.opts["workload-after"] is not None:
            run(self.protocol, self.opts["workload-after"], self.ports, self.opts["timeout-after"])
    def __exit__(self, exc = None, ty = None, tb = None):
        if self.opts["workload-during"] is not None:
            for cwl in self.continuous_workloads:
                cwl.__exit__()
