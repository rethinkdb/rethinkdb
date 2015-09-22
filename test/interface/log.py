#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import datetime, pprint, os, sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

# --

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

r = utils.import_python_driver()

# --

utils.print_with_time("Starting a server")
with driver.Process(name="./the_server") as server:
    
    server.check()
    conn = r.connect(host=server.host, port=server.driver_port)
    
    utils.print_with_time("Making a log entry")
    
    res = r.db("rethinkdb").table("server_config").update({"name": "xyz"}).run(conn)
    assert res["errors"] == 0 and res["replaced"] == 1, res
    
    utils.print_with_time("Verifying the log entry")
    
    log = list(r.db("rethinkdb").table("logs").order_by("timestamp").run(conn))
    assert len(log) > 5

    entry = log[-1]
    assert "xyz" in entry["message"], pprint.pformat(entry)
    assert entry["level"] == "info", pprint.pformat(entry)
    assert 0 < entry["uptime"] < 120, pprint.pformat(entry)
    assert entry["server"] in ["the_server", "xyz"], pprint.pformat(entry)
    now = datetime.datetime.now(entry["timestamp"].tzinfo)
    assert entry["timestamp"] <= now, pprint.pformat(entry)
    assert entry["timestamp"] >= now - datetime.timedelta(minutes = 1), pprint.pformat(entry)
    
    utils.print_with_time("Verifying that point gets work")
    
    for e in log:
        # Point gets use a separate code path, so we want to test them specifically
        e2 = r.db("rethinkdb").table("logs").get(e["id"]).run(conn)
        assert e == e2, (e, e2)

    utils.print_with_time("Verifying that the table is not writable")
    res = r.db("rethinkdb").table("logs").limit(1).delete().run(conn)
    assert res["errors"] == 1, res
    res = r.db("rethinkdb").table("logs").insert({}).run(conn)
    assert res["errors"] == 1, res

    utils.print_with_time("Cleaning up")
utils.print_with_time("Done.")
