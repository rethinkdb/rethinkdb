#!/usr/bin/python
import random, time, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def test(opts, mc, repli_mc, test_dir):
    #so this is a hack I admit it, we should actually just pass these addresses
    #in when we don't want to use the mc clients. But hey this is python your
    #code sucks no matter what.

    master_port = int(mc.addresses[0].split(':')[1])

    stress_client(port=master_port, workload=
                                  { "deletes":  opts["ndeletes"],
                                    "updates":  opts["nupdates"],
                                    "inserts":  opts["ninserts"],
                                    "gets":     opts["nreads"],
                                    "appends":  opts["nappends"],
                                    "prepends": opts["nprepends"] },
                  duration="%ds" % opts["duration"], test_dir=test_dir)

if __name__ == "__main__":
    op = make_option_parser()
    op["ndeletes"]  = IntFlag("--ndeletes",  1)
    op["nupdates"]  = IntFlag("--nupdates",  4)
    op["ninserts"]  = IntFlag("--ninserts",  8)
    op["nreads"]    = IntFlag("--nreads",    64)
    op["nappends"]  = IntFlag("--nappends",  0)
    op["nprepends"] = IntFlag("--nprepends", 0)
    opts = op.parse(sys.argv)
    opts["netrecord"] = False   # We don't want to slow down the network
    replication_test_main(test, opts, timeout = (100 if not opts.has_key("duration") else opts["duration"] + 60))
