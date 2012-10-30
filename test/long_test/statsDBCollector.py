# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python
import sys, os, signal, math, time
import _mysql
from datetime import datetime

sys.path.append(os.path.abspath(os.path.join(os.path.join(os.path.dirname(__file__), ".."), 'common')))
from test_common import *


class StatsDBCollector(object):
    # Please note: Monitored stats must have an entry in the stat_names table!

    def read_monitored_stats(self):
        self.monitoring = []
        self.db_conn.query("SELECT `name` FROM `stat_names`")
        result = self.db_conn.use_result()
        rows = result.fetch_row(maxrows=0) # Fetch all rows
        for row in rows:
            self.monitoring.append(row[0])

    def insert_stat(self, timestamp, stat, value):
        timestamp = self.db_conn.escape_string(str(timestamp))
        stat = self.db_conn.escape_string(stat)
        value = self.db_conn.escape_string(str(value))
        self.db_conn.query("INSERT INTO `stats` VALUES ('%s', (SELECT `id` FROM `stat_names` WHERE `name` = '%s' LIMIT 1), '%s', '%d')" % (timestamp, stat, value, self.run_timestamp) )
        if self.db_conn.affected_rows() != 1:
            raise Exception("Cannot insert stat %s into database" % stat)

    def __init__(self, opts, server):
        def stats_aggregator(stats):
            ts = time.time()
            for k in self.monitoring:
                if k in stats:
                    self.insert_stat(ts, k, stats[k]) # TODO: Use timestamp from stats


        self.opts = opts

        self.run_timestamp = time.time()

        self.db_conn = _mysql.connect(self.opts['db_server'], self.opts['db_user'], self.opts['db_password'], self.opts['db_database'])
        self.read_monitored_stats()

        self.rdbstat = RDBStat(('localhost', server.port), interval=5, stats_callback=stats_aggregator)
        # For testing:
        #self.rdbstat = RDBStat(('electro', 8952), interval=5, stats_callback=stats_aggregator)
        self.rdbstat.start()

    def stop(self):
        self.rdbstat.stop()
        self.db_conn.close()


