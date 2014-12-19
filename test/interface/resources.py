#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import os, sys, time, urllib2

startTime = time.time()

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, scenario_common, utils, vcoptparse

op = vcoptparse.OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
_, command_prefix, serve_options = scenario_common.parse_mode_flags(op.parse(sys.argv))

print("Spinning up a server (%.2fs)" % (time.time() - startTime))
with driver.Process(output_folder='.', command_prefix=command_prefix, extra_options=serve_options, wait_until_ready=True) as server:
    
    baseURL = 'http://%s:%d/' % (server.host, server.http_port)
    
    print("Getting root (%.2fs)" % (time.time() - startTime))
    
    fetchResult = urllib2.urlopen(baseURL, timeout=2)
    fetchData = fetchResult.read()
    assert fetchResult.getcode() == 200, 'Got a non 200 code when requesting the root: %s' % str(fetchResult.getcode())
    assert fetchResult.headers['content-type'] == 'text/html'
    assert '<html' in fetchData, 'Data from root did not include "html": %s' % fetchData
    
    print("Getting invalid page (%.2fs)" % (time.time() - startTime))
    
    # open a log file iterator and flush out the existing lines
    logFile = utils.nonblocking_readline(server.logfile_path)
    while next(logFile) is not None:
        pass
    
    try:
        fetchResult = urllib2.urlopen(os.path.join(baseURL, 'foobar'), timeout=2)
    except urllib2.HTTPError as e:
        assert e.code == 403, 'Got a non 403 code when requesting bad url /foobar: %s' % str(e.code)
    else:
        assert False, "Did not raise a 403 error code when requesting a bad url"
    
    print("Checking that the bad access was recorded (%.2fs)" % (time.time() - startTime))
    
    deadline = time.time() + 2
    foundIt = False
    while time.time() < deadline:
        thisEntry = next(logFile)
        while thisEntry is not None:
            if 'Someone asked for the nonwhitelisted file "/foobar"' in thisEntry:
                foundIt = True
                break
            thisEntry = next(logFile)
        if foundIt:
            break
        time.sleep(0.05)
    else:
        assert False, "Timed out waiting for the bad access marker to be written to the log"
    
    print("Getting ajax/me (%.2fs)" % (time.time() - startTime))
    
    fetchResult = urllib2.urlopen(os.path.join(baseURL, 'ajax/me'), timeout=2)
    fetchData = fetchResult.read()
    assert fetchResult.getcode() == 200, 'Got a non 200 code when requesting /me: %s' % str(fetchResult.getcode())
    assert fetchResult.headers['content-type'] == 'application/json'
    assert fetchData == '"%s"' % server.uuid, 'Data from ajax/me did not match the expected server uuid: %s vs %s' % (fetchData, server.uuid)
    
    # -- ending
    
    print("Cleaning up (%.2fs)" % (time.time() - startTime))
print("Done. (%.2fs)" % (time.time() - startTime))
