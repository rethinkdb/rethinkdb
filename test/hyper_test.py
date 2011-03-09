#!/usr/bin/python
from cloud_retester import do_test, do_test_cloud, report_cloud, setup_testing_nodes, terminate_testing_nodes

# Hyper tests: they're faster than smoke tests.

# Just build valgrind mode.
do_test("cd ../src/; make -j" { "DEBUG" : 1, "VALGRIND" : 1, "UNIT_TESTS" : 1 }, cmd_format="make")

# serializer-bench tends to break and only takes 1.6 seconds to build.
do_test("cd ../bench/serializer-bench/; make clean; make", {}, cmd_format="make")

# For safety: ensure that nodes are terminated in case of exceptions.
# Of course we shouldn't be using USE_CLOUD anyway.
try:
    # serial_mix tends to fail when you screw things up, tests fsck, and, takes 30 seconds.
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "duration"    : 10,
                    "fsck"        : True},
                  repeat=1, timeout=90)
    # cas is the most easily breakable metadata, takes 28 seconds.
    do_test_cloud("integration/cas.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol })

    # I've been working on large values a lot, so we have to include this, it takes 38 seconds.
    do_test_cloud("integration/big_values.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol })

    report_cloud()

finally:
    terminate_testing_nodes()
