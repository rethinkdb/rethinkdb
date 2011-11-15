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
with open("build-log.txt", "w") as build_log:
    build_log_lock = threading.Lock()
    def run_build(build):
        with build_log_lock:
            print "Thread", threading.current_thread(), "entering"
        try:
            with open("builds/%s.txt" % build.name, "w") as output:
                def on_begin_script():
                    with build_log_lock:
                        print >>build_log, time.ctime(), "begin", build.name
                def on_end_script():
                    with build_log_lock:
                        print >>build_log, time.ctime(), "pass", build.name
                try:
                    remotely.run(
                        command_line = build.command_line,
                        stdout = output,
                        inputs = ["rethinkdb"],
                        outputs = build.products,
                        on_begin_script = on_begin_script,
                        on_end_script = on_end_script)
                except remotely.ScriptFailedError:
                    with build_log_lock:
                        print >>build_log, time.ctime(), "fail", build.name
        except Exception, e:
            with build_log_lock:
                print >>build_log, time.ctime(), "bug", build.name
                traceback.print_exc()
        with build_log_lock:
            print "Thread", threading.current_thread(), "exiting"
    threads = []
    for build in builds:
        thread = threading.Thread(target = run_build, args = (build,))
        thread.start()
        threads.append(thread)
    for thread in threads:
        with build_log_lock:
            print "Joining thread", thread
        thread.join()

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
        tests.append(Test(str(test_counter), inputs, command_line))
        test_counter += 1

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
with open("test-log.txt", "w") as test_log:
    test_log_lock = threading.Lock()
    thread_cap_semaphore = threading.Semaphore(100)
    def run_test(test):
        try:
            os.mkdir(os.path.join("tests", test.name))
            with open(os.path.join("tests", test.name, "output.txt")) as output_file:
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
                                self.file.write(line[line.find(":")+1:])
                            elif line.startswith("exitcode:"):
                                self.exit_code = int(line[line.find(":")+1:])
                            else:
                                raise ValueError("Bad line: %r" % line)
                        self.buffer = lines[-1]
                demuxer = ExitCodeDemuxer(output_file)
                def on_begin_script():
                    with test_log_lock:
                        print >>test_log, time.ctime(), "begin", test.name
                def on_end_script(exit_code):
                    with test_log_lock:
                        if demuxer.exit_code == 0:
                            print >>test_log, time.ctime(), "pass", test.name
                        else:
                            print >>test_log, time.ctime(), "fail", test.name
                remotely.run(
                    """
set -e
mkdir -p tests/%(name)s/
cd tests/%(name)s/
(../../rethinkdb/test/%(name)s 2>&1 | sed s//stdout:/)
echo "exitcode:$?"
""" % { "name": test.name},
                    stdout = demuxer,
                    inputs = test.inputs,
                    output = [os.path.join("tests", test.name, "output_from_test")],
                    on_begin_script = on_begin_script,
                    on_end_script = on_end_script
                    )
        except Exception, e:
            with test_log_lock:
                print >>test_log, time.ctime(), "bug", test.name
                traceback.print_exc()
        thread_cap_semaphore.release()
    threads = []
    for test in tests:
        if all(os.path.exists(input) for input in test.inputs):
            thread_cap_semaphore.acquire()
            thread = threading.Thread(target = run_test, args = (test,))
            thread.start()
            threads.append(thread)
        else:
            with test_log_lock:
                print >>test_log, time.ctime(), "didn't-try", test.name
    for thread in threads:
        thread.join()

