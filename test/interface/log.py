#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time, pprint, datetime
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common, utils
from vcoptparse import *
r = utils.import_python_driver()

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)

    print "Spinning up a process..."
    files = driver.Files(metacluster, db_path="db", console_output="create-output",
        server_name="the_server", command_prefix=command_prefix)
    proc = driver.Process(cluster, files, console_output="serve-output", command_prefix=command_prefix, extra_options=serve_options)
    proc.wait_until_started_up()
    cluster.check()
    conn = r.connect(proc.host, proc.driver_port)

    print "Making a log entry..."
    res = r.db("rethinkdb").table("server_config").update({"tags": ["xyz"]}).run(conn)
    assert res["errors"] == 0 and res["replaced"] == 1, res

    print "Verifying the log entry..."
    log = list(r.db("rethinkdb").table("logs").order_by("timestamp").run(conn))
    assert len(log) > 5

    entry = log[-1]
    pprint.pprint(entry)
    assert "xyz" in entry["message"]
    assert entry["level"] == "info"
    assert 0 < entry["uptime"] < 120
    assert entry["server"] == "the_server"
    now = datetime.datetime.now(entry["timestamp"].tzinfo)
    assert entry["timestamp"] <= now
    assert entry["timestamp"] >= now - datetime.timedelta(minutes = 1)

    print "Verifying that point gets work..."
    for e in log:
        # Point gets use a separate code path, so we want to test them specifically
        e2 = r.db("rethinkdb").table("logs").get(e["id"]).run(conn)
        assert e == e2, (e, e2)

    cluster.check_and_stop()
print "Done."
