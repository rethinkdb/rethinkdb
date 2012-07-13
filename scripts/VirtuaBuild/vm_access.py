#!/usr/bin/python
import sys, subprocess, os, time
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
            pass
        if not os.system("ssh %s 'true'" % self.host):
            print >>sys.stderr, "(VM successfully started)"
        else:
            sys_exit("Error: Failed to connect to VM", -1)
    def command(self, cmd, output = False):
        print >>sys.stderr, "Executing on VM:", cmd
        proc = subprocess.Popen(["ssh %s '%s'" % (self.host, cmd)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, shell = True)
        while proc.poll() == None: # TODO: add timeout?
            if output:
                line = proc.stdout.readline()
                if line:
                    print >>sys.stderr, line.strip()
            else:
                pass
        if proc.poll():
            sys_exit("Error: command \"%s\" finished with exit value %d." % (cmd, proc.poll()), proc.poll(), True)
        return proc
    def shut_down(self):
        subprocess.Popen(["ssh %s 'VBoxManage controlvm %s poweroff'" % (control_user, self.uuid)], shell = True).wait()

def sys_exit(message, exit_code, shut_down = False):
    print >>sys.stderr, message
    if shut_down:
        target.shut_down()
    sys.exit(exit_code)

suse = VM('suse', '765127b8-2007-43ff-8668-fe4c60176a2b', 'root@192.168.0.173')
redhat5_1 = VM('redhat5_1', '32340f79-cea9-42ca-94d5-2da13d408d02', 'root@192.168.0.159')
ubuntu = VM('ubuntu', '1f4521a0-6e74-4d20-b4b9-9ffd8e231423', 'root@192.168.0.172')
debian = VM('debian', 'cc76e2a5-92c0-4208-be08-5c02429c2c50', 'root@192.168.0.176')
centos5_5 = VM('centos5_5', '25710682-666f-4449-bd28-68b25abd8bea', 'root@192.168.0.153')
centos6 = VM('centos6', 'd9058650-a45a-44a5-953f-c2402253a614', 'root@192.168.0.178')

vm_list = {"suse" : suse, "redhat5_1" : redhat5_1, "ubuntu" : ubuntu, "debian" : debian, "centos5_5" : centos5_5, "centos6" : centos6}

def help():
    print >>sys.stderr, "VM Access:"
    print >>sys.stderr, "       Runs a command on a remote virtual machine. Starts the virtual machine if necessary, and shuts it down on completion. If the command fails or if the virtual machine is inaccessible, then this script will throw an exception. All virtual machines are controlled through " + control_user + ". Before commands are run, the curent directory is compressed and sent over. The command is run in a temporary directory and all its resulting contents are copied back."
    print >>sys.stderr, "       --help      Print this help."
    print >>sys.stderr, "       --vm-name   The target virtual machine to run the command on. Options are:"
    print >>sys.stderr, "                   ", vm_list.keys()
    print >>sys.stderr, "       --command   The command to run on the virtual machine. Must be specified after the vm-name."

o = OptParser()
o["help"] = BoolFlag("--help")
o["vm-name"] = StringFlag("--vm-name", None)
o["command"] = AllArgsAfterFlag("--command", default = None)

try:
    opts = o.parse(sys.argv)
except OptError:
    sys_exit("Argument parsing error", -1)

if opts["help"]:
    help()
    sys_exit("", 0)

if not opts["vm-name"]:
    sys_exit("Error: must specify a VM name.", -1)

if not opts["command"]:
    sys_exit("Error: must specify a command.", -1)

if opts["vm-name"] not in vm_list:
    sys_exit("Error: invalid VM name.", -1)

target = vm_list[opts["vm-name"]]

print >>sys.stderr, "Begin: Running command:", " ".join(opts["command"])
print "\ton VM", target.name

# Start VM
print >>sys.stderr, "***Starting VM..."
target.start()

# Make a temporary directory on VM
print >>sys.stderr, "***Making a temporary directory on the virtual machine..."
proc = target.command("cd /tmp && mktemp -d test.XXXXXXXXXX")
dir_name = "/tmp/" + proc.stdout.readline().strip()
print >>sys.stderr, "***Will be working in directory " + dir_name

# Move files to VM
print >>sys.stderr, "***Transferring files to virtual machine..."
subprocess.Popen(" ".join(["tar", "czf", "tmp.tar.gz", "*", "&&", "scp", "tmp.tar.gz", "%s:%s/tmp.tar.gz" % (target.host, dir_name)]), shell = True).wait()
target.command(" ".join(["cd", dir_name, "&&", "tar", "xzf", "tmp.tar.gz"]))

# Execute command
print >>sys.stderr, "***Executing command..."
proc = target.command(("cd %s && " % dir_name) + " ".join(opts["command"]), output = True)

# Move files from VM
print >>sys.stderr, "***Transferring files from virtual machine..."
proc = target.command(" ".join(["cd", dir_name, "&&", "tar", "czf", "tmp.tar.gz", "*", "&&", "scp", "%s:%s/tmp.tar.gz tmp.tar.gz" % (target.host, dir_name)]))
subprocess.Popen(" ".join(["tar", "xzf", "tmp.tar.gz"]), shell = True).wait()

# Shut down VM
print >>sys.stderr, "***Shutting down VM..."
vm_list[opts["vm-name"]].shut_down()

sys_exit("Done.", 0)
