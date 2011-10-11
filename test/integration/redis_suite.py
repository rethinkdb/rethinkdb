#!/usr/bin/python
import os, subprocess, sys 
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

tcl_interpreter = "tclsh8.5"
unit_tests_directory = os.path.join("unit", "type")
redis_test_dir = os.path.join(os.getcwd(), "redis_suite")
redis_test_script = os.path.join(redis_test_dir, "tests", "rethink_redis_test.tcl")
full_unit_tests_directory = os.path.join(redis_test_dir, "tests", "unit", "type")

def test(opts, port, test_dir):
    starting_dir = os.getcwd()
    os.chdir(redis_test_dir)

    cmd_args = [tcl_interpreter, redis_test_script, os.path.join(unit_tests_directory, opts["suite-test"])] 
    proc = subprocess.Popen(cmd_args)
    test_succeeded = proc.wait() == 0
    os.chdir(starting_dir)
    assert test_succeeded

def run_test(options, dir, tests):
    for t in tests:
        if t.find('.tcl') >= 0:
            test_name = t[:-4]
            options['suite-test'] = test_name
            auto_server_test_main(test, options, timeout=800)

if __name__ == "__main__":
    try:
        # Run the specific test requested 
        op = make_option_parser()
        op["suite-test"] = StringFlag("--suite-test")
        options = op.parse(sys.argv)
        auto_server_test_main(test, options, timeout=800)
    except OptError:
        # Just run all the redis tests
        op = make_option_parser()
        options = op.parse(sys.argv)
        options['auto'] = True
        options['valgrind'] = False
        os.path.walk(full_unit_tests_directory, run_test, options)

    
