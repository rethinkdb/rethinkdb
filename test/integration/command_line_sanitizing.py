#!/usr/bin/python
import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

# returns (return_value, stdout, stderr) or None in case the process did not terminate within timeout seconds
# run_rethinkdb() automatically fills in -f, -s, -c and -p if not specified in flags
def run_rethinkdb(opts, test_dir, flags = [], timeout = 10):
    executable_path = get_executable_path(opts, "rethinkdb")
    db_data_dir = test_dir.p("db_data")
    if not os.path.isdir(db_data_dir): os.mkdir(db_data_dir)
    db_data_path = os.path.join(db_data_dir, "data_file")
    
    has_c_flag = False
    has_s_flag = False
    has_f_flag = False
    has_p_flag = False
    for flag in flags:
        has_c_flag = has_c_flag or flag == "-c"
        has_s_flag = has_s_flag or flag == "-s"
        has_f_flag = has_f_flag or flag == "-f"
        has_p_flag = has_p_flag or flag == "-p"
    
    if not has_c_flag:
        flags += ["-c", str(opts["cores"])]
    if not has_s_flag:
        flags += ["-s", str(opts["slices"])]
    if not has_f_flag:
        flags += ["-f", db_data_path]
    if not has_p_flag:
        flags += ["-p", str(find_unused_port())]
        
    command_line = [executable_path] + flags
    
    print "Executing " + subprocess.list2cmdline(command_line)
    
    # Redirect output to temporary files to circumvent all the problems with pipe buffers filling up
    stdout_path = test_dir.p("rethinkdb_stdout.txt")
    stderr_path = test_dir.p("rethinkdb_stderr.txt")
    rethinkdb_stdout = file(stdout_path, "w")
    rethinkdb_stderr = file(stderr_path, "w")
    
    server = subprocess.Popen(command_line,
        stdout = rethinkdb_stdout, stderr = rethinkdb_stderr);
    
    return_value = wait_with_timeout(server, timeout)
    dead = return_value is not None
    if not dead:
        # did not terminate
        return None
    else:
        return (return_value, open(stdout_path, 'r').read(), open(stderr_path, 'r').read())


# expected_return_value = None means we expect server to not terminate
def check_rethinkdb_flags(opts, flags, expected_return_value):
    assert opts["database"] == "rethinkdb"
    
    create_db_timeout = 15
    rethinkdb_check_timeout = 10
    
    test_dir = TestDir()
    # Create an empty database file
    create_result = run_rethinkdb(opts, test_dir, ["create", "--force"], create_db_timeout)
    if create_result is None:
        raise ValueError("Server took longer than %d seconds to create database." % server_create_time)
    if create_result[0] != 0:
        raise ValueError("Server failed to create database.")
    
    # Run RethinkDB with the specified flags
    rethinkdb_result = run_rethinkdb(opts, test_dir, flags, rethinkdb_check_timeout)
    if expected_return_value is None and rethinkdb_result is not None:
        raise ValueError("RethinkDB did exit with a return value of %i, although it was not expected to. Flags were: %s" % (rethinkdb_result[0], flags))
    else:
        if rethinkdb_result[0] != expected_return_value:
            raise ValueError("RethinkDB gave a return value of %i, expected a value of %i. Flags were: %s" % (rethinkdb_result[0], expected_return_value, flags))

if __name__ == "__main__":
    op = make_option_parser()
    opts = op.parse(sys.argv)
    opts["mclib"] = "memcache"
    
    exit_code_expected_on_error = 255 # we return -1 given a bad command line input

    for (flags, expected_return_value) in [
            (["-c", "string"], exit_code_expected_on_error),
            (["-c", "128"], exit_code_expected_on_error),
            (["-c", "1.5"], exit_code_expected_on_error),
            (["-c", "-1"], exit_code_expected_on_error),
            (["-m", "-1"], exit_code_expected_on_error),
            (["--active-data-extents", "65"], exit_code_expected_on_error),
            (["--active-data-extents", "0"], exit_code_expected_on_error),
            (["--active-data-extents", "-1"], exit_code_expected_on_error),
            (["--active-data-extents", "5.5"], exit_code_expected_on_error),
            (["--active-data-extents", "nonumber"], exit_code_expected_on_error),
            (["--unsaved-data-limit", "-1"], exit_code_expected_on_error),
            (["--unsaved-data-limit", "nonumber"], exit_code_expected_on_error),
            (["-p", "-1"], exit_code_expected_on_error),
            (["-p", "65536"], exit_code_expected_on_error),
            (["-p", "string"], exit_code_expected_on_error),
            (["-p", "5.5"], exit_code_expected_on_error),
            (["-f", "/"], exit_code_expected_on_error),
            (["-s", "-1"], exit_code_expected_on_error),
            (["-s", "0"], exit_code_expected_on_error),
            (["-s", "string"], exit_code_expected_on_error),
            (["-s", "5.5"], exit_code_expected_on_error),
            (["--block-size", "4096000000", "--extent-size", "4096000000"], exit_code_expected_on_error)]:
        print "Testing command line flags %s, expecting a return value of %i" % (flags, expected_return_value)
        check_rethinkdb_flags(opts, flags, expected_return_value)
        
        # TODO: Also check -l behvior on missing permissions and slashes in file name

