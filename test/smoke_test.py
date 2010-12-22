#!/usr/bin/python
from cloud_retester import do_test, do_test_cloud, report_cloud, setup_testing_nodes, terminate_testing_nodes

# Clean the repo
do_test("cd ../src/; make clean")

# Make every target first, as some tests depend on multiple targets
for mode in ["debug", "release"]:
    for checker in ["valgrind", None]:
        for mock_io in [True, False]:
            for mock_cache in [True, False]:
                # Build our targets
                do_test("cd ../src/; nice make -j",
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

    # Do quick smoke tests in some environments
    for (mode, checker, protocol) in [("debug", "valgrind", "text")]:
        # VERY basic tests
        do_test_cloud("integration/append_prepend.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol })
        
        do_test_cloud("integration/cas.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol })
        
        do_test_cloud("integration/flags.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol })
        
        do_test_cloud("integration/incr_decr.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol })
        
        do_test_cloud("integration/expiration.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol })
        
        # More advanced tests in various cores/slices configuration
        for (cores, slices) in [(1, 1)]:
            do_test_cloud("integration/many_keys.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "num-keys"    : 500 })

            do_test_cloud("integration/serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "duration"    : 10 },
                    repeat=2, timeout=25)
        
            do_test_cloud("integration/serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "duration"    : 10,
                      "restart-server-prob" : "0.0005" },
                    repeat=2, timeout=25)
        
            do_test_cloud("integration/multi_serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "duration"    : 10 },
                    repeat=2, timeout=25)
            
            do_test_cloud("integration/multi_serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "memory"      : 5,
                      "duration"    : 10 },
                    repeat=2, timeout=25)
        
    # Report the results
    report_cloud()
    
finally:
    terminate_testing_nodes()

