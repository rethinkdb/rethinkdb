#!/usr/bin/env python
# Copyright 2012-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, random, socket, sys, time

startTime = time.time()

try:
    xrange
except NameError:
    xrange = range

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

def garbage(n):
    return "".join(chr(random.randint(0, 255)) for i in xrange(n))

print("Starting first server (%.2fs)" % (time.time() - startTime))
with driver.Process(files='db1', output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    server.check()
    
    print("Generating garbage traffic (%.2fs)" % (time.time() - startTime))
    for i in xrange(30):
        print(i + 1, end=' ')
        sys.stdout.flush()
        s = socket.socket()
        s.connect((server.host, server.cluster_port))
        s.send(garbage(random.randint(0, 500)))
        time.sleep(3)
        s.close()
        server.check()
    print("Cleaning up first server (%.2fs)" % (time.time() - startTime))

print("Starting second server (%.2fs)" % (time.time() - startTime))
with driver.Process(files='db2', output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    server.check()
    
    print("Opening and holding a connection (%.2fs)" % (time.time() - startTime))
    s = socket.socket()
    s.connect((server.host, server.cluster_port))
    
    print("Stopping the server (%.2fs)" % (time.time() - startTime))
    server.check_and_stop()
    s.close()
    
    print("Cleaning up second server (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))

# TODO: Corrupt actual traffic between two processes instead of generating
# complete garbage. This might be tricky.
