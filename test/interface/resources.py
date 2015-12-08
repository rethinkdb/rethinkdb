#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, sys, time, urllib2

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_unittest, utils

class ResourcesTestCase(rdb_unittest.RdbTestCase):
    
    @property
    def baseURL(self):
        return 'http://%s:%d/' % (self.cluster[0].host, self.cluster[0].http_port)
    
    def test_get_root(self):
        fetchResult = urllib2.urlopen(self.baseURL, timeout=2)
        fetchData = fetchResult.read()
        self.assertEqual(fetchResult.getcode(), 200, 'Got a non 200 code when requesting the root: %s' % str(fetchResult.getcode()))
        self.assertEqual(fetchResult.headers['content-type'], 'text/html')
        self.assertTrue('<html' in fetchData, 'Data from root did not include "html": %s' % fetchData)
    
    def test_get_invalid_page(self):
        
        # - open the log file, iterate through all current lines
        
        logFile = utils.nonblocking_readline(self.cluster[0].logfile_path)
        while next(logFile) is not None:
            pass
        
        # - make the bad access
        
        try:
            fetchResult = urllib2.urlopen(os.path.join(self.baseURL, 'foobar'), timeout=2)
        except urllib2.HTTPError as e:
            self.assertEqual(e.code, 403, 'Got a non 403 code when requesting bad url /foobar: %s' % str(e.code))
        else:
            self.fail("Did not raise a 403 error code when requesting a bad url")
        
        # - check that the bad access was recorded
    
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
            self.fail("Timed out waiting for the bad access marker to be written to the log")

rdb_unittest.main()
