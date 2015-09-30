#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import os, random, sys, time

try:
    xrange
except NameError:
    xrange = range

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import rdb_workload_common, vcoptparse, utils

errorToleranceSecs = 5
batchSize = 100

op = rdb_workload_common.option_parser_for_connect()
op["num_rows"] = vcoptparse.IntFlag("--num-rows", 5000)
op["sequential"] = vcoptparse.BoolFlag("--sequential")
op["phase"] = vcoptparse.ChoiceFlag("--phase", ["w", "r", "wr"], "wr")
op["tolerate_errors"] = vcoptparse.BoolFlag("--tolerate-errors", invert=True)
opts = op.parse(sys.argv)

with rdb_workload_common.make_table_and_connection(opts) as (table, conn):    
    keys = None
    
    if "w" in opts["phase"]:
        utils.print_with_time("Inserting rows")
        
        # - generate the ids
        
        keys = []
        if opts["sequential"]:
            keys = xrange(opts["num_rows"])
        else:
            keys = [x for x in xrange(opts["num_rows"])]
            random.shuffle(keys)
        
        # - open key file if not in 'wr' mode (pwd is the test output folder)
        
        keys_file = None
        if "r" not in opts["phase"]:
            keys_file = open("keys", "w")
        
        # - insert keys
        
        currentBatch = []
        
        for key in keys:
            deadline = time.time() + errorToleranceSecs
            lastError = None
            response = None
            
            # - collect a batch
            currentBatch.append(key)
            
            # - process the batch
            if len(currentBatch) >= batchSize:
                while time.time() < deadline:
                    try:
                        response = table.insert([{'id': x, 'val': x} for x in currentBatch], durability="soft").run(conn)
                        break
                    except conn._r.ReqlError as e:
                        if opts["tolerate_errors"]:
                            lastError = e
                            time.sleep(.1)
                        else:
                            raise
                else:
                    if lastError:
                        raise lastError
            
                assert response['inserted'] == len(currentBatch), 'Batch insert failed: %r' % response
                
                if keys_file:
                    keys_file.write(''.join(['%r\n' % x for x in currentBatch]))
                
                utils.print_with_time('\tWrote keys %r to %r... %d records' % (currentBatch[0], currentBatch[-1], len(currentBatch)))
                currentBatch = []
        
        # - clean up
        
        response = table.sync().run(conn)
        assert response['synced'] == 1, "Failed to sync table: %r" % response
        if keys_file:
            keys_file.close()

    if "r" in opts["phase"]:
        
        if keys:
            utils.print_with_time("Checking records")
        else:
            utils.print_with_time("Checking records using ids from disk")
        
        currentBatch = []
        
        if keys is None:
            keys = open("keys", "r")
        
        def readAndCompare(keys, table, conn):
            '''fetch all of the records for keys and comapre their values'''
            
            if not keys:
                return
            
            # - check that all records look correct
            deadline = time.time() + errorToleranceSecs
            lastError = None
            while time.time() < deadline:
                try:
                    startKey = keys[0]
                    endKey = keys[-1]
                    keyCount = len(keys)
                    for row in table.get_all(*keys).run(conn):
                        assert row['id'] in keys, 'Unexpected id in fetched result: %r' % row
                        assert 'val' in row and row['id'] == row['id'] == row['val'], 'Val did not match id: %r' % row
                        keys.remove(row['id'])
                    assert keys == [], 'Database did not have all expected values, missing at least: %r' % keys
                    utils.print_with_time('\tVerified keys %r to %r... %d records' % (startKey, endKey, keyCount))
                    break
                except conn._r.ReqlError as e:
                    if opts["tolerate_errors"]:
                        lastError = e
                        time.sleep(.1)
                    else:
                        raise
            else:
                if lastError:
                    raise lastError
        
        for key in keys:
            
            # - eval to transform keys back into values
            if hasattr(keys, 'read'):
                try:
                    key = eval(key.strip())
                except Exception as e:
                    raise ValueError('Unusable key value: %r (%s)' % (key, str(e)))
            
            # - collect a batch
            currentBatch.append(key)
            
            # - process the batch
            if len(currentBatch) >= batchSize:
                readAndCompare(currentBatch, table, conn)
                currentBatch = []
        else:
            # - process the final batch
            readAndCompare(currentBatch, table, conn)
        if hasattr(keys, 'read'):
            keys.close()

    utils.print_with_time("many_keys success")
