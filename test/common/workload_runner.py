# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import subprocess, os, time, string, signal, sys

import vcoptparse

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

def run(command_line, ports, timeout):
    assert isinstance(ports, RDBPorts)

    start_time = time.time()
    end_time = start_time + timeout
    print("Running workload %r..." % command_line)

    # Set up environment
    new_environ = os.environ.copy()
    ports.add_to_environ(new_environ)

    proc = subprocess.Popen(command_line, shell=True, env=new_environ, preexec_fn=lambda: os.setpgid(0, 0))

    try:
        while time.time() < end_time:
            result = proc.poll()
            if result is None:
                time.sleep(1)
            elif result == 0:
                print("Done (%d seconds)" % (time.time() - start_time))
                return
            else:
                print("Failed (%d seconds)" % (time.time() - start_time))
                sys.stderr.write("workload '%s' failed with error code %d\n" % (command_line, result))
                exit(1)
        sys.stderr.write("\nWorkload timed out after %d seconds (%s)\n"  % (time.time() - start_time, command_line))
    finally:
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except OSError:
            pass
    exit(1)

class ContinuousWorkload(object):
    def __init__(self, command_line, ports):
        assert isinstance(ports, RDBPorts)
        self.command_line = command_line
        self.ports = ports
        self.running = False

    def __enter__(self):
        return self

    def start(self):
        assert not self.running
        print("Starting workload %r..." % self.command_line)

        # Set up environment
        new_environ = os.environ.copy()
        self.ports.add_to_environ(new_environ)

        self.proc = subprocess.Popen(self.command_line, shell=True, env=new_environ, preexec_fn=lambda: os.setpgid(0, 0))

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
        print("Stopping %r..." % self.command_line)
        os.killpg(self.proc.pid, signal.SIGINT)
        shutdown_grace_period = 10   # seconds
        end_time = time.time() + shutdown_grace_period
        while time.time() < end_time:
            result = self.proc.poll()
            if result is None:
                time.sleep(1)
            elif result == 0 or result == -signal.SIGINT:
                print("OK")
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

def prepare_option_parser_for_split_or_continuous_workload(op, allow_between=False):
    # `--workload-during` specifies one or more workloads that will be run
    # continuously while other stuff is happening. `--extra-*` specifies how
    # many seconds to sit while the workloads are running.
    op["workload-during"] = vcoptparse.ValueFlag("--workload-during", converter=str, default=[], combiner=vcoptparse.append_combiner)
    op["extra-before"] = vcoptparse.IntFlag("--extra-before", 10)
    op["extra-between"] = vcoptparse.IntFlag("--extra-between", 10)
    op["extra-after"] = vcoptparse.IntFlag("--extra-after", 10)

    # `--workload-*` specifies a workload to run at some point in the scenario.
    # `--timeout-*` specifies the timeout to enforce for `--workload-*`.
    op["workload-before"] = vcoptparse.StringFlag("--workload-before", None)
    op["timeout-before"] = vcoptparse.IntFlag("--timeout-before", 600)
    if allow_between:
        op["workload-between"] = vcoptparse.StringFlag("--workload-between", None)
        op["timeout-between"] = vcoptparse.IntFlag("--timeout-between", 600)
    op["workload-after"] = vcoptparse.StringFlag("--workload-after", None)
    op["timeout-after"] = vcoptparse.IntFlag("--timeout-after", 600)

class SplitOrContinuousWorkload(object):
    def __init__(self, opts, ports):
        self.opts = opts
        self.ports = ports
    def __enter__(self):
        self.continuous_workloads = []
        for cl in self.opts["workload-during"]:
            cwl = ContinuousWorkload(cl, self.ports)
            cwl.__enter__()
            self.continuous_workloads.append(cwl)
        return self
    def _spin_continuous_workloads(self, seconds):
        assert self.opts["workload-during"]
        if seconds != 0:
            print("Letting %s run for %d seconds..." % (" and ".join(repr(x) for x in self.opts["workload-during"]), seconds))
            for i in xrange(seconds):
                time.sleep(1)
                self.check()
    def run_before(self):
        if self.opts["workload-before"] is not None:
            run(self.opts["workload-before"], self.ports, self.opts["timeout-before"])
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
            run(self.opts["workload-between"], self.ports, self.opts["timeout-between"])
        if self.opts["workload-during"]:
            self._spin_continuous_workloads(self.opts["extra-between"])
    def run_after(self):
        if self.opts["workload-during"]:
            self._spin_continuous_workloads(self.opts["extra-after"])
            for cwl in self.continuous_workloads:
                cwl.stop()
        if self.opts["workload-after"] is not None:
            run(self.opts["workload-after"], self.ports, self.opts["timeout-after"])
    def __exit__(self, exc = None, ty = None, tb = None):
        if self.opts["workload-during"] is not None:
            for cwl in self.continuous_workloads:
                cwl.__exit__()
