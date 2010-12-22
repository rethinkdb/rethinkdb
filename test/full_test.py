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

# Running canonical tests
def run_canonical_tests(mode, checker, protocol, cores, slices):
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
    
    do_test_cloud("integration/big_values.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices },
                  repeat=3, timeout=200)
    
    # Note: we can't send too many things via the pipeline test now
    # because it saturates network buffers. TODO: fix the pipeline
    # test (see issue 134).
    do_test_cloud("integration/pipeline.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "num-ints"    : 50000 if (mode == "release" and not checker) else 1000 },
                  repeat=3, timeout = 180)
    
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

# Running all tests
def run_all_tests(mode, checker, protocol, cores, slices):
    # Run the longest running tests first. This gives the
    # dispatcher more chance to parallelize in case of
    # running cloud based test runs
    do_test_cloud("integration/many_keys.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "num-keys"    : 50000},
                  repeat=3, timeout=720)
    
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 340 },
                  repeat=5, timeout=400)
    
    # TODO: This should really only be run under one environment...
    do_test_cloud("regression/gc_verification.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 10000000 },
                  repeat=3, timeout=400)
    
    # Run the serial mix test also with the other valgrind tools, drd and helgrind
    if checker == "valgrind":
        for valgrind_tool in ["drd", "helgrind"]:
            do_test_cloud("integration/serial_mix.py",
                          { "auto"        : True,
                            "mode"        : mode,
                            "no-valgrind" : not checker,
                            "valgrind-tool" : valgrind_tool,
                            "protocol"    : protocol,
                            "cores"       : cores,
                            "slices"      : slices,
                            "duration"    : 300 },
                          repeat=3, timeout=400)
            
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

    do_test_cloud("regression/issue_69.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=60)

    do_test_cloud("regression/issue_90.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=60)

    do_test_cloud("regression/issue_95.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=60)

    do_test_cloud("regression/issue_133.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=10, timeout=30)

    for suite_test in glob.glob('integration/memcached_suite/*.t'):
        do_test_cloud("integration/memcached_suite.py",
                      { "auto"        : True,
                        "mode"        : mode,
                        "no-valgrind" : not checker,
                        "protocol"    : protocol,
                        "cores"       : cores,
                        "slices"      : slices,
                        "suite-test"  : suite_test},
                      repeat=3, timeout=120)
                
    # Canonical tests are included in all tests
    run_canonical_tests(mode, checker, protocol, cores, slices)

# For safety: ensure that nodes are terminated in case of exceptions
try:
    # Setup the EC2 testing nodes
    setup_testing_nodes()

    # GO THROUGH CANONICAL ENVIRONMENTS
    for (mode, checker) in [
            ("debug", "valgrind"),
            ("release", None)]:
        for protocol in ["text"]:
            for (cores, slices) in [(2, 8)]:
                # RUN ALL TESTS
                run_all_tests(mode, checker, protocol, cores, slices)

    # GO THROUGH ALL OUR ENVIRONMENTS
    for (mode, checker) in [
            ("debug", "valgrind"),
            ("release", "valgrind"),
            ("release", None),
            ("release-mockio", None),
            ("debug-mockcache", "valgrind")]:
        for protocol in ["text"]: # ["text", "binary"]:
            for (cores, slices) in [(1, 1), (2, 8)]:
                # RUN CANONICAL TESTS
                run_canonical_tests(mode, checker, protocol, cores, slices)
                
    # Report the results
    report_cloud()

finally:
    terminate_testing_nodes()

