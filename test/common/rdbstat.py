# Copyright 2010-2012 RethinkDB, all rights reserved.
from line import *
import socket
import time
import threading
import re

class StatRange():
    def __init__(self, mini = None, maxi = None):
        self.min = mini
        self.max = maxi

    def __contains__(self, x):
        if self.min and self.min > x:
            return False
        if self.max and self.max <= x:
            return False
        return True

class StatError(Exception):
    def __init__(self, stat, val, ranje):
        self.stat = stat
        self.val = val
        self.range = ranje

    def __repr__(self):
        return "Stat: %s had value: %s outside of range [%s, %s]" % (self.stat, repr(self.val), repr(self.range[0]), repr(self.range[1]))


class RDBStat():
    int_line = line("^STAT\s+([\w\[\]]+)\s+(\d+|-)$", [('name', 's'), ('value', 'd')])
    flt_line = line("^STAT\s+([\w\[\]]+)\s+([\d\.]+|-)$", [('name', 's'), ('value', 'f')])
    str_line = line("^STAT\s+([\w\[\]]+)\s+(.+)$", [('name', 's'), ('value', 's')])
    end_line = line("END", [])

    def __init__(self, addrinfo, limits = {}, interval = 1, stats_callback=None):
        self.socket = socket.socket()
        self.socket.connect(addrinfo)
        self.socket.setblocking(True)
        self.limits = limits
        self.interval = interval
        self.stats_callback = stats_callback    # we pass a dictionary with stats whenever we get the new stats
        self.last_stats = {}
        self.limits = None
        self.thr = None

    def check(self, limits = None):
        if limits:
            self.limits = limits

        # assert self.limits

        #send the stat request
        self.socket.send("rdb stats\r\n")

        #recv the response
        data = ''
        while True:
            try:
                if re.search("END", data):
                    break
                new_data = self.socket.recv(1000)
                if len(new_data) == 0:
                    self.keep_going = False
                    return
                data += new_data
            except:
                self.keep_going = False
                return

        assert data #make sure the server actually gave us back something

        data = data.splitlines()
        data.reverse()

        matches = take_while([self.int_line, self.flt_line, self.str_line], data)
        if not matches:
            return

        stat_dict = {}
        for stat in matches:
            stat_dict[stat["name"]] = stat["value"]

        self.last_stats = stat_dict
        if self.stats_callback:
            self.stats_callback(stat_dict)

        for name, acceptable_range in self.limits.iteritems():
            if not stat_dict[name] in acceptable_range:
                raise(StatError(name, stat_dict[name], acceptable_range))

    def run(self):
        self.keep_going = True
        time.sleep(5)
        try:
            while self.keep_going:
                self.check()
                time.sleep(self.interval)
        finally:
            self.keep_going = False
            try: self.socket.close()
            except: pass

    def start(self):
        self.thr = threading.Thread(target = self.run)
        self.thr.start()

    def stop(self):
        self.keep_going = False
        if self.thr and self.thr.is_alive():
            self.thr.join()
