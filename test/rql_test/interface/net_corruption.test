#!/usr/bin/env python
# Copyright 2012-2015 RethinkDB, all rights reserved.

from __future__ import print_function

import os, random, socket, sys, time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, vcoptparse, utils

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

def garbage(n):
    return "".join(chr(random.randint(0, 255)) for i in range(n))

utils.print_with_time("Starting first server")
with driver.Process(name='./db1', command_prefix=command_prefix, extra_options=serve_options) as server:
    server.check()
    
    utils.print_with_time("Generating garbage traffic")
    for i in range(30):
        print(i + 1, end=' ')
        sys.stdout.flush()
        s = socket.socket()
        s.connect((server.host, server.cluster_port))
        s.send(garbage(random.randint(0, 500)))
        time.sleep(3)
        s.close()
        server.check()
    utils.print_with_time("Cleaning up first server")

utils.print_with_time("Starting second server")
with driver.Process(name='./db2', command_prefix=command_prefix, extra_options=serve_options) as server:
    server.check()
    
    utils.print_with_time("Opening and holding a connection")
    s = socket.socket()
    s.connect((server.host, server.cluster_port))
    
    utils.print_with_time("Stopping the server")
    server.check_and_stop()
    s.close()
    
    utils.print_with_time("Cleaning up second server")
utils.print_with_time("Done.")

# TODO: Corrupt actual traffic between two processes instead of generating complete garbage. This might be tricky.
