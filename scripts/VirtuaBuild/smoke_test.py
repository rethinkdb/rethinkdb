#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.

# usage: ./smoke_test.py --mode OS_NAME --num-keys SOME_NUMBER_HERE

import time, sys, os, socket, random, time, signal, subprocess
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'test', 'common')))
import driver, memcached_workload_common
from vcoptparse import *

op = OptParser()
op["num_keys"] = IntFlag("--num-keys", 500)
op["mode"] = StringFlag("--mode", "debug")
op["pkg_type"] = StringFlag("--pkg-type", "deb") # "deb" or "rpm"
opts = op.parse(sys.argv)

num_keys = opts["num_keys"]
base_port = 11213 # port that RethinkDB runs from by default

if opts["pkg_type"] == "rpm":
    def install(path):
        return "rpm -i %s --nodeps" % path

    def get_binary(path):
        return "rpm -qpil %s | grep /usr/bin" % path

    def uninstall(cmd_name): 
        return "which %s | xargs readlink -f | xargs rpm -qf | xargs rpm -e" % cmd_name
elif opts["pkg_type"] == "deb":
    def install(path):
        return "dpkg -i %s" % path

    def get_binary(path):
        return "dpkg -c %s | grep /usr/bin/rethinkdb-.* | sed 's/^.*\(\/usr.*\)$/\\1/'" % path

    def uninstall(cmd_name): 
        return "which %s | xargs readlink -f | xargs dpkg -S | sed 's/^\(.*\):.*$/\\1/' | xargs dpkg -r" % cmd_name
else:
    print >>sys.stderr, "Error: Unknown package type."
    exit(0)

def purge_installed_packages():
    try:
        old_binaries_raw = exec_command(["ls", "/usr/bin/rethinkdb*"], shell = True).stdout.readlines()
    except Exception, e:
        print "Nothing to remove."
        return
    old_binaries = map(lambda x: x.strip('\n'), old_binaries_raw)
    print "Binaries scheduled for removal: ", old_binaries

    try:
    	exec_command(uninstall(old_binaries[0]), shell = True)
    except Exception, e:
        exec_command('rm -f ' + old_binaries[0])
    purge_installed_packages()

def exec_command(cmd, bg = False, shell = False):
    if type(cmd) == type("") and not shell:
        cmd = cmd.split(" ")
    elif type(cmd) == type([]) and shell:
        cmd = " ".join(cmd)
    print cmd
    if bg:
        return subprocess.Popen(cmd, stdout = subprocess.PIPE, shell = shell) # doesn't actually run in background: it just skips the waiting part
    else:
        proc = subprocess.Popen(cmd, stdout = subprocess.PIPE, shell = shell)
        proc.wait()
        if proc.poll():
            raise RuntimeError("Error: command ended with signal %d." % proc.poll())
        return proc

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
    with memcached_workload_common.make_memcache_connection({"address": (host, port), "mclib": "pylibmc", "protocol": "text"}) as mc:
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

cur_dir = exec_command("pwd").stdout.readline().strip('\n')
p = exec_command("find build/%s -name *.%s" % (opts["mode"], opts["pkg_type"]))
raw = p.stdout.readlines()
res_paths = map(lambda x: os.path.join(cur_dir, x.strip('\n')), raw)
print "Packages to install:", res_paths
failed_test = False

for path in res_paths:
    print "TESTING A NEW PACKAGE"

    print "Uninstalling old packages..."
    purge_installed_packages()
    print "Done uninstalling..."

    print "Installing RethinkDB..."
    target_binary_name = exec_command(get_binary(path), shell = True).stdout.readlines()[0].strip('\n')
    print "Target binary name:", target_binary_name
    exec_command(install(path))

    print "Starting RethinkDB..."

    exec_command("rm -rf rethinkdb_data")
    exec_command("rm -f core.*")
    proc = exec_command("rethinkdb", bg = True)

    # gets the IP address
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("rethinkdb.com", 80))
    ip = s.getsockname()[0]
    s.close()

    print "IP Address detected:", ip

    wait_until_started_up(proc, ip, base_port)

    print "Testing..."

    res = test_against(ip, base_port)

    print "Tests completed. Killing instance now..."
    proc.send_signal(signal.SIGINT)

    timeout = 60 # 1 minute to shut down
    time_limit = time.time() + timeout
    while proc.poll() is None and time.time() < time_limit:
        pass

    if proc.poll() != 0:
        print "RethinkDB failed to shut down properly. (TEST FAILED)"
        failed_test = False

    if res != (num_keys, num_keys):
        print "Done: FAILED"
        print "Results: %d successful sets, %d successful gets (%d total)" % (res[0], res[1], num_keys)
        failed_test = True
    else:
        print "Done: PASSED"

print "Done."

if failed_test:
    exit(1)
else:
    exit(0)
