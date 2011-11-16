#!/usr/bin/env python

# Usage: ./full_test_driver.sh <branch-name>
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

import sys, subprocess32, time, os, traceback, socket, threading
import remotely

# Parse input

if __name__ != "__main__":
    raise ImportError("It doesn't make any sense to import this as a module")

(prog_name, git_branch) = sys.argv

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
print "The branch we are testing is:", git_branch
print "Our working directory is:", os.getcwd()
print "The machine we are on is:", socket.gethostname()
print "We are running as user:", os.getlogin()

class SynchronizedLog(object):
    def __init__(self, path):
        self.file = open(path, "w")
        self.lock = threading.Lock()
    def __enter__(self):
        return self
    def __exit__(self, type, value, traceback):
        self.file.close()
    def write(self, string):
        with self.lock:
            print >>self.file, string
            self.file.flush()

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
if not int(os.environ.get("SKIP_BUILDS", 0)):

    # Check out git repository

    print "Checking out RethinkDB..."

    subprocess32.check_call(["git", "clone", "git@github.com:coffeemug/rethinkdb.git", "--depth", "0", "rethinkdb"])
    subprocess32.check_call(["git", "checkout", git_branch], cwd="rethinkdb")

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

    # Run builds

    os.mkdir("builds")
    with SynchronizedLog("build-log.txt") as build_log:
        output_lock = threading.Lock()
        def run_build(build):
            try:
                with open("builds/%s.txt" % build.name, "w") as output:
                    try:
                        remotely.run(
                            command_line = build.command_line,
                            stdout = output,
                            inputs = ["rethinkdb"],
                            outputs = build.products,
                            on_begin_script = lambda: build_log.write("begin %s %s" % (build.name, time.ctime())),
                            on_end_script = lambda: build_log.write("pass %s %s" % (build.name, time.ctime()))
                            )
                    except remotely.ScriptFailedError:
                        build_log.write("fail %s %s" % (build.name, time.ctime()))
            except Exception, e:
                build_log.write("bug %s %s" % (build.name, time.ctime()))
                with output_lock:
                    print >>sys.stderr, "Bug when running build %s:" % build.name
                    traceback.print_exc()
        run_in_threads((lambda: run_build(b)) for b in builds)

# Plan what tests to run

class Test(object):
    def __init__(self, name, inputs, command_line):
        self.name = name
        self.inputs = inputs
        self.command_line = command_line

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

    for i in xrange(repeat):
        test_name = str(test_counter)
        test_counter += 1
        print test_name + ":", repr(command_line)
        tests.append(Test(test_name, inputs, command_line))

for (dirpath, dirname, filenames) in os.walk("rethinkdb/test/full_test/"):
    for filename in filenames:
        full_path = os.path.join(dirpath, filename)
        print "Specification file %r:" % full_path
        try:
            execfile(full_path, {"__builtins__": __builtins__, "do_test_fog": do_test_fog, "ec2": False})
        except Exception, e:
            traceback.print_exc()

# Run tests

os.mkdir("tests")
with SynchronizedLog("test-log.txt") as test_log:
    output_lock = threading.Lock()
    def run_test(test):
        try:
            os.mkdir(os.path.join("tests", test.name))
            with open(os.path.join("tests", test.name, "output.txt"), "w") as output_file:
                # We must have an exit code of zero even if the test-script
                # fails so that `remotely` will copy `output_from_test` even if
                # the test script fails. So we have to trap a non-zero exit
                # status and communicate it some other way. We do this by
                # printing it to `stdout` with `exitcode:` before it. But then
                # we need a way to distinguish that from normal lines of
                # `stdout`. So then we use `sed` to put `stdout:` before every
                # line of real output. This is the same trick that `remotely`
                # uses internally; every line of script output is actually
                # transmitted over the network as `STDOUT:stdout:<actual line>`.
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
                demuxer = ExitCodeDemuxer(output_file)
                remotely.run(
                    """
set -e
mkdir -p tests/%(name)s/
cd tests/%(name)s/
export PYTHONUNBUFFERED=1
set +e
(../../rethinkdb/test/%(command)s 2>&1 | sed -u s/^/stdout:/)
echo "exitcode:$?"
""" % { "name": test.name, "command": test.command_line },
                    stdout = demuxer,
                    inputs = test.inputs,
                    outputs = [os.path.join("tests", test.name, "output_from_test")],
                    on_begin_script = lambda: test_log.write("begin %s %s" % (test.name, time.ctime())),
                    on_end_script = lambda: test_log.write("%s %s %s" % ("pass" if demuxer.exit_code == 0 else "fail", test.name, time.ctime()))
                    )
        except Exception, e:
            test_log.write("bug %s %s" % (test.name, time.ctime()))
            with output_lock:
                print >>sys.stderr, "Bug when running test %s:" % test.name
                traceback.print_exc()
    funs = []
    for test in tests:
        if all(os.path.exists(input) for input in test.inputs):
            funs.append(lambda test=test: run_test(test))
        else:
            test_log.write("ignore %s %s" % (test.name, time.ctime()))
    run_in_threads(funs)

print "Done."