#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

from __future__ import print_function

import datetime, pprint, os, sys, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

# --

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

# --

print("Starting a server (%.2fs)" % (time.time() - startTime))
with driver.Process(files="the_server", output_folder='.') as server:
    
    server.check()
    conn = r.connect(host=server.host, port=server.driver_port)
    
    print("Making a log entry (%.2fs)" % (time.time() - startTime))
    
    res = r.db("rethinkdb").table("server_config").update({"tags": ["xyz"]}).run(conn)
    assert res["errors"] == 0 and res["replaced"] == 1, res
    
    print("Verifying the log entry (%.2fs)" % (time.time() - startTime))
    
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
    
    print("Verifying that point gets work (%.2fs)" % (time.time() - startTime))
    
    for e in log:
        # Point gets use a separate code path, so we want to test them specifically
        e2 = r.db("rethinkdb").table("logs").get(e["id"]).run(conn)
        assert e == e2, (e, e2)

    print("Verifying that the table is not writable (%.2fs)" % (time.time() - startTime))
    res = r.db("rethinkdb").table("logs").limit(1).delete().run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("logs").insert({}).run(conn)
    assert res["errors"] == 1, res

    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done (%.2fs)" % (time.time() - startTime))
