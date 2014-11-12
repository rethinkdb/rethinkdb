#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time, pprint, fcntl
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils, scenario_common
from vcoptparse import *
r = utils.import_python_driver()

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

    print "Spinning up a process..."
    files = driver.Files(metacluster, db_path="db", server_name="the_server",
        console_output="create-output", command_prefix=command_prefix)
    process = driver.Process(cluster, files, console_output="log",
        command_prefix=command_prefix, extra_options=serve_options)
    process.wait_until_started_up()
    cluster.check()
    conn = r.connect(process.host, process.driver_port)
    server_uuid = r.db("rethinkdb").table("server_config").nth(0)["id"].run(conn)

    # When we need to force the server to try to write to its log file, this will do the
    # job; the server logs every name change.
    number = 1
    def make_server_write_to_log():
        number += 1
        res = r.db("rethinkdb").table("server_config").get(server_uuid) \
           .update({"name": "the_server_%d" % number}).run(conn)
        assert res["errors"] == 0 and res["replaced"] == 1, res

    print "Locking log file..."
    logfile_path = os.path.join(files.db_path, "log_file")
    logfile = open(logfile_path)
    fcntl.lockf(logfile.fileno(), fcntl.LOCK_SH)
    make_server_write_to_log()

    print "Checking for an issue..."
    issues = list(r.db("rethinkdb").table("issues").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 1
    assert issues[0]["type"] == "log_write_error"
    assert issues[0]["critical"] == False
    assert issues[0]["info"]["servers"] == ["the_server"]
    assert isinstance(issues[0]["info"]["message"], basestring)

    print "Checking issue with identifier_format='uuid'..."
    issues = list(r.db("rethinkdb").table("issues", identifier_format="uuid").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 1
    assert issues[0]["info"]["servers"] == [server_uuid]

    print "Unlocking the log file..."
    fcntl.lockf(logfile, fcntl.LOCK_UN)
    logfile.close()
    make_server_write_to_log()

    print "Checking that issue is gone..."
    issues = list(r.db("rethinkdb").table("issues").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 0

print "Done."
