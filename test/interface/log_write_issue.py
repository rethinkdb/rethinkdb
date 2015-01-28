#!/usr/bin/env python
# Copyright 2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, pprint, resource, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils, scenario_common, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

with driver.Cluster(output_folder='.') as cluster:

    print("Creating process files (%.2fs)" % (time.time() - startTime))
    files = driver.Files(metacluster=cluster.metacluster, db_path="db", server_name="the_server", console_output="create-output", command_prefix=command_prefix)

    print("Setting resource limit (%.2fs)" % (time.time() - startTime))
    size_limit = 10 * 1024 * 1024
    resource.setrlimit(resource.RLIMIT_FSIZE, (size_limit, resource.RLIM_INFINITY))

    print("Spinning up server process (which will inherit resource limit) (%.2fs)" % (time.time() - startTime))
    process = driver.Process(cluster, files, console_output="log", extra_options=serve_options)
    conn = r.connect(process.host, process.driver_port)
    server_uuid = r.db("rethinkdb").table("server_config").nth(0)["id"].run(conn)

    print("Un-setting resource limit (%.2fs)" % (time.time() - startTime))
    resource.setrlimit(resource.RLIMIT_FSIZE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))

    print("Filling log file to %d bytes (%.2fs)" % (size_limit, time.time() - startTime))
    dummy_10k = ("X"*100 + "\n")*100
    with open(os.path.join(files.db_path, "log_file"), "a") as f:
        while f.tell() < size_limit:
            f.write(dummy_10k)

    # When we need to force the server to try to write to its log file, this will do the
    # job; the server logs every name change.
    def make_server_write_to_log():
        res = r.db("rethinkdb").table("server_config").get(server_uuid) \
           .update({"name": "the_server_2"}).run(conn)
        assert res["errors"] == 0 and res["replaced"] == 1, res
        res = r.db("rethinkdb").table("server_config").get(server_uuid) \
           .update({"name": "the_server"}).run(conn)
        assert res["errors"] == 0 and res["replaced"] == 1, res

    print("Making server try to write to log (%.2fs)" % (time.time() - startTime))
    make_server_write_to_log()

    print("Checking for an issue (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("current_issues").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 1
    assert issues[0]["type"] == "log_write_error"
    assert issues[0]["critical"] == False
    assert issues[0]["info"]["servers"] == ["the_server"]
    assert "File too large" in issues[0]["info"]["message"]

    print("Checking issue with identifier_format='uuid' (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("current_issues", identifier_format="uuid").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 1
    assert issues[0]["info"]["servers"] == [server_uuid]

    print("Making sure issues table is not writable (%.2fs)" % (time.time() - startTime))
    res = r.db("rethinkdb").table("current_issues").delete().run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("current_issues").update({"critical": True}).run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("current_issues").insert({}).run(conn)
    assert res["errors"] == 1, res

    print("Emptying log file (%.2fs)" % (time.time() - startTime))
    open(os.path.join(files.db_path, "log_file"), "w").close()

    print("Making server try to write to log (%.2fs)" % (time.time() - startTime))
    make_server_write_to_log()

    print("Checking that issue is gone (%.2fs)" % (time.time() - startTime))
    issues = list(r.db("rethinkdb").table("current_issues").run(conn))
    pprint.pprint(issues)
    assert len(issues) == 0
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
