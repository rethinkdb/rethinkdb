# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
from __future__ import division
import os, sys, socket, random
import time

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

extent_size = 8 * 1024 * 1024
block_size = 4096
lbaent_size = 16
lba_shard_factor = 16
live_data_extents = 8
lba_gc_threshold = 0.15
dblock_gc_threshold = 0.3

def summarize(stats):

    # There might be blocks in the cache that the serializer doesn't know about, but there's not
    # much that we can do about that. dblocks_live might exceed dblocks_total_max.
    dblocks_live = int(stats["blocks_total[blocks]"])
    # We don't know how full the data block extents are so we consider a full range of
    # possibilities. It would be fun to use the uncertainties module for this.
    dextents_max = int(stats["serializer_data_extents"])
    dextents_min = dextents_max - live_data_extents
    dblocks_total_max = dextents_max * extent_size / block_size
    dblocks_total_min = max(dextents_min * extent_size / block_size, dblocks_live)
    dblocks_dead_max = dblocks_total_max - dblocks_live
    dblocks_dead_min = dblocks_total_min - dblocks_live
    print "Data blocks: %d-%d total, %d live, %d-%d dead, %d%%-%d%% live" % \
        (dblocks_total_min, dblocks_total_max,
         dblocks_live,
         dblocks_dead_min, dblocks_dead_max,
         (dblocks_live/dblocks_total_max)*100, (dblocks_live/dblocks_total_min)*100)
    
    # Verify that LBA GC appears to be functioning
    
    lbaents_live = dblocks_live
    lbaextents_max = int(stats["serializer_lba_extents"])
    if lbaextents_max > lba_shard_factor: lbaextents_max -= lba_shard_factor   # Subtract the superblocks
    lbaextents_min = lbaextents_max - lba_shard_factor   # Incomplete extents
    lbaents_total_max = lbaextents_max * extent_size / lbaent_size
    lbaents_total_min = max(lbaextents_min * extent_size / lbaent_size, lbaents_live)
    lbaents_dead_max = lbaents_total_max - lbaents_live
    lbaents_dead_min = lbaents_total_min - lbaents_live
    print "LBA entries: %d-%d total, %d live, %d-%d dead, %d%%-%d%% live" % \
        (lbaents_total_min, lbaents_total_max,
         lbaents_live,
         lbaents_dead_min, lbaents_dead_max,
         (lbaents_live/lbaents_total_max)*100, (lbaents_live/lbaents_total_min)*100)
    print "LBA GCs: %d" % int(stats["serializer_lba_gcs"])
    
    assert (lbaents_live / lbaents_total_min) > (lba_gc_threshold - 0.05)
    assert (dblocks_live / dblocks_total_min) > (dblock_gc_threshold - 0.05)

def test_function(opts, port, test_dir):
    end_time = time.time() + opts["duration"]
    
    inserts = 0
    inserts_per_round = 100000
    while time.time() < end_time:
        print "The current time is", time.strftime("%H:%M:%S")
        
        stress_client(port=port, workload={"inserts":1}, duration="%dq" % inserts_per_round, test_dir=test_dir)
        inserts += inserts_per_round
        print "Have inserted %d keys total up to this point." % inserts
        
        print "Waiting for cache flush..."
        while True:
            stats = rdb_stats(port)
            if int(stats["blocks_dirty[blocks]"]) != 0:
                time.sleep(1)
            else:
                break
        
        summarize(stats)
        
        if int(stats["blocks_evicted"]) > 0:
            raise RuntimeError("Server has gone out of memory. (blocks_total=%d, "
                "blocks_in_memory=%d, blocks_evicted=%d) Aborting test because it would "
                "be very slow to continue. Try again with a smaller duration or a larger memory "
                "parameter (this time we were going %d inserts with a %dMB cache, and we went "
                "out of memory after %d inserts)" % (int(stats["blocks_total[blocks]"]),
                int(stats["blocks_in_memory[blocks]"]), int(stats["blocks_evicted"]),
                opts["duration"], opts["memory"], inserts))

if __name__ == "__main__":
    op = make_option_parser()
    op["duration"] = IntFlag("--duration", 1000000)
    op["memory"].default = 1024 * 40   # Lots and lots of cache space; we're testing the serializer here.
    opts = op.parse(sys.argv)
    assert not opts["valgrind"], "Running this test with valgrind build is not supported."
    opts["netrecord"] = False   # Nobody will want these; no need to type --no-netrecord, etc.
    auto_server_test_main(test_function, opts, timeout = opts["duration"] + 180)
