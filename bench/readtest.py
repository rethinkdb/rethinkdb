#!/usr/bin/python

import sys
import subprocess
from StringIO import StringIO
from datetime import datetime

# Load the parameters
try:
    from readtest_config import *
except:
    print "Please copy readtest_config.py.gen to readtest_config.py and set appropriate variables"
    sys.exit(-1)

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
    estimate = duration * len(blocks) * len(devices) * len(distros) / 60
    print("Estimated time: %d mins\n" % estimate)

    # Run the benchmark
    step = 0
    for block in blocks:
        # Output the block size
        stats_file.write('%d\t' % block)
        for device in devices:
            for distro in distros:
                # Output status
                print("[size: %d, device: %s, distribution: %s] - %d mins left"
                      % (block, device, distro, estimate - (duration * step / 60)))
                # Run the benchmark
                p = subprocess.Popen(['sudo', '-S', 'rebench', '-n', '-d', str(duration),
                                      '-b', str(block), '-u', distro, device],
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE)
                p.stdin.write(password)
                p.stdin.close()
                p.wait()
                # Output the result for the run
                stats_file.write('%s\t' % p.stdout.readline().split()[0])
                step += 1
        stats_file.write('\n')

    stats_file.close()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
