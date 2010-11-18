#!/usr/bin/python
from retester import do_test, report

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
            do_test("integration/append_prepend.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices },
                    repeat=3)
        
            do_test("integration/cas.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices },
                    repeat=3)
        
            do_test("integration/flags.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices },
                    repeat=3)
        
            do_test("integration/incr_decr.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices },
                    repeat=3)
        
            do_test("integration/serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "duration"    : 240 },
                    repeat=5, timeout=300)
        
            do_test("integration/serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "duration"    : 240,
                      "restart-server-prob" : "0.0005"},
                    repeat=5, timeout=300)
        
            do_test("integration/multi_serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "num-testers" : 16,
                      "duration"    : 240},
                    repeat=5, timeout=300)
        
            do_test("integration/expiration.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices },
                    repeat=5)

            do_test("integration/extraction.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices },
                    repeat=5)
        
            do_test("integration/pipeline.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices },
                    repeat=3, timeout = 180)
        
            do_test("integration/bin_pipeline.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices },
                    repeat=3)
        
            do_test("integration/many_keys.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "num-keys"    : 50000},
                    repeat=3, timeout=60*90)

            do_test("integration/fuzz.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : cores,
                      "slices"      : slices,
                      "duration"    : 240},
                    repeat=3, timeout=60*90)
            
            # Don't run the corruption test in mockio or mockcache mode because in those modes
            # we don't flush to disk at all until the server is shut down, so obviously the
            # corruption test will fail.
            if "mock" not in mode:
                do_test("integration/corruption.py",
                        { "auto"        : True,
                          "mode"        : mode,
                          "no-valgrind" : not checker,
                          "protocol"    : protocol,
                          "cores"       : cores,
                          "slices"      : slices },
                        repeat=12)
            
# Report the results
report()

