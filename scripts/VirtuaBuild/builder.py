#!/usr/bin/python
from vcoptparse import *
import vm_build
import sys

def help():
    print "Virtual builder:"
    print "     --help      Print this help."
    print "     --target target1 [target2, target3]"
    print "                 Build just one target, options are:"
    print "                 ", targets.keys()
    print "                 defaults to all of them."
    print "     --branch branch_name"
    print "                 Which branch in git to build off of."


suse = vm_build.target('12c1cf78-5dc5-4baa-8e93-ac6fdd1ebf1f', '192.168.0.173', 'rethinkdb', 'make LEGACY_LINUX=1 LEGACY_GCC=1 NO_EVENTFD=1 DEBUG=0 rpm-suse10', 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall)
redhat5_1 = vm_build.target('5eaf8089-9ae4-4493-81fc-a885dc8e08ff', '192.168.0.159', 'rethinkdb', 'make rpm DEBUG=0 LEGACY_GCC=1 LEGACY_LINUX=1 NO_EVENTFD=1', 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall)
ubuntu = vm_build.target('b555d9f6-441f-4b00-986f-b94286d122e9', '192.168.0.172', 'rethinkdb', 'make deb DEBUG=0', 'deb', vm_build.deb_install, vm_build.deb_uninstall)
debian = vm_build.target('3ba1350e-eda8-4166-90c1-714be0960724', '192.168.0.176', 'root', 'make deb DEBUG=0 NO_EVENTFD=1 LEGACY_LINUX=1', 'deb', vm_build.deb_install, vm_build.deb_uninstall)
centos5_5 = vm_build.target('46c6b842-b4ac-4cd6-9eae-fe98a7246ca9', '192.168.0.177', 'root', 'make rpm DEBUG=0 LEGACY_GCC=1 LEGACY_LINUX=1', 'rpm', vm_build.rpm_install, vm_build.rpm_uninstall)

targets = {"suse" : suse, "redhat5_1" : redhat5_1, "ubuntu" : ubuntu, "debian" : debian, "centos5_5" : centos5_5}

o = OptParser()
o["help"] = BoolFlag("--help")
o["target"] = StringFlag("--target", None)
o["branch"] = StringFlag("--branch", "master")

try:
    opts = o.parse(sys.argv)
except OptError:
    print "Argument parsing error"
    help()
    exit(-1)

if opts["help"]:
    help()
    sys.exit(0)


if (opts["target"]):
    targets = {opts["target"] : targets[opts["target"]]}

success = {}
exception = {}
for name,target in targets.iteritems():
    try:
        target.run(opts["branch"], name)
        success[name] = True
    except target.RunError as err:
        success[name] = False
        exception[name] = err

print "Build summary:"
from termcolor import colored
for name,val in success.iteritems():
    print name, "." * (20 - len(name)), colored("[Pass]", "green") if val else colored("[Fail]", "red")
    if (not val):
        print "Failed on: ", exception[name]
