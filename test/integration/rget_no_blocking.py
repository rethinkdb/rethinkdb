#!/usr/bin/python
import os, sys, socket, random, time, signal

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, "common")))
from test_common import *
from line import *

key_padding = ''.zfill(20)
def gen_key(prefix, num):
    return prefix + key_padding + str(num).zfill(6)

value_padding = ''.zfill(240)
large_value_padding = ''.zfill(512)
def gen_value(prefix, num):
    if num % 5 == 4:
        return prefix + large_value_padding + str(num).zfill(6)
    else:
        return prefix + value_padding + str(num).zfill(6)


def sock_readline(sock_file):
    ls = []
    while True:
        l = sock_file.readline()
        ls.append(l)
        if len(l) >= 2 and l[-2:] == '\r\n':
            break
    return ''.join(ls)

value_line = line("^VALUE\s+([^\s]+)\s+(\d+)\s+(\d+)\r\n$", [('key', 's'), ('flags', 'd'), ('length', 'd')])

def get_results(s):
    res = []

    f = s.makefile()
    while True:
        l = sock_readline(f)
        if l == 'END\r\n':
            break
        val_def = value_line.parse_line(l)
        if not val_def:
            raise ValueError("received unexpected line from rget: %s" % l)
        val = sock_readline(f).rstrip()
        if len(val) != val_def['length']:
            raise ValueError("received value of unexpected length (expected %d, got %d: '%s')" % (val_def['length'], len(val), val))

        res.append({'key': val_def['key'], 'value': val})
    return res

class TimeoutException(Exception):
    pass

# Test:
# start rget query, then start a write concurrently (but after rget got to the bottom of the tree)
# if the write blocks, then we do not do copy-on-write/snapshotting/etc.
# Also check that rget gets consistent data (i.e. is not affected by concurrent write), and that
# the write actually updates the data
def test_function(opts, port, test_dir):
    rget_keys = 10000
    updated_key_id = rget_keys-1
    updated_key = gen_key('foo', updated_key_id)
    updated_value = gen_value('changed', updated_key_id)
    orig_value = gen_value('foo', updated_key_id)

    print "Creating test data"
    mc = connect_to_port(opts, port)
    for i in range(0, rget_keys):
        mc.set(gen_key('foo', i), gen_value('foo', i))

    print "Starting rget"

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))
    s.send('rget %s %s %d %d %d\r\n' % (gen_key('foo', 0), gen_key('fop', 0), 0, 1, rget_keys))
    print "Started rget"

    # we don't read the data, we just stop here, preventing rget from proceding

    # rget is slow to start, so we need to wait a bit, before it locks down the path.
    # This is a crude way, but is probably the simplest
    time.sleep(5)

    us = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    us.connect(("localhost", port))

    print "Starting concurrent update"

    try:
        us.setblocking(1)
        print "  Sending concurrent set"
        us.send('set %s 0 0 %d\r\n%s\r\n' % (updated_key, len(updated_value), updated_value))
        uf = us.makefile()
        us.settimeout(10.0)
        print "  Waiting for set result"
        set_res = sock_readline(uf).rstrip()
        if set_res != 'STORED':
            raise ValueError("update failed: %s" % set_res)
        print "  Concurrent set finished"
    finally:
        us.close()

    v = mc.get(updated_key)
    if v != updated_value:
        raise ValueError("update didn't take effect")

    res = get_results(s)
    if len(res) != rget_keys:
        raise ValueError("received unexpected number of results from rget (expected %d, got %d)" % (rget_keys, len(res)))
    if res[updated_key_id]['value'] != orig_value:
        raise ValueError("rget results are not consistent (update changed the contents of a part of running rget query)")
    s.close()

    v = mc.get(updated_key)
    if v != updated_value:
        raise ValueError("update didn't take effect")

    mc.disconnect_all()
    print "Done"


if __name__ == "__main__":
    op = make_option_parser()
    opts = op.parse(sys.argv)
    auto_server_test_main(test_function, opts, timeout = 210)

