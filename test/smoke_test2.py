#!/usr/bin/python
from retester import do_test, report

# Clean the repo
do_test("pwd; cd ../src/; make clean")

# Build everything
for mode in ["debug", "release"]:
    for checker in ["valgrind", None]:
        # Build our targets
        do_test("cd ../src/; make -j8",
                { "DEBUG"    : 1 if mode    == "debug"    else 0,
                  "VALGRIND" : 1 if checker == "valgrind" else 0 },
                cmd_format="make")

# Report the results
report()

