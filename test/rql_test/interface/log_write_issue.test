#!/usr/bin/env python
# Copyright 2014-2016 RethinkDB, all rights reserved.

import os, pprint, resource, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils, scenario_common, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

utils.print_with_time("Setting resource limit")
size_limit = 10 * 1024 * 1024
resource.setrlimit(resource.RLIMIT_FSIZE, (size_limit, resource.RLIM_INFINITY))

with driver.Process(name='.', command_prefix=command_prefix, extra_options=serve_options) as process:
    
    conn = r.connect(process.host, process.driver_port)
    
    utils.print_with_time("Un-setting resource limit")
    resource.setrlimit(resource.RLIMIT_FSIZE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
    
    utils.print_with_time("log size: %d" % os.path.getsize(process.logfile_path))
    
    utils.print_with_time("Filling log file to %d bytes" % size_limit)
    with open(process.logfile_path, "a") as f:
        while f.tell() < size_limit:
            f.write("X" * min(100, (size_limit - f.tell()) - 1) + "\n")
    
    utils.print_with_time("log size: %d" % os.path.getsize(process.logfile_path))
    
    # Get the server to write over the limit by toggling the server name
    
    def make_server_write_to_log():
        res = r.db("rethinkdb").table("server_config").get(process.uuid).update({"name": "the_server_2"}).run(conn)
        assert res["errors"] == 0 and res["replaced"] == 1, res
        res = r.db("rethinkdb").table("server_config").get(process.uuid).update({"name": "the_server"}).run(conn)
        assert res["errors"] == 0 and res["replaced"] == 1, res

    utils.print_with_time("Making server try to write to log")
    make_server_write_to_log()

    utils.print_with_time("Checking for an issue")
    
    deadline = time.time() + 5
    last_error = None
    while time.time() < deadline:
        try:
            utils.print_with_time("log size: %d" % os.path.getsize(process.logfile_path))
            issues = list(r.db("rethinkdb").table("current_issues").filter(r.row["type"] != "memory_error").run(conn))
            assert len(issues) == 1, "Wrong number of issues. Should have been one, was: % s. Known on Macos: #3699" % pprint.pformat(issues)
            assert issues[0]["type"] == "log_write_error", pprint.pformat(issues)
            assert issues[0]["critical"] == False, pprint.pformat(issues)
            assert issues[0]["info"]["servers"] == ["the_server"], pprint.pformat(issues)
            assert "File too large" in issues[0]["info"]["message"], pprint.pformat(issues)
            break
        except AssertionError as e:
            last_error = e
    else:
        raise last_error

    utils.print_with_time("Checking issue with identifier_format='uuid'")
    issues = list(r.db("rethinkdb").table("current_issues", identifier_format="uuid").filter(r.row["type"] != "memory_error").run(conn))
    assert len(issues) == 1, pprint.pformat(issues)
    assert issues[0]["info"]["servers"] == [process.uuid], pprint.pformat(issues)

    utils.print_with_time("Making sure issues table is not writable")
    res = r.db("rethinkdb").table("current_issues").delete().run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("current_issues").update({"critical": True}).run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("current_issues").insert({}).run(conn)
    assert res["errors"] == 1, res

    utils.print_with_time("Emptying log file")
    open(process.logfile_path, "w").close()

    utils.print_with_time("Making server try to write to log")
    make_server_write_to_log()

    utils.print_with_time("Checking that issue is gone")
    
    deadline = time.time() + 5
    last_error = None
    while time.time() < deadline:
        try:
            issues = list(r.db("rethinkdb").table("current_issues").filter(r.row["type"] != "memory_error").run(conn))
            assert len(issues) == 0, pprint.pformat(issues)
            break
        except AssertionError as e:
            last_error = e
    else:
        raise last_error
    
    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
