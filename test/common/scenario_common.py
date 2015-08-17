# Copyright 2010-2014 RethinkDB, all rights reserved.

import random, shlex

import driver, utils, vcoptparse, workload_runner

def prepare_option_parser_mode_flags(opt_parser):
    opt_parser["valgrind"] = vcoptparse.BoolFlag("--valgrind")
    opt_parser["valgrind-options"] = vcoptparse.StringFlag("--valgrind-options", "--leak-check=full --track-origins=yes --child-silent-after-fork=yes")
    opt_parser["wrapper"] = vcoptparse.StringFlag("--wrapper", None)
    opt_parser["mode"] = vcoptparse.StringFlag("--mode", "")
    opt_parser["serve-flags"] = vcoptparse.StringFlag("--serve-flags", "")

def parse_mode_flags(parsed_opts):
    mode = parsed_opts["mode"]

    if parsed_opts["valgrind"]:
        assert parsed_opts["wrapper"] is None
        command_prefix = ["valgrind"]
        for valgrind_option in shlex.split(parsed_opts["valgrind-options"]):
            command_prefix.append(valgrind_option)

        # Make sure we use the valgrind build
        # this assumes that the 'valgrind' substring goes at the end of the specific build string
        if "valgrind" not in mode and mode != "":
            mode = mode + "-valgrind"

    elif parsed_opts["wrapper"] is not None:
        command_prefix = shlex.split(parsed_opts["wrapper"])

    else:
        command_prefix = []

    return utils.find_rethinkdb_executable(mode=mode), command_prefix, shlex.split(parsed_opts["serve-flags"])

def prepare_table_for_workload(http, **kwargs):
    db = http.add_database(name="test")
    return http.add_table(database=db, **kwargs)

def get_workload_ports(processes, tableName, dbName='test'):
    for process in processes:
        assert isinstance(process, (driver.Process, driver.ProxyProcess))
    process = random.choice(processes)
    return workload_runner.RDBPorts(
        host = process.host,
        http_port = process.http_port,
        rdb_port = process.driver_port,
        table_name = tableName,
        db_name = dbName)
