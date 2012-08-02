import shlex, random
from vcoptparse import *
import driver
import workload_runner

def prepare_option_parser_mode_flags(opt_parser):
    opt_parser["valgrind"] = BoolFlag("--valgrind")
    opt_parser["valgrind-options"] = StringFlag("--valgrind-options", "--leak-check=full --track-origins=yes --child-silent-after-fork=yes")
    opt_parser["mode"] = StringFlag("--mode", "debug")
    opt_parser["serve-flags"] = StringFlag("--serve-flags", "")

def parse_mode_flags(parsed_opts):
    mode = parsed_opts["mode"]
    command_prefix = [ ]

    if parsed_opts["valgrind"]:
        command_prefix.append("valgrind")
        for valgrind_option in shlex.split(parsed_opts["valgrind-options"]):
            command_prefix.append(valgrind_option)

        # Make sure we use the valgrind build
        # this assumes that the 'valgrind' substring goes at the end of the specific build string
        if "valgrind" not in mode:
            mode = mode + "-valgrind"

    return driver.find_rethinkdb_executable(mode), command_prefix, shlex.split(parsed_opts["serve-flags"])

def get_workload_ports(namespace_port, processes):
    for process in processes:
        assert isinstance(process, (driver.Process, driver.ProxyProcess))
    process = random.choice(processes)
    return workload_runner.Ports(
        host = "localhost",
        http_port = process.http_port,
        memcached_port = namespace_port + process.port_offset
        )