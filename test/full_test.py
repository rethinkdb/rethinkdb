#!/usr/bin/python
from retester import do_test, report

for mode in ["debug", "release"]:
    for checker in ["valgrind", None]:
        # Build our targets
        do_test("cd ../src/; make -j8",
                { "DEBUG"    : 1 if mode    == "debug"    else 0,
                  "VALGRIND" : 1 if checker == "valgrind" else 0 },
                cmd_format="make")

        # Run protocol tests
        for protocol in ["text", "binary"]:
            # Many clients hitting one slice
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
            
# Report the results
report()

