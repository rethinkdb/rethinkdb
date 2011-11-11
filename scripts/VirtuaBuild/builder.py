#!/usr/bin/python
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
        except self.target.RunError, err:
            self.exception = err
        finally:
            semaphore.release()

def help():
    print >>sys.stderr, "Virtual builder:"
    print >>sys.stderr, "     --help      Print this help."
    print >>sys.stderr, "     --target target1 [target2, target3]"
    print >>sys.stderr, "                 Build just one target, options are:"
    print >>sys.stderr, "                 ", targets.keys()
    print >>sys.stderr, "                 defaults to all of them."
    print >>sys.stderr, "     --branch branch_name"
    print >>sys.stderr, "                 Build from a branch mutually exclusive with --tag."
    print >>sys.stderr, "     --tag tag-name"
    print >>sys.stderr, "                 Build from a tag mutually exclusive with --branch."
    print >>sys.stderr, "     --threads number"
    print >>sys.stderr, "                 The number of parallel threads to run."
    print >>sys.stderr, "     --interact"
    print >>sys.stderr, "                 This starts a target so that you can interact with it."
    print >>sys.stderr, "                 Requires a target."
    print >>sys.stderr, "     --clean-up"
    print >>sys.stderr, "                 Shutdown all running vms"


suse = vm_build.target('12c1cf78-5dc5-4baa-8e93-ac6fdd1ebf1f', '192.168.0.173', 'rethinkdb', 'LANG=C make LEGACY_LINUX=1 LEGACY_GCC=1 NO_EVENTFD=1 DEBUG=0 rpm-suse10', 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall, vm_build.rpm_get_binary)
redhat5_1 = vm_build.target('5eaf8089-9ae4-4493-81fc-a885dc8e08ff', '192.168.0.159', 'rethinkdb', 'LANG=C make rpm DEBUG=0 LEGACY_GCC=1 LEGACY_LINUX=1 NO_EVENTFD=1', 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall, vm_build.rpm_get_binary)
ubuntu = vm_build.target('b555d9f6-441f-4b00-986f-b94286d122e9', '192.168.0.172', 'rethinkdb', 'LANG=C make deb DEBUG=0', 'deb', vm_build.deb_install, vm_build.deb_uninstall, vm_build.deb_get_binary)
debian = vm_build.target('3ba1350e-eda8-4166-90c1-714be0960724', '192.168.0.176', 'root', 'LANG=C make deb DEBUG=0 NO_EVENTFD=1 LEGACY_LINUX=1', 'deb', vm_build.deb_install, vm_build.deb_uninstall, vm_build.deb_get_binary)
centos5_5 = vm_build.target('46c6b842-b4ac-4cd6-9eae-fe98a7246ca9', '192.168.0.177', 'root', 'LANG=C make rpm DEBUG=0 LEGACY_GCC=1 LEGACY_LINUX=1', 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall, vm_build.rpm_get_binary)

targets = {"suse" : suse, "redhat5_1" : redhat5_1, "ubuntu" : ubuntu, "debian" : debian, "centos5_5" : centos5_5}

o = OptParser()
o["help"] = BoolFlag("--help")
o["target"] = StringFlag("--target", None)
o["branch"] = StringFlag("--branch", "master")
o["tag"] = StringFlag("--tag", None)
o["threads"] = IntFlag("--threads", 3)
o["clean-up"] = BoolFlag("--clean-up")
o["interact"] = BoolFlag("--interact")


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
    rspec = Branch("master")

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
