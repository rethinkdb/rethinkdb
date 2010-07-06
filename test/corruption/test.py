import subprocess
import signal
import os
import time

import interactor   # see interactor.py

# This script is meant to be run from the directory rethinkdb/test/corruption.

port = 11215
dir = "runs"
rethinkdb_executable = "../../src/rethinkdb"
test_script_dir = "../integration"
test_script_module = "parallel_mix"



def run_mix(name, subname):
    with file("%s/%s/server%s.txt" % (dir, name, subname), "w") as server_log:
        with file("%s/%s/client%s.txt" % (dir, name, subname), "w") as client_log:
        
            print "Launching server."
            server_cmd = [rethinkdb_executable, "-p", str(port), "-m", "0", "%s/%s/data.file" % (dir, name)]
            server = subprocess.Popen(
                server_cmd,   # ["gdb", "--args"] + server_cmd,
                stdout = subprocess.PIPE,
                stderr = subprocess.PIPE,
                stdin = subprocess.PIPE
                )
            # def tell_gdb(msg):
            #     server.stdin.write(msg + "\n")
            #     server.stdin.flush()
            #     server_log.write("[To GDB] " + msg + "\n")
            #     print "[To GDB]", msg
            # tell_gdb("run")
            # time.sleep(1)   # Give GDB time to start up and stuff
                
            print "Launching client."
            client_cmd = "import sys; sys.path.append(\"%s\"); import %s as p; p.PORT = \"%d\"; p.main([])" % (
                test_script_dir, test_script_module, port)
            client = subprocess.Popen(
                ["python", "-c", client_cmd],
                stdout = subprocess.PIPE,
                stderr = subprocess.PIPE,
                stdin = subprocess.PIPE
                )
            
            server_stdout_reader = interactor.NBReader(server.stdout)
            server_stderr_reader = interactor.NBReader(server.stderr)
            client_stdout_reader = interactor.NBReader(client.stdout)
            client_stderr_reader = interactor.NBReader(client.stderr)
            command_reader = interactor.NBReader()
            
            server_running = True
            client_running = True
            sent_sigint = False
            
            verbose = True
            
            print "Running test..."
            while server_running or client_running:
            
                for line in server_stdout_reader.get_lines():
                    server_log.write(line+"\n")
                    if verbose: print "[Server]", line
                
                for line in server_stderr_reader.get_lines():
                    server_log.write(line+"\n")
                    if verbose: print "[Server (err)]", line
                
                for line in client_stdout_reader.get_lines() + client_stderr_reader.get_lines():
                    client_log.write(line + "\n")
                    if verbose: print "[Client]", line
                    
                for line in command_reader.get_lines():
                    if line.startswith("[P]"):
                        line = line[3:].lstrip()
                        try:
                            print eval(line, globals(), locals())
                        except Exception, e:
                            print e
                    # elif line.startswith("[S]"):
                    #     line = line[3:].lstrip()
                    #     tell_gdb(line)
                    else:
                        print "Don't know how to handle", repr(line)
                        
                if server_running and server.poll() is not None:
                    server_running = False
                    print "Server terminated with code %d." % server.returncode
                    server_log.write("[Terminated with code %d.]\n" % server.returncode)
                    
                if client_running and client.poll() is not None:
                    client_running = False
                    print "Client terminated with code %d." % client.returncode
                    client_log.write("[Terminated with code %d.]\n" % client.returncode)
                    
                if server_running and not client_running and client.returncode == 0 and not sent_sigint:
                    print "Auto-terminating server because client succeeded."
                    server.send_signal(signal.SIGINT)
                    # server.send_signal(signal.SIGINT)   # This will cause us to drop into debugger...
                    # tell_gdb("signal SIGINT")   # and this will cause debugger to signal rethinkdb.
                    sent_sigint = True
                    
            print "Test is over."
            
            server_stdout_reader.kill()
            server_log.write(server_stdout_reader.buffer)
            server_stderr_reader.kill()
            server_log.write(server_stderr_reader.buffer)
            client_stdout_reader.kill()
            client_log.write(client_stdout_reader.buffer)
            client_stderr_reader.kill()
            client_log.write(client_stderr_reader.buffer)
            command_reader.kill()
            
            if client.returncode != 0 or server.returncode != 0:
                raise Exception("Something terminated badly.")

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

for i in xrange(1, 1+1):
    print "Starting run %d." % i
    try:
        run_test(str(i))
    except Exception, e:
        print e
        print "Run failed."
    else:
        print "Run succeeded."
