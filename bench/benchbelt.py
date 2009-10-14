#!/usr/bin/python

import sys

# Add current dir path, if provided
if len(sys.argv) > 1:
    sys.path.append(sys.argv[1])

import subprocess
import inspect
from copy import deepcopy
from StringIO import StringIO
from datetime import datetime
from statistics import stats

# Load the parameters
try:
    from benchbelt_config import *
except Exception as ex:
    print ex
    print
    print "Please copy benchbelt_config.py.gen to benchbelt_config.py and set appropriate variables"
    sys.exit(-1)

try:
    rebench_except.append('device')
except NameError:
    rebench_except = ['device']

try:
    rebench_args_ex
except NameError:
    rebench_args_ex = []

# Run variables
least_runs = margins[0]
most_runs = margins[1]
error_ratio = margins[2]

def total_benchmarks(runs):
    def acc_fn(acc, i):
        if type(acc) == type(1):
            return acc * len(i)
        else:
            return len(acc) * len(i)
    return len(devices) * reduce(acc_fn, map(lambda i: i[1:][0], runs))

# Time calculation
def calc_estimate(benchmarks_left):
    try:
        duration
    except NameError:
        return 0
    return int(duration / 60.0
               * benchmarks_left
               * ((most_runs + least_runs) / 2.0))

# Find the device in the args
def get_device(args):
    for arg in args:
        if arg[0] == 'device':
            return arg[1]

# Inner loop
step = 0
stats_file = None
def run_bench_loop(args, fn, fn_args=[]):
    global step
    if not args:
        # Print the parameters
        for arg in fn_args:
            stats_file.write('%s\t' % arg[1])
        # Call the benchmark function
        fn(fn_args, step)
        # One, two, step :)
        step += 1
    else:
        arg_name = args[0][0]
        arg_vals = args[0][1]
        if(len(args[0]) == 3):
            arg_conf = args[0][2]
        else:
            arg_conf = {}
        for arg_val in arg_vals:
            # Copy a list of vars to pass on
            lst = deepcopy(fn_args)
            lst.append((arg_name, arg_val))
            # Setup the benchmark
            setup_fn = arg_conf.get('setup')
            if setup_fn:
                if len(inspect.getargspec(setup_fn).args) == 2:
                    _lst = setup_fn(arg_val, lst)
                    if _lst:
                        lst = _lst
                else:
                    setup_fn(arg_val)
            # Run the benchmark
            run_bench_loop(args[1:], fn, lst)
            # Teardown the benchmark
            teardown_fn = arg_conf.get('teardown')
            if teardown_fn:
                if len(inspect.getargspec(teardown_fn).args) == 2:
                    teardown_fn(arg_val, lst)
                else:
                    teardown_fn(arg_val)
            # Honor line breaks
            if arg_conf.get('line-break') == True:
                stats_file.write('\n')

def do_benchmark(args, step):
    # Go through the runs
    values = []
    rebench_args = ['sudo', '-S', 'rebench', '-n']
    try:
        duration
        rebench_args.extend(['-d', str(duration)])
    except NameError:
        pass
    rebench_args.extend(rebench_args_ex)
    exceptions = []
    for arg in args:
        if not arg[0] in rebench_except:
            rebench_args.append('--%s' % arg[0])
            rebench_args.append(str(arg[1]))
        else:
            exceptions.append((arg[0], str(arg[1])))
    rebench_args.append(get_device(args))
    for ex in exceptions:
        sys.stdout.write("%s: %s; " % (ex[0], ex[1]))
    print
    run_name = reduce(lambda acc, i: '%s %s' % (str(acc), str(i)), rebench_args[2:])
    print("%s, ~%d mins left"
          % (run_name, int(calc_estimate(total_benchmarks(run_config) - step))))
    for run in range(1, most_runs + 1):
        print("Run %d of ~%d..%d" % (run, least_runs, most_runs))
        # Runs the benchmark
        p = subprocess.Popen(rebench_args,
                             stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        p.stdin.write(password)
        p.stdin.close()
        p.wait()
        # Compute the stats
        rebench_value = p.stdout.readline()
        try:
            rebench_value = int(rebench_value.split()[0])
        except:
            print("Could not parse '%s' returned by rebench" % rebench_value)
            sys.exit()
        values.append(rebench_value)
        if run >= least_runs:
            (mean, median, std, minimum, maximum, confidence) = stats(values)
            confidence_reqs_met = confidence / 2.0 < mean * error_ratio
            if(confidence_reqs_met):
                print("Met confidence requirements at %dth run\n" % run)
                break
            if(run == most_runs and (not confidence_reqs_met)):
                sys.stderr.write("Confidence requirements weren't met for %s\n\n" % run_name)
    # Output the result for the run
    stats_file.write('%d\t' % mean)
    stats_file.write('\n')

def main(argv):
    global stats_file
    # Open the results file
    now = datetime.now()
    stats_file = open('%s@%s' % (logname, now.strftime("%Y-%m-%d.%H-%M-%S")), 'w')

    # Print the headers
    stats_file.write('# device\n')
    for arg in run_config:
        stats_file.write('# %s\n' % arg[0])
    stats_file.write('# iops\n')

    # Time estimation
    print("Estimated time: ~%d mins\n" % calc_estimate(total_benchmarks(run_config)))
    
    # Run the benchmark
    for device in devices:
        run_bench_loop(run_config, do_benchmark, [('device', device)])

    stats_file.close()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
