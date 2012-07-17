#!/usr/bin/python

# usage: ./smoke_test.py --mode OS_NAME --num-keys SOME_NUMBER_HERE

import time, sys, os, socket, random, time, signal, subprocess
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'test', 'common')))
import workload_common, driver
from vcoptparse import *

op = OptParser()
op["num_keys"] = IntFlag("--num-keys", 500)
op["mode"] = StringFlag("--mode", "debug")
opts = op.parse(sys.argv)

num_keys = opts["num_keys"]

def wait_until_started_up(proc, host, port, timeout = 600):
    time_limit = time.time() + timeout
    while time.time() < time_limit:
        if proc.poll() is not None:
            raise RuntimeError("Process stopped unexpectedly with return code %d." % proc.poll())
        s = socket.socket()
        try:
            s.connect((host, port))
        except socket.error, e:
            time.sleep(1)
        else:
            break
        finally:
            s.close()
    else:
        raise RuntimeError("Could not connect to process.")

def test_against(host, port, timeout = 600):
    with workload_common.make_memcache_connection({"address": (host, port), "mclib": "pylibmc", "protocol": "text"}) as mc:
        temp = 0
        time_limit = time.time() + timeout
        while not temp and time.time() < time_limit:
            try:
                temp = mc.set("test", "test")
                print temp
            except Exception, e:
                print e
                pass
            time.sleep(1)

        goodsets = 0
        goodgets = 0

        for i in range(num_keys):
            try:
                if mc.set(str(i), str(i)):
                    goodsets += 1
            except:
                pass

        for i in range(num_keys):
            try:
                if mc.get(str(i)) == str(i):
                    goodgets += 1
            except:
                pass
        
        return goodsets, goodgets

executable = driver.find_rethinkdb_executable(opts["mode"])

base = 11213
port = random.randint(base + 1, 65535 - 100)
port2 = random.randint(base + 1, 65535)
port_offset = port - base

print "Starting RethinkDB..."

proc = subprocess.Popen([executable, "--port", str(port2), "--port-offset", str(port_offset)])

# gets the IP address
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.connect(("rethinkdb.com", 80))
ip = s.getsockname()[0]
s.close()

print "IP Address detected:", ip

wait_until_started_up(proc, ip, port)

print "Testing..."

res = test_against(ip, port)

print "Tests completed. Killing instance now..."
proc.send_signal(signal.SIGINT)

if res != (num_keys, num_keys):
    print "Done: FAILED"
    print "Results: %d successful sets, %d successful gets (%d total)" % (res[0], res[1], num_keys)
    exit(1)
else:
    print "Done: PASSED ALL"
    exit(0)
