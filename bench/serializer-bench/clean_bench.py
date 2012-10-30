# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python
import subprocess, stat, os, re, time

def format_args(d):
    """Formats a dictionary of key-value pairs into a list of arguments of the form
    ['--KEY1', 'VALUE1', '--KEY2', 'VALUE2', ...] suitable for passing to the subprocess module."""
    args = []
    for (k, v) in d.iteritems():
        assert k[0] != '-'
        args.append("--" + k)
        args.append(str(v))
    return args

def run_clean_bench(drive_path, parameters, workload):
    
    """Benchmark the given drive with the RethinkDB serializer.
    
    'parameters' is the parameters for the serializer; keys should be flags that can be passed to
    RethinkDB without the leading "--". Example: { "extent-size": 1024*1024, "block-size": 4096 }
    
    'workload' is workload parameters for serializer-bench, in the same format as 'parameters'.
    
    Return value will be None. Test output will be put in "transactions.txt"."""
    
    drive_path = os.path.abspath(drive_path)
    assert stat.S_ISBLK(os.stat(drive_path)[stat.ST_MODE])
    assert " " not in drive_path
    
    assert "duration" not in parameters
    
    # Secure-erase the drive as the first step of putting it in a steady state
    # Note that 'hdparm --secure-erase' is only safe for use on solid state drives!
    if False:
        print "Secure erasing %r..." % drive_path
        start_time = time.time()
        drive_password = "password"
        subprocess.check_call(["hdparm", "--user-master", "u", "--security-set-pass", drive_password, drive_path])
        try:
            subprocess.check_call(["hdparm", "--user-master", "u", "--security-erase", drive_password, drive_path])
        finally:
            subprocess.check_call(["hdparm", "--user-master", "u", "--security-unlock", drive_password, drive_path])
        print "Took %.3f seconds." % (time.time() - start_time)
    
    # Now fill it with the RethinkDB serializer, the second step of putting it into a steady state
    print "Putting %r into a steady state..." % drive_path
    start_time = time.time()
    serv = subprocess.Popen(
        ["./serializer-bench", "-f", drive_path, "--forever"] + format_args(parameters),
        stderr = subprocess.PIPE, stdout = subprocess.PIPE)
    try:
        output = serv.communicate()[1]
    finally:
        try: serv.terminate()
        except RuntimeError: pass
    print "Took %.3f seconds." % (time.time() - start_time)
    assert "RethinkDB ran out of disk space." in output
    
    # Now run the actual test on it
    print "Running the actual test..."
    start_time = time.time()
    serv = subprocess.Popen(
        ["./serializer-bench", "-f", drive_path, "--log", "transactions.txt"] + format_args(parameters) + format_args(workload),
        stderr = subprocess.PIPE, stdout = subprocess.PIPE)
    try:
        serv.wait()
    finally:
        try: serv.terminate()
        except RuntimeError: pass
    if serv.returncode != 0:
        raise RuntimeError("RethinkDB serializer failed:\n" + output)
    print "Took %.3f seconds." % (time.time() - start_time)
    
    print "Done."

if __name__ == "__main__":
    
    parameter_flags = ["block-size", "extent-size", "active-data-extents", "file-zone-size"]
    workload_flags = ["duration", "concurrent", "inserts-per-txn", "updates-per-txn"]
    
    import sys
    del sys.argv[0]
    drive_path = sys.argv.pop(0)
    parameters = {}
    workload = {}
    while sys.argv:
        flag = sys.argv.pop(0)
        if flag.startswith("--") and flag[2:] in parameter_flags:
            parameters[flag[2:]] = int(sys.argv.pop(0))
        elif flag.startswith("--") and flag[2:] in workload_flags:
            workload[flags[2:]] = int(sys.argv.pop(0))
        else:
            raise ValueError("Bad argument argument %r.", flag)
    
    result = run_clean_bench(drive_path, parameters, workload)
    
    print "The test took %.3f seconds." % result
