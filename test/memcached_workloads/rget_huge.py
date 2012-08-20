#!/usr/bin/python
import os, sys, random, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
import memcached_workload_common
from line import *
from vcoptparse import *

def sock_readline(sock_file):
    ls = []
    while True:
        l = sock_file.readline()
        ls.append(l)
        if len(l) >= 2 and l[-2:] == '\r\n':
            break
    return ''.join(ls)

def expect_line(sock_file, expected_line):
    actual_line = sock_readline(sock_file)
    if actual_line != expected_line:
        raise ValueError("Expected %r; got %r" % (expected_line, actual_line))

op = memcached_workload_common.option_parser_for_socket()
op["count"] = IntFlag("--count", 100000)
opts = op.parse(sys.argv)

print_interval = 1
while print_interval * 100 < opts["count"]:
    print_interval *= 10

alphabet = "abcdefghijklmnopqrstuvwxyz"
pairs = []
for i in range(0, opts["count"]):
    key = random.choice(alphabet) + random.choice(alphabet) + random.choice(alphabet) + str(i)
    value = 'x' * (50 + 500 * i % 2)
    pairs.append((key, value))

with memcached_workload_common.make_socket_connection(opts) as s:
    f = s.makefile()
    print "Creating test data"
    for i, (key, value) in enumerate(pairs):
        if (i + 1) % print_interval == 0:
            s.send('set %s 0 0 %d\r\n%s\r\n' % (key, len(value), value))
            expect_line(f, "STORED\r\n")
            print i + 1,
            sys.stdout.flush()
        else:
            s.send('set %s 0 0 %d noreply\r\n%s\r\n' % (key, len(value), value))
    print
    print "Testing rget"
    s.send('rget null null -1 -1 %d\r\n' % (opts["count"] * 2))
    for i, (key, value) in enumerate(sorted(pairs)):
        expect_line(f, "VALUE %s 0 %d\r\n" % (key, len(value)))
        expect_line(f, "%s\r\n" % value)
        if (i + 1) % print_interval == 0:
            print i + 1,
            sys.stdout.flush()
    print
    expect_line(f, "END\r\n")

