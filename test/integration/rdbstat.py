from line import *
import socket
import time
import threading
import re

tick = 1 #how frequently to poll stats

class StatRange():
    def __init__(self, min, max):
        self.min = min
        self.max = max

    def __contains__(self, x):
        return (self.min <= x and x <= self.max)

class StatError(Exception):
    def __init__(self, stat, val, range):
        self.stat = stat
        self.val = val
        self.range = range

    def __str__(self):
        return "Stat: %s had value: %s outside of range [%s, %s]" % (self.stat, repr(self.val), repr(self.range[0]), repr(self.range[1]))


class RDBStat():
    int_line = line("^STAT\s+([\w\[\]]+)\s+(\d+|-)$", [('name', 's'), ('value', 'd')])
    flt_line = line("^STAT\s+([\w\[\]]+)\s+([\d\.]+|-)$", [('name', 's'), ('value', 'f')])
    end_line = line("END", [])

    def __init__(self, addrinfo, limits = None):
        self.socket = socket.socket()
        self.socket.connect(addrinfo)
        self.socket.setblocking(True)
        self.limits = limits

    def check(self, limits = None):
        if limits:
            self.limits = limits

        assert self.limits

        #send the stat request
        self.socket.send("stat\r\n")

        #recv the response
        data = ''
        while True:
            try:
                if re.search("END", data):
                    break
                data += self.socket.recv(1000)
            except:
                break

        assert data #make sure the server actually gave us back something

        data = data.splitlines()
        data.reverse()

        matches = take_while([self.int_line, self.flt_line], data)
        if not matches:
            return

        stat_dict = {}
        for stat in matches:
            stat_dict[stat["name"]] = stat["value"]

        for name, acceptable_range in self.limits.iteritems():
            if not stat_dict[name] in acceptable_range:
                import pdb
                pdb.set_trace()
                raise(StatError(name, stat_dict[name], acceptable_range))

    def run(self):
        self.keep_going = True
        while self.keep_going:
            self.check()
            time.sleep(tick)

    def start(self):
        self.thr = threading.Thread(target = self.run)
        self.thr.start()

    def stop(self):
        self.keep_going = False
        if self.thr:
            self.thr.join()
