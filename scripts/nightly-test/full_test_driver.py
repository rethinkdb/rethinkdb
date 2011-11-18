#!/usr/bin/env python

# Usage: ./full_test_driver.sh [--just-tests] [--git-branch <branch-name>] [--build-mode (two|all)]
# Environment variables: SLURM_CONF
#
# Check out the given RethinkDB branch from GitHub and run a full test against
# it. The full test results are stored in the current working directory in the
# following locations:
#   * `./rethinkdb`: RethinkDB source tree
#   * `./rethinkdb/build/*/rethinkdb`, `./rethinkdb/bench/stress-client/stress`,
#     `./rethinkdb/bench/stress-client/libstress.so`: Build products
#   * `./build-list.txt`: Pass/fail information for all builds
#   * `./build/*.txt`: `stdout` and `stderr` from `make` during the build
#   * `./test-list.txt`: Pass/fail information for all tests
#   * `./test/*/output.txt`: Things that the test script printed
#   * `./test/*/output_from_test/`: The `output_from_test` directory
#     from the test scripts that were run

import sys, subprocess32, time, os, traceback, socket, threading, optparse
import remotely, simple_linear_db

# Parse input

if __name__ != "__main__":
    raise ImportError("It doesn't make any sense to import this as a module")

parser = optparse.OptionParser()
parser.add_option("--build-mode", action = "store", choices = ["minimal", "all"], dest = "build_mode")
parser.add_option("--git-branch", action = "store", dest = "git_branch")
parser.add_option("--just-tests", action = "store_true", dest = "just_tests")
parser.set_defaults(build_mode = "all", git_branch = "master")
(options, args) = parser.parse_args()
assert not args

# We want to flush output aggressively so that we can report the most up-to-date
# information possible to anyone following this build. The easiest way to do
# this is to wrap `sys.stdout`.
class Flusher(object):
    def __init__(self, f):
        self.f = f
    def write(self, text):
        self.f.write(text)
        self.f.flush()
sys.stdout = Flusher(sys.stdout)

print "Full test begun."
print "The branch we are testing is:", options.git_branch
print "Our working directory is:", os.getcwd()
print "The machine we are on is:", socket.gethostname()
print "We are running as user:", os.getlogin()

def run_in_threads(functions, max = 10):
    """Run the given functions each in their own thread. Run at most `max` at
    the same time."""
    cap_semaphore = threading.Semaphore(max)
    def run(fun):
        try:
            fun()
        finally:
            cap_semaphore.release()
    threads = []
    try:
        for fun in functions:
            # For some reason calling `cap_semaphore.acquire(blocking = True)`
            # doesn't work when we get a `SIGINT`. This works better. I don't
            # know why.
            while not cap_semaphore.acquire(blocking = False):
                time.sleep(1)
            th = threading.Thread(target = run, args = (fun,))
            th.start()
            threads.append(th)
    finally:
        for thread in threads:
            thread.join()

# For debugging purposes
if not options.just_tests:

    # Check out git repository

    print "Checking out RethinkDB..."

    subprocess32.check_call(["git", "clone", "git@github.com:coffeemug/rethinkdb.git", "--depth", "0", "rethinkdb"])
    subprocess32.check_call(["git", "checkout", options.git_branch], cwd="rethinkdb")

    print "Done checking out RethinkDB."

    rethinkdb_version = subprocess32.check_output(["scripts/gen-version.sh"], cwd="rethinkdb").strip()
    print "RethinkDB version:", rethinkdb_version

    rethinkdb_shortversion = subprocess32.check_output(["scripts/gen-version.sh", "-s"], cwd="rethinkdb").strip()
    print "RethinkDB version (short):", rethinkdb_shortversion

    # Plan what builds to run

    class Build(object):
        def __init__(self, name, command_line, products):
            self.name = name
            self.command_line = command_line
            self.products = products

    builds = []

    if options.build_mode == "all":
        builds.append(Build(
            "stress-client",
            "cd rethinkdb/bench/stress-client; make -j12",
            ["rethinkdb/bench/stress-client/stress", "rethinkdb/bench/stress-client/libstress.so"]))
        builds.append(Build(
            "serializer-bench",
            "cd rethinkdb/bench/serializer-bench; make -j12",
            []))
        builds.append(Build(
            "deb",
            "cd rethinkdb/src; make -j12 deb",
            ["rethinkdb/build/packages/rethinkdb-%s_%s_amd64.deb" % (rethinkdb_shortversion, rethinkdb_version+"-1"),
             "rethinkdb/build/packages/rethinkdb-%s_%s_amd64.deb" % (rethinkdb_shortversion, rethinkdb_version+"-unstripped-1"),
             ]))
        builds.append(Build(
            "rpm",
            "cd rethinkdb/src; make -j12 rpm",
            ["rethinkdb/build/packages/rethinkdb-%s-%s-1.x86_64.rpm" % (rethinkdb_shortversion, (rethinkdb_version+"-1").replace("-", "_")),
             "rethinkdb/build/packages/rethinkdb-%s-%s-1.x86_64.rpm" % (rethinkdb_shortversion, (rethinkdb_version+"-unstripped-1").replace("-", "_"))
             ]))
        config_bits = [
            ("DEBUG",            [(0, "release"), (1, "debug")     ]),
            ("MOCK_CACHE_CHECK", [(0, ""),        (1, "-mockcache")]),
            ("NO_EPOLL",         [(0, ""),        (1, "-noepoll")  ]),
            ("VALGRIND",         [(0, ""),        (1, "-valgrind") ]),
            ]
        def generate_configs(configs, make_flags, config_name):
            if configs:
                (make_flag, options) = configs[0]
                configs = configs[1:]
                for (make_flag_value, config_name_part) in options:
                    generate_configs(configs,
                        make_flags + ["%s=%s" % (make_flag, make_flag_value)],
                        config_name + config_name_part)
            else:
                product_path = "rethinkdb/build/%s/rethinkdb" % config_name
                builds.append(Build(
                    config_name,
                    "cd rethinkdb/src; make -j12 " + " ".join(make_flags),
                    [product_path]))
        generate_configs(config_bits, [], "")

    elif options.build_mode == "minimal":
        builds.append(Build(
            "stress-client",
            "cd rethinkdb/bench/stress-client; make -j12",
            ["rethinkdb/bench/stress-client/stress", "rethinkdb/bench/stress-client/libstress.so"]))
        builds.append(Build(
            "debug-valgrind",
            "cd rethinkdb/src; make -j12 VALGRIND=1",
            ["rethinkdb/build/debug-valgrind/rethinkdb"]))
        builds.append(Build(
            "release",
            "cd rethinkdb/src; make -j12 DEBUG=0",
            ["rethinkdb/build/release/rethinkdb"]))

    # Run builds

    os.mkdir("builds")
    with simple_linear_db.LinearDBWriter("build-log.txt") as build_log:
        def run_build(build):
            try:
                with open("builds/%s.txt" % build.name, "w") as output:
                    try:
                        remotely.run(
                            command_line = build.command_line,
                            stdout = output,
                            inputs = ["rethinkdb"],
                            outputs = build.products,
                            on_begin_script = lambda: build_log.write(build.name, status = "running", start_time = time.time()),
                            on_end_script = lambda: build_log.write(build.name, status = "ok", end_time = time.time())
                            )
                    except remotely.ScriptFailedError:
                        build_log.write(build.name, status = "fail", end_time = time.time())
            except Exception, e:
                build_log.write(build.name, status = "bug", end_time = time.time(), traceback = traceback.format_exc())
        for build in builds:
            build_log.write(build.name, status = "waiting", command_line = b.command_line)
        run_in_threads((lambda: run_build(b)) for b in builds)

# Plan what tests to run

class Test(object):
    def __init__(self, name, inputs, command_line, repeats):
        self.name = name
        self.inputs = inputs
        self.command_line = command_line
        self.repeats = repeats

tests = []
test_counter = 1

def do_test_fog(executable, arguments, repeat=1, timeout=60):
    global test_counter

    # The `timeout` parameter is ignored

    inputs = []

    inputs.append("rethinkdb/test")

    # We assume that all tests need `stress.py` and `libstress.so`. This is more
    # conservative than necessary, but it's not a big deal.
    inputs.append("rethinkdb/bench/stress-client/stress.py")
    inputs.append("rethinkdb/bench/stress-client/libstress.so")
    inputs.append("rethinkdb/bench/stress-client/stress")

    # Try to infer which version(s) of the server the test needs on the basis of
    # the command line.
    inputs.append("rethinkdb/build/" + arguments.get("mode", "debug") +
        ("-valgrind" if not arguments.get("no-valgrind", False) else "") +
        "/rethinkdb")
    if executable == "integration/corruption.py" and arguments.get("no-valgrind", False):
        # The corruption test always uses the no-valgrind version of RethinkDB
        # in addition to whatever version is specified on the command line
        inputs.append("rethinkdb/build/" + arguments.get("mode", "debug") + "/rethinkdb")

    # Try to figure out how many cores the test will use
    # cores = arguments.get("cores", "exclusive")

    # Put together test command line
    command_line = [executable]
    for (key, value) in arguments.iteritems():
        if isinstance(value, bool):
            if value:
                command_line.append("--%s" % key)
        else:
            command_line.append("--%s" % key)
            command_line.append(str(value))
    command_line = " ".join(remotely.escape_shell_arg(x) for x in command_line)

    tests.append(Test(str(test_counter), inputs, command_line, repeat))
    test_counter += 1

for (dirpath, dirname, filenames) in os.walk("rethinkdb/test/full_test/"):
    for filename in filenames:
        full_path = os.path.join(dirpath, filename)
        print "Reading specification file %r" % full_path
        try:
            execfile(full_path, {"__builtins__": __builtins__, "do_test_fog": do_test_fog, "ec2": False})
        except Exception, e:
            traceback.print_exc()

# Run tests

def run_test_remotely(command, stdout = sys.stdout, inputs = [], outputs = [], on_begin_script = lambda: None, on_end_script = lambda rc: None):
    """Runs the given command remotely. The difference between this and
    `remotely.run()` is that if the remote command returns a non-zero exit
    status, `remotely.run()` will throw an exception and will not copy the
    outputs, but this will still copy the outputs and will report failures by
    passing the return code to `on_end_script()`."""
    # We must have an exit code of zero even if the test-script fails so that
    # `remotely` will copy `output_from_test` even if the test script fails. So
    # we have to trap a non-zero exit status and communicate it some other way.
    # We do this by printing it to `stdout` with `exitcode:` before it. But then
    # we need a way to distinguish that from normal lines of `stdout`. So then
    # we use `sed` to put `stdout:` before every line of real output. This is
    # the same trick that `remotely` uses internally; every line of script
    # output is actually transmitted over the network as
    # `STDOUT:stdout:<actual line>`.
    class ExitCodeDemuxer(object):
        def __init__(self, f):
            self.file = f
            self.buffer = ""
        def write(self, text):
            self.buffer += text
            lines = self.buffer.split("\n")
            for line in lines[:-1]:
                if line.startswith("stdout:"):
                    self.file.write(line[line.find(":")+1:]+"\n")
                elif line.startswith("exitcode:"):
                    self.exit_code = int(line[line.find(":")+1:].strip())
                else:
                    raise ValueError("Bad line: %r" % line)
            self.buffer = lines[-1]
            if len(lines) > 1:
                self.file.flush()
    demuxer = ExitCodeDemuxer(stdout)
    remotely.run("""
set +e
((%s) 2>&1 | sed -u s/^/stdout:/)
echo "exitcode:$?"
""" % command,
        stdout = demuxer,
        inputs = inputs,
        outputs = outputs,
        on_begin_script = on_begin_script,
        on_end_script = lambda: on_end_script(demuxer.exit_code)
        )

os.mkdir("tests")
with simple_linear_db.LinearDBWriter("test-log.txt") as test_log:
    def run_test(test, round):
        try:
            directory = os.path.join("tests", test.name, str(round))
            os.mkdir(directory)
            with open(os.path.join(directory, "output.txt"), "w") as output_file:
                run_test_remotely(
                    """
set -e
mkdir -p %(directory)s
cd %(directory)s
PYTHONUNBUFFERED=1 %(command)s
""" % { "directory": directory, "command": test.command_line },
                    stdout = output_file,
                    inputs = test.inputs,
                    outputs = [os.path.join("tests", test.name, "output_from_test")],
                    on_begin_script = lambda: test_log.write(test.name, "rounds", round, status = "running", start_time = time.time()),
                    on_end_script = lambda rc: test_log.write(test.name, "rounds", round, status = "pass" if rc == 0 else "fail", end_time = time.time())
                    )
        except Exception, e:
            test_log.write(test.name, "rounds", round, status = "bug", traceback = traceback.format_exc())
    funs = []
    for test in tests:
        test_log.write(test.name, command_line = test.command_line)
        missing_prereqs = [i for i in test.inputs if not os.path.exists(i)]
        if not missing_prereqs:
            for i in xrange(test.repeats):
                test_log.write(test.name, "rounds", i, status = "waiting")
                funs.append(lambda test=test, i=i: run_test(test, i))
        else:
            test_log.write(test.name, missing = missing_prereqs)
    run_in_threads(funs)

print "Done."
