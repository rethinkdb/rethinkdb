#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
from vcoptparse import *
import vm_build
import sys
from threading import Thread, Semaphore

class Builder(Thread):
    def __init__(self, name, branch, target, semaphore):
        Thread.__init__(self)
        self.name = name
        self.branch = branch
        self.target = target
        self.semaphore = semaphore
    def run(self):
        self.success = False
        try:
            semaphore.acquire()
            self.target.run(self.branch, self.name)
            self.success = True
        except vm_build.RunError, err:
            self.exception = err
        finally:
            semaphore.release()

target_names = ["suse", "redhat5_1", "ubuntu", "debian", "centos5_5", "centos6"]

def help():
    print >>sys.stderr, "Virtual builder:"
    print >>sys.stderr, "     --help      Print this help."
    print >>sys.stderr, "     --target target1 [target2, target3]"
    print >>sys.stderr, "                 Build just one target, options are:"
    print >>sys.stderr, "                 ", target_names
    print >>sys.stderr, "                 defaults to all of them."
    print >>sys.stderr, "     --branch branch_name"
    print >>sys.stderr, "                 Build from a branch mutually exclusive with --tag."
    print >>sys.stderr, "     --tag tag-name"
    print >>sys.stderr, "                 Build from a tag mutually exclusive with --branch."
    print >>sys.stderr, "     --threads number"
    print >>sys.stderr, "                 The number of parallel threads to run."
    print >>sys.stderr, "     --debug"
    print >>sys.stderr, "                 Whether to build the packages with debugging enabled."
    print >>sys.stderr, "     --interact"
    print >>sys.stderr, "                 This starts a target so that you can interact with it."
    print >>sys.stderr, "                 Requires a target."
    print >>sys.stderr, "     --clean-up"
    print >>sys.stderr, "                 Shutdown all running vms"
    print >>sys.stderr, "     --username"
    print >>sys.stderr, "                 Starts the Virtual Machine using VirtualBox from the specified username."
    print >>sys.stderr, "     --hostname"
    print >>sys.stderr, "                 Starts the Virtual Machine using VirtualBox from the specified host machine."

o = OptParser()
o["help"] = BoolFlag("--help")
o["target"] = StringFlag("--target", None)
o["branch"] = StringFlag("--branch", None)
o["tag"] = StringFlag("--tag", None)
o["threads"] = IntFlag("--threads", 3)
o["clean-up"] = BoolFlag("--clean-up")
o["interact"] = BoolFlag("--interact")
o["debug"] = BoolFlag("--debug");
o["username"] = StringFlag("--username", "rethinkdb") # For now, these default values should always be the ones you should use
o["hostname"] = StringFlag("--hostname", "deadshot") # because the UUID values below are hard-coded to correspond with rethinkdb@deadshot

try:
    opts = o.parse(sys.argv)
except OptError:
    print >>sys.stderr, "Argument parsing error"
    help()
    exit(-1)

if opts["help"]:
    help()
    sys.exit(0)
    
if opts["branch"] and opts["tag"]:
    print >>sys.stderr, "Error cannot use --tag and --branch together."
    help()
    sys.exit(1)

if opts["branch"]:
    rspec = vm_build.Branch(opts["branch"])
elif opts["tag"]:
    rspec = vm_build.Tag(opts["tag"])
else:
    rspec = vm_build.Branch("master")

#Prepare the build flags
flags = "" #this will be given to the makefile
if opts["debug"]:
    flags += " DEBUG=1 UNIT_TESTS=0"
else:
    flags += " DEBUG=0"

suse = vm_build.target('765127b8-2007-43ff-8668-fe4c60176a2b', '192.168.0.173', 'rethinkdb', 'make LEGACY_LINUX=1 LEGACY_GCC=1 NO_EVENTFD=1 rpm-suse10 ' + flags, 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall, vm_build.rpm_get_binary, opts["username"], opts["hostname"])
redhat5_1 = vm_build.target('32340f79-cea9-42ca-94d5-2da13d408d02', '192.168.0.159', 'rethinkdb', 'make rpm LEGACY_GCC=1 LEGACY_LINUX=1 NO_EVENTFD=1' + flags, 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall, vm_build.rpm_get_binary, opts["username"], opts["hostname"])
ubuntu = vm_build.target('1f4521a0-6e74-4d20-b4b9-9ffd8e231423', '192.168.0.172', 'rethinkdb', 'make deb' + flags, 'deb', vm_build.deb_install, vm_build.deb_uninstall, vm_build.deb_get_binary, opts["username"], opts["hostname"])
debian = vm_build.target('cc76e2a5-92c0-4208-be08-5c02429c2c50', '192.168.0.176', 'root', 'make deb NO_EVENTFD=1 LEGACY_LINUX=1 ' + flags, 'deb', vm_build.deb_install, vm_build.deb_uninstall, vm_build.deb_get_binary, opts["username"], opts["hostname"])
centos5_5 = vm_build.target('25710682-666f-4449-bd28-68b25abd8bea', '192.168.0.153', 'root', 'make rpm LEGACY_GCC=1 LEGACY_LINUX=1 ' + flags, 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall, vm_build.rpm_get_binary, opts["username"], opts["hostname"])
centos6 = vm_build.target('d9058650-a45a-44a5-953f-c2402253a614', '192.168.0.178', 'rethinkdb', 'make rpm LEGACY_GCC=1 LEGACY_LINUX=1 ' + flags, 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall, vm_build.rpm_get_binary, opts["username"], opts["hostname"])

targets = {"suse" : suse, "redhat5_1" : redhat5_1, "ubuntu" : ubuntu, "debian" : debian, "centos5_5" : centos5_5, "centos6" : centos6}

if (opts["target"]):
    targets = {opts["target"] : targets[opts["target"]]}

if opts["clean-up"]:
    map(lambda x: x[1].clean_up(), targets.iteritems())
    exit(0)

if opts["interact"]:
    if not opts["target"]:
        print >>sys.stderr, "Error must specify a --target for --interact mode."
        exit(1)
    for name, target in targets.iteritems():
        target.interact(name)
else:
    success = {}
    exception = {}
    semaphore = Semaphore(opts["threads"])

    builders = map(lambda x: Builder(x[0], rspec, x[1], semaphore), targets.iteritems())
    map(lambda x: x.start(), builders)
    map(lambda x: x.join(), builders)

    for b in builders:
        success[b.name] = b.success
        if not b.success:
            exception[b.name] = b.exception

    print "Build summary:"
    from termcolor import colored
    for name,val in success.iteritems():
        print name, "." * (20 - len(name)), colored("[Pass]", "green") if val else colored("[Fail]", "red")
        if (not val):
            print "Failed on: ", exception[name]
            raise exception[name]

print "Done."
