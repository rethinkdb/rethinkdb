#!/usr/bin/python
from retester import do_test, report

# Clean the repo
do_test("cd ../src/; make clean")

# Go through all our environments
for mode in ["debug", "release"]:
    for checker in ["valgrind", None]:
        # Build our targets
        do_test("cd ../src/; make -j8",
                { "DEBUG"    : 1 if mode    == "debug"    else 0,
                  "VALGRIND" : 1 if checker == "valgrind" else 0 },
                cmd_format="make")

        # Run protocol tests
        for protocol in ["text", "binary"]:
            do_test("integration/append_prepend.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=3)
            
            do_test("integration/cas.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=3)
            
            do_test("integration/flags.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=3)
            
            do_test("integration/incr_decr.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=3)
            
            do_test("integration/serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=20, timeout=20)
            
            do_test("integration/multi_serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "cores"       : 1,
                      "slices"      : 1,
                      "num-testers" : 16,
                      "duration"    : 30 },
                    repeat=3)
            
            do_test("integration/expiration.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=5, timeout=7)
            
            do_test("integration/multi_serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=20)
            
            do_test("integration/pipeline.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=3)
            
            do_test("integration/bin_pipeline.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=3)
            
            do_test("integration/many_keys.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "num-keys"    : 50000},
                    repeat=3, timeout=300)
            
            do_test("integration/serial_mix.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol,
                      "restart-server-prob" : "0.0005"},
                    repeat=20, timeout=20)
            
            do_test("integration/corruption.py",
                    { "auto"        : True,
                      "mode"        : mode,
                      "no-valgrind" : not checker,
                      "protocol"    : protocol },
                    repeat=12)
            
# Report the results
report()

