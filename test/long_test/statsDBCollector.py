#!/usr/bin/python
import sys, os, signal, math, time
import _mysql
from datetime import datetime


class StatsDBCollector(object):
    monitoring = [
        'blocks_dirty[blocks]',
        'blocks_in_memory[blocks]',
        'cmd_get_avg',
        'cmd_set_avg',
        'cmd_get_total',
        'cmd_set_total',
        'io_writes_started[iowrites]',
        'io_reads_started[ioreads]',
        'io_writes_completed[iowrites]',
        'io_reads_completed[ioreads]',
        'cpu_combined_avg',
        'cpu_system_avg',
        'cpu_user_avg',
        'memory_faults_max',
        'memory_real[bytes]',
        'memory_virtual[bytes]',
        'serializer_bytes_in_use',
        'serializer_old_garbage_blocks',
        'serializer_old_total_blocks',
        'serializer_reads_total',
        'serializer_writes_total',
        'serializer_lba_gcs',
        'serializer_data_extents_gced[dexts]',
        'transactions_starting_avg',
        'uptime',
    ]
    
    def insert_stat(self, timestamp, stat, value)
        timestamp = db_conn.escape_string(timestamp)
        stat = self.db_conn.escape_string(stat)
        value = self.db_conn.escape_string(value)
        self.db_conn.query("INSERT INTO `stats` VALUES ('%s', '%s', '%s')" % (timestamp, stat, value) )
        # TODO: Check success!
        #if self.db_conn.affected_rows() != 1
            #...
    
    def __init__(self, opts, server):
        def stats_aggregator(stats):
            ts = time.time()
            for k in StatsDBCollector.monitoring:
                if k in stats:
                    insert_stat(ts, k, stats[k]) # TODO: Use timestamp from stats


        self.opts = opts
        
        self.db_conn = _mysql.connect("newton", "longtest", "rethinkdb2010", "longtest") # TODO

        self.rdbstat = RDBStat(('localhost', server.port), interval=1, stats_callback=stats_aggregator)
        self.rdbstat.start()

    def stop(self):
        self.rdbstat.stop()
        self.db_conn.close()
        

