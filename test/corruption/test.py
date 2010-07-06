import subprocess
import signal
import os
import time

# This script is meant to be run from the directory rethinkdb/test/corruption.

port = 11215
dir = "runs"
rethinkdb_executable = "../../src/rethinkdb"
test_script_dir = "../integration"
test_script_module = "parallel_insert"

def run_mix(name, subname):
    with file("%s/%s/server%s.txt" % (dir, name, subname), "w") as server_stdout:
        with file("%s/%s/client%s.txt" % (dir, name, subname), "w") as client_stdout:
        
            print "Launching server."
            server = subprocess.Popen(
                [rethinkdb_executable, "-p", str(port), "-m", "0", "%s/%s/data.file" % (dir, name)],
                stdout = server_stdout,
                stderr = subprocess.STDOUT
                )
                
            print "Launching client."
            client_cmd = "import sys; sys.path.append(\"%s\"); import %s as p; p.PORT = \"%d\"; p.main([])" % (
                test_script_dir, test_script_module, port)
            client = subprocess.Popen(
                ["python", "-c", client_cmd],
                stdout = client_stdout,
                stderr = subprocess.STDOUT
                )
            
            print "Running test..."
            client.wait();
            
            print "Client done."
            if client.returncode != 0:
                print "Client failed."
                server.terminate()
                raise Exception("Client failed with code %d." % client.returncode)
            server.poll()
            if server.returncode != None:
                print "Server failed."
                raise Exception("Server terminated too early with code %d." % server.returncode)
            print "Sending server SIGINT."
            server.send_signal(signal.SIGINT)
            print "Waiting for server."
            server.wait()
            print "Server done."
            if server.returncode != 0:
                print "Server failed."
                raise Exception("Server failed with code %d." % server.returncode)

def run_test(name):
    print "Preparing..."
    subprocess.check_call("mkdir -p %s/%s" % (dir, name), shell=True)
    subprocess.check_call("rm -f %s/%s/data.file*" % (dir, name), shell=True)
    print "Starting round 1."
    run_mix(name, "1")
    subprocess.check_call("mkdir -p %s/%s/backup" % (dir, name), shell=True)
    subprocess.check_call("cp %s/%s/data.file* %s/%s/backup" % (dir, name, dir, name), shell=True)
    print "Starting round 2."
    run_mix(name, "2")
    print "Nothing interesting observed."

for i in xrange(1, 3+1):
	print "Starting run %d." % i
	try:
		run_test(str(i))
	except Exception, e:
		print e
		print "Run failed."
	else:
		print "Run succeeded."
