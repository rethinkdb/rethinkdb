#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import sys, os, time

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

print("Spinning up a process (%.2fs)" % (time.time() - startTime))
files = driver.Files(db_path="db", console_output="create-output", command_prefix=command_prefix)
server = driver.Process(files=files, console_output="serve-output", command_prefix=command_prefix, extra_options=serve_options)
server.wait_until_started_up()

# open a log file iterator and flush out the existing lines
logFile = utils.nonblocking_readline(server.logfile_path)
while next(logFile) is not None:
    pass

print("Establishing ReQL connection (%.2fs)" % (time.time() - startTime))
r = utils.import_python_driver()
conn = r.connect(host=server.host, port=server.driver_port)

num_entries = 100
print("Generating %d of log entries (%.2fs)" % (num_entries, time.time() - startTime))
for i in xrange(1, num_entries + 1):
    if i % 10 == 0 or i == 1:
        print(i, end='.. ')
        sys.stdout.flush()
    r.db('rethinkdb').table('server_config').get(server.uuid).update({'name':str(i)}).run(conn)
print()

# == Actual tests

deadline = time.time() + 20

# = ReQL admin

# ToDo: add in tests for ReQL admin solution for logs
raise NotImplementedError('Wating for new logs solution: https://github.com/rethinkdb/rethinkdb/issues/2884')

# = Log file

print("Testing that log file got the entries (%.2fs)" % (time.time() - startTime))
log_file_entries = 0
while time.time() < deadline:
    thisEntry = next(logFile)
    while thisEntry is not None:
        if "Changed server's name from" in thisEntry:
            log_file_entries += 1
        thisEntry = next(logFile)
    if log_file_entries == num_entries:
        break
    time.sleep(0.05)
assert log_file_entries == num_entries, 'Did not get the correct number of name change entries: %d vs %d' % (log_file_entries, num_entries)

# == close off the server

server.check_and_stop()
print("Done. (%.2fs)" % (time.time() - startTime))
