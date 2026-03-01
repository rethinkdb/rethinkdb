# Copyright 2010-2015 RethinkDB, all rights reserved.

import os
import signal
import subprocess
import sys
import time

import utils, vcoptparse


class RDBPorts:
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


def normalize_ports(ports, db_name=None, table_name=None):
    if isinstance(ports, RDBPorts):
        if db_name is not None:
            ports.db_name = db_name
        if table_name is not None:
            ports.table_name = table_name
        return ports

    assert db_name is not None, 'db_name must be supplied when using a non-RDBPorts object'
    assert table_name is not None, 'table_name must be supplied when using a non-RDBPorts object'
    assert hasattr(ports, 'http_port'), f'Missing attribute http_port in ports: {ports}'
    assert hasattr(ports, 'driver_port'), f'Missing attribute driver_port in ports: {ports}'
    return RDBPorts(host=ports.host, http_port=ports.http_port,
                    rdb_port=ports.driver_port, db_name=db_name, table_name=table_name)


def run(command_line, ports, timeout, db_name=None, table_name=None):
    ports = normalize_ports(ports, db_name, table_name)
    utils.print_with_time(f"Running workload {command_line!r}...")
    new_environ = os.environ.copy()
    ports.add_to_environ(new_environ)
    proc = subprocess.Popen(
        command_line,
        shell=True,
        env=new_environ,
        preexec_fn=lambda: os.setpgid(0, 0),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    start_time = time.time()
    end_time = start_time + timeout

    try:
        while time.time() < end_time:
            result = proc.poll()
            if result is None:
                time.sleep(1)
                continue
            if result == 0:
                utils.print_with_time("Done")
                return
            stdout, stderr = proc.communicate()
            sys.stderr.write(
                f"Workload {command_line!r} failed with error code {result}\n"
                f"stdout:\n{stdout.decode(errors='ignore')}\n"
                f"stderr:\n{stderr.decode(errors='ignore')}\n"
            )
            sys.exit(result)
        sys.stderr.write(f"\nWorkload timed out after {timeout} seconds ({command_line})\n")
    finally:
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except OSError:
            pass
    sys.exit(1)


class ContinuousWorkload:
    def __init__(self, command_line, ports, db_name=None, table_name=None):
        self.command_line = command_line
        self.ports = normalize_ports(ports, db_name, table_name)
        self.running = False

    def __enter__(self):
        return self

    def start(self):
        if self.running:
            raise RuntimeError(f"Workload {self.command_line!r} is already running")
        utils.print_with_time(f"Starting workload {self.command_line!r}...")
        new_environ = os.environ.copy()
        self.ports.add_to_environ(new_environ)
        self.proc = subprocess.Popen(
            self.command_line,
            shell=True,
            env=new_environ,
            preexec_fn=os.setpgrp
        )
        self.running = True
        self.check()

    def check(self):
        if not self.running:
            raise RuntimeError("Workload is not running.")
        result = self.proc.poll()
        if result is not None:
            self.running = False
            raise RuntimeError(f"Workload {self.command_line!r} stopped prematurely with error code {result}")

    def stop(self):
        self.check()
        utils.print_with_time(f"Stopping {self.command_line!r}...")
        os.killpg(self.proc.pid, signal.SIGINT)
        shutdown_grace_period = 10
        end_time = time.time() + shutdown_grace_period

        while time.time() < end_time:
            result = self.proc.poll()
            if result is None:
                time.sleep(1)
                continue
            if result in (0, -signal.SIGINT):
                utils.print_with_time("OK")
                self.running = False
                return
            self.running = False
            raise RuntimeError(f"Workload {self.command_line!r} failed when interrupted with error code {result}")

        raise RuntimeError(
            f"Workload {self.command_line!r} failed to terminate within {shutdown_grace_period} seconds of SIGINT"
        )

    def __exit__(self, exc=None, ty=None, tb=None):
        if self.running:
            try:
                os.killpg(self.proc.pid, signal.SIGTERM)
            except OSError:
                pass


def prepare_option_parser_for_split_or_continuous_workload(op, allow_between=False):
    op["workload-during"] = vcoptparse.ValueFlag(
        "--workload-during",
        converter=str,
        default=[],
        combiner=vcoptparse.append_combiner
    )
    op["extra-before"] = vcoptparse.IntFlag("--extra-before", 10)
    op["extra-between"] = vcoptparse.IntFlag("--extra-between", 10)
    op["extra-after"] = vcoptparse.IntFlag("--extra-after", 10)
    op["workload-before"] = vcoptparse.StringFlag("--workload-before", None)
    op["timeout-before"] = vcoptparse.IntFlag("--timeout-before", 600)
    if allow_between:
        op["workload-between"] = vcoptparse.StringFlag("--workload-between", None)
        op["timeout-between"] = vcoptparse.IntFlag("--timeout-between", 600)
    op["workload-after"] = vcoptparse.StringFlag("--workload-after", None)
    op["timeout-after"] = vcoptparse.IntFlag("--timeout-after", 600)


class SplitOrContinuousWorkload:
    def __init__(self, opts, ports, db_name=None, table_name=None):
        self.opts = opts
        self.ports = normalize_ports(ports, db_name, table_name)

    def __enter__(self):
        self.continuous_workloads = [
            ContinuousWorkload(cl, self.ports).__enter__()
            for cl in self.opts["workload-during"]
        ]
        return self

    def _spin_continuous_workloads(self, seconds):
        if seconds:
            workloads_str = " and ".join(repr(x) for x in self.opts["workload-during"])
            utils.print_with_time(f"Letting {workloads_str} run for {seconds} seconds...")
            for _ in range(seconds):
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

    def __exit__(self, exc=None, ty=None, tb=None):
        if self.opts["workload-during"]:
            for cwl in self.continuous_workloads:
                cwl.__exit__()
