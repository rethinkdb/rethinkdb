#!/usr/bin/python
from cloud_retester import do_test, do_test_cloud, report_cloud, setup_testing_nodes, terminate_testing_nodes

# Clean the repo
do_test("cd ../src/; make clean")

# Make every target first, as some tests depend on multiple targets

# Build the version of the server that the tests will use
do_test("cd ../src/; nice make -j 9",
        { "DEBUG"      : 1,
          "VALGRIND"   : 1,
          "UNIT_TESTS" : 1 },
        cmd_format="make", timeout=180)

# Build with all the special flags to make sure they all compile. We use
# all the special flags simultaneously so that the smoke test doesn't have
# to perform very many builds.
do_test("cd ../src/; nice make -j 9",
        { "DEBUG"            : 0,
          "NO_EPOLL"         : 1,
          "MOCK_IO_LAYER"    : 1,
          "MOCK_CACHE_CHECK" : 1,
          "UNIT_TESTS" : 1 },
        cmd_format="make", timeout=180)

# Make sure auxillary tools compile
do_test("cd ../bench/stress-client/; make clean; make stress libstress.so",
        {},
        cmd_format="make")
do_test("cd ../bench/serializer-bench/; make clean; make",
        {},
        cmd_format="make")

# For safety: ensure that nodes are terminated in case of exceptions
try:
    # Setup the EC2 testing nodes
    setup_testing_nodes()

    # Do quick smoke tests

    # Run the unit tests
    do_test("../build/debug-valgrind/rethinkdb-unittest")

    # Replication is compliated and tests most of the major server features.
    # Running one test with replication will catch a lot of bugs, and by only
    # running one test we keep the smoke test as quick as possible.
    do_test_cloud("integration/serial_mix.py",
            { "auto"        : True,
              "mode"        : "debug",
              "no-valgrind" : False,
              "duration"    : 30,
              "cores"       : 3,
              "slices"      : 1,
              "failover"    : True,
              "fsck"        : True},
            repeat=1, timeout=120)


    # Report the results
    report_cloud()

finally:
    terminate_testing_nodes()

