#!/usr/bin/python
import os
import glob
from cloud_retester import do_test, do_test_cloud, report_cloud, setup_testing_nodes, terminate_testing_nodes

ec2 = 4

# Clean the repo
do_test("cd ../src/; make clean")

# Make every target first, as some tests depend on multiple targets
for mode in ["debug", "release"]:
    for checker in ["valgrind", None]:
        for mock_cache in [True, False]:
            for poll_mode in ["poll", "epoll"]:
                # Build our targets
                do_test("cd ../src/; make -j 9",
                        { "DEBUG"            : 1 if mode    == "debug"    else 0,
                          "VALGRIND"         : 1 if checker == "valgrind" else 0,
                          "MOCK_CACHE_CHECK" : 1 if mock_cache            else 0,
                          "NO_EPOLL"         : 1 if poll_mode == "poll"   else 0 },
                        cmd_format="make", timeout=300)

# Make sure auxillary tools compile
do_test("cd ../bench/stress-client/; make clean; make -j 9 MYSQL=0 LIBMEMCACHED=0 LIBGSL=0 stress libstress.so",
        {},
        cmd_format="make")
do_test("cd ../bench/serializer-bench/; make clean; make -j 9",
        {},
        cmd_format="make")

# Running canonical tests
def run_canonical_tests(mode, checker, protocol, cores, slices):
    sigint_timeout = 360 if "mock" in mode else 60
    do_test_cloud("integration/multi_serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "num-testers" : 16,
                    "duration"    : 420,
                    "sigint-timeout" : sigint_timeout },
                  repeat=5, timeout = 480 + sigint_timeout)

    do_test_cloud("integration/multi_serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "num-testers" : 16,
                    "memory"      : 12,
                    "diff-log-size" : 2,
                    "duration"    : 420,
                    "sigint-timeout" : sigint_timeout },
                  repeat=5, timeout = 540 + sigint_timeout)

    do_test_cloud("integration/big_values.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "sigint-timeout" : sigint_timeout },
                  repeat=3, timeout = 320 + sigint_timeout)

    do_test_cloud("integration/pipeline.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "chunk-size"  : 1000 if (mode == "release" and not checker) else 100,
                    "num-ints"    : 1000000 if (mode == "release" and not checker) else 1000,
                    "sigint-timeout" : sigint_timeout,
                    "force-timeout" : 720 },
                  repeat=3, timeout = 180 * ec2)

    # Don't run the corruption test in mockcache mode because in that modes
    # we don't flush to disk at all until the server is shut down, so obviously the
    # corruption test will fail.
    if "mock" not in mode:
        for dur in [2, 5, 10, 60, 300, 420]:
            do_test_cloud("integration/corruption.py",
                          { "auto"          : True,
                            "mode"          : mode,
                            "no-valgrind"   : not checker,
                            "protocol"      : protocol,
                            "cores"         : cores,
                            "slices"        : slices,
                            "duration"      : dur,
                            "verify-timeout": dur * 5 * ec2 + 740 },
                          repeat=2, timeout=dur * 5 * ec2 + 740)

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
                  repeat=3, timeout=1200 * ec2)

    # Run a canonical workload for half hour
    do_test_cloud("integration/stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 1800,
                    "fsck"        : False},
                  repeat=2, timeout=2400 * ec2)

    # Run an modify-heavy workload for half hour
    do_test_cloud("integration/stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 1800,
                    "ndeletes"    : 1,
                    "nupdates"    : 5,
                    "ninserts"    : 10,
                    "nreads"      : 1,
                    "fsck"        : False},
                  repeat=2, timeout=2400)

    do_test_cloud("integration/stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 1800,
                    "memory"      : 1000,
                    "mem-cap"     : 1200,
                    "ndeletes"    : 0,
                    "nupdates"    : 0,
                    "ninserts"    : 1,
                    "nreads"      : 0,
                    "fsck"        : False,
                    #"min-qps"     : 20 # a very reasonable limit #this is temporarily disabled because cmd_set_persec isn't available on servers
                    },
                  repeat=2, timeout=2400)

    # Run a canonical + rget workload for twenty minutes
    do_test_cloud("integration/stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 1200,
                    "nrgets"      : 1,
                    "fsck"        : False},
                  repeat=2, timeout=2400 * ec2)

    # Run a canonical + rget workload for twenty minutes under memory pressure
    do_test_cloud("integration/stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "memory"      : 8,
                    "diff-log-size" : 4,
                    "duration"    : 1200,
                    "nrgets"      : 1,
                    "fsck"        : False},
                  repeat=2, timeout=2400 * ec2)

    do_test_cloud("integration/replication_stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 1800,
                    "ndeletes"    : 0,
                    "nupdates"    : 0,
                    "ninserts"    : 1,
                    "nreads"      : 0,
                    "fsck"        : False,
                    #"min-qps"     : 20 # a very reasonable limit #this is temporarily disabled because cmd_set_persec isn't available on servers
                    },
                  repeat=3, timeout=2400)

    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 340 },
                  repeat=5, timeout=460 * ec2)

    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 60,
                    "fsck"        : True},
                  repeat=5, timeout=600)


    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "duration"    : 10,
                    "cores"       : cores,
                    "slices"      : 2 * slices,
                    "fsck"        : True,
                    "num-files"   : 2 },
                  repeat=1, timeout=120)

    # Regression test for https://github.com/coffeemug/rethinkdb/issues/269 and https://github.com/coffeemug/rethinkdb/issues/267
    do_test_cloud("integration/multi_serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "memory"      : 1,
                    "diff-log-size" : 0,
                    "serve-flags" : "--flush-timer 0 --wait-for-flush y",
                    "num-testers" : 128,
                    "duration"    : 420 },
                  repeat=3, timeout = 600)

    # TODO: This should really only be run under one environment...
    if not checker:
        do_test_cloud("regression/gc_verification.py",
                      { "auto"        : True,
                        "mode"        : mode,
                        "no-valgrind" : True,
                        "protocol"    : protocol,
                        "cores"       : cores,
                        "slices"      : slices,
                        "duration"    : 400 },
                      repeat=3, timeout=600)

    # Run the serial mix test also with drd
    if checker == "valgrind":
        for valgrind_tool in ["drd"]:
            # Use do_test() instead of do_test_cloud() because DRD produces hundreds of apparently
            # bogus errors when run on EC2, but runs just fine locally.
            do_test("integration/serial_mix.py",
                          { "auto"        : True,
                            "mode"        : mode,
                            "no-valgrind" : not checker,
                            "valgrind-tool" : valgrind_tool,
                            "protocol"    : protocol,
                            "cores"       : cores,
                            "slices"      : slices,
                            "duration"    : 300 },
                          repeat=3, timeout=400)

            do_test("integration/multi_serial_mix.py",
                    { "auto" : True,
                      "mode": mode,
                      "no-valgrind" : not checker,
                      "valgrind-tool" : valgrind_tool,
                      "protocol" : protocol,
                      "cores" : cores,
                      "slices" : slices,
                      "duration" : 120 },
                    repeat=2, timeout=200)

    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 340,
                    "restart-server-prob" : "0.0005"},
                          repeat=5, timeout=600)

    # Replication
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 340,
                    "failover"    : True},
                          repeat=10, timeout=800)

    # Replication with shorter heartbeat timeout (to provoke disconnects)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 340,
                    "serve-flags" : "--heartbeat-timeout 10",
                    "failover"    : True},
                          repeat=10, timeout=800)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 340,
                    "serve-flags" : "--heartbeat-timeout 4",
                    "failover"    : True},
                          repeat=5, timeout=800)
    # ...and one more if we don't use Valgrind (otherwise stuff is too slow for this to work)
    if not checker:
        do_test_cloud("integration/serial_mix.py",
                      { "auto"        : True,
                        "mode"        : mode,
                        "no-valgrind" : not checker,
                        "protocol"    : protocol,
                        "cores"       : cores,
                        "slices"      : slices,
                        "duration"    : 340,
                        "serve-flags" : "--heartbeat-timeout 1",
                        "failover"    : True},
                              repeat=5, timeout=800)

    # Replication with large values
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "valuesize"   : 2 * 1024 * 1024,
                    "duration"    : 340,
                    "failover"    : True,
                    "kill-failover-server-prob": 0.1,
                    "resurrect-failover-server-prob": 0.1},
                          repeat=5, timeout=800)

    # Replication with CAS
    do_test_cloud("integration/cas.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "failover"    : True,
                    "kill-failover-server-prob": 0.1,
                    "resurrect-failover-server-prob": 0.1,
                    "timeout"     : 800},
                          repeat=5, timeout=800)

    # Replication with flags
    do_test_cloud("integration/flags.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "failover"    : True,
                    "kill-failover-server-prob": 0.1,
                    "resurrect-failover-server-prob": 0.1,
                    "timeout"     : 240},
                          repeat=5, timeout=800)

    do_test_cloud("integration/append_prepend.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices },
                  repeat=3, timeout=120)

    do_test_cloud("integration/append_stress.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices },
                  repeat=3, timeout=270)

    do_test_cloud("integration/cas.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices },
                  repeat=3, timeout=240)

    do_test_cloud("integration/flags.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices },
                  repeat=3, timeout=240)

    do_test_cloud("integration/shrink.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices },
                  repeat=3, timeout=240)

    do_test_cloud("integration/incr_decr.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices },
                  repeat=3, timeout=240)

    do_test_cloud("integration/expiration.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices },
                  repeat=5, timeout=240)

    do_test_cloud("integration/fuzz.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 340},
                  repeat=3, timeout=500)

    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "serve-flags" : "--flush-timer 50",
                    "duration"    : 60},
                  repeat=1, timeout = 240)

    do_test_cloud("integration/rget.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=90*ec2)

    do_test_cloud("integration/rget_no_blocking.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=90*ec2)

    do_test_cloud("integration/command_line_sanitizing.py",
                  { "auto"        : False,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=180 * ec2)

    do_test_cloud("regression/issue_69.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=240)

    do_test_cloud("regression/issue_90.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=240)

    do_test_cloud("regression/issue_95.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=240)

    do_test_cloud("regression/issue_133.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=10, timeout=30 * ec2)

    do_test_cloud("regression/issue_197.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=30 * ec2)

    do_test_cloud("regression/issue_327.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices},
                  repeat=1, timeout=60 * ec2)

    do_test_cloud("integration/replication.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 340 },
                  repeat=5, timeout=460 * ec2)

    do_test_cloud("integration/replication_noise.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 90
                  },
                  repeat=1, timeout=150*ec2)

    for suite_test in os.listdir('integration/memcached_suite'):
        if not suite_test.endswith(".t"): continue

        do_test_appropriately = do_test_cloud
        if suite_test.endswith('expirations.t'): # EC2 seems to be too slow for expirations.t some of the time.
            do_test_appropriately = do_test

        do_test_appropriately("integration/memcached_suite.py",
                      { "auto"        : True,
                        "mode"        : mode,
                        "no-valgrind" : not checker,
                        "protocol"    : protocol,
                        "cores"       : cores,
                        "slices"      : slices,
                        "sigint-timeout" : 1020,
                        "suite-test"  : suite_test},
                      repeat=3, timeout=1000)

    # Test different command line configurations (defaults less thoroughly)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "restart-server-prob" : "0.002",
                    "serve-flags" : "--read-ahead y"},
                          repeat=3, timeout=600)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "restart-server-prob" : "0.002",
                    "serve-flags" : "--read-ahead n"},
                          repeat=1, timeout=600)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "restart-server-prob" : "0.002",
                    "serve-flags" : "--flush-concurrency 20 --flush-timer 500"},
                          repeat=3, timeout=600)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "restart-server-prob" : "0.002",
                    "serve-flags" : "--flush-concurrency 1 --flush-timer 500"},
                          repeat=1, timeout=600)
    do_test_cloud("integration/stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "serve-flags" : "--flush-concurrency 20 --flush-timer 500",
                    "fsck"        : False},
                  repeat=3, timeout=300 * ec2)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "restart-server-prob" : "0.002",
                    "serve-flags" : "--io-backend pool"},
                          repeat=3, timeout=300)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "restart-server-prob" : "0.002",
                    "serve-flags" : "--io-backend native"},
                          repeat=1, timeout=300)
    do_test_cloud("integration/stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "serve-flags" : "--io-backend pool",
                    "fsck"        : False},
                  repeat=3, timeout=300 * ec2)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "diff-log-size" : 0,
                    "restart-server-prob" : "0.002"},
                          repeat=3, timeout=300)
    do_test_cloud("integration/serial_mix.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "diff-log-size" : 8,
                    "restart-server-prob" : "0.002"},
                          repeat=1, timeout=300)
    do_test_cloud("integration/stress_load.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : cores,
                    "slices"      : slices,
                    "duration"    : 180,
                    "diff-log-size" : 0,
                    "fsck"        : False},
                  repeat=3, timeout=300 * ec2)

    do_test_cloud("integration/failover_script.py",
                  { "auto"        : True,
                    "mode"        : mode,
                    "no-valgrind" : not checker,
                    "protocol"    : protocol,
                    "cores"       : 1,
                    "slices"      : 1,
                    "duration"    : 180,
                    "diff-log-size" : 0,
                    "fsck"        : False,
                    "failover-script" : "assets/failover_script_test.py"
                    },
                  repeat=1, timeout=300 * ec2)



    # Canonical tests are included in all tests
    run_canonical_tests(mode, checker, protocol, cores, slices)

# Tests with special requirements that will supply their own mode and such.
def run_fixed_tests():
    # This test takes too long in debug mode, unfortunately, so we won't run it
    # nightly for now.
    #for (mode,checker) in [("debug", None),
    #                       ("debug", "valgrind"),
    #                       ("release", None)]:
    #    for (max_key,key_len,val_len,pattern) in [("5000",   "5","45", "fwd"),
    #                                              ("50000",  "5","45", "fwd"),
    #                                              ("50000",  "5","45", "rev"),
    #                                              ("30000",  "5","45", "fwd"),
    #                                              ("50000",  "6","45", "fwd"),
    #                                              ("50000",  "5","230","mid"),
    #                                              ("1000000","7","230","midrev")]:
    #        do_test_cloud("integration/deletion.py",
    #                      { "auto"        : True,
    #                        "mode"        : mode,
    #                        "no-valgrind" : not checker,
    #                        "protocol"    : "text",
    #                        "cores"       : 1,
    #                        "slices"      : 1,
    #                        "max-key"     : max_key,
    #                        "key-len"     : key_len,
    #                        "val-len"     : val_len,
    #                        "pattern"     : pattern },
    #                      repeat=1, timeout=2400) # TODO: Decrease this once we see how long these tests actually take.
    pass

# For safety: ensure that nodes are terminated in case of exceptions
try:
    # Setup the EC2 testing nodes
    setup_testing_nodes()

    canonical_modes = [("debug", "valgrind"), ("release", None)]
    canonical_cores_slices = [(2, 8)]

    # GO THROUGH CANONICAL ENVIRONMENTS
    for (mode, checker) in canonical_modes:
        for protocol in ["text"]:
            for (cores, slices) in canonical_cores_slices:
                # RUN ALL TESTS
                run_all_tests(mode, checker, protocol, cores, slices)

    run_fixed_tests()

    # GO THROUGH ALL OUR ENVIRONMENTS
    for (mode, checker) in [
            ("debug", "valgrind"),
            ("release", "valgrind"),
            ("release", None),
            ("debug-mockcache", "valgrind"),
            ("debug-noepoll", "valgrind")]:
        for protocol in ["text"]:
            for (cores, slices) in [(1, 1), (2, 8)]:
                if not ((mode, checker) in canonical_modes and (cores, slices) in canonical_cores_slices):
                    # RUN CANONICAL TESTS
                    run_canonical_tests(mode, checker, protocol, cores, slices)

    # Report the results
    report_cloud()

finally:
    terminate_testing_nodes()

