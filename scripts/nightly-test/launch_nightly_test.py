# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python

# Usage: ./launch_nightly_test.py
#            --test-host <hostname>[:<port>]
#            (--email <name>@<address>)*
#            [--title "<Title>"]
#            [-- <flags for full_test_driver.py>]

import sys, subprocess32, os, optparse

if __name__ != "__main__":
    raise ImportError("It doesn't make any sense to import this as a module")

parser = optparse.OptionParser()
parser.add_option("--test-host", action = "store", dest = "test_host")
parser.add_option("--email", action = "append", dest = "emailees")
parser.add_option("--title", action = "store", dest = "title")
parser.set_defaults(title = "Nightly test", emailees = [])
(options, args) = parser.parse_args()
if options.test_host is None:
    parser.error("You must specify --test-host.")

def escape(arg):
    return "'" + arg.replace("'", "'\''") + "'"

tar_proc = subprocess32.Popen(
    ["tar", "--create", "--gzip", "--file=-", "-C", os.path.dirname(__file__), "--"] +
        ["full_test_driver.py", "remotely.py", "simple_linear_db.py", "renderer"],
    stdout = subprocess32.PIPE
    )
try:
    command = "SLURM_CONF=/home/teapot/slurm/slurm.conf ./full_test_driver.py %s >output.txt 2>&1" % " ".join(escape(x) for x in args)
    curl_cmd_line = ["curl", "-f", "-X", "POST", "http://%s/spawn/" % options.test_host]
    curl_cmd_line += ["-F", "tarball=@-"]
    curl_cmd_line += ["-F", "command=%s" % command]
    curl_cmd_line += ["-F", "title=%s" % options.title]
    for emailee in options.emailees:
        curl_cmd_line += ["-F", "emailee=%s" % emailee]
    subprocess32.check_call(curl_cmd_line, stdin = tar_proc.stdout)
finally:
    try:
        tar_proc.terminate()
    except IOError:
        pass