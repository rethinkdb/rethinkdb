#!/usr/bin/python
from retester import do_test, report

# Clean the repo
do_test("cd ../src/; make clean")

# Build everything
for mode in ["debug", "release"]:
    for checker in ["valgrind", None]:
        # Build our targets
        do_test("cd ../src/; make -j8",
                { "DEBUG"                     : 1 if mode    == "debug"    else 0,
                  "VALGRIND"                  : 1 if checker == "valgrind" else 0,
                  "SEMANTIC_SERIALIZER_CHECK" : 1 },
                cmd_format="make")

# Make sure auxillary tools compile
do_test("cd ../bench/stress-client/; make clean; make",
        {},
        cmd_format="make")
do_test("cd ../bench/serializer-bench/; make clean; make",
        {},
        cmd_format="make")

# Do quick smoke tests in some environments
for (mode, checker, protocol) in [("debug", "valgrind", "text")]:
    # VERY basic tests
    do_test("integration/append_prepend.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol })
    
    do_test("integration/cas.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol })
    
    do_test("integration/flags.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol })
    
    do_test("integration/incr_decr.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol })
    
    do_test("integration/expiration.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol })
    
    # More advanced tests in various cores/slices configuration
    for (cores, slices) in [(1, 1)]:
        do_test("integration/many_keys.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol,
                  "cores"       : cores,
                  "slices"      : slices,
                  "num-keys"    : 500 })

        do_test("integration/serial_mix.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol,
                  "cores"       : cores,
                  "slices"      : slices,
                  "duration"    : 10 },
                repeat=2, timeout=25)
    
        do_test("integration/serial_mix.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol,
                  "cores"       : cores,
                  "slices"      : slices,
                  "duration"    : 10,
                  "restart-server-prob" : "0.0005" },
                repeat=2, timeout=25)
    
        do_test("integration/multi_serial_mix.py",
                { "auto"        : True,
                  "mode"        : mode,
                  "no-valgrind" : not checker,
                  "protocol"    : protocol,
                  "cores"       : cores,
                  "slices"      : slices,
                  "duration"    : 10 },
                repeat=2, timeout=25)
        
        do_test("integration/multi_serial_mix.py",
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
report()

