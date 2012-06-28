#!/usr/bin/env python

# Usage: ./full_test_driver.sh [--just-tests] [--git-branch <branch-name>] [--build-mode (two|all)]
# Environment variables: SLURM_CONF
#
# Check out the given RethinkDB branch from GitHub and run a full test against
# it. The full test results are stored in the current working directory in a
# format that can be parsed by the `renderer` script.

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

def run_rethinkdb_test_remotely(dependencies, command_line, stdout_file, zipfile_path, on_begin_transfer = lambda: None, on_begin_script = lambda: None, on_end_script = lambda status: None, constraint = None):
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
        def __init__(self):
            self.buffer = ""
        def write(self, text):
            self.buffer += text
            lines = self.buffer.split("\n")
            for line in lines[:-1]:
                if line.startswith("stdout:"):
                    stdout_file.write(line[line.find(":")+1:]+"\n")
                elif line.startswith("exitcode:"):
                    self.exit_code = int(line[line.find(":")+1:].strip())
                else:
                    raise ValueError("Bad line: %r" % line)
            self.buffer = lines[-1]
            if len(lines) > 1:
                stdout_file.flush()
    demuxer = ExitCodeDemuxer()
    remotely.run("""
set +e
export RETHINKDB=$(pwd)/rethinkdb/
mkdir output_from_test
echo "stdout:About to start test..."
(cd output_from_test; echo "stdout:$PWD"; PYTHONUNBUFFERED=1 %(command_line)s 2>&1 | sed -u s/^/stdout:/)
echo "exitcode:$?"
zip -r %(zipfile_name)s output_from_test >/dev/null
""" % { "command_line": command_line, "zipfile_name": os.path.basename(zipfile_path) },
        stdout = demuxer,
        inputs = dependencies,
        outputs = [os.path.basename(zipfile_path)],
        output_root = os.path.dirname(zipfile_path),
        on_begin_transfer = on_begin_transfer,
        on_begin_script = on_begin_script,
        on_end_script = lambda: on_end_script("pass" if demuxer.exit_code == 0 else "fail"),
        constraint = constraint
        )

with simple_linear_db.LinearDBWriter("result_log.txt") as result_log:

    # For debugging purposes
    if not options.just_tests:

        # Check out git repository

        print "Checking out RethinkDB..."

        subprocess32.check_call(["git", "clone", "git@github.com:rethinkdb/rethinkdb.git", "--depth", "0", "rethinkdb"])
        subprocess32.check_call(["git", "checkout", options.git_branch], cwd="rethinkdb")

        print "Done checking out RethinkDB."

        rethinkdb_version = subprocess32.check_output(["scripts/gen-version.sh"], cwd="rethinkdb").strip()
        print "RethinkDB version:", rethinkdb_version

        rethinkdb_shortversion = subprocess32.check_output(["scripts/gen-version.sh", "-s"], cwd="rethinkdb").strip()
        print "RethinkDB version (short):", rethinkdb_shortversion

        # Plan what builds to run

        builds = { }

        if options.build_mode == "all":
            builds["stress-client"] = {
                "command_line": "cd rethinkdb/bench/stress-client; make -j12",
                "products": ["rethinkdb/bench/stress-client/stress", "rethinkdb/bench/stress-client/libstress.so"]
                }
            builds["serializer-bench"] = {
                "command_line": "cd rethinkdb/bench/serializer-bench; make -j12",
                "products": []
                }
            builds["deb"] = {
                "command_line": "cd rethinkdb/src; make -j12 deb",
                "products": [
                    "rethinkdb/build/packages/rethinkdb-%s_%s_amd64.deb" % (rethinkdb_shortversion, rethinkdb_version+"-1"),
                    "rethinkdb/build/packages/rethinkdb-%s_%s_amd64.deb" % (rethinkdb_shortversion, rethinkdb_version+"-unstripped-1"),
                    ]
                }
            builds["rpm"] = {
                "command_line": "cd rethinkdb/src; make -j12 rpm",
                "products": [
                    "rethinkdb/build/packages/rethinkdb-%s-%s-1.x86_64.rpm" % (rethinkdb_shortversion, (rethinkdb_version+"-1").replace("-", "_")),
                    "rethinkdb/build/packages/rethinkdb-%s-%s-1.x86_64.rpm" % (rethinkdb_shortversion, (rethinkdb_version+"-unstripped-1").replace("-", "_"))
                    ]
                }
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
                    build_path = "rethinkdb/build/%s" % config_name
                    products = [build_path + "/rethinkdb", build_path + "/web/"]
                    if "DEBUG=0" not in make_flags:
                        products = products + [build_path + "/rethinkdb-unittest"]
                    builds[config_name] = {
                        "command_line": "cd rethinkdb/src; make -j12 " + " ".join(make_flags),
                        "products": products
                        }
            generate_configs(config_bits, [], "")

        elif options.build_mode == "minimal":
            builds["stress-client"] = {
                "command_line": "cd rethinkdb/bench/stress-client; make -j12",
                "products": ["rethinkdb/bench/stress-client/stress", "rethinkdb/bench/stress-client/libstress.so"]
                }
            builds["debug-valgrind"] = {
                "command_line": "cd rethinkdb/src; make -j12 VALGRIND=1",
                "products": ["rethinkdb/build/debug-valgrind/rethinkdb"]
                }
            builds["release"] = {
                "command_line": "cd rethinkdb/src; make -j12 DEBUG=0",
                "products": ["rethinkdb/build/release/rethinkdb"]
                }

        # Run builds

        subprocess32.check_call(["tar", "--create", "--gzip", "--file=rethinkdb.tar.gz", "--", "rethinkdb"])

        os.mkdir("builds")
        def run_build(name, build):
            try:
                with open("builds/%s.txt" % name, "w") as output:
                    try:
                        remotely.run(
                            command_line = """
tar --extract --gzip --touch --file=rethinkdb.tar.gz -- rethinkdb
(%(command_line)s) 2>&1
""" % { "command_line": build["command_line"] },
                            stdout = output,
                            inputs = ["rethinkdb.tar.gz"],
                            outputs = build["products"],
                            on_begin_transfer = lambda: result_log.write("builds", name, status = "uploading", start_time = time.time()),
                            on_begin_script = lambda: result_log.write("builds", name, status = "running", start_time = time.time()),
                            on_end_script = lambda: result_log.write("builds", name, status = "ok", end_time = time.time()),
                            constraint = "build-ready"
                            )
                    except remotely.ScriptFailedError:
                        result_log.write("builds", name, status = "fail", end_time = time.time())
            except Exception, e:
                result_log.write("builds", name, status = "bug", end_time = time.time(), traceback = traceback.format_exc())

        for name in builds:
            builds[name]["status"] = "waiting"
        result_log.write(builds = builds)

        run_in_threads((lambda name=name: run_build(name, builds[name])) for name in builds)

    # Plan what tests to run

    tests_as_list = [ ]

    def do_test(command_line, repeat = 1, inputs = []):
        tests_as_list.append({
            "inputs": [os.path.join("rethinkdb", i) for i in inputs],
            "command_line": command_line,
            "repeat": repeat
            })

    for (dirpath, dirname, filenames) in os.walk("rethinkdb/test/full_test/"):
        for filename in filenames:
            full_path = os.path.join(dirpath, filename)
            print "Reading specification file %r" % full_path
            try:
                execfile(full_path, {"__builtins__": __builtins__, "do_test": do_test})
            except Exception, e:
                traceback.print_exc()

    def compare_tests(t1, t2):
        return cmp(t1["command_line"], t2["command_line"])
    tests_as_list.sort(compare_tests)

    tests = { }
    for number, test in enumerate(tests_as_list):
        tests[str(number + 1)] = test

    # Run tests

    os.mkdir("tests")
    def run_test(name, test, round):
        try:
            directory = os.path.join("tests", name, str(round))
            os.mkdir(directory)
            with open(os.path.join(directory, "output.txt"), "w") as output_file:
                run_rethinkdb_test_remotely(
                    test["inputs"],
                    test["command_line"],
                    output_file,
                    os.path.join(directory, "output_from_test.zip"),
                    on_begin_script = lambda: result_log.write("tests", name, "rounds", round, status = "running", start_time = time.time()),
                    on_end_script = lambda status: result_log.write("tests", name, "rounds", round, status = status, end_time = time.time())
                    )
        except Exception, e:
            result_log.write("tests", name, "rounds", round, status = "bug", traceback = traceback.format_exc())
    funs = []
    for name in tests:
        test = tests[name]
        missing_prereqs = [i for i in test["inputs"] if not os.path.exists(i)]
        if not missing_prereqs:
            os.mkdir(os.path.join("tests", name))
            test["rounds"] = { }
            for i in xrange(test["repeat"]):
                test["rounds"][i] = { "status": "waiting" }
                funs.append(lambda name=name, test=test, i=i: run_test(name, test, i))
        else:
            test["missing"] = missing_prereqs
    result_log.write(tests = tests)
    run_in_threads(funs)

    print "Done."

