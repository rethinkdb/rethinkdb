#!/usr/bin/python
import glob
from cloud_retester import do_test, do_test_cloud, report_cloud, setup_testing_nodes, terminate_testing_nodes

# Clean the repo
do_test("cd ../src/; make clean")

# Make every target first, as some tests depend on multiple targets
for mode in ["debug", "release"]:
    for checker in ["valgrind", None]:
        for mock_io in [True, False]:
            for mock_cache in [True, False]:
                # Build our targets
                do_test("cd ../src/; make -j8",
                        { "DEBUG"            : 1 if mode    == "debug"    else 0,
                          "VALGRIND"         : 1 if checker == "valgrind" else 0,
                          "MOCK_IO_LAYER"    : 1 if mock_io               else 0,
                          "MOCK_CACHE_CHECK" : 1 if mock_cache            else 0},
                        cmd_format="make")

# Make sure auxillary tools compile
do_test("cd ../bench/stress-client/; make clean; make",
        {},
        cmd_format="make")
do_test("cd ../bench/serializer-bench/; make clean; make",
        {},
        cmd_format="make")

# For safety: ensure that nodes are terminated in case of exceptions
try:
    # Setup the EC2 testing nodes
    setup_testing_nodes()

    # Go through all our environments
    for (mode, checker) in [
            ("debug", "valgrind"),
            ("release", "valgrind"),
            ("release", None),
            ("release-mockio", None),
            ("debug-mockcache", "valgrind")]:
        for protocol in ["text"]: # ["text", "binary"]:
            for (cores, slices) in [(1, 1), (2, 3)]:
                # Run tests

                # Run the longest running tests first. This gives the dispatcher more chance to parallelize in case of running cloud based test runs
                do_test_cloud("integration/many_keys.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices,
                          "num-keys"    : 50000},
                        repeat=3, timeout=60*90)
                        
                do_test_cloud("integration/serial_mix.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices,
                          "duration"    : 340 },
                        repeat=5, timeout=400)
            
                do_test_cloud("integration/serial_mix.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices,
                          "duration"    : 340,
                          "restart-server-prob" : "0.0005"},
                        repeat=5, timeout=400)
            
                do_test_cloud("integration/multi_serial_mix.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices,
                          "num-testers" : 16,
                          "duration"    : 340},
                        repeat=5, timeout=400)
                
                do_test_cloud("integration/multi_serial_mix.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices,
                          "num-testers" : 16,
                          "memory"      : 10,
                          "duration"    : 340},
                        repeat=5, timeout=400)
                
                do_test_cloud("integration/append_prepend.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=3)

                do_test_cloud("integration/append_stress.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=3)
                
                do_test_cloud("integration/big_values.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=3, timeout=120)
                
                do_test_cloud("integration/cas.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=3)
            
                do_test_cloud("integration/flags.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=3)

                do_test_cloud("integration/shrink.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=3)

                do_test_cloud("integration/incr_decr.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=3)

                do_test_cloud("integration/expiration.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=5)

                do_test_cloud("integration/extraction.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=5)
            
                do_test_cloud("integration/pipeline.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices,
                          "num_ints"    : 1000000 },
                        repeat=3, timeout = 180)
            
#                do_test_cloud("integration/bin_pipeline.py",
#                        { "auto"        : True,
#                          "mode"        : mode,
#                          "no-valgrind" : not checker,
#                          "protocol"    : protocol,
#                          "cores"       : cores,
#                          "slices"      : slices },
#                        repeat=3)

                do_test_cloud("integration/fuzz.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices,
                          "duration"    : 340},
                        repeat=3, timeout=400)

                do_test_cloud("integration/command_line_sanitizing.py",
                        { "auto"        : False,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices},
                        repeat=1, timeout=180)

                for suite_test in glob.glob('integration/memcached_suite/*.t'):
                    do_test_cloud("integration/memcached_suite.py",
                            { "auto"        : True,
                              "mode"        : mode,
                              "no-valgrind" : not checker,
                              "protocol"    : protocol,
                              "cores"       : cores,
                              "slices"      : slices,
                              "suite-test"  : suite_test},
                            repeat=3)

                
                # Don't run the corruption test in mockio or mockcache mode because in those modes
                # we don't flush to disk at all until the server is shut down, so obviously the
                # corruption test will fail.
                if "mock" not in mode:
                    do_test_cloud("integration/corruption.py",
                            { "auto"        : True,
                              "mode"        : mode,
                              "no-valgrind" : not checker,
                              "protocol"    : protocol,
                              "cores"       : cores,
                              "slices"      : slices },
                            repeat=12)
                
    # Report the results
    report_cloud()

finally:
    terminate_testing_nodes()

