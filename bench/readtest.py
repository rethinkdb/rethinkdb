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

    # Run the benchmark
    for block in blocks:
        # Output the block size
        stats_file.write('%d\t' % block)
        for device in devices:
            for distro in distros:
                # Run the benchmark
                p = subprocess.Popen(['sudo', '-S', 'rebench', '-n', '-d', str(duration),
                                      '-b', str(block), '-u', distro, device],
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE)
                p.stdin.write(password)
                p.stdin.close()
                p.wait()
                # Output the result for the run
                stats_file.write('%s\t' % p.stdout.readline().split()[0])
        stats_file.write('\n')

    stats_file.close()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
