#!/usr/bin/python
import sys
from os import execv
from optparse import OptionParser
from cloud_retester import do_test, do_test_cloud, report_cloud, setup_testing_nodes, terminate_testing_nodes

long_test_branch = "long-test"
no_checkout_arg = "--no-checkout"

def exec_self(args):
    execv("/usr/bin/env", ["python", sys.argv[0]] + args)

def build_args_parser():
    parser = OptionParser()
    parser.add_option(no_checkout_arg, dest="no_checkout", action="store_true")
    return parser

def git_checkout(branch):
    do_test("git fetch -f origin {b}:refs/remotes/origin/{b} && git checkout -f origin/{b}".format(b=branch))

def git_local_changes():
    return None

(options, args) = build_args_parser().parse_args(sys.argv[1:])

if not(options.no_checkout):
    if git_local_changes():
        print "found local changes, bailing out"
        sys.exit(1)
    print "Checking out"
    git_checkout(long_test_branch)
    # import pdb; pdb.set_trace()
    exec_self(sys.argv[1:] + [no_checkout_arg])
else:
    print "Checked out"

# Clean the repo
do_test("cd ../src/; make clean")


