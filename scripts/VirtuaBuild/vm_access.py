#!/usr/bin/python
import sys, subprocess, os, time, signal
from vcoptparse import *

control_user = "rethinkdb@deadshot"

class VM(object):
    def __init__(self, name, uuid, host):
        self.name = name
        self.uuid = uuid
        self.host = host
    def start(self):
        subprocess.Popen(["ssh %s 'VBoxManage startvm --type headless %s'" % (control_user, self.uuid)], shell = True).wait()
        start_time = time.time()
        while os.system("ssh %s 'true'" % self.host) and time.time() - start_time < 5 * 60: # timeout after 5 minutes
            time.sleep(1)
        if not os.system("ssh %s 'true'" % self.host):
            print "(VM successfully started)"
        else:
            sys_exit("Error: Failed to connect to VM", -1)
    def command(self, cmd, output = False, timeout = 1200):
        print "Executing on VM:", cmd
        proc = subprocess.Popen(["ssh %s '%s'" % (self.host, cmd)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, shell = True)
        start_time = time.time()
        while proc.poll() == None and time.time() - start_time < timeout:
            if output:
                line = proc.stdout.readline()
                if line:
                    print line.strip()
            else:
                pass
        if proc.poll() == None:
            proc.send_signal(signal.SIGKILL)
            sys_exit("Error: process did not finish within the time limit.", -1)
        if proc.poll():
            sys_exit("Error: command \"%s\" finished with exit value %d." % (cmd, proc.poll()), proc.poll())
        return proc
    def shut_down(self, remove_temp = False):
        if remove_temp:
            self.command("rm -rf /tmp/test.*")
        subprocess.Popen(["ssh %s 'VBoxManage controlvm %s poweroff'" % (control_user, self.uuid)], shell = True).wait()

def sys_exit(message, exit_code, shut_down = False):
    print message
    if shut_down:
        target.shut_down()
    sys.exit(exit_code)

suse = VM('suse', '765127b8-2007-43ff-8668-fe4c60176a2b', 'root@192.168.0.173')
redhat5_1 = VM('redhat5_1', '32340f79-cea9-42ca-94d5-2da13d408d02', 'root@192.168.0.159')
ubuntu = VM('ubuntu', '1f4521a0-6e74-4d20-b4b9-9ffd8e231423', 'root@192.168.0.172')
debian = VM('debian', 'cc76e2a5-92c0-4208-be08-5c02429c2c50', 'root@192.168.0.176')
centos5_5 = VM('centos5_5', '7595c315-9be0-4e6d-a757-33f018182937', 'root@192.168.0.177')
centos6 = VM('centos6', 'a48b00d0-98e8-4b13-a9ea-d58dde484061', 'root@192.168.0.178')

vm_list = {"suse" : suse, "redhat5_1" : redhat5_1, "ubuntu" : ubuntu, "debian" : debian, "centos5_5" : centos5_5, "centos6" : centos6}

def help():
    print "VM Access:"
    print "       Runs a command on a remote virtual machine. Starts the virtual machine if necessary, and shuts it down on completion. If the command fails or if the virtual machine is inaccessible, then this script will throw an exception. All virtual machines are controlled through " + control_user + ". Before commands are run, the curent directory is compressed and sent over. The command is run in a temporary directory and all its resulting contents are copied back."
    print "       --help      Print this help."
    print "       --vm-name   The target virtual machine to run the command on. Options are:"
    print "                   ", vm_list.keys()
    print "       --command   The command to run on the virtual machine. Either command or shut-down must be specified."
    print "       --shut-down Use this flag to shut down the specified VM."

o = OptParser()
o["help"] = BoolFlag("--help")
o["vm-name"] = StringFlag("--vm-name", None)
o["command"] = StringFlag("--command", default = None)
o["shut-down"] = BoolFlag("--shut-down")

try:
    opts = o.parse(sys.argv)
except OptError:
    sys_exit("Argument parsing error", -1)

if opts["help"]:
    help()
    sys_exit("", 0)

if not opts["vm-name"]:
    sys_exit("Error: must specify a VM name.", -1)

if not opts["command"] and not opts["shut-down"]:
    sys_exit("Error: must specify a command or call shut-down.", -1)

if opts["vm-name"] not in vm_list:
    sys_exit("Error: invalid VM name.", -1)

target = vm_list[opts["vm-name"]]

if opts["shut-down"]:
    target.shut_down(remove_temp = True)
    exit(0)

print "Begin: Running command:", opts["command"]
print "\ton VM", target.name

# Start VM
print "***Starting VM..."
target.start()

# Make a temporary directory on VM
print "***Making a temporary directory on the virtual machine..."
proc = target.command("cd /tmp && mktemp -d test.XXXXXXXXXX")
dir_name = "/tmp/" + proc.stdout.readline().strip()
print "***Will be working in directory " + dir_name

# Move files to VM
print "***Transferring files to virtual machine..."
if "RETHINKDB" in os.environ:
    print "*****(debug: we are currently running a test)"
subprocess.Popen(" ".join((["cd", os.environ["RETHINKDB"] + "/..", "&&"] if "RETHINKDB" in os.environ else []) + ["tar", "czf", "tmp.tar.gz", "*", "&&", "scp", "tmp.tar.gz", "%s:%s/tmp.tar.gz" % (target.host, dir_name)]), shell = True).wait()
target.command(" ".join(["cd", dir_name, "&&", "tar", "xzf", "tmp.tar.gz"]), output = True)

# Execute command
print "***Executing command..."
# modifications
opts["command"] = opts["command"].replace('_HOST', '$HOST').replace('_PORT', '$PORT')
proc = target.command(("cd %s && " % dir_name) + opts["command"], output = True)

# Move files from VM
print "***Transferring files from virtual machine..."
proc = target.command(" ".join(["cd", dir_name, "&&", "rm", "tmp.tar.gz", "&&", "tar", "czf", "tmp.tar.gz", "*"]), output = True)
subprocess.Popen(" ".join(["scp", "%s:%s/tmp.tar.gz tmp.tar.gz" % (target.host, dir_name)]), shell = True).wait()
subprocess.Popen(" ".join(["tar", "xzf", "tmp.tar.gz"]), shell = True).wait()

# Note: VMs are not shut down here in case multiple tests are going on.

sys_exit("Done.", 0)
