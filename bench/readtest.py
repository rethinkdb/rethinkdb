#!/usr/bin/python

import sys
import subprocess
from StringIO import StringIO
from datetime import datetime
from statistics import stats

# Load the parameters
try:
    from readtest_config import *
except:
    print "Please copy readtest_config.py.gen to readtest_config.py and set appropriate variables"
    sys.exit(-1)

# Run variables
least_runs = runs[0]
most_runs = runs[1]
error_ratio = runs[2]

def total_benchmarks():
    return len(blocks) * len(devices) * len(distros)

def calc_estimate(benchmarks_left):
    return int(duration / 60.0
               * benchmarks_left
               * ((most_runs - least_runs) / 2.0))

def main(argv):
    # Open the results file
    now = datetime.now()
    stats_file = open('%s@%s' % (logname, now.strftime("%Y-%m-%d.%H-%M-%S")), 'w')

    # Print the headers
    stats_file.write('# block-size\n')
    for device in devices:
        for distro in distros:
            stats_file.write('# %s (%s)\n' % (device, distro))

    # Time estimation
    print("Estimated time: ~%d mins\n" % calc_estimate(total_benchmarks()))

    # Run the benchmark
    step = 0
    for block in blocks:
        # Output the block size
        stats_file.write('%d\t' % block)
        for device in devices:
            for distro in distros:
                values = []
                # Status for this benchmark
                run_name = ("[size: %d, device: %s, distribution: %s]"
                            % (block, device, distro))
                print("%s, ~%d mins left"
                      % (run_name, int(calc_estimate(total_benchmarks() - step))))
                for run in range(1, most_runs + 1):
                    # Status for this run
                    print("run %d of ~%d..%d" % (run, least_runs, most_runs))
                    
                    # Run the benchmark
                    p = subprocess.Popen(['sudo', '-S', 'rebench', '-n', '-d', str(duration),
                                          '-b', str(block), '-u', distro, device],
                                         stdin=subprocess.PIPE, stdout=subprocess.PIPE)
                    p.stdin.write(password)
                    p.stdin.close()
                    p.wait()
                    
                    # Compute the stats
                    value = int(p.stdout.readline().split()[0])
                    values.append(value)
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
                step += 1
        stats_file.write('\n')

    stats_file.close()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
