#!/usr/bin/python
from retester import do_test, report

# Clean the repo
do_test("cd ../src/; make clean")

# Build everything
for mode in ["debug", "release"]:
    for checker in ["valgrind", None]:
        # Build our targets
        do_test("cd ../src/; make -j8",
                { "DEBUG"    : 1 if mode    == "debug"    else 0,
                  "VALGRIND" : 1 if checker == "valgrind" else 0 },
                cmd_format="make")

# Do quick smoke tests in some environments
for (mode, checker, protocol) in [("debug", "valgrind", "text"),
                                  ("release", None, "text")]:
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
              "protocol"    : protocol },
            timeout=7)
    
    do_test("integration/many_keys.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol })

    do_test("integration/serial_mix.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol },
            repeat=10, timeout=20)
    
    do_test("integration/serial_mix.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol,
              "restart-server-prob" : "0.0005"},
            repeat=10, timeout=20)
    
    do_test("integration/multi_serial_mix.py",
            { "auto"        : True,
              "mode"        : mode,
              "no-valgrind" : not checker,
              "protocol"    : protocol },
            repeat=10, timeout=20)
    
# Report the results
report()

